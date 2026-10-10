/* wm/main.c — lazydesk-wm: the native compositor (native-compositor change, task 1.1).
 *
 * Skeleton: wlroots display + autocreate backend + allocator/renderer + scene graph,
 * output layout, compositor basics. Runs nested (under any Wayland session) or
 * headless (WLR_BACKENDS=headless). Window engine/workspaces/keymap/IPC: task plan.
 */
#include <stdio.h>
#include <stdlib.h>

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/util/log.h>

#include "lazydesk-build.h"

struct wm_server {
	struct wl_display *display;
	struct wl_event_loop *loop;
	struct wlr_backend *backend;
	struct wlr_allocator *allocator;
	struct wlr_renderer *renderer;
	struct wlr_scene *scene;
	struct wlr_output_layout *layout;
	struct wlr_scene_output_layout *scene_layout;
	struct wl_listener new_output;
	struct wl_listener new_input;
	struct wl_list outputs;
};

struct wm_output {
	struct wlr_output *wlr;
	struct wlr_scene_output *scene_output;
	struct wm_server *server;
	struct wl_listener frame;
	struct wl_list link;
};

static void output_frame(struct wl_listener *listener, void *data)
{
	struct wm_output *o = wl_container_of(listener, o, frame);
	wlr_scene_output_commit(o->scene_output, NULL);
	(void)data;
}

static void server_new_output(struct wl_listener *listener, void *data)
{
	struct wm_server *s = wl_container_of(listener, s, new_output);
	struct wlr_output *w = data;

	wlr_output_init_render(w, s->allocator, s->renderer);

	struct wlr_output_state state;
	wlr_output_state_init(&state);
	wlr_output_state_set_mode(&state, wlr_output_preferred_mode(w));
	wlr_output_state_set_enabled(&state, true);
	if (wlr_output_commit_state(w, &state))
		wlr_log(WLR_INFO, "lazydesk-wm: output %s initialized", w->name);
	wlr_output_state_finish(&state);

	wlr_output_layout_add_auto(s->layout, w);

	struct wm_output *o = calloc(1, sizeof *o);
	o->wlr = w;
	o->server = s;
	o->scene_output = wlr_scene_output_create(s->scene, w);
	o->frame.notify = output_frame;
	wl_signal_add(&w->events.frame, &o->frame);
	wl_list_insert(&s->outputs, &o->link);

	wlr_log(WLR_INFO, "lazydesk-wm: output %s (%ux%u)", w->name, w->width, w->height);
}

static void server_new_input(struct wl_listener *listener, void *data)
{
	(void)listener; (void)data; /* seats arrive in task 2.2 */
}

int main(void)
{
	wlr_log_init(WLR_INFO, NULL);
	wlr_log(WLR_INFO, "lazydesk-wm %s (native compositor skeleton)", LAZYDESK_VERSION);

	struct wm_server s = { 0 };
	wl_list_init(&s.outputs);

	s.display = wl_display_create();
	if (!s.display) {
		wlr_log(WLR_ERROR, "cannot create display");
		return 1;
	}
	s.loop = wl_display_get_event_loop(s.display);

	s.backend = wlr_backend_autocreate(s.loop, NULL);
	if (!s.backend) {
		wlr_log(WLR_ERROR, "cannot create backend (need a seat or WLR_BACKENDS=wayland/headless)");
		return 1;
	}

	s.renderer = wlr_renderer_autocreate(s.backend);
	s.allocator = wlr_allocator_autocreate(s.backend, s.renderer);
	if (!s.renderer || !s.allocator) {
		wlr_log(WLR_ERROR, "cannot create renderer/allocator");
		return 1;
	}
	if (!wlr_renderer_init_wl_display(s.renderer, s.display)) {
		wlr_log(WLR_ERROR, "cannot bind renderer to display");
		return 1;
	}

	wlr_compositor_create(s.display, 5, s.renderer);
	wlr_subcompositor_create(s.display);
	wlr_data_device_manager_create(s.display);

	s.layout = wlr_output_layout_create(s.display);
	s.scene = wlr_scene_create();
	s.scene_layout = wlr_scene_attach_output_layout(s.scene, s.layout);

	s.new_output.notify = server_new_output;
	wl_signal_add(&s.backend->events.new_output, &s.new_output);
	s.new_input.notify = server_new_input;
	wl_signal_add(&s.backend->events.new_input, &s.new_input);

	if (!wlr_backend_start(s.backend)) {
		wlr_log(WLR_ERROR, "cannot start backend");
		return 1;
	}
	wlr_log(WLR_INFO, "lazydesk-wm: running");
	wl_display_run(s.display);

	wl_display_destroy(s.display);
	return 0;
}
