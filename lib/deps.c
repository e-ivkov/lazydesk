/* deps.c — load per-distro dependency maps (TOML) and probe installation state.
 *
 * Map schema (share/lazydesk/deps/<distro>.toml):
 *   [packages]
 *   hyprland = "Hyprland"           # component -> distro package
 *   [optional]
 *   wf-recorder = "wf-recorder"
 */
#include "lazydesk.h"

#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "toml.h"

const char *ld_datadir(void)
{
	const char *env = getenv("LAZYDESK_PKGDATADIR");
	if (env && *env)
		return env;
#ifdef LAZYDESK_PKGDATADIR
	return LAZYDESK_PKGDATADIR;
#else
	return ".";
#endif
}

static void deps_add(ld_deps_map *m, const char *name, const char *pkg, bool optional)
{
	if (m->ndeps >= LAZYDESK_MAX_PKGS)
		return;
	ld_dep *d = &m->deps[m->ndeps++];
	snprintf(d->name, sizeof d->name, "%s", name);
	snprintf(d->pkg, sizeof d->pkg, "%s", pkg ? pkg : name);
	d->optional = optional;
}

bool ld_deps_load(ld_deps_map *m, ld_distro d, const char *datadir,
                  char *errbuf, size_t errlen)
{
	memset(m, 0, sizeof *m);
	if (d == LD_DISTRO_UNKNOWN) {
		snprintf(errbuf, errlen, "unsupported or unknown distro");
		return false;
	}
	static const char *layouts[] = { "%s/deps/%s.toml", "%s/data/deps/%s.toml" };
	FILE *f = nullptr;
	for (size_t i = 0; i < sizeof layouts / sizeof layouts[0]; i++) {
		snprintf(m->path, sizeof m->path, layouts[i], datadir, ld_distro_id(d));
		if ((f = fopen(m->path, "r")) != nullptr)
			break;
	}
	if (!f) {
		snprintf(errbuf, errlen, "cannot open deps/%s.toml under %s", ld_distro_id(d), datadir);
		return false;
	}
	char err[256];
	toml_table_t *root = toml_parse_file(f, err, sizeof err);
	fclose(f);
	if (!root) {
		snprintf(errbuf, errlen, "parse %s: %s", m->path, err);
		return false;
	}

	static const char *sections[] = { "packages", "optional" };
	for (int s = 0; s < 2; s++) {
		toml_table_t *t = toml_table_in(root, sections[s]);
		if (!t)
			continue;
		bool optional = s == 1;
		for (int i = 0; ; i++) {
			const char *key = toml_key_in(t, i);
			if (!key)
				break;
			toml_datum_t val = toml_string_in(t, key);
			if (!val.ok) {
				snprintf(errbuf, errlen,
				         "%s: [%s] %s must be a package-name string",
				         m->path, sections[s], key);
				toml_free(root);
				return false;
			}
			deps_add(m, key, val.u.s, optional);
			free(val.u.s);
		}
	}
	toml_free(root);

	for (size_t i = 0; i < m->ndeps; i++)
		m->deps[i].installed = ld_pkg_installed(d, m->deps[i].pkg);

	return true;
}

/* ---------- shipped archetypes ---------- */

static const ld_archetype shipped[] = {
	{ "agent",     1, "ghostty -e pi",         "ghostty" },
	{ "editor",    2, "zed",                   "zed" },
	{ "terminal",  3, "ghostty",               "ghostty" },
	{ "browser",   4, "chromium",              "chromium" },
	{ "messenger", 5, "telegram-desktop",      "telegram-desktop" },
	{ "cockpit",   6, "lazydesk-cockpit",      nullptr }, /* shipped by us */
	{ "lazygit",   7, "ghostty -e lazygit",    "lazygit" },
	{ "desk",      0, "lazydesk-settings",     nullptr }, /* shipped by us */
	{ "files",     0, "thunar",                "thunar" },
};

const ld_archetype *ld_archetypes(size_t *count)
{
	*count = sizeof shipped / sizeof shipped[0];
	return shipped;
}

/* ---------- command resolution ---------- */

char *ld_resolve_command(const char *command)
{
	char first[256];
	size_t i = 0;
	while (*command && !isspace((unsigned char)*command) && i < sizeof first - 1)
		first[i++] = *command++;
	first[i] = '\0';
	if (!first[0])
		return nullptr;

	const char *path_env = getenv("PATH");
	if (!path_env)
		path_env = "/usr/bin:/bin";
	char *paths = strdup(path_env);
	char *save = nullptr;
	char *seg = strtok_r(paths, ":", &save);
	char *result = nullptr;
	char full[1024];
	while (seg) {
		snprintf(full, sizeof full, "%s/%s", seg, first);
		if (access(full, X_OK) == 0) {
			result = strdup(full);
			break;
		}
		seg = strtok_r(nullptr, ":", &save);
	}
	free(paths);
	return result;
}
