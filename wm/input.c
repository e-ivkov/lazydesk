/* wm/input.c — seat, keyboards, cursor, keymap dispatch from the action registry. */
#define _POSIX_C_SOURCE 200809L

#include <strings.h>

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <wlr/util/log.h>

#include "wm.h"
#include "lazydesk-build.h"
#include "bindings.h"

/* ---------- spawn ---------- */

void wm_spawn(struct wm_server *s, const char *cmd, const char *archetype)
{
	pid_t pid = fork();
	if (pid < 0)
		return;
	if (pid == 0) {
		setenv("LAZYDESK", "1", 1);
		if (archetype && archetype[0])
			setenv("LAZYDESK_ARCHETYPE", archetype, 1);
		char full[4096];
		snprintf(full, sizeof full, "exec %s", cmd);
		execl("/bin/sh", "sh", "-c", full, (char *)nullptr);
		_exit(127);
	}
	if (archetype && archetype[0] && s->nlineage < 64) {
		s->lineage[s->nlineage].pid = pid;
		snprintf(s->lineage[s->nlineage].archetype, 32, "%s", archetype);
		s->nlineage++;
	}
	wm_ipc_event("{\"event\":\"session.spawn-result\",\"pid\":%ld,\"archetype\":\"%s\"}",
	             (long)pid, archetype ? archetype : "");
}

/* ---------- action dispatch ---------- */

static void spawn_archetype(struct wm_server *s, const char *id, bool here)
{
	const ld_cfg_archetype *a = ld_config_archetype(&s->cfg, id);
	if (!a)
		return;
	if (here) {
		/* spawn without archetype routing: plain command on active ws */
		wm_spawn(s, a->command, nullptr);
	} else {
		wm_spawn(s, a->command, id);
	}
}

/* focus-or-spawn: used by harness.summon and cockpit/desk/files opens */
static void focus_or_spawn(struct wm_server *s, const char *id)
{
	struct wm_workspace *ws = wm_workspace_get(s, id);
	if (!ws) {
		spawn_archetype(s, id, false);
		return;
	}
	/* window on it? */
	struct wm_window *w;
	bool any = false;
	wl_list_for_each(w, &ws->windows, link)
		if (w->mapped) any = true;
	if (any)
		wm_workspace_focus(s, ws);
	else
		spawn_archetype(s, id, false);
}

void wm_reload_config(struct wm_server *s)
{
	ld_config cfg;
	char err[512];
	if (!ld_config_load(&cfg, s->cfg_path, err, sizeof err)) {
		wlr_log(WLR_ERROR, "config reload rejected: %s", err);
		wm_ipc_event("{\"event\":\"config.reloaded\",\"ok\":false,\"error\":\"%s\"}", err);
		return;
	}
	s->cfg = cfg;
	wlr_log(WLR_INFO, "config reloaded");
	wm_ipc_event("{\"event\":\"config.reloaded\",\"ok\":true}");
}

void wm_run_action(struct wm_server *s, const char *action)
{
	if (!strcmp(action, "window.cycle.next")) wm_cycle_focus(s, +1);
	else if (!strcmp(action, "window.cycle.prev")) wm_cycle_focus(s, -1);
	else if (!strcmp(action, "window.close") && s->focused) wm_close_window(s->focused);
	else if (!strcmp(action, "window.fullscreen") && s->focused)
		wm_window_toggle_fullscreen(s->focused);
	else if (!strcmp(action, "window.snap.left") && s->focused) wm_window_set_snap(s->focused, WM_MODE_SNAP_L);
	else if (!strcmp(action, "window.snap.right") && s->focused) wm_window_set_snap(s->focused, WM_MODE_SNAP_R);
	else if (!strcmp(action, "window.snap.up") && s->focused) wm_window_set_snap(s->focused, WM_MODE_SNAP_T);
	else if (!strcmp(action, "window.snap.down") && s->focused) wm_window_set_snap(s->focused, WM_MODE_SNAP_B);
	else if (!strcmp(action, "palette.open")) wm_spawn(s, "lazydesk-menu", nullptr);
	else if (!strcmp(action, "harness.summon")) focus_or_spawn(s, "agent");
	else if (!strcmp(action, "terminal.spawn")) spawn_archetype(s, "terminal", true);
	else if (!strcmp(action, "cockpit.open")) focus_or_spawn(s, "cockpit");
	else if (!strcmp(action, "settings.open")) focus_or_spawn(s, "desk");
	else if (!strcmp(action, "files.open")) focus_or_spawn(s, "files");
	else if (!strcmp(action, "workspace.new")) {
		struct wm_output *o = wm_output_at_cursor(s);
		if (o)
			wm_workspace_focus(s, wm_workspace_new_adhoc(o));
	}
	else if (!strcmp(action, "scratchpad.toggle"))
		wm_spawn(s, "ghostty +toggle-quick-terminal", nullptr);
	else if (!strcmp(action, "config.reload")) wm_reload_config(s);
	else if (!strcmp(action, "session.quit")) wl_display_terminate(s->display);
	else wlr_log(WLR_DEBUG, "unhandled action %s", action);
}

/* ---------- keymap: registry combos ---------- */

static bool combo_matches(const char *combo, uint32_t mods, xkb_keysym_t sym)
{
	/* parse "Super+Ctrl+Left" into mods mask + keysym */
	uint32_t want = 0;
	char buf[64];
	snprintf(buf, sizeof buf, "%s", combo);
	char *save = nullptr;
	char *key = nullptr;
	for (char *tok = strtok_r(buf, "+", &save); tok; tok = strtok_r(nullptr, "+", &save)) {
		if (!strncasecmp(tok, "super", 5)) want |= WLR_MODIFIER_LOGO;
		else if (!strncasecmp(tok, "ctrl", 4)) want |= WLR_MODIFIER_CTRL;
		else if (!strncasecmp(tok, "shift", 5)) want |= WLR_MODIFIER_SHIFT;
		else if (!strncasecmp(tok, "alt", 3)) want |= WLR_MODIFIER_ALT;
		else key = tok;
	}
	if (!key)
		return false;
	xkb_keysym_t want_sym = xkb_keysym_from_name(key, XKB_KEYSYM_CASE_INSENSITIVE);
	return want_sym == sym && want == mods;
}

void wm_handle_keybind(struct wm_server *s, uint32_t mods, xkb_keysym_t sym)
{
	size_t nb;
	const ld_binding *bs = ld_bindings(&nb);
	for (size_t i = 0; i < nb; i++)
		if (combo_matches(bs[i].combo, mods, sym)) {
			if (bs[i].kind == LD_ACT_CUSTOM || bs[i].kind == LD_ACT_EXEC) {
				/* map registry cmd "lazydeskctl X Y" to internal actions where
				 * they exist (native path), else spawn it */
				const char *cmd = bs[i].cmd;
				if (!strncmp(cmd, "lazydeskctl ", 12)) {
					/* "lazydeskctl window cycle next" -> action "window.cycle.next"-ish:
					 * dispatch via a small translation table */
					char action[128] = "";
					const char *p = cmd + 12;
					int off = 0;
					while (*p == ' ') p++;
					while (*p && off < 127) {
						if (*p == ' ' && (p[1] == '-' || p[1] == 0)) { }
						if (*p == ' ') action[off++] = '.';
						else if (*p != '-') action[off++] = *p;
						p++;
					}
					action[off] = 0;
					/* drop trailing verb forms handled natively below */
					wm_run_action(s, action);
					return;
				}
				wm_spawn(s, cmd, nullptr);
			}
			return;
		}

	/* numbered workspace binds come from the config model, not the registry */
	for (int n = 1; n <= 9; n++) {
		char combo[16];
		snprintf(combo, sizeof combo, "Super+%d", n);
		if (combo_matches(combo, mods, sym)) {
			struct wm_workspace *ws = wm_workspace_by_number(s, n);
			if (ws)
				wm_workspace_focus(s, ws);
			return;
		}
		snprintf(combo, sizeof combo, "Super+Shift+%d", n);
		if (combo_matches(combo, mods, sym) && s->focused) {
			struct wm_workspace *ws = wm_workspace_by_number(s, n);
			if (ws) {
				wl_list_remove(&s->focused->link);
				wl_list_append(&ws->windows, &s->focused->link);
				s->focused->workspace = ws;
				wlr_scene_node_reparent(&s->focused->tree->node, ws->tree);
				wm_workspace_show(ws, true);
				wm_window_apply_mode(s->focused);
			}
			return;
		}
	}
}

/* ---------- keyboards ---------- */

static void kb_key(struct wl_listener *li, void *data)
{
	struct wm_keyboards *k = wl_container_of(li, k, key);
	struct wlr_keyboard_key_event *ev = data;

	uint32_t mods = wlr_keyboard_get_modifiers(k->wlr);
	if (ev->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		xkb_keysym_t sym = xkb_state_key_get_one_sym(k->wlr->xkb_state, ev->keycode + 8);
		size_t nb;
		const ld_binding *bs = ld_bindings(&nb);
		bool matched = false;
		for (size_t i = 0; i < nb && !matched; i++)
			matched = combo_matches(bs[i].combo, mods, sym);
		for (int n = 1; n <= 9 && !matched; n++) {
			char combo[16];
			snprintf(combo, sizeof combo, "Super+%d", n);
			matched = combo_matches(combo, mods, sym);
		}
		if (matched) {
			wm_handle_keybind(k->server, mods, sym);
			return; /* consumed: do not forward */
		}
	}
	wlr_seat_keyboard_notify_key(k->server->seat, ev->time_msec, ev->keycode, ev->state);
}

static void kb_modifiers(struct wl_listener *li, void *data)
{
	struct wm_keyboards *k = wl_container_of(li, k, modifiers);
	wlr_seat_keyboard_notify_modifiers(k->server->seat, &k->wlr->modifiers);
	(void)data;
}

static void kb_destroy(struct wl_listener *li, void *data)
{
	struct wm_keyboards *k = wl_container_of(li, k, destroy);
	wl_list_remove(&k->link);
	wl_list_remove(&k->destroy.link);
	wl_list_remove(&k->key.link);
	wl_list_remove(&k->modifiers.link);
	free(k);
	(void)data;
}

static void seat_keyboard_enter_change(struct wl_listener *li, void *data);

static void handle_new_input(struct wl_listener *li, void *data)
{
	struct wm_server *s = wl_container_of(li, s, new_input);
	struct wlr_input_device *dev = data;

	switch (dev->type) {
	case WLR_INPUT_DEVICE_KEYBOARD: {
		struct wlr_keyboard *kb = wlr_keyboard_from_input_device(dev);
		struct xkb_rule_names rules = { .layout = "us" };
		struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
		struct xkb_keymap *keymap = xkb_keymap_new_from_names(ctx, &rules,
			XKB_KEYMAP_COMPILE_NO_FLAGS);
		wlr_keyboard_set_keymap(kb, keymap);
		xkb_keymap_unref(keymap);
		xkb_context_unref(ctx);

		struct wm_keyboards *k = calloc(1, sizeof *k);
		k->wlr = kb;
		k->server = s;
		wl_list_insert(&s->keyboards, &k->link);
		k->destroy.notify = kb_destroy;
		wl_signal_add(&dev->events.destroy, &k->destroy);
		k->key.notify = kb_key;
		wl_signal_add(&kb->events.key, &k->key);
		k->modifiers.notify = kb_modifiers;
		wl_signal_add(&kb->events.modifiers, &k->modifiers);
		wlr_seat_set_keyboard(s->seat, kb);
		break;
	}
	default:
		break;
	}
	uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
	if (!wl_list_empty(&s->keyboards))
		caps |= WL_SEAT_CAPABILITY_KEYBOARD;
	wlr_seat_set_capabilities(s->seat, caps);
}

/* ---------- cursor ---------- */

static void cursor_motion(struct wl_listener *li, void *data)
{
	struct wm_server *s = wl_container_of(li, s, cursor_motion);
	struct wlr_pointer_motion_event *ev = data;
	wlr_cursor_move(s->cursor, ev->pointer ? &ev->pointer->base : nullptr,
	                ev->delta_x, ev->delta_y);
	wlr_cursor_set_xcursor(s->cursor, s->xcursor, "left_ptr");
	wlr_seat_pointer_notify_clear_focus(s->seat);
	(void)data;
}

static void cursor_motion_abs(struct wl_listener *li, void *data)
{
	struct wm_server *s = wl_container_of(li, s, cursor_motion_abs);
	struct wlr_pointer_motion_absolute_event *ev = data;
	wlr_cursor_warp_absolute(s->cursor, nullptr, ev->x, ev->y);
	wlr_seat_pointer_notify_clear_focus(s->seat);
}

static void cursor_button(struct wl_listener *li, void *data)
{
	struct wm_server *s = wl_container_of(li, s, cursor_button);
	/* click focuses the top window of the active workspace (P2: surface picking) */
	if (s->focused)
		wm_focus_window(s->focused);
	(void)data;
}

static void cursor_axis(struct wl_listener *li, void *data)
{
	(void)li; (void)data;
}

void wm_input_init(struct wm_server *s)
{
	s->seat = wlr_seat_create(s->display, "seat0");
	wl_list_init(&s->keyboards);

	s->cursor = wlr_cursor_create();
	wlr_cursor_attach_output_layout(s->cursor, s->layout);
	s->xcursor = wlr_xcursor_manager_create(nullptr, 24);
	wlr_xcursor_manager_load(s->xcursor, 1);

	s->cursor_motion.notify = cursor_motion;
	wl_signal_add(&s->cursor->events.motion, &s->cursor_motion);
	s->cursor_motion_abs.notify = cursor_motion_abs;
	wl_signal_add(&s->cursor->events.motion_absolute, &s->cursor_motion_abs);
	s->cursor_button.notify = cursor_button;
	wl_signal_add(&s->cursor->events.button, &s->cursor_button);
	s->cursor_axis.notify = cursor_axis;
	wl_signal_add(&s->cursor->events.axis, &s->cursor_axis);

	s->new_input.notify = handle_new_input;
	wl_signal_add(&s->backend->events.new_input, &s->new_input);
}
