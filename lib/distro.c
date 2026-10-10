/* distro.c — /etc/os-release detection and installed-package probes. */
#include "lazydesk.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_small_file(const char *path)
{
	FILE *f = fopen(path, "rb");
	if (!f)
		return nullptr;
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (n <= 0 || n > 64 * 1024) {
		fclose(f);
		return nullptr;
	}
	char *buf = malloc((size_t)n + 1);
	if (!buf) {
		fclose(f);
		return nullptr;
	}
	size_t rd = fread(buf, 1, (size_t)n, f);
	fclose(f);
	buf[rd] = '\0';
	return buf;
}

/* Extract KEY= value from os-release content (unquotes, returns strdup or NULL). */
static char *osr_get(const char *content, const char *key)
{
	size_t klen = strlen(key);
	const char *p = content;
	while ((p = strstr(p, key)) != nullptr) {
		bool at_bol = (p == content || p[-1] == '\n');
		if (at_bol && p[klen] == '=') {
			const char *v = p + klen + 1;
			while (*v == ' ' || *v == '\t')
				v++;
			char buf[128];
			size_t i = 0;
			if (*v == '"' || *v == '\'') {
				char q = *v++;
				while (*v && *v != q && *v != '\n' && i < sizeof(buf) - 1)
					buf[i++] = *v++;
			} else {
				while (*v && *v != '\n' && i < sizeof(buf) - 1)
					buf[i++] = *v++;
			}
			buf[i] = '\0';
			/* take first word of ID_LIKE lists */
			char *sp = strchr(buf, ' ');
			if (sp)
				*sp = '\0';
			return strdup(buf);
		}
		p += klen;
	}
	return nullptr;
}

ld_distro ld_distro_detect(void)
{
	const char *override = getenv("LAZYDESK_DISTRO");
	if (override && *override) {
		if (strcmp(override, "void") == 0)   return LD_DISTRO_VOID;
		if (strcmp(override, "debian") == 0) return LD_DISTRO_DEBIAN;
		if (strcmp(override, "ubuntu") == 0) return LD_DISTRO_UBUNTU;
	}
	char *content = read_small_file("/etc/os-release");
	if (!content)
		content = read_small_file("/usr/lib/os-release");
	if (!content)
		return LD_DISTRO_UNKNOWN;

	ld_distro d = LD_DISTRO_UNKNOWN;
	char *id = osr_get(content, "ID");
	char *like = osr_get(content, "ID_LIKE");

	if (id) {
		if (strcmp(id, "void") == 0)         d = LD_DISTRO_VOID;
		else if (strcmp(id, "debian") == 0)  d = LD_DISTRO_DEBIAN;
		else if (strcmp(id, "ubuntu") == 0)  d = LD_DISTRO_UBUNTU;
	}
	if (d == LD_DISTRO_UNKNOWN && like) {
		if (strcmp(like, "debian") == 0)     d = LD_DISTRO_DEBIAN; /* ubuntu sets ID_LIKE=debian */
	}
	free(id);
	free(like);
	free(content);
	return d;
}

const char *ld_distro_id(ld_distro d)
{
	switch (d) {
	case LD_DISTRO_VOID:   return "void";
	case LD_DISTRO_DEBIAN: return "debian";
	case LD_DISTRO_UBUNTU: return "ubuntu";
	default:               return "unknown";
	}
}

const char *ld_distro_name(ld_distro d)
{
	switch (d) {
	case LD_DISTRO_VOID:   return "Void Linux";
	case LD_DISTRO_DEBIAN: return "Debian";
	case LD_DISTRO_UBUNTU: return "Ubuntu";
	default:               return "unknown distro";
	}
}

bool ld_pkg_installed(ld_distro d, const char *pkg)
{
	if (d == LD_DISTRO_UNKNOWN || !pkg || !*pkg)
		return false;

	char cmd[512];
	if (d == LD_DISTRO_VOID)
		snprintf(cmd, sizeof cmd, "xbps-query '%s' >/dev/null 2>&1", pkg);
	else
		snprintf(cmd, sizeof cmd,
		         "dpkg-query -W -f='${Status}' '%s' 2>/dev/null | grep -q 'ok installed'",
		         pkg);
	return system(cmd) == 0;
}
