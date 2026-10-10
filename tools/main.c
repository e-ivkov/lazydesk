/* tools/main.c — the `lazydesk` CLI: install / update / config / doctor.
 *
 * Task 1.3 scope: distro detection, dependency maps, install planning with
 * dry-run, archetype-completeness verification (doctor). Real execution of
 * installs lands with the packaging tasks; the plan is the contract.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "lazydesk.h"
#include "toml.h"

static const char *usage =
"lazydesk " LAZYDESK_VERSION " — development-first desktop profile\n"
"\n"
"usage: lazydesk [--distro void|debian|ubuntu] [--dry-run] <command>\n"
"\n"
"commands:\n"
"  install        install/verify dependencies and lazydesk (plan with --dry-run)\n"
"  update         update lazydesk and regenerate configs\n"
"  config regen   validate config and regenerate managed artifacts\n"
"  doctor         verify session health (deps, archetype completeness)\n"
"                 (doctor --archetypes: only archetype completeness)\n"
"  version        print version\n";

static ld_distro opt_distro = LD_DISTRO_UNKNOWN;
static bool opt_dry_run = false;
static bool opt_have_distro = false;

static void die(const char *msg)
{
	fprintf(stderr, "lazydesk: %s\n", msg);
	exit(2);
}

static int cmd_install(void)
{
	ld_distro d = opt_have_distro ? opt_distro : ld_distro_detect();
	if (d == LD_DISTRO_UNKNOWN)
		die("unsupported distro (need /etc/os-release with ID=void|debian|ubuntu, or --distro)");

	printf("lazydesk %s install — %s%s\n\n", LAZYDESK_VERSION, ld_distro_name(d),
	       opt_dry_run ? " (dry run)" : "");

	ld_deps_map map;
	char err[512];
	if (!ld_deps_load(&map, d, ld_datadir(), err, sizeof err))
		die(err);

	/* 1. backend + component dependencies */
	size_t missing = 0, missing_opt = 0;
	printf("dependencies:\n");
	for (size_t i = 0; i < map.ndeps; i++) {
		ld_dep *dep = &map.deps[i];
		printf("  [%s] %-14s %s%s\n", dep->installed ? "ok" :
		       (dep->optional ? "opt" : "MISSING"),
		       dep->name, dep->pkg, dep->optional ? " (optional)" : "");
		if (!dep->installed)
			dep->optional ? missing_opt++ : missing++;
	}
	if (missing || missing_opt) {
		const char *tool = (d == LD_DISTRO_VOID) ? "xbps-install -S" : "apt-get install";
		printf("\nplan: %s ", tool);
		for (size_t i = 0; i < map.ndeps; i++)
			if (!map.deps[i].installed && !map.deps[i].optional)
				printf("%s ", map.deps[i].pkg);
		printf("\n      (%zu optional skipped, install by hand if wanted)\n", missing_opt);
	}

	/* 2. lazydesk files + session entry */
	printf("\nplan: install lazydesk binaries to %s\n", LAZYDESK_BINDIR);
	printf("plan: install themes/config/deps/skill to %s\n", ld_datadir());
	printf("plan: register session entry (wayland-sessions/lazydesk.desktop)\n");

	/* 3. archetype completeness (post-install expectation) */
	printf("\narchetype completeness (post-install expectation):\n");
	size_t unfilled = 0;
	size_t narch;
	const ld_archetype *ar = ld_archetypes(&narch);
	for (size_t i = 0; i < narch; i++) {
		char *res = ld_resolve_command(ar[i].command);
		printf("  [%s] %-9s %s\n", res ? "ok" : "pkg", ar[i].id, ar[i].command);
		if (!res) {
			unfilled++;
			printf("         -> will be provided by package '%s'\n",
			       ar[i].pkg ? ar[i].pkg : "lazydesk itself");
		}
		free(res);
	}

	if (!opt_dry_run)
		die("real install not implemented yet (packaging tasks); use --dry-run for the plan");
	printf("\ndry run complete: plan OK\n");
	return 0;
}

static int cmd_doctor(bool archetypes_only)
{
	ld_distro d = opt_have_distro ? opt_distro : ld_distro_detect();
	int rc = 0;

	printf("lazydesk %s doctor\n\n", LAZYDESK_VERSION);

	ld_deps_map map;
	char err[512];
	bool have_map = archetypes_only ? false :
		ld_deps_load(&map, d, ld_datadir(), err, sizeof err);
	if (archetypes_only) {
		/* scope: skip deps probing entirely */
	} else
	if (!have_map)
		printf("deps map: unavailable (%s)\n", err);
	else {
		size_t missing = 0;
		for (size_t i = 0; i < map.ndeps; i++)
			if (!map.deps[i].installed && !map.deps[i].optional) {
				printf("UNFILLED dependency: %s (package '%s') — install with your package manager\n",
				       map.deps[i].name, map.deps[i].pkg);
				missing++;
			}
		if (!missing)
			printf("deps map: all required packages present (%s)\n", ld_distro_name(d));
		else
			rc = 1;
	}

	size_t narch, unfilled = 0;
	const ld_archetype *ar = ld_archetypes(&narch);
	printf("\narchetypes:\n");
	for (size_t i = 0; i < narch; i++) {
		char *res = ld_resolve_command(ar[i].command);
		if (res) {
			printf("  ok       %-9s -> %s\n", ar[i].id, res);
			free(res);
		} else {
			rc = 1;
			unfilled++;
			const char *pkg = ar[i].pkg ? ar[i].pkg : "lazydesk itself (reinstall)";
			printf("  UNFILLED %-9s command '%s' not found\n", ar[i].id, ar[i].command);
			printf("           fix: install package '%s', or rebind archetype in ~/.config/lazydesk/lazydesk.toml\n", pkg);
		}
	}
	printf("\n%s\n", unfilled ? "session incomplete — see fixes above"
	                          : "all archetypes filled");
	return rc;
}

static int cmd_config_regen(void)
{
	/* Full validation + generation is task 2.x; parse check now. */
	const char *home = getenv("HOME");
	char path[512];
	const char *src = nullptr;
	FILE *f;

	snprintf(path, sizeof path, "%s/.config/lazydesk/lazydesk.toml", home);
	if ((f = fopen(path, "r")) != nullptr) {
		fclose(f);
		src = path;
	} else {
		snprintf(path, sizeof path, "%s/config/lazydesk.toml", ld_datadir());
		if ((f = fopen(path, "r")) != nullptr) {
			fclose(f);
			src = path;
		} else {
			snprintf(path, sizeof path, "%s/data/config/lazydesk.toml", ld_datadir());
			if ((f = fopen(path, "r")) != nullptr) {
				fclose(f);
				src = path;
			}
		}
	}
	if (!src)
		die("no config found (looked in ~/.config/lazydesk and installed defaults)");

	char err[256];
	f = fopen(src, "r");
	toml_table_t *root = toml_parse_file(f, err, sizeof err);
	fclose(f);
	if (!root) {
		fprintf(stderr, "lazydesk: invalid %s: %s\n", src, err);
		return 1;
	}
	toml_free(root);
	printf("config %s: parse OK (full validation + generator in task group 2)\n", src);
	return 0;
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		fputs(usage, stderr);
		return 2;
	}
	int i = 1;
	for (; i < argc; i++) {
		if (strcmp(argv[i], "--distro") == 0 && i + 1 < argc) {
			const char *s = argv[++i];
			if (!strcmp(s, "void"))        { opt_distro = LD_DISTRO_VOID;   opt_have_distro = true; }
			else if (!strcmp(s, "debian")) { opt_distro = LD_DISTRO_DEBIAN; opt_have_distro = true; }
			else if (!strcmp(s, "ubuntu")) { opt_distro = LD_DISTRO_UBUNTU; opt_have_distro = true; }
			else die("--distro must be void|debian|ubuntu");
		} else if (strcmp(argv[i], "--dry-run") == 0) {
			opt_dry_run = true;
		} else {
			break;
		}
	}
	const char *cmd = argv[i];
	const char *sub = (i + 1 < argc) ? argv[i + 1] : nullptr;

	if (!cmd || !strcmp(cmd, "help") || !strcmp(cmd, "-h")) {
		fputs(usage, stderr);
		return cmd ? 0 : 2;
	}
	if (!strcmp(cmd, "version"))
		return printf("lazydesk %s\n", LAZYDESK_VERSION) < 0;
	if (!strcmp(cmd, "install"))
		return cmd_install();
	if (!strcmp(cmd, "doctor"))
		return cmd_doctor(sub && !strcmp(sub, "--archetypes"));
	if (!strcmp(cmd, "update")) {
		printf("update: not implemented yet (packaging tasks)\n");
		return 0;
	}
	if (!strcmp(cmd, "config") && sub && !strcmp(sub, "regen"))
		return cmd_config_regen();

	fprintf(stderr, "lazydesk: unknown command '%s'\n\n%s", cmd, usage);
	return 2;
}
