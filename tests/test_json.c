/* cJSON round-trip: parse -> print -> parse -> identical print. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

static char *slurp(const char *path)
{
	FILE *f = fopen(path, "rb");
	if (!f) { perror(path); exit(1); }
	fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
	char *buf = malloc(n + 1);
	if (fread(buf, 1, n, f) != (size_t)n) exit(1);
	buf[n] = 0; fclose(f);
	return buf;
}

int main(int argc, char **argv)
{
	if (argc < 2) return 2;

	/* 1. generated document round-trip */
	cJSON *doc = cJSON_CreateObject();
	cJSON_AddStringToObject(doc, "op", "workspace.focus");
	cJSON_AddNumberToObject(doc, "workspace", 2);
	cJSON_AddStringToObject(doc, "name", "editor");
	cJSON *arr = cJSON_AddArrayToObject(doc, "windows");
	cJSON_AddItemToArray(arr, cJSON_CreateString("ghostty"));
	cJSON_AddItemToArray(arr, cJSON_CreateString("zed"));

	char *p1 = cJSON_PrintUnformatted(doc);
	cJSON *re = cJSON_Parse(p1);
	if (!re) { fprintf(stderr, "reparse failed\n"); return 1; }
	char *p2 = cJSON_PrintUnformatted(re);
	if (strcmp(p1, p2) != 0) { fprintf(stderr, "round trip mismatch:\n%s\n%s\n", p1, p2); return 1; }
	if (!cJSON_HasObjectItem(re, "op")) return 1;
	free(p1); free(p2); cJSON_Delete(doc); cJSON_Delete(re);

	/* 2. parse a real file (ipc examples) and verify access */
	char *in = slurp(argv[1]);
	cJSON *j = cJSON_Parse(in);
	if (!j) { fprintf(stderr, "parse %s failed\n", argv[1]); return 1; }
	cJSON *op = cJSON_GetObjectItem(j, "op");
	if (!cJSON_IsString(op) || strcmp(op->valuestring, "windows.list") != 0) {
		fprintf(stderr, "unexpected op in fixture\n");
		return 1;
	}
	cJSON_Delete(j);
	free(in);

	printf("json round-trip: OK\n");
	return 0;
}
