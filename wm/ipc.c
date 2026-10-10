/* wm/ipc.c — the NDJSON facade socket hosted in-process on the compositor loop. */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <wlr/util/log.h>

#include "wm.h"
#include "lazydesk-build.h"
#include "cJSON.h"

#define MAX_CLIENTS 32
#define IPC_LINE_MAX (64 * 1024)

struct ipc_client {
	int fd;
	struct wl_event_source *source;
	struct wm_server *server;
	struct wl_listener readable;
	struct wl_listener hup;
	bool subscribed;
	char buf[IPC_LINE_MAX];
	size_t len;
	size_t id;      /* window filter: 0 = events only */
	struct {
		struct ipc_client *c;
	} noop;
};

static struct ipc_client *clients[MAX_CLIENTS];
static size_t nclients;
static struct wm_server *g_server;

/* ---------- send helpers ---------- */

static void ipc_send_raw(int fd, const char *s)
{
	size_t len = strlen(s);
	size_t off = 0;
	while (off < len) {
		ssize_t n = write(fd, s + off, len - off);
		if (n <= 0) return;
		off += (size_t)n;
	}
	(void)!write(fd, "\n", 1);
}

static void ipc_send_json(int fd, cJSON *j)
{
	char *s = cJSON_PrintUnformatted(j);
	ipc_send_raw(fd, s);
	free(s);
}

static void ipc_send_ok(int fd, cJSON *result)
{
	cJSON *r = cJSON_CreateObject();
	cJSON_AddBoolToObject(r, "ok", true);
	if (result) cJSON_AddItemToObject(r, "result", result);
	ipc_send_json(fd, r);
	cJSON_Delete(r);
}

static void ipc_send_err(int fd, const char *op, const char *error, const char *detail)
{
	cJSON *r = cJSON_CreateObject();
	cJSON_AddBoolToObject(r, "ok", false);
	cJSON_AddStringToObject(r, "op", op ? op : "");
	cJSON_AddStringToObject(r, "error", error);
	if (detail) cJSON_AddStringToObject(r, "detail", detail);
	ipc_send_json(fd, r);
	cJSON_Delete(r);
}

void wm_ipc_event(const char *fmt, ...)
{
	if (!g_server || nclients == 0)
		return;
	char line[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(line, sizeof line, fmt, ap);
	va_end(ap);
	for (size_t i = 0; i < nclients; i++)
		if (clients[i] && clients[i]->subscribed)
			ipc_send_raw(clients[i]->fd, line);
}

/* ---------- window/workspace lookups ---------- */

static struct wm_window *find_window(struct wm_server *s, uint32_t id)
{
	struct wm_workspace *ws;
	struct wm_window *w;
	wl_list_for_each(ws, &s->workspaces, link)
		wl_list_for_each(w, &ws->windows, link)
			if (w->id == id)
				return w;
	return nullptr;
}

/* ---------- ops ---------- */

static void op_version(int fd)
{
	cJSON *r = cJSON_CreateObject();
	cJSON_AddNumberToObject(r, "protocol", 1);
	cJSON_AddStringToObject(r, "lazydesk", LAZYDESK_VERSION);
	cJSON_AddStringToObject(r, "backend", "lazydesk-wm");
	ipc_send_ok(fd, r);
}

static void op_windows_list(struct wm_server *s, int fd)
{
	cJSON *arr = cJSON_CreateArray();
	struct wm_workspace *ws;
	struct wm_window *w;
	wl_list_for_each(ws, &s->workspaces, link) {
		wl_list_for_each(w, &ws->windows, link) {
			cJSON *o = cJSON_CreateObject();
			cJSON_AddNumberToObject(o, "id", w->id);
			cJSON_AddStringToObject(o, "app_id", w->app_id);
			cJSON_AddStringToObject(o, "title", w->title);
			cJSON_AddNumberToObject(o, "workspace", ws->number);
			cJSON_AddStringToObject(o, "workspace_name", ws->name);
			cJSON_AddBoolToObject(o, "focused", s->focused == w);
			cJSON_AddBoolToObject(o, "mapped", w->mapped);
			cJSON_AddItemToArray(arr, o);
		}
	}
	ipc_send_ok(fd, arr);
}

static void op_workspaces_list(struct wm_server *s, int fd)
{
	cJSON *arr = cJSON_CreateArray();
	struct wm_workspace *ws;
	wl_list_for_each(ws, &s->workspaces, link) {
		cJSON *o = cJSON_CreateObject();
		cJSON_AddNumberToObject(o, "number", ws->number);
		cJSON_AddStringToObject(o, "name", ws->name);
		cJSON_AddBoolToObject(o, "archetype", ws->archetype);
		cJSON_AddBoolToObject(o, "focused",
			ws->output && ws->output->active == ws);
		cJSON_AddNumberToObject(o, "windows", wl_list_length(&ws->windows));
		cJSON_AddItemToArray(arr, o);
	}
	ipc_send_ok(fd, arr);
}

static void handle_line(struct ipc_client *c, char *line)
{
	struct wm_server *s = c->server;
	cJSON *req = cJSON_Parse(line);
	if (!req) {
		ipc_send_err(c->fd, nullptr, "malformed request", "line is not valid JSON");
		return;
	}
	const char *op = cJSON_GetStringValue(cJSON_GetObjectItem(req, "op"));
	if (!op) {
		ipc_send_err(c->fd, nullptr, "missing op", nullptr);
		cJSON_Delete(req);
		return;
	}

	if (!strcmp(op, "version")) op_version(c->fd);
	else if (!strcmp(op, "subscribe")) {
		c->subscribed = true;
		ipc_send_ok(c->fd, nullptr);
	}
	else if (!strcmp(op, "windows.list")) op_windows_list(s, c->fd);
	else if (!strcmp(op, "workspaces.list")) op_workspaces_list(s, c->fd);
	else if (!strcmp(op, "workspace.focus")) {
		cJSON *jn = cJSON_GetObjectItem(req, "name");
		struct wm_workspace *ws = nullptr;
		if (cJSON_IsString(jn))
			ws = wm_workspace_get(s, jn->valuestring);
		else if (cJSON_IsNumber(jn))
			ws = wm_workspace_by_number(s, (int)jn->valuedouble);
		else {
			/* default: first archetype (agent) */
			ws = wm_workspace_get(s, "agent");
		}
		if (ws)
			wm_workspace_focus(s, ws), ipc_send_ok(c->fd, nullptr);
		else
			ipc_send_err(c->fd, op, "no such workspace", nullptr);
	}
	else if (!strcmp(op, "window.focus")) {
		cJSON *ji = cJSON_GetObjectItem(req, "id");
		struct wm_window *w = cJSON_IsNumber(ji) ? find_window(s, (uint32_t)ji->valuedouble) : nullptr;
		if (w) {
			wm_workspace_focus(s, w->workspace);
			wm_focus_window(w);
			ipc_send_ok(c->fd, nullptr);
		} else
			ipc_send_err(c->fd, op, "no such window", nullptr);
	}
	else if (!strcmp(op, "window.close") || !strcmp(op, "window.kill")) {
		cJSON *ji = cJSON_GetObjectItem(req, "id");
		struct wm_window *w = cJSON_IsNumber(ji) ? find_window(s, (uint32_t)ji->valuedouble) : s->focused;
		if (w) {
			wm_close_window(w);
			ipc_send_ok(c->fd, nullptr);
		} else
			ipc_send_err(c->fd, op, "no such window", nullptr);
	}
	else if (!strcmp(op, "window.snap")) {
		cJSON *ji = cJSON_GetObjectItem(req, "id");
		const char *edge = cJSON_GetStringValue(cJSON_GetObjectItem(req, "edge"));
		struct wm_window *w = cJSON_IsNumber(ji) ? find_window(s, (uint32_t)ji->valuedouble) : s->focused;
		wm_mode m = WM_MODE_MAXIMIZED;
		if (edge) {
			if (!strcmp(edge, "left")) m = WM_MODE_SNAP_L;
			else if (!strcmp(edge, "right")) m = WM_MODE_SNAP_R;
			else if (!strcmp(edge, "top") || !strcmp(edge, "up")) m = WM_MODE_SNAP_T;
			else if (!strcmp(edge, "bottom") || !strcmp(edge, "down")) m = WM_MODE_SNAP_B;
			else if (!strcmp(edge, "restore")) m = WM_MODE_MAXIMIZED;
		}
		if (w) {
			wm_window_set_snap(w, m);
			ipc_send_ok(c->fd, nullptr);
		} else
			ipc_send_err(c->fd, op, "no such window", nullptr);
	}
	else if (!strcmp(op, "window.fullscreen")) {
		if (s->focused) {
			wm_window_toggle_fullscreen(s->focused);
			ipc_send_ok(c->fd, nullptr);
		} else
			ipc_send_err(c->fd, op, "no focused window", nullptr);
	}
	else if (!strcmp(op, "spawn")) {
		const char *cmd = cJSON_GetStringValue(cJSON_GetObjectItem(req, "command"));
		const char *arch = cJSON_GetStringValue(cJSON_GetObjectItem(req, "archetype"));
		if (cmd && cmd[0]) {
			wm_spawn(s, cmd, arch);
			ipc_send_ok(c->fd, nullptr);
		} else
			ipc_send_err(c->fd, op, "missing command", nullptr);
	}
	else if (!strcmp(op, "config.reload")) {
		wm_reload_config(s);
		ipc_send_ok(c->fd, nullptr);
	}
	else if (!strcmp(op, "session.quit")) {
		ipc_send_ok(c->fd, nullptr);
		wl_display_terminate(s->display);
	}
	else
		ipc_send_err(c->fd, op, "unknown op", nullptr);

	cJSON_Delete(req);
}

/* ---------- socket plumbing ---------- */

static void client_read(struct ipc_client *c)
{
	char chunk[4096];
	ssize_t n = read(c->fd, chunk, sizeof chunk);
	if (n <= 0) {
		wl_event_source_remove(c->source);
		for (size_t i = 0; i < nclients; i++)
			if (clients[i] == c) clients[i] = nullptr;
		close(c->fd);
		free(c);
		return;
	}
	for (ssize_t i = 0; i < n; i++) {
		if (chunk[i] == '\n') {
			c->buf[c->len] = '\0';
			if (c->len)
				handle_line(c, c->buf);
			c->len = 0;
		} else if (c->len + 1 < sizeof c->buf) {
			c->buf[c->len++] = chunk[i];
		}
	}
}

static int on_readable(int fd, uint32_t mask, void *data)
{
	struct ipc_client *c = data;
	(void)fd;
	if (mask & WL_EVENT_READABLE)
		client_read(c);
	return 1;
}

static int on_accept(int fd, uint32_t mask, void *data)
{
	struct wm_server *s = data;
	(void)mask;
	int cfd = accept(fd, nullptr, nullptr);
	if (cfd < 0)
		return 1;
	size_t slot = 0;
	for (; slot < MAX_CLIENTS; slot++)
		if (!clients[slot])
			break;
	if (slot == MAX_CLIENTS) {
		close(cfd);
		return 1;
	}
	fcntl(cfd, F_SETFL, O_NONBLOCK);
	struct ipc_client *c = calloc(1, sizeof *c);
	c->fd = cfd;
	c->server = s;
	clients[slot] = c;
	nclients = (slot + 1 > nclients) ? slot + 1 : nclients;
	c->source = wl_event_loop_add_fd(s->loop, cfd, WL_EVENT_READABLE, on_readable, c);
	return 1;
}

void wm_ipc_init(struct wm_server *s)
{
	g_server = s;
	const char *rtd = getenv("XDG_RUNTIME_DIR");
	if (!rtd)
		rtd = "/tmp";
	char dir[512], path[512];
	snprintf(dir, sizeof dir, "%s/lazydesk", rtd);
	mkdir(dir, 0700);
	snprintf(path, sizeof path, "%s/ipc.sock", dir);

	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	struct sockaddr_un addr = { 0 };
	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, sizeof addr.sun_path, "%s", path);
	unlink(path);
	umask(0077);
	if (bind(fd, (struct sockaddr *)&addr, sizeof addr) < 0) {
		wlr_log(WLR_ERROR, "ipc bind %s: %s", path, strerror(errno));
		return;
	}
	listen(fd, 16);
	wl_event_loop_add_fd(s->loop, fd, WL_EVENT_READABLE, on_accept, s);
	wlr_log(WLR_INFO, "lazydesk-wm: ipc on %s", path);
}
