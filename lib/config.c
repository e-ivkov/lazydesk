/* config.c — parse and validate ~/.config/lazydesk/lazydesk.toml. */
#define _POSIX_C_SOURCE 200809L

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "toml.h"

static const ld_archetype *shipped(size_t *n) { return ld_archetypes(n); }

static bool str_copy(char *dst, size_t dstlen, toml_datum_t d,
                     const char *ctx, char *err, size_t errlen)
{
	if (!d.ok) {
		snprintf(err, errlen, "%s: required string missing", ctx);
		return false;
	}
	snprintf(dst, dstlen, "%s", d.u.s);
	free(d.u.s);
	if (!dst[0]) {
		snprintf(err, errlen, "%s: must not be empty", ctx);
		return false;
	}
	return true;
}

/* Fill defaults from the shipped archetype table so a config may list only
 * overrides. Unknown archetype ids are rejected. */
static void apply_shipped_defaults(ld_config *c)
{
	size_t n;
	const ld_archetype *a = shipped(&n);
	for (size_t i = 0; i < n && c->narchetypes < LAZYDESK_MAX_ARCHETYPES; i++) {
		if (ld_config_archetype(c, a[i].id))
			continue;
		ld_cfg_archetype *dst = &c->archetypes[c->narchetypes++];
		snprintf(dst->id, sizeof dst->id, "%s", a[i].id);
		snprintf(dst->command, sizeof dst->command, "%s", a[i].command);
		/* derive class regex from --class= in the command, else first token */
		const char *cl = strstr(a[i].command, "--class=");
		if (cl)
			snprintf(dst->class_regex, sizeof dst->class_regex, "%s", cl + strlen("--class="));
		else
			snprintf(dst->class_regex, sizeof dst->class_regex, "%s", a[i].command);
		/* trim class regex at first space */
		char *sp = strchr(dst->class_regex, ' ');
		if (sp) *sp = '\0';
		dst->number = a[i].number;
	}
}

bool ld_config_load(ld_config *c, const char *path, char *errbuf, size_t errlen)
{
	memset(c, 0, sizeof *c);
	/* defaults first */
	apply_shipped_defaults(c);
	c->idle_minutes = 10;
	c->scratchpad_height_pct = 40;
	snprintf(c->theme, sizeof c->theme, "catppuccin-mocha");
	snprintf(c->terminal, sizeof c->terminal, "ghostty");
	snprintf(c->scratchpad_command, sizeof c->scratchpad_command, "ghostty");
	snprintf(c->notes_dir, sizeof c->notes_dir, "~/notes");
	snprintf(c->notes_todo, sizeof c->notes_todo, "~/notes/todo.md");

	char err[256];
	FILE *f = fopen(path, "r");
	if (!f) {
		snprintf(errbuf, errlen, "cannot open %s", path);
		return false;
	}
	toml_table_t *root = toml_parse_file(f, err, sizeof err);
	fclose(f);
	if (!root) {
		snprintf(errbuf, errlen, "%s: %s", path, err);
		return false;
	}
	bool ok = true;

	/* scalars */
	toml_datum_t d;
	if ((d = toml_string_in(root, "theme")).ok) {
		snprintf(c->theme, sizeof c->theme, "%s", d.u.s); free(d.u.s);
		if (!c->theme[0]) { snprintf(errbuf, errlen, "theme: must not be empty"); ok = false; goto out; }
	}
	if ((d = toml_string_in(root, "terminal")).ok) {
		if (!str_copy(c->terminal, sizeof c->terminal, d, "terminal", errbuf, errlen)) { ok = false; goto out; }
	}
	if ((d = toml_int_in(root, "idle_minutes")).ok) {
		c->idle_minutes = d.u.i;
		if (c->idle_minutes < 1) { snprintf(errbuf, errlen, "idle_minutes: must be >= 1"); ok = false; goto out; }
	}

	/* archetypes */
	toml_table_t *ar = toml_table_in(root, "archetypes");
	if (ar) {
		for (int i = 0; ; i++) {
			const char *key = toml_key_in(ar, i);
			if (!key) break;
			toml_table_t *t = toml_table_in(ar, key);
			if (!t) { snprintf(errbuf, errlen, "archetypes.%s: must be a table", key); ok = false; goto out; }

			/* find shipped meta by id */
			const ld_archetype *found = nullptr;
			{
				size_t nn; const ld_archetype *all = ld_archetypes(&nn);
				for (size_t j = 0; j < nn; j++)
					if (strcmp(all[j].id, key) == 0) { found = &all[j]; break; }
			}
			if (!found) { snprintf(errbuf, errlen, "archetypes.%s: unknown archetype", key); ok = false; goto out; }

			ld_cfg_archetype *dst = (ld_cfg_archetype *)ld_config_archetype(c, key);
			if (!dst) {
				if (c->narchetypes >= LAZYDESK_MAX_ARCHETYPES) { snprintf(errbuf, errlen, "archetypes: too many"); ok = false; goto out; }
				dst = &c->archetypes[c->narchetypes++];
				memset(dst, 0, sizeof *dst);
				snprintf(dst->id, sizeof dst->id, "%s", key);
				dst->number = found->number;
			}
			char ctx[96];
			snprintf(ctx, sizeof ctx, "archetypes.%s.command", key);
			bool had_class = toml_string_in(t, "class").ok;
			if ((d = toml_string_in(t, "command")).ok) {
				if (!str_copy(dst->command, sizeof dst->command, d, ctx, errbuf, errlen)) { ok = false; goto out; }
				if (!had_class) {
					/* re-derive class: --class= in command wins, else first token */
					const char *cl = strstr(dst->command, "--class=");
					if (cl)
						snprintf(dst->class_regex, sizeof dst->class_regex, "%s", cl + 8);
					else
						snprintf(dst->class_regex, sizeof dst->class_regex, "%s", dst->command);
					char *sp = strchr(dst->class_regex, ' ');
					if (sp) *sp = '\0';
				}
			}
			snprintf(ctx, sizeof ctx, "archetypes.%s.class", key);
			if ((d = toml_string_in(t, "class")).ok) {
				if (!str_copy(dst->class_regex, sizeof dst->class_regex, d, ctx, errbuf, errlen)) { ok = false; goto out; }
			}
			if ((d = toml_int_in(t, "number")).ok) {
				if (d.u.i < 0 || d.u.i > 9) {
					snprintf(errbuf, errlen, "archetypes.%s.number: must be 0..9", key);
					ok = false; goto out;
				}
				dst->number = (int)d.u.i;
			}
		}
	}

	/* spawns */
	toml_array_t *sp = toml_array_in(root, "spawn");
	if (sp) {
		for (int i = 0; ; i++) {
			toml_table_t *t = toml_table_at(sp, i);
			if (!t) break;
			if (c->nspawns >= 16) { snprintf(errbuf, errlen, "spawn: too many entries"); ok = false; goto out; }
			ld_cfg_spawn *s = &c->spawns[c->nspawns];
			memset(s, 0, sizeof *s);
			char ctx[64];
			snprintf(ctx, sizeof ctx, "spawn[%d].archetype", i);
			toml_datum_t a = toml_string_in(t, "archetype");
			if (!str_copy(s->archetype, sizeof s->archetype, a, ctx, errbuf, errlen)) { ok = false; goto out; }
			if (!ld_config_archetype(c, s->archetype)) {
				snprintf(errbuf, errlen, "spawn[%d].archetype: unknown archetype '%s'", i, s->archetype);
				ok = false; goto out;
			}
			if ((d = toml_bool_in(t, "autostart")).ok) s->autostart = d.u.b;
			c->nspawns++;
		}
	}

	/* harness */
	toml_table_t *h = toml_table_in(root, "harness");
	if (h) {
		if ((d = toml_string_in(h, "command")).ok) {
			if (!str_copy(c->harness_command, sizeof c->harness_command, d, "harness.command", errbuf, errlen)) { ok = false; goto out; }
		}
		if ((d = toml_string_in(h, "fallback")).ok) {
			if (!str_copy(c->harness_fallback, sizeof c->harness_fallback, d, "harness.fallback", errbuf, errlen)) { ok = false; goto out; }
		}
	}

	/* scratchpad */
	toml_table_t *sc = toml_table_in(root, "scratchpad");
	if (sc) {
		if ((d = toml_string_in(sc, "command")).ok) {
			if (!str_copy(c->scratchpad_command, sizeof c->scratchpad_command, d, "scratchpad.command", errbuf, errlen)) { ok = false; goto out; }
		}
		if ((d = toml_int_in(sc, "height_pct")).ok) {
			c->scratchpad_height_pct = d.u.i;
			if (c->scratchpad_height_pct < 10 || c->scratchpad_height_pct > 90) {
				snprintf(errbuf, errlen, "scratchpad.height_pct: must be 10..90");
				ok = false; goto out;
			}
		}
	}

	/* notes */
	toml_table_t *nt = toml_table_in(root, "notes");
	if (nt) {
		if ((d = toml_string_in(nt, "dir")).ok) {
			if (!str_copy(c->notes_dir, sizeof c->notes_dir, d, "notes.dir", errbuf, errlen)) { ok = false; goto out; }
		}
		if ((d = toml_string_in(nt, "todo_file")).ok) {
			if (!str_copy(c->notes_todo, sizeof c->notes_todo, d, "notes.todo_file", errbuf, errlen)) { ok = false; goto out; }
		}
	}

	/* every archetype must end up with a command */
	for (size_t i = 0; i < c->narchetypes; i++) {
		if (!c->archetypes[i].command[0]) {
			snprintf(errbuf, errlen, "archetypes.%s.command: must not be empty", c->archetypes[i].id);
			ok = false; goto out;
		}
	}

out:
	toml_free(root);
	return ok;
}

const ld_cfg_archetype *ld_config_archetype(const ld_config *c, const char *id)
{
	for (size_t i = 0; i < c->narchetypes; i++)
		if (strcmp(c->archetypes[i].id, id) == 0)
			return &c->archetypes[i];
	return nullptr;
}

const ld_config *ld_config_defaults(const char *datadir)
{
	static ld_config c;
	static bool init;
	if (!init) {
		char path[512], err[256];
		snprintf(path, sizeof path, "%s/config/lazydesk.toml", datadir);
		if (!ld_config_load(&c, path, err, sizeof err)) {
			/* fall back to hardcoded shipped table alone */
			memset(&c, 0, sizeof c);
			apply_shipped_defaults(&c);
			c.idle_minutes = 10;
			c.scratchpad_height_pct = 40;
		}
		init = true;
	}
	return &c;
}
