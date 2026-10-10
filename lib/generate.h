/* generate.h — artifact generator + drift detection for managed files. */
#ifndef LAZYDESK_GENERATE_H
#define LAZYDESK_GENERATE_H

#include <stddef.h>

#include "lazydesk-build.h"
#include "config.h"   /* ld_config model (lib/config.h) */

#define LD_MANAGED_HEADER "# lazydesk-managed v" LAZYDESK_VERSION " — generated from lazydesk.toml; do not edit (lazydesk config regen)"
#define LD_MANAGED_HEADER_KDL "// lazydesk-managed v" LAZYDESK_VERSION " — generated from lazydesk.toml; do not edit (lazydesk config regen)"

/* theme tokens loaded from themes/<name>.toml [palette] */
typedef struct {
	char name[64];
	char bg[16], bg_alt[16], surface[16], text[16], muted[16], dim[16];
	char accent[16], success[16], warning[16], error[16], info[16];
	long font_size;
	long corner;
} ld_theme;

bool ld_theme_load(ld_theme *t, const char *datadir, const char *name,
                   char *errbuf, size_t errlen);

/* Emitters: return bytes written to buf. */
size_t ld_emit_niri(const ld_config *c, char *buf, size_t buflen);
size_t ld_emit_ghostty(const ld_config *c, const ld_theme *t, char *buf, size_t buflen);
size_t ld_emit_theme_qml(const ld_theme *t, char *buf, size_t buflen);
size_t ld_emit_shell_qml(char *buf, size_t buflen);

/* registry.json: every action (menu + cheatsheet data source) -> malloc'd JSON string */
char *ld_emit_registry_json(void);

/* Managed-file write with drift detection.
 * - file absent                  -> write
 * - file identical               -> no-op
 * - file differs, has marker,
 *   journal matches old content  -> write (regeneration)
 * - file differs from journal    -> DRIFT (user edited) -> refuse unless force
 * - file lacks marker            -> not ours -> refuse unless force
 * journal: <dir>/.lazydesk-hashes
 */
typedef enum {
	LD_WRITE_OK = 0,
	LD_WRITE_NOOP,
	LD_WRITE_DRIFT,
	LD_WRITE_FOREIGN,
	LD_WRITE_ERR,
} ld_write_result;

ld_write_result ld_write_managed(const char *path, const char *content,
                                 bool force, char *errbuf, size_t errlen);

/* Generate everything into out_dir. Returns first failing write via rc. */
bool ld_generate_all(const ld_config *c, const ld_theme *t, const char *out_dir,
                     bool force, char *errbuf, size_t errlen);

#endif
