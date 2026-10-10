/* wm/main.c — lazydesk-wm: the native compositor. */
#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/util/log.h>

#include "lazydesk-build.h"
#include "lazydesk.h"
#include "wm.h"

static void reap(int sig)
{
	(void)sig;
	while (waitpid(-1, nullptr, WNOHANG) > 0)
		;
}

static void output_frame(struct wl_listener *li, void *data)
{
	struct wm_output *o = wl_container_of(li, o, frame);
	wlr_scene_output_commit(o->scene_output, NULL);
	(void)data;
}

static void output_destroy(struct wl_listener *li, void *data)
{
	struct wm_output *o = wl_container_of(li, o, destroy);
	wl_list_remove(&o->link);
	wl_list_remove(&o->frame.link);
	wl_list_remove(&o->destroy.link);
	free(o);
	(void)data;
}

/* attach the archetype workspaces to an output and create ad-hoc slot 1 */
static void output_attach_workspaces(struct wm_server *s, struct wm_output *o)
{
	bool first = wl_list_empty(&s->workspaces);
	struct wm_workspace *first_ws = nullptr;
	for (size_t i = 0; i < s->cfg.narchetypes; i++) {
		const ld_cfg_archetype *a = &s->cfg.archetypes[i];
		if (!a->number)
			continue;
		struct wm_workspace *ws = calloc(1, sizeof *ws);
		snprintf(ws->name, sizeof ws->name, "%s", a->id);
		ws->number = a->number;
		ws->archetype = true;
		ws->output = o;
		wl_list_init(&ws->windows);
		ws->tree = wlr_scene_tree_create(&s->scene->tree);
		wlr_scene_node_set_enabled(&ws->tree->node, false);
		wl_list_append(&s->workspaces, &ws->link);
		if (!first_ws)
			first_ws = ws;
	}
	o->active = first_ws;
	if (o->active)
		wlr_scene_node_set_enabled(&o->active->tree->node, true);
	(void)first;
}

static void server_new_output(struct wl_listener *li, void *data)
{
	struct wm_server *s = wl_container_of(li, s, new_output);
	struct wlr_output *w = data;

	wlr_output_init_render(w, s->allocator, s->renderer);

	struct wlr_output_state state;
	wlr_output_state_init(&state);
	wlr_output_state_set_mode(&state, wlr_output_preferred_mode(w));
	wlr_output_state_set_enabled(&state, true);
	wlr_output_commit_state(w, &state);
	wlr_output_state_finish(&state);

	wlr_output_layout_add_auto(s->layout, w);

	struct wm_output *o = calloc(1, sizeof *o);
	o->wlr = w;
	o->server = s;
	o->scene_output = wlr_scene_output_create(s->scene, w);
	o->frame.notify = output_frame;
	wl_signal_add(&w->events.frame, &o->frame);
	o->destroy.notify = output_destroy;
	wl_signal_add(&w->events.destroy, &o->destroy);
	wl_list_insert(&s->outputs, &o->link);

	output_attach_workspaces(s, o);
	wlr_log(WLR_INFO, "lazydesk-wm: output %s (%ux%u)", w->name, w->width, w->height);
}

static void server_new_input(struct wl_listener *li, void *data);

int main(void)
{
	wlr_log_init(WLR_INFO, NULL);
	wlr_log(WLR_INFO, "lazydesk-wm %s", LAZYDESK_VERSION);
	{ struct sigaction sa = { 0 }; sa.sa_handler = reap; sa.sa_flags = SA_RESTART | SA_NOCLDSTOP; sigaction(SIGCHLD, &sa, nullptr); }

	struct wm_server s = { 0 };
	wl_list_init(&s.outputs);
	wl_list_init(&s.workspaces);

	s.display = wl_display_create();
	if (!s.display)
		return 1;
	s.loop = wl_display_get_event_loop(s.display);

	s.backend = wlr_backend_autocreate(s.loop, NULL);
	if (!s.backend) {
		wlr_log(WLR_ERROR, "backend failed (need a seat or WLR_BACKENDS=wayland/headless)");
		return 1;
	}

	s.renderer = wlr_renderer_autocreate(s.backend);
	s.allocator = wlr_allocator_autocreate(s.backend, s.renderer);
	if (!s.renderer || !s.allocator)
		return 1;
	wlr_renderer_init_wl_display(s.renderer, s.display);

	wlr_compositor_create(s.display, 5, s.renderer);
	wlr_subcompositor_create(s.display);
	wlr_data_device_manager_create(s.display);

	s.layout = wlr_output_layout_create(s.display);
	s.scene = wlr_scene_create();
	s.scene_layout = wlr_scene_attach_output_layout(s.scene, s.layout);

	/* config: user config, else shipped template */
	{
		const char *home = getenv("HOME");
		char path[512];
		FILE *f;
		snprintf(path, sizeof path, "%s/.config/lazydesk/lazydesk.toml", home);
		if (!(f = fopen(path, "r"))) {
			const char *dd = ld_datadir();
			snprintf(path, sizeof path, "%s/config/lazydesk.toml", dd);
			if (!(f = fopen(path, "r"))) {
				snprintf(path, sizeof path, "%s/data/config/lazydesk.toml", dd);
			} else {
				fclose(f);
			}
		} else {
			fclose(f);
		}
		char err[512];
		if (ld_config_load(&s.cfg, path, err, sizeof err))
			wlr_log(WLR_INFO, "config %s loaded", path);
		else
			wlr_log(WLR_ERROR, "config %s: %s (archetypes degraded)", path, err);
		snprintf(s.cfg_path, sizeof s.cfg_path, "%s", path);
	}

	s.new_output.notify = server_new_output;
	wl_signal_add(&s.backend->events.new_output, &s.new_output);

	wm_xdg_init(&s);
	wm_input_init(&s);

	if (!wlr_backend_start(s.backend)) {
		wlr_log(WLR_ERROR, "backend start failed");
		return 1;
	}

	const char *socket_name = wl_display_add_socket_auto(s.display);
	if (!socket_name) {
		wlr_log(WLR_ERROR, "cannot add display socket");
		return 1;
	}
	setenv("WAYLAND_DISPLAY", socket_name, true);
	wlr_log(WLR_INFO, "lazydesk-wm: wayland display %s", socket_name);

	wm_ipc_init(&s);

	wlr_log(WLR_INFO, "lazydesk-wm: running");
	wl_display_run(s.display);

	wl_display_destroy(s.display);
	return 0;
}
