/* daemon/main.c — lazydeskd: the LazyDesk IPC facade.
 *
 * NDJSON over $XDG_RUNTIME_DIR/lazydesk/ipc.sock (see docs/ipc.md and the
 * ipc spec): every user action is an op; malformed input is a structured
 * error, never a crash. Backend (niri) is reached via `niri msg`; where no
 * compositor is reachable the facade reports a structured backend error.
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include "lazydesk-build.h"
#include "lazydesk.h"
#include "bindings.h"
#include "config.h"
#include "generate.h"
#include "cJSON.h"

#define MAX_LINE (64 * 1024)

static volatile sig_atomic_t g_stop;

static void on_signal(int sig) { (void)sig; g_stop = 1; }

/* ---------- helpers ---------- */

static void send_json(int fd, const cJSON *j)
{
	char *s = cJSON_PrintUnformatted(j);
	size_t len = strlen(s);
	size_t off = 0;
	while (off < len) {
		ssize_t n = write(fd, s + off, len - off);
		if (n < 0) break;
		off += (size_t)n;
	}
	write(fd, "\n", 1);
	free(s);
}

static void send_ok(int fd, cJSON *result)
{
	cJSON *r = cJSON_CreateObject();
	cJSON_AddBoolToObject(r, "ok", true);
	if (result)
		cJSON_AddItemToObject(r, "result", result);
	send_json(fd, r);
	cJSON_Delete(r);
}

static void send_err(int fd, const char *op, const char *error, const char *detail)
{
	cJSON *r = cJSON_CreateObject();
	cJSON_AddBoolToObject(r, "ok", false);
	cJSON_AddStringToObject(r, "op", op ? op : "");
	cJSON_AddStringToObject(r, "error", error);
	if (detail)
		cJSON_AddStringToObject(r, "detail", detail);
	send_json(fd, r);
	cJSON_Delete(r);
}

/* Run a command synchronously, return exit status; out/errbuf capture output. */
static int run_capture(char *out, size_t outlen, const char *fmt, ...)
{
	char cmd[2048];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(cmd, sizeof cmd, fmt, ap);
	va_end(ap);

	FILE *p = popen(cmd, "r");
	if (!p) return -1;
	size_t off = 0;
	if (out && outlen)
		out[0] = '\0';
	while (out && off + 1 < outlen) {
		size_t n = fread(out + off, 1, outlen - 1 - off, p);
		if (n == 0) break;
		off += n;
	}
	out[off] = '\0';
	int st = pclose(p);
	return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}

/* ---------- op implementations ---------- */

static void op_version(int fd)
{
	cJSON *r = cJSON_CreateObject();
	cJSON_AddNumberToObject(r, "protocol", 1);
	cJSON_AddStringToObject(r, "lazydesk", LAZYDESK_VERSION);
	cJSON_AddStringToObject(r, "backend", "niri");
	send_ok(fd, r);
}

static void op_windows_list(int fd)
{
	char out[64 * 1024];
	int rc = run_capture(out, sizeof out, "niri msg -j windows 2>/dev/null");
	if (rc != 0) {
		send_err(fd, "windows.list", "backend unavailable",
		         "niri msg failed (is a niri session running?)");
		return;
	}
	cJSON *arr = cJSON_Parse(out);
	if (!arr) {
		send_err(fd, "windows.list", "backend returned unparsable output", nullptr);
		return;
	}
	/* normalize: id, app_id, title, workspace, state */
	cJSON *norm = cJSON_CreateArray();
	cJSON *w;
	cJSON_ArrayForEach(w, arr) {
		cJSON *o = cJSON_CreateObject();
		cJSON *id = cJSON_GetObjectItem(w, "id");
		if (id) cJSON_AddNumberToObject(o, "id", id->valuedouble);
		cJSON *app = cJSON_GetObjectItem(w, "app_id");
		if (cJSON_IsString(app)) cJSON_AddStringToObject(o, "app_id", app->valuestring);
		cJSON *title = cJSON_GetObjectItem(w, "title");
		if (cJSON_IsString(title)) cJSON_AddStringToObject(o, "title", title->valuestring);
		cJSON *wsid = cJSON_GetObjectItem(w, "workspace_id");
		if (wsid) cJSON_AddNumberToObject(o, "workspace", wsid->valuedouble);
		cJSON *wsn = cJSON_GetObjectItem(w, "workspace_name");
		if (cJSON_IsString(wsn)) cJSON_AddStringToObject(o, "workspace_name", wsn->valuestring);
		cJSON_AddItemToArray(norm, o);
	}
	cJSON_Delete(arr);
	send_ok(fd, norm);
}

static void op_workspaces_list(int fd)
{
	char out[32 * 1024];
	int rc = run_capture(out, sizeof out, "niri msg -j workspaces 2>/dev/null");
	if (rc != 0) {
		send_err(fd, "workspaces.list", "backend unavailable",
		         "niri msg failed (is a niri session running?)");
		return;
	}
	cJSON *arr = cJSON_Parse(out);
	if (!arr) {
		send_err(fd, "workspaces.list", "backend returned unparsable output", nullptr);
		return;
	}
	cJSON *norm = cJSON_CreateArray();
	cJSON *w;
	cJSON_ArrayForEach(w, arr) {
		cJSON *o = cJSON_CreateObject();
		cJSON *idx = cJSON_GetObjectItem(w, "idx");
		if (idx) cJSON_AddNumberToObject(o, "number", idx->valuedouble);
		cJSON *name = cJSON_GetObjectItem(w, "name");
		if (cJSON_IsString(name)) cJSON_AddStringToObject(o, "name", name->valuestring);
		cJSON *active = cJSON_GetObjectItem(w, "is_active");
		if (active) cJSON_AddBoolToObject(o, "focused", cJSON_IsTrue(active));
		cJSON_AddItemToArray(norm, o);
	}
	cJSON_Delete(arr);
	send_ok(fd, norm);
}

static void op_niri_action(int fd, const char *op, const char *action, const char *arg)
{
	int rc = run_capture(nullptr, 0, "niri msg action %s %s 2>/dev/null", action, arg ? arg : "");
	if (rc != 0)
		send_err(fd, op, "backend rejected action", action);
	else
		send_ok(fd, nullptr);
}

/* spawn: fork/exec with LAZYDESK_* env; windows route via generated rules. */
static void op_spawn(int fd, cJSON *args)
{
	const cJSON *jcmd = cJSON_GetObjectItem(args, "command");
	if (!cJSON_IsString(jcmd) || !jcmd->valuestring[0]) {
		send_err(fd, "spawn", "missing command", "spawn requires a command string");
		return;
	}
	const cJSON *jarch = cJSON_GetObjectItem(args, "archetype");
	char env_arch[64] = "";
	if (cJSON_IsString(jarch))
		snprintf(env_arch, sizeof env_arch, "%s", jarch->valuestring);

	pid_t pid = fork();
	if (pid < 0) {
		send_err(fd, "spawn", "fork failed", strerror(errno));
		return;
	}
	if (pid == 0) {
		setenv("LAZYDESK", "1", 1);
		if (env_arch[0])
			setenv("LAZYDESK_ARCHETYPE", env_arch, 1);
		char shell_cmd[4096];
		snprintf(shell_cmd, sizeof shell_cmd, "exec %s", jcmd->valuestring);
		execl("/bin/sh", "sh", "-c", shell_cmd, (char *)nullptr);
		_exit(127);
	}
	cJSON *r = cJSON_CreateObject();
	cJSON_AddNumberToObject(r, "pid", pid);
	send_ok(fd, r);
	/* TODO(task 4.2): reap and emit session.spawn-result */
}

/* config.reload: regenerate artifacts via the same library as the CLI,
 * then reload the backend; emit config.reloaded to subscribers (task 4.3). */
static void op_config_reload(int fd)
{
	const char *home = getenv("HOME");
	char err[512];
	char path[512], outdir[512];
	snprintf(path, sizeof path, "%s/.config/lazydesk/lazydesk.toml", home);
	snprintf(outdir, sizeof outdir, "%s/.config/lazydesk/generated", home);

	/* fall back to shipped template when no user config exists */
	FILE *f = fopen(path, "r");
	if (!f)
		snprintf(path, sizeof path, "%s/config/lazydesk.toml", ld_datadir());
	else
		fclose(f);

	ld_config cfg;
	if (!ld_config_load(&cfg, path, err, sizeof err)) {
		send_err(fd, "config.reload", "invalid config", err);
		return;
	}
	ld_theme theme;
	if (!ld_theme_load(&theme, ld_datadir(), cfg.theme, err, sizeof err)) {
		send_err(fd, "config.reload", "invalid theme", err);
		return;
	}
	if (!ld_generate_all(&cfg, &theme, outdir, false, err, sizeof err)) {
		send_err(fd, "config.reload", "generation refused", err);
		return;
	}
	send_ok(fd, nullptr);
}

/* ---------- dispatch ---------- */

static void handle_line(int fd, const char *line)
{
	cJSON *req = cJSON_Parse(line);
	if (!req) {
		send_err(fd, nullptr, "malformed request", "line is not valid JSON");
		return;
	}
	const cJSON *jop = cJSON_GetObjectItem(req, "op");
	const char *op = cJSON_IsString(jop) ? jop->valuestring : nullptr;
	if (!op) {
		send_err(fd, nullptr, "missing op", nullptr);
		cJSON_Delete(req);
		return;
	}

	if (!strcmp(op, "version"))             op_version(fd);
	else if (!strcmp(op, "windows.list"))   op_windows_list(fd);
	else if (!strcmp(op, "workspaces.list")) op_workspaces_list(fd);
	else if (!strcmp(op, "workspace.focus")) {
		const cJSON *a = cJSON_GetObjectItem(req, "name");
		char arg[128];
		if (cJSON_IsString(a)) snprintf(arg, sizeof arg, "\"%s\"", a->valuestring);
		else if (cJSON_IsNumber(a)) snprintf(arg, sizeof arg, "%d", (int)a->valuedouble);
		else arg[0] = '\0';
		op_niri_action(fd, op, "focus-workspace", arg);
	}
	else if (!strcmp(op, "spawn"))          op_spawn(fd, req);
	else if (!strcmp(op, "config.reload"))  op_config_reload(fd);
	else if (!strcmp(op, "session.quit")) {
		send_ok(fd, nullptr);
		op_niri_action(fd, op, "quit", nullptr);
	}
	else
		send_err(fd, op, "unknown op", nullptr);

	cJSON_Delete(req);
}

/* ---------- connection loop ---------- */

static void serve_conn(int fd)
{
	FILE *f = fdopen(fd, "r");
	if (!f) { close(fd); return; }
	char *line = malloc(MAX_LINE);
	while (!g_stop && fgets(line, MAX_LINE, f)) {
		size_t n = strlen(line);
		while (n && (line[n-1] == '\n' || line[n-1] == '\r'))
			line[--n] = '\0';
		if (n)
			handle_line(fd, line);
	}
	free(line);
	fclose(f);
}

int main(void)
{
	signal(SIGINT, on_signal);
	signal(SIGTERM, on_signal);
	signal(SIGCHLD, SIG_IGN); /* reap spawns */

	const char *rtd = getenv("XDG_RUNTIME_DIR");
	if (!rtd || !rtd[0]) {
		fprintf(stderr, "lazydeskd: XDG_RUNTIME_DIR not set\n");
		return 1;
	}
	char sockpath[512];
	snprintf(sockpath, sizeof sockpath, "%s/lazydesk/ipc.sock", rtd);

	char dir[512];
	snprintf(dir, sizeof dir, "%s/lazydesk", rtd);
	mkdir(dir, 0700);

	int s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s < 0) { perror("socket"); return 1; }
	struct sockaddr_un addr = { 0 };
	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, sizeof addr.sun_path, "%s", sockpath);
	unlink(sockpath);
	umask(0077);
	if (bind(s, (struct sockaddr *)&addr, sizeof addr) < 0) {
		perror("bind"); return 1;
	}
	if (listen(s, 16) < 0) { perror("listen"); return 1; }

	fprintf(stderr, "lazydeskd %s listening on %s\n", LAZYDESK_VERSION, sockpath);

	while (!g_stop) {
		int c = accept(s, nullptr, nullptr);
		if (c < 0) {
			if (errno == EINTR) continue;
			break;
		}
		serve_conn(c); /* single client at a time in the skeleton;
		                  concurrent clients + event fanout: task 4.3 */
	}
	unlink(sockpath);
	return 0;
}
