#include "arena.h"
#include "vtest.h"
#define ZSTRING_IMPL
#include "zstring.h"

TEST_WITH_ARG(Slice, _ss_arena)
{
	arena_t* arena = _ss_arena;
	char     str[] = "Sea, sex and sun.";
	zstring* zstr  = zstring_on_heap(arena, str, sizeof(str));
	zstring* szstr = zstring_slice_on_heap(arena, zstr, 5, 8);

	ASSERT_SUCCESS(zstring_cmp(&zstring_on_stack(str + 5, 3), szstr));
	ASSERT_SUCCESS(strcmp(str, zstring_as_cstring(arena, zstr)) == 0);
}

TEST_WITH_ARG(Cat, _ss_arena)
{
	arena_t* arena = _ss_arena;
	zstring* z1 = &zstring_on_stack("Sea, sex ", 9);
	zstring* z2 = &zstring_on_stack("and sun.", 8);
	zstring* z3 = zstring_cat(arena, z1, z2);

	ASSERT_SUCCESS(zstring_cmp(&zstring_on_stack("Sea, sex and sun.", 17), z3));
}

TEST_WITH_ARG(SplitJoin, _ss_arena)
{
	arena_t* arena = _ss_arena;

	u64      count = 0;
	char     str[] = "Sea, sex and sun.";
	zstring* zstr  = &zstring_on_stack(str, sizeof(str));
	zstring* sstr  = zstring_split(arena, zstr, &zstring_on_stack(", sex ", 6), &count);
	zstring* jstr  = zstring_join(arena, sstr, count, &zstring_on_stack(", sex ", 6));

	ASSERT_SUCCESS(count == 2);
	ASSERT_SUCCESS(strcmp("Sea", zstring_as_cstring(arena, sstr)) == 0);
	ASSERT_SUCCESS(strcmp("and sun.", zstring_as_cstring(arena, sstr + 1)) == 0);
	ASSERT_SUCCESS(zstring_cmp(zstr, jstr));
}

TEST_WITH_ARG(Trim, _ss_arena)
{
	arena_t* arena = _ss_arena;
	char     str[] = "Sea, sex and sun.";
	zstring* zstr  = &zstring_on_stack(str, sizeof(str) - 1);
	zstring  lstr  = zstring_trim_start(zstr, "Sea,");
	zstring  rstr  = zstring_trim_end(&lstr, "sun.");
	zstring  sstr  = zstring_trim(&rstr, " and");
	
	ASSERT_SUCCESS(strcmp(" sex and sun.", zstring_as_cstring(arena, &lstr)) == 0);
	ASSERT_SUCCESS(strcmp(" sex and ", zstring_as_cstring(arena, &rstr)) == 0);
	ASSERT_SUCCESS(strcmp("sex", zstring_as_cstring(arena, &sstr)) == 0);
}

SETUP_SUITE()
{
	TEST_INPUT_PTR(Slice, _ss_arena);
	TEST_INPUT_PTR(Cat, _ss_arena);
	TEST_INPUT_PTR(SplitJoin, _ss_arena);
	TEST_INPUT_PTR(Trim, _ss_arena);
}
