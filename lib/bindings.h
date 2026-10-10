/* bindings.h — the action registry: ONE source for keybindings, the bar
 * action menu, the cheatsheet, palette verbs, and IPC ops.
 */
#ifndef LAZYDESK_BINDINGS_H
#define LAZYDESK_BINDINGS_H

#include "config.h"

typedef enum {
	LD_ACT_EXEC,      /* exec a command (cmd field) */
	LD_ACT_WORKSPACE, /* focus workspace (target = number or name) */
	LD_ACT_MOVE,      /* move window to workspace */
	LD_ACT_CUSTOM,    /* reserved for facade verbs (cmd = lazydeskctl ...) */
} ld_action_kind;

typedef struct {
	const char *action;    /* registry id: "workspace.focus.3" */
	const char *group;     /* menu/cheatsheet group */
	const char *combo;     /* "Super+1" (default; rebound via config) */
	const char *description;
	ld_action_kind kind;
	const char *cmd;       /* exec command or lazydeskctl verb */
	const char *target;    /* workspace target for WORKSPACE/MOVE */
} ld_binding;

const ld_binding *ld_bindings(size_t *count);

/* Niri KDL bind for a registry entry, e.g.
 *   Mod+Return { spawn "lazydesk-menu"; }
 *   Mod+Shift+1 { move-column-to-workspace "agent"; }
 * Returns bytes written. */
size_t ld_binding_niri(const ld_binding *b, const ld_config *c,
                       char *out, size_t outlen);

#endif
