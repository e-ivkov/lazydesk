/* tomlc: parse shipped config + a theme; assert values. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "toml.h"

static int fail(const char *msg) { fprintf(stderr, "toml test: %s\n", msg); return 1; }

int main(int argc, char **argv)
{
	if (argc < 3) return 2;
	char err[256];

	/* config: archetypes + spawn entries */
	FILE *f = fopen(argv[1], "r");
	if (!f) return fail("open config");
	toml_table_t *cfg = toml_parse_file(f, err, sizeof err);
	fclose(f);
	if (!cfg) return fail(err);

	toml_table_t *agent = toml_table_in(cfg, "archetypes");
	if (!agent) return fail("missing [archetypes.*]");
	toml_table_t *ed = toml_table_in(agent, "editor");
	if (!ed) return fail("missing [archetypes.editor]");
	toml_datum_t cmd = toml_string_in(ed, "command");
	if (!cmd.ok || strstr(cmd.u.s, "zed") == NULL)
		return fail("editor command wrong");
	free(cmd.u.s);

	toml_array_t *spawns = toml_array_in(cfg, "spawn");
	if (!spawns) return fail("missing [[spawn]]");
	if (toml_array_nelem(spawns) < 3) return fail("expected >=3 spawn entries");
	toml_free(cfg);

	/* theme: palette tokens */
	f = fopen(argv[2], "r");
	if (!f) return fail("open theme");
	toml_table_t *theme = toml_parse_file(f, err, sizeof err);
	fclose(f);
	if (!theme) return fail(err);
	toml_table_t *pal = toml_table_in(theme, "palette");
	if (!pal) return fail("missing [palette]");
	toml_datum_t accent = toml_string_in(pal, "accent");
	if (!accent.ok || accent.u.s[0] != '#') return fail("accent token wrong");
	free(accent.u.s);
	toml_free(theme);

	printf("toml parse: OK\n");
	return 0;
}
