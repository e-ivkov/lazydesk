/* ctl/main.c — lazydeskctl: CLI client for the facade IPC (task 4.4).
 *
 * Verbs mirror the palette verbs and IPC ops one-to-one (action registry
 * family). Headless- and pipeline-friendly: --json everywhere, non-zero
 * exit with readable error on protocol failure.
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "lazydesk-build.h"
#include "cJSON.h"

static const char *usage =
"lazydeskctl " LAZYDESK_VERSION " — control the LazyDesk session\n"
"\n"
"usage: lazydeskctl [--json] <verb> [args]\n"
"\n"
"verbs:\n"
"  version                          protocol + component versions\n"
"  windows list                     list windows (id, workspace, app_id, title)\n"
"  workspaces list                  list workspaces\n"
"  workspace focus <n|name>         focus a workspace (same as Super+<n>)\n"
"  spawn [--archetype NAME] [--] <cmd...>   spawn a command (window routes by archetype)\n"
"  config reload                    regenerate artifacts + reload backend\n"
"  session quit                     quit the session\n";

static bool opt_json = false;

static int dial(char *sockpath, size_t n)
{
	const char *rtd = getenv("XDG_RUNTIME_DIR");
	if (!rtd || !rtd[0]) {
		fprintf(stderr, "lazydeskctl: XDG_RUNTIME_DIR not set\n");
		return -1;
	}
	snprintf(sockpath, n, "%s/lazydesk/ipc.sock", rtd);
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) return -1;
	struct sockaddr_un addr = { 0 };
	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, sizeof addr.sun_path, "%s", sockpath);
	if (connect(fd, (struct sockaddr *)&addr, sizeof addr) < 0) {
		close(fd);
		return -1;
	}
	return fd;
}

/* Send one request line; read one reply line; return parsed JSON or NULL. */
static cJSON *rpc(int fd, const char *request)
{
	size_t len = strlen(request);
	if (write(fd, request, len) != (ssize_t)len || write(fd, "\n", 1) != 1)
		return nullptr;

	size_t cap = 64 * 1024;
	char *buf = malloc(cap);
	size_t off = 0;
	while (off + 1 < cap) {
		ssize_t n = read(fd, buf + off, 1);
		if (n <= 0) break;
		if (buf[off] == '\n') break;
		off++;
	}
	buf[off] = '\0';
	cJSON *j = cJSON_Parse(buf);
	free(buf);
	return j;
}

static int report(cJSON *reply)
{
	if (!reply) {
		fprintf(stderr, "lazydeskctl: no/invalid reply from lazydeskd\n");
		return 1;
	}
	bool ok = cJSON_IsTrue(cJSON_GetObjectItem(reply, "ok"));
	if (ok) {
		cJSON *result = cJSON_GetObjectItem(reply, "result");
		if (opt_json) {
			char *s = cJSON_PrintUnformatted(result ? result : reply);
			printf("%s\n", s);
			free(s);
		} else if (result) {
			char *s = cJSON_Print(result); /* pretty for humans */
			printf("%s\n", s);
			free(s);
		} else {
			printf("ok\n");
		}
		cJSON_Delete(reply);
		return 0;
	}
	const char *op = cJSON_GetStringValue(cJSON_GetObjectItem(reply, "op"));
	const char *err = cJSON_GetStringValue(cJSON_GetObjectItem(reply, "error"));
	const char *det = cJSON_GetStringValue(cJSON_GetObjectItem(reply, "detail"));
	if (opt_json) {
		char *s = cJSON_PrintUnformatted(reply);
		printf("%s\n", s);
		free(s);
	} else {
		fprintf(stderr, "lazydeskctl: %s%s%s%s%s\n",
		        err ? err : "error",
		        op && *op ? " (op " : "", op && *op ? op : "", op && *op ? ")" : "",
		        det ? ": " : "");
		if (det && !opt_json)
			fprintf(stderr, "  %s\n", det);
	}
	cJSON_Delete(reply);
	return 1;
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		fputs(usage, stderr);
		return 2;
	}
	int i = 1;
	if (!strcmp(argv[i], "--json")) { opt_json = true; i++; }
	if (i >= argc) { fputs(usage, stderr); return 2; }

	char request[8192];
	const char *verb = argv[i];
	const char *sub = (i + 1 < argc) ? argv[i + 1] : "";

	if (!strcmp(verb, "help") || !strcmp(verb, "-h")) {
		fputs(usage, stderr);
		return 0;
	}

	if (!strcmp(verb, "version"))
		snprintf(request, sizeof request, "{\"op\":\"version\"}");
	else if (!strcmp(verb, "windows") && !strcmp(sub, "list"))
		snprintf(request, sizeof request, "{\"op\":\"windows.list\"}");
	else if (!strcmp(verb, "workspaces") && !strcmp(sub, "list"))
		snprintf(request, sizeof request, "{\"op\":\"workspaces.list\"}");
	else if (!strcmp(verb, "workspace") && !strcmp(sub, "focus") && i + 2 < argc) {
		char *a = argv[i + 2];
		if (a[0] >= '0' && a[0] <= '9')
			snprintf(request, sizeof request, "{\"op\":\"workspace.focus\",\"name\":%s}", a);
		else
			snprintf(request, sizeof request, "{\"op\":\"workspace.focus\",\"name\":\"%s\"}", a);
	}
	else if (!strcmp(verb, "config") && !strcmp(sub, "reload"))
		snprintf(request, sizeof request, "{\"op\":\"config.reload\"}");
	else if (!strcmp(verb, "session") && !strcmp(sub, "quit"))
		snprintf(request, sizeof request, "{\"op\":\"session.quit\"}");
	else if (!strcmp(verb, "spawn")) {
		char arch[64] = "";
		int k = i + 1;
		if (k < argc && !strcmp(argv[k], "--archetype") && k + 1 < argc) {
			snprintf(arch, sizeof arch, "%s", argv[k + 1]);
			k += 2;
		}
		if (k < argc && !strcmp(argv[k], "--")) k++;
		if (k >= argc) {
			fprintf(stderr, "lazydeskctl: spawn needs a command\n");
			return 2;
		}
		char cmd[4096] = "";
		size_t off = 0;
		for (; k < argc; k++) {
			/* shell-quote each argument so -c 'cmd args' survive the round trip */
			off += snprintf(cmd + off, sizeof cmd - off, "%s'", off ? " " : "");
			for (const char *a = argv[k]; *a; a++) {
				if (*a == '\'') off += snprintf(cmd + off, sizeof cmd - off, "'\\''");
				else             cmd[off++] = *a;
			}
			off += snprintf(cmd + off, sizeof cmd - off, "'");
		}
		cJSON *rq = cJSON_CreateObject();
		cJSON_AddStringToObject(rq, "op", "spawn");
		if (arch[0]) cJSON_AddStringToObject(rq, "archetype", arch);
		cJSON_AddStringToObject(rq, "command", cmd);
		char *rs = cJSON_PrintUnformatted(rq);
		snprintf(request, sizeof request, "%s", rs);
		free(rs);
		cJSON_Delete(rq);
	}
	else {
		fprintf(stderr, "lazydeskctl: unknown verb '%s %s'\n\n%s", verb, sub, usage);
		return 2;
	}

	char sockpath[512];
	int fd = dial(sockpath, sizeof sockpath);
	if (fd < 0) {
		fprintf(stderr, "lazydeskctl: cannot connect to %s (is lazydeskd running?)\n", sockpath);
		return 1;
	}
	cJSON *reply = rpc(fd, request);
	close(fd);
	return report(reply);
}
