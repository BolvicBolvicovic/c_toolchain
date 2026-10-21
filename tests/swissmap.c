#include "vtest.h"
#include "swissmap.h"

#include <assert.h>
#include <arena.h>

SWISSMAP_DECLARE_DEFAULT(floatmap, float);
SWISSMAP_DEFINE_FUNCTIONS_DEFAULT(floatmap, float);

static floatmap* swissmap = 0;
static char*     test     = "test";
static f64       value    = 42;

TEST(GetFailure)
{
	ASSERT_FAILURE(floatmap_get(swissmap, test));
}

TEST(FullCycleSuccess)
{
	floatmap_put(swissmap, test, value);

	ASSERT_SUCCESS(floatmap_get(swissmap, test));
	ASSERT_SUCCESS(*floatmap_get(swissmap, test) == 42);

	floatmap_delete(swissmap, test);

	ASSERT_FAILURE(floatmap_get(swissmap, test));
}

TEST_WITH_ARG(DynamicStringKey, _ss_arena)
{
	arena_t*  arena   = _ss_arena;
	char*     literal = ARENA_PUSH_ARRAY(arena, char, 5);
	const u32 sm_cap  = 16;
	floatmap* sm	  = floatmap_new(arena, sm_cap);

	memcpy(literal, test, 5);

	for (u32 i = 0; i <= sm_cap; i++)
	{
		literal[1] += i;

		char*	key = ARENA_PUSH_ARRAY(arena, char, 5);

		memcpy(key, literal, 5);
		floatmap_put(sm, key, (f64)i);

		if (i == sm_cap)
			ASSERT_FAILURE(floatmap_get(sm, literal));
		else
		{
			ASSERT_SUCCESS(floatmap_get(sm, literal));
			ASSERT_SUCCESS(*floatmap_get(sm, literal) == i);
		}
	}
}

SETUP_SUITE()
{
	swissmap   = floatmap_new(_ss_arena, 1);

	TEST_INPUT_PTR(DynamicStringKey, _ss_arena);
	ASSERT_SUCCESS(swissmap);
}
