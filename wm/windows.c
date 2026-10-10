/* wm/windows.c — xdg-shell windows: map/maximize/float/snap, focus stack. */
#include <stdlib.h>
#include <string.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/util/box.h>
#include <wlr/util/log.h>

#include "wm.h"

static void xdg_toplevel_handler(struct wl_listener *listener, void *data);

/* ---------- geometry ---------- */

struct wlr_box wm_usable_area(struct wm_output *o)
{
	struct wlr_box box;
	wlr_output_layout_get_box(o->server->layout, o->wlr, &box);
	/* layer-shell reserves arrive in P2; usable == output box for now */
	return box;
}

static struct wlr_box snap_box(struct wlr_box u, wm_mode m)
{
	struct wlr_box b = u;
	switch (m) {
	case WM_MODE_SNAP_L: b.width = u.width / 2; break;
	case WM_MODE_SNAP_R: b.x = u.x + u.width / 2; b.width = u.width - u.width / 2; break;
	case WM_MODE_SNAP_T: b.height = u.height / 2; break;
	case WM_MODE_SNAP_B: b.y = u.y + u.height / 2; b.height = u.height - u.height / 2; break;
	default: break;
	}
	return b;
}

void wm_window_apply_mode(struct wm_window *w)
{
	if (!w->mapped)
		return;
	struct wm_output *o = w->workspace ? w->workspace->output : nullptr;
	if (!o)
		return;
	struct wlr_box u = wm_usable_area(o);
	struct wlr_box b = u;
	switch (w->mode) {
	case WM_MODE_MAXIMIZED:
		wlr_xdg_toplevel_set_maximized(w->toplevel, true);
		wlr_xdg_toplevel_set_size(w->toplevel, u.width, u.height);
		w->tree->node.x = u.x;
		w->tree->node.y = u.y;
		break;
	case WM_MODE_FULLSCREEN: {
		struct wlr_box full = u; /* P2: full output incl. reserves */
		wlr_xdg_toplevel_set_fullscreen(w->toplevel, true);
		wlr_xdg_toplevel_set_size(w->toplevel, full.width, full.height);
		w->tree->node.x = full.x;
		w->tree->node.y = full.y;
		break;
	}
	case WM_MODE_FLOAT: {
		b = w->last_float;
		wlr_xdg_toplevel_set_maximized(w->toplevel, false);
		wlr_xdg_toplevel_set_size(w->toplevel, b.width, b.height);
		w->tree->node.x = b.x;
		w->tree->node.y = b.y;
		break;
	}
	default: /* snaps */
		wlr_xdg_toplevel_set_maximized(w->toplevel, false);
		b = snap_box(u, w->mode);
		wlr_xdg_toplevel_set_size(w->toplevel, b.width, b.height);
		w->tree->node.x = b.x;
		w->tree->node.y = b.y;
		break;
	}
	wlr_scene_node_raise_to_top(&w->tree->node);
}

void wm_window_set_snap(struct wm_window *w, wm_mode m)
{
	if (m == WM_MODE_MAXIMIZED) {
		w->mode = WM_MODE_MAXIMIZED;
	} else if (w->mode == WM_MODE_FLOAT && m != WM_MODE_FLOAT) {
		w->mode = m; /* snapping from float */
	} else {
		w->mode = (w->mode == m) ? WM_MODE_MAXIMIZED : m; /* same snap = restore */
	}
	wm_window_apply_mode(w);
}

void wm_window_toggle_fullscreen(struct wm_window *w)
{
	w->mode = (w->mode == WM_MODE_FULLSCREEN) ? WM_MODE_MAXIMIZED : WM_MODE_FULLSCREEN;
	wm_window_apply_mode(w);
}

/* ---------- focus ---------- */

void wm_focus_window(struct wm_window *w)
{
	if (!w || !w->mapped)
		return;
	struct wm_server *s = w->server;
	struct wlr_seat *seat = s->seat;
	struct wlr_surface *prev = seat->keyboard_state.focused_surface;

	wlr_scene_node_raise_to_top(&w->tree->node);
	/* move to top of workspace stack */
	wl_list_remove(&w->link);
	wl_list_append(&w->workspace->windows, &w->link);
	s->focused = w;

	if (prev != w->xdg->surface)
		wlr_seat_keyboard_notify_enter(seat, w->xdg->surface, nullptr, 0, nullptr);
	wm_ipc_event("{\"event\":\"window.focus\",\"id\":%u}", w->id);
}

void wm_cycle_focus(struct wm_server *s, int dir)
{
	if (!s->focused || wl_list_length(&s->focused->workspace->windows) < 2)
		return;
	struct wl_list *cur = &s->focused->link;
	struct wl_list *next = (dir > 0) ? cur->prev : cur->next;
	if (next == &s->focused->workspace->windows)
		next = (dir > 0) ? next->prev : next->next;
	if (next == &s->focused->workspace->windows)
		return;
	struct wm_window *nw = nullptr;
	wl_list_for_each(nw, &s->focused->workspace->windows, link)
		if (&nw->link == next)
			break;
	if (nw)
		wm_focus_window(nw);
}

void wm_close_window(struct wm_window *w)
{
	wlr_xdg_toplevel_send_close(w->toplevel);
}

/* ---------- workspace assignment ---------- */

static bool is_dialog(struct wm_window *w)
{
	(void)w;
	return false; /* P2: xdg toplevel type + transient parent heuristic */
}

/* ---------- listeners ---------- */

static void handle_map(struct wl_listener *li, void *data)
{
	struct wm_window *w = wl_container_of(li, w, map);
	w->mapped = true;
	wlr_scene_node_set_enabled(&w->tree->node, true);

	wm_window_assign(w->server, w, w->app_id);

	/* lineage: pid of the client */
	struct wl_client *client = wl_resource_get_client(w->xdg->resource);
	pid_t cred_pid = 0;
	uid_t cred_uid = 0;
	gid_t cred_gid = 0;
	wl_client_get_credentials(client, &cred_pid, &cred_uid, &cred_gid);
	long pid = (long)cred_pid;
	const char *arch = nullptr;
	for (size_t i = 0; i < w->server->nlineage; i++)
		if (w->server->lineage[i].pid == pid)
			arch = w->server->lineage[i].archetype;
	if (arch) {
		struct wm_workspace *ws = wm_workspace_get(w->server, arch);
		if (ws) {
			wl_list_remove(&w->link);
			wl_list_append(&ws->windows, &w->link);
			w->workspace = ws;
			wm_workspace_show(ws, true);
		}
	}

	if (is_dialog(w)) {
		w->mode = WM_MODE_FLOAT;
		struct wlr_box u = wm_usable_area(w->workspace ? w->workspace->output
		                                      : wm_output_at_cursor(w->server));
		struct wlr_box nat = w->xdg->current.geometry;
		w->last_float.x = u.x + (u.width - nat.width) / 2;
		w->last_float.y = u.y + (u.height - nat.height) / 2;
		w->last_float.width = nat.width > 0 ? nat.width : 640;
		w->last_float.height = nat.height > 0 ? nat.height : 480;
	} else {
		w->mode = WM_MODE_MAXIMIZED;
	}
	wm_window_apply_mode(w);
	/* only visible if its workspace is the active one */
	wm_workspace_show(w->workspace, true);
	wm_focus_window(w);
	wm_ipc_event("{\"event\":\"window.map\",\"id\":%u,\"app_id\":\"%s\"}", w->id, w->app_id);
	(void)data;
}

static void handle_unmap(struct wl_listener *li, void *data)
{
	struct wm_window *w = wl_container_of(li, w, unmap);
	w->mapped = false;
	wlr_scene_node_set_enabled(&w->tree->node, false);
	if (w->server->focused == w)
		w->server->focused = nullptr;
	wm_ipc_event("{\"event\":\"window.unmap\",\"id\":%u}", w->id);
	(void)data;
}

static void handle_destroy(struct wl_listener *li, void *data)
{
	struct wm_window *w = wl_container_of(li, w, destroy);
	wl_list_remove(&w->link);
	wl_list_remove(&w->map.link);
	wl_list_remove(&w->unmap.link);
	wl_list_remove(&w->destroy.link);
	wl_list_remove(&w->commit.link);
	free(w);
	(void)data;
}

static void attach_toplevel_listeners(struct wm_window *w);

static void handle_commit(struct wl_listener *li, void *data)
{
	struct wm_window *w = wl_container_of(li, w, commit);
	if (w->xdg->initial_commit) {
		/* role is assigned by now: resolve the toplevel lazily */
		if (!w->toplevel) {
			w->toplevel = wlr_xdg_toplevel_try_from_wlr_surface(w->xdg->surface);
			if (w->toplevel) {
				attach_toplevel_listeners(w);
				if (w->toplevel->app_id)
					snprintf(w->app_id, sizeof w->app_id, "%s", w->toplevel->app_id);
				if (w->toplevel->title)
					snprintf(w->title, sizeof w->title, "%s", w->toplevel->title);
			}
		}
		if (w->toplevel) {
			if (!w->tree) {
				w->tree = wlr_scene_xdg_surface_create(&w->server->scene->tree, w->xdg);
				wlr_scene_node_set_enabled(&w->tree->node, false);
			}
			/* first commit: handshake with a natural-size configure so the
			 * client commits a buffer and maps; placement happens at map */
			wlr_xdg_toplevel_set_size(w->toplevel, 0, 0);
		}
	}
	(void)data;
}

static void handle_req_maximize(struct wl_listener *li, void *data)
{
	struct wm_window *w = wl_container_of(li, w, req_maximize);
	struct wlr_xdg_toplevel_requested *req = data;
	w->mode = req->maximized ? WM_MODE_MAXIMIZED : WM_MODE_FLOAT;
	wm_window_apply_mode(w);
}

static void handle_req_fullscreen(struct wl_listener *li, void *data)
{
	struct wm_window *w = wl_container_of(li, w, req_fullscreen);
	struct wlr_xdg_toplevel_requested *req = data;
	w->mode = req->fullscreen ? WM_MODE_FULLSCREEN : WM_MODE_MAXIMIZED;
	wm_window_apply_mode(w);
}

static void handle_req_move(struct wl_listener *li, void *data)
{
	(void)li; (void)data; /* pointer move: P2 (float only) */
}

static void handle_req_resize(struct wl_listener *li, void *data)
{
	(void)li; (void)data; /* pointer resize: P2 (float only) */
}

static void handle_new_xdg_surface(struct wl_listener *li, void *data)
{
	struct wm_server *s = wl_container_of(li, s, new_xdg_surface);
	struct wlr_xdg_surface *xdg = data;

	/* attach at surface level: the toplevel role does not exist yet
	 * (dwl pattern); resolve it at the initial commit */
	struct wm_window *w = calloc(1, sizeof *w);
	w->id = ++s->next_window_id;
	w->xdg = xdg;
	w->server = s;

	/* place on the output's active workspace for now; assign() may move it */
	struct wm_output *o = wm_output_at_cursor(s);
	struct wm_workspace *ws = o ? o->active : nullptr;
	if (!ws && !wl_list_empty(&s->workspaces)) {
		struct wm_workspace *any = nullptr;
		wl_list_for_each(any, &s->workspaces, link)
			{ ws = any; break; }
	}
	if (!ws) {
		free(w);
		return;
	}
	w->workspace = ws;
	wl_list_insert(&ws->windows, &w->link);

	w->map.notify = handle_map;
	wl_signal_add(&xdg->surface->events.map, &w->map);
	w->unmap.notify = handle_unmap;
	wl_signal_add(&xdg->surface->events.unmap, &w->unmap);
	w->destroy.notify = handle_destroy;
	wl_signal_add(&xdg->events.destroy, &w->destroy);
	w->commit.notify = handle_commit;
	wl_signal_add(&xdg->surface->events.commit, &w->commit);
}

static void handle_toplevel_destroy(struct wl_listener *li, void *data)
{
	struct wm_window *w = wl_container_of(li, w, toplevel_destroy);
	wl_list_remove(&w->req_maximize.link);
	wl_list_remove(&w->req_fullscreen.link);
	wl_list_remove(&w->req_move.link);
	wl_list_remove(&w->req_resize.link);
	wl_list_remove(&w->toplevel_destroy.link);
	w->toplevel = nullptr;
	(void)data;
}

static void attach_toplevel_listeners(struct wm_window *w)
{
	w->req_maximize.notify = handle_req_maximize;
	wl_signal_add(&w->toplevel->events.request_maximize, &w->req_maximize);
	w->req_fullscreen.notify = handle_req_fullscreen;
	wl_signal_add(&w->toplevel->events.request_fullscreen, &w->req_fullscreen);
	w->req_move.notify = handle_req_move;
	wl_signal_add(&w->toplevel->events.request_move, &w->req_move);
	w->req_resize.notify = handle_req_resize;
	wl_signal_add(&w->toplevel->events.request_resize, &w->req_resize);
	w->toplevel_destroy.notify = handle_toplevel_destroy;
	wl_signal_add(&w->toplevel->events.destroy, &w->toplevel_destroy);
}

void wm_xdg_init(struct wm_server *s)
{
	s->xdg_shell = wlr_xdg_shell_create(s->display, 2);
	s->new_xdg_surface.notify = handle_new_xdg_surface;
	wl_signal_add(&s->xdg_shell->events.new_surface, &s->new_xdg_surface);
}
