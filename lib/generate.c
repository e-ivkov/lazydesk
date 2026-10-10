/* generate.c — emit managed artifacts (hyprland.conf, ghostty include,
 * Quickshell starter, registry.json) with drift-checked writes.
 */
#define _POSIX_C_SOURCE 200809L

#include "generate.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "bindings.h"
#include "cJSON.h"
#include "toml.h"

/* ---------- theme ---------- */

static void theme_str(toml_table_t *pal, const char *key, char *dst, size_t n)
{
	toml_datum_t d = toml_string_in(pal, key);
	if (d.ok) {
		snprintf(dst, n, "%s", d.u.s);
		free(d.u.s);
	}
}

bool ld_theme_load(ld_theme *t, const char *datadir, const char *name,
                   char *errbuf, size_t errlen)
{
	memset(t, 0, sizeof *t);
	char path[512];
	snprintf(path, sizeof path, "%s/themes/%s.toml", datadir, name);
	char err[256];
	FILE *f = fopen(path, "r");
	if (!f) {
		snprintf(errbuf, errlen, "theme '%s' not found (%s missing)", name, path);
		return false;
	}
	toml_table_t *root = toml_parse_file(f, err, sizeof err);
	fclose(f);
	if (!root) {
		snprintf(errbuf, errlen, "theme %s: %s", name, err);
		return false;
	}
	snprintf(t->name, sizeof t->name, "%s", name);
	toml_table_t *pal = toml_table_in(root, "palette");
	if (!pal) {
		snprintf(errbuf, errlen, "theme %s: missing [palette]", name);
		toml_free(root);
		return false;
	}
	theme_str(pal, "bg", t->bg, sizeof t->bg);
	theme_str(pal, "bg_alt", t->bg_alt, sizeof t->bg_alt);
	theme_str(pal, "surface", t->surface, sizeof t->surface);
	theme_str(pal, "text", t->text, sizeof t->text);
	theme_str(pal, "muted", t->muted, sizeof t->muted);
	theme_str(pal, "dim", t->dim, sizeof t->dim);
	theme_str(pal, "accent", t->accent, sizeof t->accent);
	theme_str(pal, "success", t->success, sizeof t->success);
	theme_str(pal, "warning", t->warning, sizeof t->warning);
	theme_str(pal, "error", t->error, sizeof t->error);
	theme_str(pal, "info", t->info, sizeof t->info);
	toml_table_t *font = toml_table_in(root, "font");
	if (font) {
		toml_datum_t d = toml_int_in(font, "size");
		if (d.ok) t->font_size = d.u.i;
	}
	toml_table_t *rad = toml_table_in(root, "radius");
	if (rad) {
		toml_datum_t d = toml_int_in(rad, "corner");
		if (d.ok) t->corner = d.u.i;
	}
	toml_free(root);
	return t->bg[0] && t->text[0];
}

/* ---------- hyprland.conf ---------- */

static void niri_workspace_binds(FILE *f, const ld_config *c)
{
	/* numbered archetypes bind to named workspaces; free numbers are ad-hoc */
	for (int n = 1; n <= 9; n++) {
		const ld_cfg_archetype *a = nullptr;
		for (size_t i = 0; i < c->narchetypes; i++)
			if (c->archetypes[i].number == n) { a = &c->archetypes[i]; break; }
		if (a) {
			fprintf(f, "    Mod+%d { focus-workspace \"%s\"; }\n", n, a->id);
			fprintf(f, "    Mod+Shift+%d { move-column-to-workspace \"%s\"; }\n", n, a->id);
		} else {
			fprintf(f, "    Mod+%d { focus-workspace %d; }\n", n, n);
			fprintf(f, "    Mod+Shift+%d { move-column-to-workspace %d; }\n", n, n);
		}
	}
	fprintf(f, "    Mod+0 { focus-workspace 10; }\n");
	fprintf(f, "    Mod+Shift+0 { move-column-to-workspace 10; }\n");
}

size_t ld_emit_niri(const ld_config *c, char *buf, size_t buflen)
{
	size_t cap = 32768;
	char *tmp = malloc(cap);
	FILE *f = fmemopen(tmp, cap, "w");

	fprintf(f, "%s\n\n", LD_MANAGED_HEADER_KDL);
	fprintf(f, "// --- environment\n");
	fprintf(f, "environment {\n");
	fprintf(f, "    LAZYDESK \"1\"\n");
	fprintf(f, "    LAZYDESK_SOCKET \"$XDG_RUNTIME_DIR/lazydesk/ipc.sock\"\n");
	fprintf(f, "    XDG_CURRENT_DESKTOP \"lazydesk\"\n");
	fprintf(f, "}\n\n");

	fprintf(f, "// --- window policy: maximized by default, dialogs float\n");
	fprintf(f, "window-rule {\n    open-maximized true\n}\n\n");

	fprintf(f, "// --- archetype routing (app-id -> named workspace)\n");
	for (size_t i = 0; i < c->narchetypes; i++) {
		const ld_cfg_archetype *a = &c->archetypes[i];
		if (a->number && a->class_regex[0])
			fprintf(f, "window-rule {\n    match app-id=r#\"^%s$\"#\n    open-on-workspace \"%s\"\n}\n\n",
			        a->class_regex, a->id);
	}

	fprintf(f, "// --- binds (from the action registry)\n");
	fprintf(f, "binds {\n");
	size_t nb;
	const ld_binding *bs = ld_bindings(&nb);
	char line[512];
	for (size_t i = 0; i < nb; i++) {
		size_t n = ld_binding_niri(&bs[i], c, line, sizeof line);
		fwrite(line, 1, n, f);
		fputc('\n', f);
	}
	niri_workspace_binds(f, c);
	fprintf(f, "}\n\n");

	fprintf(f, "// --- startup\n");
	fprintf(f, "spawn-at-startup \"lazydeskd\"\n");
	fprintf(f, "spawn-at-startup \"quickshell\" \"-p\" \"$HOME/.config/lazydesk/generated/shell\"\n");
	for (size_t i = 0; i < c->nspawns; i++) {
		if (!c->spawns[i].autostart)
			continue;
		const ld_cfg_archetype *a = ld_config_archetype(c, c->spawns[i].archetype);
		char args[512];
		size_t off = 0;
		char buf2[256];
		snprintf(buf2, sizeof buf2, "lazydeskctl spawn --archetype %s -- %s",
		         c->spawns[i].archetype, a ? a->command : "");
		char *save = nullptr;
		for (char *tok = strtok_r(buf2, " ", &save); tok; tok = strtok_r(nullptr, " ", &save))
			off += snprintf(args + off, sizeof args - off, "%s\"%s\"", off ? " " : "", tok);
		fprintf(f, "spawn-at-startup %s\n", args);
	}

	size_t n = (size_t)ftell(f);
	fclose(f);
	if (n >= buflen) n = buflen - 1;
	memcpy(buf, tmp, n);
	buf[n] = '\0';
	free(tmp);
	return n;
}

/* ---------- ghostty include ---------- */

size_t ld_emit_ghostty(const ld_config *c, const ld_theme *t, char *buf, size_t buflen)
{
	FILE *f = fmemopen(buf, buflen, "w");
	fprintf(f, "%s\n", LD_MANAGED_HEADER);
	fprintf(f, "\n# theme\n");
	fprintf(f, "background = %s\n", t->bg);
	fprintf(f, "foreground = %s\n", t->text);
	fprintf(f, "selection-background = %s\n", t->surface);
	fprintf(f, "cursor-color = %s\n", t->accent);
	fprintf(f, "\n# scratchpad (quick terminal): Super+grave toggles it globally\n");
	fprintf(f, "quick-terminal-position = top\n");
	fprintf(f, "quick-terminal-autohide = false\n");
	fprintf(f, "keybind = global:super+grave=toggle_quick_terminal\n");
	fprintf(f, "\n# scratchpad command: %s\n", c->scratchpad_command);
	size_t n = (size_t)ftell(f);
	fclose(f);
	return n;
}

/* ---------- quickshell starter ---------- */

size_t ld_emit_theme_qml(const ld_theme *t, char *buf, size_t buflen)
{
	FILE *f = fmemopen(buf, buflen, "w");
	fprintf(f, "// %s\npragma Singleton\nimport QtQuick\n\nQtObject {\n", LD_MANAGED_HEADER);
	fprintf(f, "    readonly property color bg:      \"%s\"\n", t->bg);
	fprintf(f, "    readonly property color bgAlt:   \"%s\"\n", t->bg_alt);
	fprintf(f, "    readonly property color surface: \"%s\"\n", t->surface);
	fprintf(f, "    readonly property color text:    \"%s\"\n", t->text);
	fprintf(f, "    readonly property color muted:   \"%s\"\n", t->muted);
	fprintf(f, "    readonly property color accent:  \"%s\"\n", t->accent);
	fprintf(f, "    readonly property color success: \"%s\"\n", t->success);
	fprintf(f, "    readonly property color warning: \"%s\"\n", t->warning);
	fprintf(f, "    readonly property color error:   \"%s\"\n", t->error);
	fprintf(f, "    readonly property color info:    \"%s\"\n", t->info);
	fprintf(f, "    readonly property int cornerRadius: %ld\n", t->corner);
	fprintf(f, "    readonly property int fontSize: %ld\n", t->font_size);
	fprintf(f, "}\n");
	size_t n = (size_t)ftell(f);
	fclose(f);
	return n;
}

size_t ld_emit_shell_qml(char *buf, size_t buflen)
{
	static const char *qml =
	"// lazydesk-managed — starter bar; grows into menu/cheatsheet/notifications (task group 5)\n"
	"import Quickshell\n"
	"import QtQuick\n"
	"import \".\"\n"
	"\n"
	"ShellRoot {\n"
	"    PanelWindow {\n"
	"        anchors { top: true; left: true; right: true }\n"
	"        implicitHeight: 28\n"
	"        color: Theme.bg\n"
	"\n"
	"        Text {\n"
	"            anchors.centerIn: parent\n"
	"            color: Theme.text\n"
	"            font.family: \"monospace\"\n"
	"            text: \"lazydesk\"\n"
	"        }\n"
	"    }\n"
	"}\n";
	size_t n = strlen(qml);
	if (n >= buflen) n = buflen - 1;
	memcpy(buf, qml, n);
	buf[n] = '\0';
	return n;
}

/* ---------- registry.json (menu + cheatsheet data) ---------- */

char *ld_emit_registry_json(void)
{
	cJSON *root = cJSON_CreateArray();
	size_t nb;
	const ld_binding *bs = ld_bindings(&nb);
	for (size_t i = 0; i < nb; i++) {
		cJSON *o = cJSON_CreateObject();
		cJSON_AddStringToObject(o, "action", bs[i].action);
		cJSON_AddStringToObject(o, "group", bs[i].group);
		cJSON_AddStringToObject(o, "combo", bs[i].combo);
		cJSON_AddStringToObject(o, "description", bs[i].description);
		cJSON_AddItemToArray(root, o);
	}
	char *s = cJSON_Print(root);
	cJSON_Delete(root);
	return s;
}

/* ---------- drift-checked managed writes ---------- */

static unsigned long long fnv1a(const char *s)
{
	unsigned long long h = 1469598103934665603ULL;
	while (*s) {
		h ^= (unsigned char)*s++;
		h *= 1099511628211ULL;
	}
	return h;
}

static void journal_path(const char *filepath, char *out, size_t n)
{
	/* <dir>/.lazydesk-hashes */
	const char *slash = strrchr(filepath, '/');
	size_t dlen = slash ? (size_t)(slash - filepath) : strlen(filepath);
	snprintf(out, n, "%.*s/.lazydesk-hashes", (int)dlen, filepath);
}

static bool journal_get(const char *jp, const char *filepath, unsigned long long *hash)
{
	FILE *f = fopen(jp, "r");
	if (!f) return false;
	char line[1024];
	bool found = false;
	while (fgets(line, sizeof line, f)) {
		unsigned long long h;
		char p[768];
		if (sscanf(line, "%llu %767s", &h, p) == 2 && strcmp(p, filepath) == 0) {
			*hash = h;
			found = true;
			break;
		}
	}
	fclose(f);
	return found;
}

static void journal_set(const char *jp, const char *filepath, unsigned long long hash)
{
	/* rewrite journal entry for filepath */
	FILE *f = fopen(jp, "r");
	char tmp[512];
	snprintf(tmp, sizeof tmp, "%s.tmp", jp);
	FILE *o = fopen(tmp, "w");
	if (!o) { if (f) fclose(f); return; }
	if (f) {
		char line[1024];
		while (fgets(line, sizeof line, f)) {
			unsigned long long h;
			char p[768];
			if (sscanf(line, "%llu %767s", &h, p) == 2 && strcmp(p, filepath) == 0)
				continue; /* drop old entry */
			fputs(line, o);
		}
		fclose(f);
	}
	fprintf(o, "%llu %s\n", hash, filepath);
	fclose(o);
	rename(tmp, jp);
}

static char *read_file_str(const char *path)
{
	FILE *f = fopen(path, "rb");
	if (!f) return nullptr;
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (n < 0) { fclose(f); return nullptr; }
	char *buf = malloc(n + 1);
	size_t rd = fread(buf, 1, n, f);
	fclose(f);
	buf[rd] = '\0';
	return buf;
}

ld_write_result ld_write_managed(const char *path, const char *content,
                                 bool force, char *errbuf, size_t errlen)
{
	char *cur = read_file_str(path);
	if (!cur) {
		if (errno != ENOENT) {
			snprintf(errbuf, errlen, "%s: %s", path, strerror(errno));
			return LD_WRITE_ERR;
		}
		/* new file */
		FILE *f = fopen(path, "w");
		if (!f) { snprintf(errbuf, errlen, "%s: %s", path, strerror(errno)); return LD_WRITE_ERR; }
		fputs(content, f);
		fclose(f);
		char jp[512]; journal_path(path, jp, sizeof jp);
		journal_set(jp, path, fnv1a(content));
		return LD_WRITE_OK;
	}

	if (strcmp(cur, content) == 0) { free(cur); return LD_WRITE_NOOP; }

	bool has_marker = strstr(cur, "lazydesk-managed") != nullptr;
	if (!has_marker) {
		free(cur);
		cur = nullptr;
		if (!force) {
			snprintf(errbuf, errlen,
			         "%s: not marked lazydesk-managed (foreign file); rerun with --force to take it over",
			         path);
			return LD_WRITE_FOREIGN;
		}
	} else {
		char jp[512]; journal_path(path, jp, sizeof jp);
		unsigned long long known;
		if (journal_get(jp, path, &known) && known != fnv1a(cur)) {
			free(cur);
			cur = nullptr;
			if (!force) {
				snprintf(errbuf, errlen,
				         "%s: drifted (hand-edited since last generation); resolve the edit or rerun with --force",
				         path);
				return LD_WRITE_DRIFT;
			}
		}
	}
	free(cur);

	FILE *f = fopen(path, "w");
	if (!f) { snprintf(errbuf, errlen, "%s: %s", path, strerror(errno)); return LD_WRITE_ERR; }
	fputs(content, f);
	fclose(f);
	char jp[512]; journal_path(path, jp, sizeof jp);
	journal_set(jp, path, fnv1a(content));
	return LD_WRITE_OK;
}

static void mkdirs(const char *path)
{
	char tmp[512];
	snprintf(tmp, sizeof tmp, "%s", path);
	for (char *p = tmp + 1; *p; p++)
		if (*p == '/') { *p = '\0'; mkdir(tmp, 0755); *p = '/'; }
	mkdir(tmp, 0755);
}

bool ld_generate_all(const ld_config *c, const ld_theme *t, const char *out_dir,
                     bool force, char *errbuf, size_t errlen)
{
	static char buf[32768];
	char path[600];
	mkdirs(out_dir);

	ld_write_result rc = LD_WRITE_OK;

	snprintf(path, sizeof path, "%s/niri.kdl", out_dir);
	ld_emit_niri(c, buf, sizeof buf);
	rc = ld_write_managed(path, buf, force, errbuf, errlen);
	if (rc == LD_WRITE_DRIFT || rc == LD_WRITE_FOREIGN || rc == LD_WRITE_ERR)
		return false;

	snprintf(path, sizeof path, "%s/ghostty-lazydesk.conf", out_dir);
	ld_emit_ghostty(c, t, buf, sizeof buf);
	rc = ld_write_managed(path, buf, force, errbuf, errlen);
	if (rc == LD_WRITE_DRIFT || rc == LD_WRITE_FOREIGN || rc == LD_WRITE_ERR)
		return false;

	snprintf(path, sizeof path, "%s/shell", out_dir);
	mkdirs(path);
	snprintf(path, sizeof path, "%s/shell/Theme.qml", out_dir);
	ld_emit_theme_qml(t, buf, sizeof buf);
	rc = ld_write_managed(path, buf, force, errbuf, errlen);
	if (rc == LD_WRITE_DRIFT || rc == LD_WRITE_FOREIGN || rc == LD_WRITE_ERR)
		return false;

	snprintf(path, sizeof path, "%s/shell/shell.qml", out_dir);
	ld_emit_shell_qml(buf, sizeof buf);
	rc = ld_write_managed(path, buf, force, errbuf, errlen);
	if (rc == LD_WRITE_DRIFT || rc == LD_WRITE_FOREIGN || rc == LD_WRITE_ERR)
		return false;

	snprintf(path, sizeof path, "%s/registry.json", out_dir);
	char *json = ld_emit_registry_json();
	rc = ld_write_managed(path, json, force, errbuf, errlen);
	free(json);
	if (rc == LD_WRITE_DRIFT || rc == LD_WRITE_FOREIGN || rc == LD_WRITE_ERR)
		return false;

	return true;
}
