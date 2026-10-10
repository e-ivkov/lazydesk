/* wm/wm.h — lazydesk-wm internal structures. */
#ifndef LAZYDESK_WM_H
#define LAZYDESK_WM_H

#include <stdint.h>
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <xkbcommon/xkbcommon.h>

#include "config.h"

#define wl_list_append(list, item) wl_list_insert((list)->prev, item)

typedef enum {
	WM_MODE_MAXIMIZED = 0,
	WM_MODE_SNAP_L, WM_MODE_SNAP_R, WM_MODE_SNAP_T, WM_MODE_SNAP_B,
	WM_MODE_FLOAT,
	WM_MODE_FULLSCREEN,
} wm_mode;

struct wm_server;

struct wm_window {
	uint32_t id;
	struct wlr_xdg_surface *xdg;
	struct wlr_xdg_toplevel *toplevel;
	struct wlr_scene_tree *tree;
	struct wm_server *server;
	struct wl_listener map, unmap, destroy, commit;
	struct wl_listener req_maximize, req_fullscreen, req_move, req_resize;
	struct wl_listener toplevel_destroy;
	bool mapped;
	wm_mode mode;
	struct wlr_box last_float;      /* floating geometry */
	char app_id[128];
	char title[256];
	struct wm_workspace *workspace;
	struct wl_list link;            /* workspace->windows stack */
};

struct wm_workspace {
	char name[32];                  /* "" for ad-hoc */
	int number;                     /* 1..; ad-hoc gets next free */
	bool archetype;
	struct wm_output *output;
	struct wl_list windows;         /* stack: focused window = last */
	struct wl_list link;
	struct wlr_scene_tree *tree;    /* visibility via enable/disable */
};

struct wm_output {
	struct wlr_output *wlr;
	struct wlr_scene_output *scene_output;
	struct wm_server *server;
	struct wl_listener frame, destroy;
	struct wl_list link;
	struct wm_workspace *active;
};

struct wm_keyboards {
	struct wl_list link;
	struct wlr_keyboard *wlr;
	struct wm_server *server;
	struct wl_listener destroy, key, modifiers;
};

struct wm_server {
	struct wl_display *display;
	struct wl_event_loop *loop;
	struct wlr_backend *backend;
	struct wlr_allocator *allocator;
	struct wlr_renderer *renderer;
	struct wlr_scene *scene;
	struct wlr_output_layout *layout;
	struct wlr_scene_output_layout *scene_layout;

	struct wlr_xdg_shell *xdg_shell;
	struct wl_listener new_xdg_surface;

	struct wlr_seat *seat;
	struct wlr_cursor *cursor;
	struct wlr_xcursor_manager *xcursor;
	struct wl_listener cursor_motion, cursor_motion_abs, cursor_button, cursor_axis;
	struct wl_listener new_output;
	struct wl_listener new_input;
	struct wl_list keyboards;

	struct wl_list outputs;
	struct wl_list workspaces;
	struct wm_window *focused;
	uint32_t next_window_id;

	/* spawn lineage: pid -> archetype */
	struct { long pid; char archetype[32]; } lineage[64];
	size_t nlineage;

	ld_config cfg;
	char cfg_path[512];
};

/* windows.c */
void wm_xdg_init(struct wm_server *s);
void wm_focus_window(struct wm_window *w);
void wm_close_window(struct wm_window *w);
void wm_cycle_focus(struct wm_server *s, int dir);
void wm_window_apply_mode(struct wm_window *w);
void wm_window_set_snap(struct wm_window *w, wm_mode m);
void wm_window_toggle_fullscreen(struct wm_window *w);
struct wlr_box wm_usable_area(struct wm_output *o);
struct wm_output *wm_output_at_cursor(struct wm_server *s);

/* workspace.c */
void wm_workspaces_init(struct wm_server *s, const char *cfg_path);
struct wm_workspace *wm_workspace_get(struct wm_server *s, const char *name);
struct wm_workspace *wm_workspace_by_number(struct wm_server *s, int n);
struct wm_workspace *wm_workspace_new_adhoc(struct wm_output *o);
void wm_workspace_focus(struct wm_server *s, struct wm_workspace *ws);
void wm_workspace_show(struct wm_workspace *ws, bool show);
void wm_window_assign(struct wm_server *s, struct wm_window *w, const char *app_id);
void wm_workspace_cleanup(struct wm_server *s);
struct wm_workspace *wm_output_active_ws(struct wm_output *o);

/* input.c */
void wm_input_init(struct wm_server *s);
void wm_handle_keybind(struct wm_server *s, uint32_t modifiers, xkb_keysym_t keysym);
void wm_run_action(struct wm_server *s, const char *action);
void wm_reload_config(struct wm_server *s);

/* ipc.c */
void wm_ipc_init(struct wm_server *s);
void wm_ipc_event(const char *fmt, ...);

/* spawn (input.c: needs env) */
void wm_spawn(struct wm_server *s, const char *cmd, const char *archetype);

#endif
