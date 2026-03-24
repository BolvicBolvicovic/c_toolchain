#ifndef ZSTRING_H
#define ZSTRING_H

#include <c_types.h>
#include <arena.h>

// Name: zstring
// Description: ZII-string struct which is fast, secure and interoperable.
// Once defined, a zstring should remain constant.
typedef struct zstring zstring;
struct zstring
{
	u64	size;	// There is no need for a zstring_len() function. Just access the struct member.
	char*	data;
};

// --- Initialization ---
// Note that for an empty zstring, one should do the following:
// zstring	zstr = zstring_on_stack(0, 0);
// or
// zstring*	zstr = &zstring_on_stack(0, 0);
// or
// zstring	zstr = { 0 };
// or
// zstring*	zstr = &(zstring){ 0 };
// or
// zstring*	zstr = ARENA_PUSH_STRUCT(arena, zstring);
// or
// zstring*	zstr = zstring_on_heap(arena, 0, 0);

#define		zstring_on_stack(dt, sz) (zstring){.data=(dt), .size=(sz)}
zstring*	zstring_on_heap(arena_t* arena, char* data, u64 size);
zstring		zstring_slice_on_stack(zstring* z, u64 start, u64 end);
zstring*	zstring_slice_on_heap(arena_t* arena, zstring* z, u64 start, u64 end);
char*		zstring_as_cstring(arena_t* arena, zstring* z);

// --- Updates ---
// Note that updates that are modifying the lenght of a zstring *necessarly* create copies
// since strings are considered to be constant.
// However, I might add an update in-place section which does not change the size of a string.

zstring*	zstring_cat(arena_t* arena, zstring* z1, zstring* z2);
zstring*	zstring_split(arena_t* arena, zstring* z, zstring* pattern, u64* count);
zstring*	zstring_join(arena_t* arena, zstring* zstrs, u64 size, zstring* pattern);
zstring		zstring_trim(zstring* z, char* list);
zstring		zstring_trim_start(zstring* z, char* list);
zstring		zstring_trim_end(zstring* z, char* list);

// --- Compare ---

bool		zstring_cmp(zstring* z1, zstring* z2);

// --- Debug ---
// Note that if you want to use printf, use:
// - zstring_as_cstring_on_stack()
// - zstring_as_cstring_on_heap()

s32		zstring_print(zstring* z);

#endif
