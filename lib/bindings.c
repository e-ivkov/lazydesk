/* bindings.c — the shipped action registry (see bindings.h). */
#include "bindings.h"

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <strings.h>

static const ld_binding registry[] = {
	/* launch & command group */
	{ "palette.open",     "Launch",  "Super+Enter",     "Command palette",             LD_ACT_EXEC,     "lazydesk-menu", nullptr },
	{ "scratchpad.toggle","Launch",  "Super+grave",     "Toggle scratchpad terminal",  LD_ACT_EXEC,     "ghostty +toggle-quick-terminal", nullptr },
	{ "harness.summon",   "Launch",  "Super+a",         "Focus or spawn AI harness",   LD_ACT_CUSTOM,   "lazydeskctl harness summon", nullptr },
	{ "terminal.spawn",   "Launch",  "Super+Backspace", "Terminal on this workspace",  LD_ACT_CUSTOM,   "lazydeskctl spawn --here --archetype terminal", nullptr },
	{ "cockpit.open",     "Launch",  "Super+c",         "Cockpit dashboard",           LD_ACT_CUSTOM,   "lazydeskctl spawn --archetype cockpit", nullptr },
	{ "settings.open",    "Launch",  "Super+comma",     "Settings",                    LD_ACT_CUSTOM,   "lazydeskctl spawn --archetype desk", nullptr },
	{ "files.open",       "Launch",  "Super+e",         "File manager at ~/projects",  LD_ACT_CUSTOM,   "lazydeskctl spawn --archetype files --at ~/projects", nullptr },
	/* windows group */
	{ "window.cycle.next","Windows", "Super+j",         "Focus next window",           LD_ACT_CUSTOM,   "lazydeskctl window cycle next", nullptr },
	{ "window.cycle.prev","Windows", "Super+k",         "Focus previous window",       LD_ACT_CUSTOM,   "lazydeskctl window cycle prev", nullptr },
	{ "window.snap.left", "Windows", "Super+Ctrl+Left", "Snap window left half",       LD_ACT_CUSTOM,   "lazydeskctl window snap left", nullptr },
	{ "window.snap.right","Windows", "Super+Ctrl+Right","Snap window right half",      LD_ACT_CUSTOM,   "lazydeskctl window snap right", nullptr },
	{ "window.snap.up",   "Windows", "Super+Ctrl+Up",   "Snap window top / restore",   LD_ACT_CUSTOM,   "lazydeskctl window snap up", nullptr },
	{ "window.snap.down", "Windows", "Super+Ctrl+Down", "Snap window bottom half",     LD_ACT_CUSTOM,   "lazydeskctl window snap down", nullptr },
	{ "window.switcher",  "Windows", "Super+Tab",       "Fuzzy window switcher",       LD_ACT_CUSTOM,   "lazydeskctl window switcher", nullptr },
	{ "window.fullscreen","Windows", "Super+f",         "Toggle fullscreen",           LD_ACT_CUSTOM,   "lazydeskctl window fullscreen", nullptr },
	{ "window.close",     "Windows", "Super+q",         "Close window",                LD_ACT_CUSTOM,   "lazydeskctl window close", nullptr },
	/* session group */
	{ "workspace.new",    "Session", "Super+n",         "New workspace",               LD_ACT_CUSTOM,   "lazydeskctl workspace new", nullptr },
	{ "config.reload",    "Session", "Super+Escape",    "Reload configuration",        LD_ACT_CUSTOM,   "lazydeskctl config reload", nullptr },
	{ "session.quit",     "Session", "Super+Shift+Escape", "Quit LazyDesk (confirm)",  LD_ACT_CUSTOM,   "lazydeskctl session quit", nullptr },
};

const ld_binding *ld_bindings(size_t *count)
{
	*count = sizeof registry / sizeof registry[0];
	return registry;
}

/* combo -> hypr mods/key ("Super+Shift+Escape" -> "SUPER SHIFT", "Escape") */
static void split_combo(const char *combo, char *mods, size_t mlen, char *key, size_t klen)
{
	mods[0] = key[0] = '\0';
	char buf[64];
	snprintf(buf, sizeof buf, "%s", combo);

	/* tokenize on '+' first (strtok destroys the separators) */
	char *parts[8] = { 0 };
	size_t np = 0;
	char *save = nullptr;
	for (char *tok = strtok_r(buf, "+", &save); tok && np < 8; tok = strtok_r(nullptr, "+", &save))
		parts[np++] = tok;

	if (np == 0) {
		snprintf(mods, mlen, "SUPER");
		snprintf(key, klen, "%s", combo);
		return;
	}
	snprintf(key, klen, "%s", parts[np - 1]);
	/* user-facing names -> xkb keysym names */
	if (!strcmp(key, "Enter"))          snprintf(key, klen, "Return");
	else if (!strcmp(key, "Backspace")) snprintf(key, klen, "BackSpace");

	size_t off = 0;
	for (size_t i = 0; i + 1 < np; i++) {
		const char *seg = parts[i];
		const char *h = (!strncasecmp(seg, "super", 5)) ? "SUPER"
		              : (!strncasecmp(seg, "ctrl", 4)) ? "CTRL"
		              : (!strncasecmp(seg, "shift", 5)) ? "SHIFT"
		              : (!strncasecmp(seg, "alt", 3)) ? "ALT" : seg;
		off += snprintf(mods + off, mlen - off, off ? " %s" : "%s", h);
	}
	if (!mods[0])
		snprintf(mods, mlen, "SUPER"); /* registry default mod */
}

/* quote-split cmd into niri spawn args: spawn "a" "b" "c" */
static size_t niri_spawn_args(const char *cmd, char *out, size_t outlen)
{
	size_t off = 0;
	char buf[256];
	snprintf(buf, sizeof buf, "%s", cmd);
	char *save = nullptr;
	bool first = true;
	for (char *tok = strtok_r(buf, " ", &save); tok; tok = strtok_r(nullptr, " ", &save)) {
		off += snprintf(out + off, outlen - off, "%s\"%s\"", first ? "" : " ", tok);
		first = false;
	}
	return off;
}

size_t ld_binding_niri(const ld_binding *b, const ld_config *c,
                       char *out, size_t outlen)
{
	(void)c;
	char mods[32], key[32];
	split_combo(b->combo, mods, sizeof mods, key, sizeof key);

	/* niri mod names joined with '+': SUPER CTRL SHIFT -> Mod+Ctrl+Shift */
	char nmods[32];
	{
		size_t off = 0;
		char mbuf[32];
		snprintf(mbuf, sizeof mbuf, "%s", mods);
		char *save2 = nullptr;
		bool firstm = true;
		for (char *tok = strtok_r(mbuf, " ", &save2); tok; tok = strtok_r(nullptr, " ", &save2)) {
			const char *h = (!strcmp(tok, "SUPER")) ? "Mod"
			              : (!strcmp(tok, "CTRL")) ? "Ctrl"
			              : (!strcmp(tok, "SHIFT")) ? "Shift"
			              : (!strcmp(tok, "ALT")) ? "Alt" : tok;
			off += snprintf(nmods + off, sizeof nmods - off, "%s%s", firstm ? "" : "+", h);
			firstm = false;
		}
	}

	if (b->kind == LD_ACT_WORKSPACE || b->kind == LD_ACT_MOVE) {
		return snprintf(out, outlen, "%s+%s { %s \"%s\"; }",
		                nmods, key,
		                b->kind == LD_ACT_MOVE ? "move-column-to-workspace" : "focus-workspace",
		                b->target);
	}
	char args[512];
	niri_spawn_args(b->cmd, args, sizeof args);
	return snprintf(out, outlen, "%s+%s { spawn %s; }", nmods, key, args);
}
