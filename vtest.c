#include "vtest.h"
#include "c_types.h"
#include "arena.h"
#include "swissmap.h"
#include <stdio.h>
#include <stdbool.h>

typedef struct Suite Suite;
struct Suite
{
#define X(type) List type;
TEST_LIST
#undef X
#define X(type) Vtest* type;
META_TEST_LIST
#undef X
	char*     file;
};

inline static u64
_hash(char* key)
{
	u64 x = (u64)(key);
	
	x ^= x >> 30;
	x *= 0xbf58476d1ce4e5b9ULL;
	x ^= x >> 27;
	x *= 0x94d049bb133111ebULL;
	x ^= x >> 31;
	
	return x;
}

inline static bool
_eq(char* p1, char* p2) { return p1 == p2; }

SWISSMAP_DECLARE_DEFAULT(SuiteHM, Suite*);
SWISSMAP_DEFINE_FUNCTIONS(SuiteHM, char*, Suite*, _hash, _eq);

#ifndef SUITES_CAPACITY
#define SUITES_CAPACITY 500
#endif

AssertCtx g_assert_ctx;

#define X(type) __attribute__((section(#type), used)) Vtest _guard_##type = {0};
SECTION_LIST
#undef X

// TODO: Add command line to select test/suite to run
s32
main(s32 argc, char** argv)
{
	(void)argc;
	(void)argv;
#define X(Type) s32 count##Type = (__stop_##Type - __start_##Type);
TEST_LIST
#undef X

	arena_t*  arena        = ARENA_ALLOC();
	SuiteHM*  suites       = SuiteHM_new(arena, SUITES_CAPACITY);
	s32       successCount = 0;
	const u32 testsCount   = countVtestTypeTest + countVtestTypeTestWithArg - 2; // -2 for guards
	char*     suiteFile    = 0;
	Suite*    suite        = 0;


#define NewSuite(Type)                                                                       \
	if (test == &_guard_##Type) continue;                                                 \
	do { if (test->file != suiteFile)                                                    \
	{                                                                                    \
		Suite** suite_ptr = SuiteHM_get(suites, test->file);                         \
		suite             = suite_ptr ? *suite_ptr : ARENA_PUSH_STRUCT(arena, Suite);\
		suiteFile         = test->file;                                              \
		suite->file       = test->file;                                              \
		if (suite_ptr == 0)                                                          \
		{                                                                            \
			SuiteHM_put(suites, test->file, suite);                              \
		}                                                                            \
	} } while (0)
#define X(Type)                                                         \
	for (Vtest* test = __start_##Type; test < __stop_##Type; test++)\
	{                                                               \
		NewSuite(Type);                                         \
		Node* node       = ARENA_PUSH_STRUCT(arena, Node);      \
		node->data       = test;                                \
		if (suite->Type.size == 0)                              \
		{                                                       \
			suite->Type.head = node;                        \
		}                                                       \
		else                                                    \
		{                                                       \
			suite->Type.tail->next = node;                  \
		}                                                       \
		suite->Type.tail = node;                                \
		suite->Type.size++;                                     \
	}                                                               \
	suiteFile = 0, suite = 0;
	TEST_LIST
#undef X
#define X(Type)                                                                           \
	for (Vtest* test = __start_##Type; test < __stop_##Type; test++)                  \
	{                                                                                 \
		NewSuite(Type);                                                           \
		if (suite->Type)                                                          \
		{                                                                         \
			printf("Updating already defined %s in %s\n", #Type, suite->file);\
		}                                                                         \
		suite->Type = test;                                                       \
	}                                                                                 \
	suiteFile = 0, suite = 0;
	META_TEST_LIST
#undef NewSuite
#undef X

	printf("--- VTEST START ---\n");
	printf("Discovered %u tests for %lu suits\n", testsCount, suites->size);

	for (u32 i = 0; i < suites->capacity; i++)
	{
		if (suites->ctrl[i] == SWISS_DELETED || suites->ctrl[i] == SWISS_EMPTY) continue;
		suite                 = suites->values[i];
		s32   suiteTestsCount = suite->VtestTypeTest.size + suite->VtestTypeTestWithArg.size;

		printf("\nRunning %d tests for suit %s\n", suiteTestsCount, suite->file);

		if (suite->VtestTypePreTransUnit)
		{
			suite->VtestTypePreTransUnit->func(arena);
		}
	
		for (Node* nodeTest = suite->VtestTypeTest.head; nodeTest; nodeTest=nodeTest->next)
		{
			Vtest* test = nodeTest->data;
			s32    jump = setjmp(g_assert_ctx.jmpbuffer);

			if (jump == 0)
			{
				if (suite->VtestTypePreTest)
				{
					suite->VtestTypePreTest->func(0);
				}
				test->func(0);
				if (suite->VtestTypePostTest)
				{
					suite->VtestTypePostTest->func(0);
				}
				successCount++;
			}
			else
			{
				printf("\n%s FAILED:\n", test->name);
				printf("In %s at line %d: '%s'\n", test->file, g_assert_ctx.line, g_assert_ctx.assertion);
			}
		}
		
		for (Node* nodeTest = suite->VtestTypeTestWithArg.head; nodeTest; nodeTest=nodeTest->next)
		{
			Vtest* test    = nodeTest->data;
			List*  args    = test->args;
			s32    argi    = 0;
			bool   success = true;

			if (args == 0)
			{
				printf("\nNo arguments registered for test %s. Skipping...\n", test->name);
				continue;
			}
			else
			{
				printf("\n%d arguments found for test %s\n", args->size, test->name);
			}

			for (Node* nodeArg = args->head; nodeArg; nodeArg=nodeArg->next, argi++)
			{
				s32    jump = setjmp(g_assert_ctx.jmpbuffer);

				if (jump == 0)
				{
					if (suite->VtestTypePreTest)
					{
						suite->VtestTypePreTest->func(0);
					}
					test->func(nodeArg->data);
					if (suite->VtestTypePostTest)
					{
						suite->VtestTypePostTest->func(0);
					}
				}
				else
				{
					printf("\n%s FAILED WITH ARGS[%d]:\n", test->name, argi);
					printf("In %s at line %d: '%s'\n", test->file, g_assert_ctx.line, g_assert_ctx.assertion);
					success = false;
				}
			}

			successCount += success;
		}

		if (suite->VtestTypePostTransUnit)
		{
			suite->VtestTypePostTransUnit->func(0);
		}
	}

	printf("\nSuccessful tests: %u/%u\n", successCount, testsCount);
	printf("--- VTEST DONE ---\n");

	return 0;
}
