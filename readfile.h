#ifndef READFILE_H
#define READFILE_H

#include <stdio.h>
#include <arena.h>

static char*
readfile(arena_t* arena, const char* path)
{
	FILE* f = fopen(path, "rb");
	if (!f) {
	fprintf(stderr, "Failed to open %s\n", path);
	return NULL;
	}
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	rewind(f);
	
	char* buf = ARENA_PUSH_ARRAY(arena, char, size + 1);
	if (!buf) { fclose(f); return NULL; }
	
	size_t read = fread(buf, 1, size, f);
	buf[read] = '\0';
	fclose(f);
	return buf;
}

#endif
