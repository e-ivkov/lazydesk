/* lazydesk.h — shared library for LazyDesk components.
 *
 * Everything here is vendor-neutral: no compositor types leak in.
 */
#ifndef LAZYDESK_H
#define LAZYDESK_H

#include <stdbool.h>
#include <stddef.h>

#define LAZYDESK_MAX_ARCHETYPES 16
#define LAZYDESK_MAX_PKGS 64

/* ---------- distro ---------- */

typedef enum {
	LD_DISTRO_UNKNOWN = 0,
	LD_DISTRO_VOID,
	LD_DISTRO_DEBIAN,
	LD_DISTRO_UBUNTU,
} ld_distro;

/* Detect from /etc/os-release (ID / ID_LIKE). Never fails: returns UNKNOWN. */
ld_distro ld_distro_detect(void);
const char *ld_distro_id(ld_distro d);     /* "void" | "debian" | "ubuntu" | "unknown" */
const char *ld_distro_name(ld_distro d);   /* human name */
/* Is pkg installed? (xbps-query / dpkg-query under the hood) */
bool ld_pkg_installed(ld_distro d, const char *pkg);

/* ---------- dependency maps (share/lazydesk/deps/<distro>.toml) ---------- */

typedef struct {
	char name[64];   /* lazydesk component name */
	char pkg[96];    /* distro package name */
	bool optional;
	bool installed;  /* filled by ld_deps_load */
} ld_dep;

typedef struct {
	ld_dep deps[LAZYDESK_MAX_PKGS];
	size_t ndeps;
	char path[512];
} ld_deps_map;

/* Load + probe. Returns false with errbuf filled on parse failure. */
bool ld_deps_load(ld_deps_map *m, ld_distro d, const char *datadir,
                  char *errbuf, size_t errlen);

/* ---------- shipped archetypes ---------- */

typedef struct {
	const char *id;      /* "agent", "editor", ... */
	int number;          /* 1..7, 0 = unnumbered */
	const char *command; /* shipped default binding */
	const char *pkg;     /* package that provides the default command */
	const char *app_id;  /* Wayland app_id / window class for routing */
} ld_archetype;

const ld_archetype *ld_archetypes(size_t *count);

/* Where is dir/file? Honor LAZYDESK_PKGDATADIR env override (tests, uninstalled runs). */
const char *ld_datadir(void);

/* Resolve the first token of `command` in PATH. Returns malloc'd path or NULL. */
char *ld_resolve_command(const char *command);

#endif /* LAZYDESK_H */
