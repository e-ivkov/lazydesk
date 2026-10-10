/* config.h — LazyDesk TOML config model: parse, validate, previous-good. */
#ifndef LAZYDESK_CONFIG_H
#define LAZYDESK_CONFIG_H

#include "lazydesk.h"

typedef struct {
	char id[32];
	char command[256];
	char class_regex[128]; /* window class match; empty = derive from command token */
	int  number;           /* 1..9 or 0 = unnumbered */
} ld_cfg_archetype;

typedef struct {
	char archetype[32];
	bool autostart;
} ld_cfg_spawn;

typedef struct {
	ld_cfg_archetype archetypes[LAZYDESK_MAX_ARCHETYPES];
	size_t narchetypes;
	ld_cfg_spawn spawns[16];
	size_t nspawns;
	char theme[64];
	char terminal[64];
	long idle_minutes;
	char harness_command[256];
	char harness_fallback[256];
	char scratchpad_command[128];
	long scratchpad_height_pct;
	char notes_dir[256];
	char notes_todo[256];
} ld_config;

/* Parse + validate. On failure returns false and errbuf names the offending key:
 * "archetypes.editor.command: must not be empty". */
bool ld_config_load(ld_config *c, const char *path, char *errbuf, size_t errlen);

/* Shipped defaults (from the template) — used when no user config exists. */
const ld_config *ld_config_defaults(const char *datadir);

/* Look up by id; NULL if absent. */
const ld_cfg_archetype *ld_config_archetype(const ld_config *c, const char *id);

#endif
