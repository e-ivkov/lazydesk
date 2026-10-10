/* wm/workspace.c — the workspaces engine: named archetypes + ad-hoc, per output. */
#include <stdlib.h>
#include <string.h>

#include <wlr/util/log.h>

#include "wm.h"

struct wm_workspace *wm_output_active_ws(struct wm_output *o)
{
	return o ? o->active : nullptr;
}

struct wm_workspace *wm_workspace_get(struct wm_server *s, const char *name)
{
	struct wm_workspace *ws;
	wl_list_for_each(ws, &s->workspaces, link)
		if (!strcmp(ws->name, name))
			return ws;
	return nullptr;
}

struct wm_workspace *wm_workspace_by_number(struct wm_server *s, int n)
{
	struct wm_workspace *ws;
	wl_list_for_each(ws, &s->workspaces, link)
		if (ws->number == n)
			return ws;
	return nullptr;
}

static struct wm_workspace *ws_new(struct wm_server *s, struct wm_output *o,
                                   const char *name, int number, bool archetype)
{
	struct wm_workspace *ws = calloc(1, sizeof *ws);
	snprintf(ws->name, sizeof ws->name, "%s", name);
	ws->number = number;
	ws->archetype = archetype;
	ws->output = o;
	wl_list_init(&ws->windows);
	ws->tree = wlr_scene_tree_create(&s->scene->tree);
	wlr_scene_node_set_enabled(&ws->tree->node, false);
	wl_list_append(&s->workspaces, &ws->link);
	return ws;
}

struct wm_workspace *wm_workspace_new_adhoc(struct wm_output *o)
{
	int n = 1;
	while (wm_workspace_by_number(o->server, n))
		n++;
	return ws_new(o->server, o, "", n, false);
}

void wm_workspace_show(struct wm_workspace *ws, bool show)
{
	if (!ws)
		return;
	bool is_active = ws->output && ws->output->active == ws;
	wlr_scene_node_set_enabled(&ws->tree->node, show && is_active);
}

void wm_workspace_focus(struct wm_server *s, struct wm_workspace *ws)
{
	if (!ws || !ws->output)
		return;
	struct wm_output *o = ws->output;
	if (o->active == ws)
		return;
	if (o->active)
		wlr_scene_node_set_enabled(&o->active->tree->node, false);
	o->active = ws;
	wlr_scene_node_set_enabled(&ws->tree->node, true);

	/* reparent window trees into the workspace tree */
	struct wm_window *w;
	wl_list_for_each(w, &ws->windows, link)
		wlr_scene_node_reparent(&w->tree->node, ws->tree);

	/* focus topmost mapped window (or clear focus) */
	struct wm_window *top = nullptr;
	wl_list_for_each(w, &ws->windows, link)
		if (w->mapped)
			top = w;
	if (top)
		wm_focus_window(top);
	else {
		s->focused = nullptr;
		wlr_seat_keyboard_notify_clear_focus(s->seat);
	}

	wm_ipc_event("{\"event\":\"workspace.focus\",\"number\":%d,\"name\":\"%s\"}",
	             ws->number, ws->name);
	wm_workspace_cleanup(s);
}

/* Empty ad-hoc workspaces disappear once unfocused; archetypes persist. */
void wm_workspace_cleanup(struct wm_server *s)
{
	struct wm_workspace *ws, *tmp;
	wl_list_for_each_safe(ws, tmp, &s->workspaces, link) {
		if (ws->archetype)
			continue;
		if (ws->output && ws->output->active == ws)
			continue;
		if (!wl_list_empty(&ws->windows))
			continue;
		wl_list_remove(&ws->link);
		wlr_scene_node_destroy(&ws->tree->node);
		free(ws);
	}
}

/* Assign a newly mapped window to a workspace by app-id matching rules. */
void wm_window_assign(struct wm_server *s, struct wm_window *w, const char *app_id)
{
	if (!app_id || !app_id[0])
		return;
	for (size_t i = 0; i < s->cfg.narchetypes; i++) {
		const ld_cfg_archetype *a = &s->cfg.archetypes[i];
		if (!a->number || !a->class_regex[0])
			continue;
		/* anchored substring/regex-lite: prefix match on our simple ids */
		const char *r = a->class_regex;
		bool match = strstr(app_id, r) != nullptr;
		if (match) {
			struct wm_workspace *ws = wm_workspace_by_number(s, a->number);
			if (ws && w->workspace != ws) {
				wl_list_remove(&w->link);
				wl_list_append(&ws->windows, &w->link);
				w->workspace = ws;
				wlr_scene_node_reparent(&w->tree->node, ws->tree);
			}
			return;
		}
	}
	/* no rule: stays where the surface first attached (active workspace) */
}

void wm_workspaces_init(struct wm_server *s, const char *cfg_path)
{
	snprintf(s->cfg_path, sizeof s->cfg_path, "%s", cfg_path);
	char err[512];
	if (!ld_config_load(&s->cfg, cfg_path, err, sizeof err))
		wlr_log(WLR_ERROR, "config load failed (%s); archetypes unavailable", err);

	}

struct wm_output *wm_output_at_cursor(struct wm_server *s)
{
	if (wl_list_empty(&s->outputs))
		return nullptr;
	if (s->cursor) {
		struct wlr_output *w = wlr_output_layout_output_at(s->layout, s->cursor->x, s->cursor->y);
		if (w) {
			struct wm_output *o;
			wl_list_for_each(o, &s->outputs, link)
				if (o->wlr == w)
					return o;
		}
	}
	struct wm_output *first = nullptr;
	wl_list_for_each(first, &s->outputs, link)
		break;
	return first;
}
