/*************************************************
 *         VTEST - Simple Test Framework         *
 *                    Runner                     *
 *************************************************/

#define ARENA_IMPL
#define SWISSMAP_IMPL

#include "arena.h"
#include "swissmap.h"
#include "vtest.h"
#include "c_types.h"
#include <stdio.h>
#include <stdbool.h>
#define _GNU_SOURCE
#include <getopt.h>


/**
 * X macro list for the two test types.
 */
#define _TEST_LIST             \
	X(VtestTypeTestWithArg)\
	X(VtestTypeTest)

/**
 * X macro list for the meta functions executed around tests.
 */
#define _META_TEST_LIST          \
	X(VtestTypePreTest)      \
	X(VtestTypeSetupSuite) \
	X(VtestTypePostTest)     \
	X(VtestTypeTeardownSuite)

/**
 * X macro list for all elements using sections as compile-time typed arrays.
 */
#define _SECTION_LIST             \
	_TEST_LIST                \
	_META_TEST_LIST

/**
 * X macro to define the start and end of each array.
 * @param Type Name of the section as a discriminating type for array of registered functions.
 */
#define X(Type)                       \
	extern Vtest __start_##Type[];\
	extern Vtest __stop_##Type[];
_SECTION_LIST
#undef X

/** struct Suite!
 *  Contains all tests and meta functions related to testing of a single file.
 *  @param Type Name of the section as a discriminating type for array of registered functions.
 */
typedef struct Suite Suite;
struct Suite
{
#define X(Type) List Type;
_TEST_LIST
#undef X
#define X(Type) Vtest* Type;
_META_TEST_LIST
#undef X
	char*     file;
};


/** struct SuiteHM!
 *  Fixed size hashmap of suites.
 *  Its size can be paramerized with the -DSUITES_CAPACITY flag at compile-time.
 *  Default to 100.
 *  Defined in "swissmap.h"
 */
SWISSMAP_DECLARE_DEFAULT(SuiteHM, Suite*);
SWISSMAP_DEFINE_FUNCTIONS_DEFAULT(SuiteHM, Suite*);
#ifndef SUITES_CAPACITY
#define SUITES_CAPACITY 100
#endif


/**
 * Helper to run a test, optionnaly prefixed/suffixed with meta functions.
 * @param test  Vtest*
 * @param suite Suite*
 * @param arg   void*
 */
#define RUN_TEST(test, suite, arg) do {           \
	if (suite->VtestTypePreTest)              \
		suite->VtestTypePreTest->func(0); \
	test->func(arg);                          \
	if (suite->VtestTypePostTest)             \
		suite->VtestTypePostTest->func(0);\
	} while (0)

/**
 * Helper to optionnaly run the setup of a suite.
 * @param suite Suite*
 * @param arena arena_t* Defined in "arena.h"
 */
#define RUN_SETUP_SUITE(suite, arena) do {             \
	if (suite->VtestTypeSetupSuite)                 \
		suite->VtestTypeSetupSuite->func(arena);\
	} while (0)

/**
 * Helper to optionnaly run the teardown of a suite.
 * @param suite Suite*
 */
#define RUN_TEARDOWN_SUITE(suite) do {                \
	if (suite->VtestTypeTeardownSuite)             \
		suite->VtestTypeTeardownSuite->func(0);\
	} while (0)

/**
 * Helper to run a suite. Do not use outside of main.
 * @param suite           Suite*
 * @param suiteTestsCount u32    Count of both tests and tests with args.
 * @param successCount    u32    Count of total successful tests.
 */
#define RUN_SUITE(suite, suiteTestsCount, successCount) do {                                       \
	printf("\nRunning %d tests for suite %s\n", suiteTestsCount, suite->file);                 \
	for (Node* nodeTest = suite->VtestTypeTest.head; nodeTest; nodeTest=nodeTest->next)        \
	{                                                                                          \
		Vtest* test = nodeTest->data;                                                      \
		s32    jump = setjmp(g_assert_ctx.jmpbuffer);                                      \
		if (jump == 0)                                                                     \
		{                                                                                  \
			RUN_TEST(test, suite, 0);                                                  \
			successCount++;                                                            \
		}                                                                                  \
		else                                                                               \
		{                                                                                  \
			TEST_FAILED(test);                                                         \
		}                                                                                  \
	}                                                                                          \
	for (Node* nodeTest = suite->VtestTypeTestWithArg.head; nodeTest; nodeTest=nodeTest->next) \
	{                                                                                          \
		Vtest* test    = nodeTest->data;                                                   \
		List*  args    = test->args;                                                       \
		s32    argi    = 0;                                                                \
		bool   success = true;                                                             \
		if (args == 0)                                                                     \
		{                                                                                  \
			printf("\nNo arguments registered for test %s. Skipping...\n", test->name);\
			continue;                                                                  \
		}                                                                                  \
		else                                                                               \
		{                                                                                  \
			printf("\n%d arguments found for test %s\n", args->size, test->name);      \
		}                                                                                  \
		for (Node* nodeArg = args->head; nodeArg; nodeArg=nodeArg->next, argi++)           \
		{                                                                                  \
			s32    jump = setjmp(g_assert_ctx.jmpbuffer);                              \
			if (jump == 0)                                                             \
			{                                                                          \
				RUN_TEST(test, suite, nodeArg->data);                              \
			}                                                                          \
			else                                                                       \
			{                                                                          \
				TEST_FAILED_ARG(test, argi);                                       \
				success = false;                                                   \
			}                                                                          \
		}                                                                                  \
		successCount += success;                                                           \
	} } while (0)

/**
 * Helper to print failure message.
 * @param test  Vtest*
 */
#define TEST_FAILED(test) do {                                                                    \
	printf("\n%s FAILED:\n", test->name);                                                     \
	printf("In %s at line %d: '%s'\n", test->file, g_assert_ctx.line, g_assert_ctx.assertion);\
	} while (0)

/**
 * Helper to print failure message with argument.
 * @param test  Vtest*
 * @param arg   void*
 */
#define TEST_FAILED_ARG(test, arg) do { TEST_FAILED(test); printf("With args[%d]\n", arg); } while (0)

/**
 * Global assertion context.
 * WARNING: Valid only as long as the framework is single-threaded.
 */
AssertCtx g_assert_ctx;

/**
 * X macro to define the section guard.
 * They are necessary to ensure the presence of the sections symbols __start_ and __stop_
 * when linking.
 */
#define X(Type) __attribute__((section(#Type), used)) Vtest _guard_##Type = {0};
_SECTION_LIST
#undef X

s32
main(s32 argc, char** argv)
{
/**
 * X macro to find testsCount.
 * @param Type Name of the section as a discriminating type for array of registered functions.
 */
#define X(Type) s32 count##Type = (__stop_##Type - __start_##Type);
_TEST_LIST
#undef X

	arena_t*  arena        = ARENA_ALLOC();
	SuiteHM*  suites       = SuiteHM_new(arena, SUITES_CAPACITY);
	u32       successCount = 0;
	const u32 testsCount   = countVtestTypeTest + countVtestTypeTestWithArg - 2; // -2 for guards
	char*     suiteFile    = 0;
	Suite*    suite        = 0;

// -- TEST DISCOVERY START --

/**
 * Routine run for each section array in case it encounters a new suite.
 * @param Type Name of the section as a discriminating type for array of registered functions.
 */
#define NewSuite(Type)                                                                       \
	if (test == &_guard_##Type) continue;                                                \
	do { if (test->file != suiteFile)                                                    \
	{                                                                                    \
		Suite** suite_ptr = SuiteHM_get(suites, test->file);                         \
		suite             = suite_ptr ? *suite_ptr : ARENA_PUSH_STRUCT(arena, Suite);\
		suiteFile         = test->file;                                              \
		suite->file       = test->file;                                              \
		if (suite_ptr == 0)                                                          \
			SuiteHM_put(suites, test->file, suite);                              \
	} } while (0)
/**
 * X macro to push tests in their respective list.
 * @param Type Name of the section as a discriminating type for array of registered functions.
 */
#define X(Type)                                                         \
	for (Vtest* test = __start_##Type; test < __stop_##Type; test++)\
	{                                                               \
		NewSuite(Type);                                         \
		Node* node       = ARENA_PUSH_STRUCT(arena, Node);      \
		node->data       = test;                                \
		if (suite->Type.size == 0)                              \
			suite->Type.head = node;                        \
		else                                                    \
			suite->Type.tail->next = node;                  \
		suite->Type.tail = node;                                \
		suite->Type.size++;                                     \
	}                                                               \
	suiteFile = 0, suite = 0;
	_TEST_LIST
#undef X
/**
 * X macro to register meta functions.
 * @param Type Name of the section as a discriminating type for array of registered functions.
 */
#define X(Type)                                                            \
	for (Vtest* test = __start_##Type; test < __stop_##Type; test++)   \
	{                                                                  \
		NewSuite(Type);                                            \
		if (suite->Type)                                           \
			printf("%s redefined in %s\n", #Type, suite->file);\
		suite->Type = test;                                        \
	}                                                                  \
	suiteFile = 0, suite = 0;
	_META_TEST_LIST
#undef NewSuite
#undef X

// -- TEST DISCOVERY END --

	if (argc == 1) goto skip_cli;

// -- CLI START --

	s32           opt;
	s32           option_idx  = 0;
	Vtest*        test        = 0;
	struct option long_opts[] =
	{
		{ "test",            optional_argument, 0, 't'  },
		{ "suite",           optional_argument, 0, 's'  },
		{ "help",            optional_argument, 0, 'h'  }
	};

	while ((opt = getopt_long(argc, argv, "t:s:h", long_opts, &option_idx)) != -1)
	{
		switch (opt)
		{
		case 't':
		{
			if (suite == 0)
			{
				fprintf(stderr, "Usage: %s --suite [FILE] --test [NAME]\n", argv[0]);
				return 1;
			}

			for (Node* nodeTest = suite->VtestTypeTest.head; nodeTest; nodeTest=nodeTest->next)
			{
				Vtest* t = nodeTest->data;
				if (strcmp(t->name, optarg)) continue;

				test = t;
				break;
			}

			if (test != 0) break;

			if (suite->VtestTypeSetupSuite == 0)
			{
				fprintf(stderr, "No pre-trans-unit function to setup test %s\n", optarg);
				return 1;
			}

			for (Node* nodeTest = suite->VtestTypeTestWithArg.head; nodeTest; nodeTest=nodeTest->next)
			{
				Vtest* t = nodeTest->data;
				if (strcmp(t->name, optarg)) continue;

				test = t;
				break;
			}
			
			if (test == 0)
			{
				fprintf(stderr, "Test %s not found in suite %s\n", optarg, suite->file);
				return 1;
			}
		} break;
		case 's':
		{
			Suite** suite_ptr = SuiteHM_get(suites, optarg);

			if (suite_ptr == 0)
			{
				fprintf(stderr, "Suite %s not found\n", optarg);
				return 1;
			}

			suite = *suite_ptr;
		} break;
		default:
		case 'h':
		{
			printf( "Usage: %s [OPTIONS]\n"
				"  [NO OPTIONS]       : Run all suites.\n"
				"  -s, --suite [FILE] : Run the first suite that matches the file.\n"
				"                       If '--test' is given, only run the test.\n"
				"  -t, --test  [NAME] : Run [NAME] test.\n"
				"                       Need the '--suite' option to be first.\n"
				"                       Tests with argument need to register it in the pre-trans-unit.\n"
				"  -h, --help         : Print this message.\n", argv[0]);
			return 1;
		} break;
		}
	}

	if (suite == 0) goto skip_cli;
	
	RUN_SETUP_SUITE(suite, arena);

	if (test == 0) goto run_suite;

	if (    test->args == 0                                &&
		(u64)test >= (u64)__start_VtestTypeTestWithArg && 
		(u64)test <= (u64) __stop_VtestTypeTestWithArg )
	{
		fprintf(stderr, "No argument for test %s in suite %s\n", test->name, suite->file);
		return 1;
	}

	if (test->args == 0) /*! Classic test */
	{
		s32    jump = setjmp(g_assert_ctx.jmpbuffer);

		if (jump == 0)
		{
			RUN_TEST(test, suite, 0);
			RUN_TEARDOWN_SUITE(suite);
			return 0;
		}
		else
		{
			TEST_FAILED(test);
			RUN_TEARDOWN_SUITE(suite);
			return 1;
		}
	}
	else /*! Test with arg */
	{
		List* args    = test->args;
		s32   result  = 0;
		s32   argi    = 0;

		for (Node* nodeArg = args->head; nodeArg; nodeArg=nodeArg->next, argi++)
		{
			s32    jump = setjmp(g_assert_ctx.jmpbuffer);

			if (jump == 0)
			{
				RUN_TEST(test, suite, nodeArg->data);
			}
			else
			{
				TEST_FAILED_ARG(test, argi);
				result = 1;
			}
		}

		RUN_TEARDOWN_SUITE(suite);
		return result;
	}

run_suite:;
	
	u32   suiteTestsCount = suite->VtestTypeTest.size + suite->VtestTypeTestWithArg.size;

	RUN_SUITE(suite, suiteTestsCount, successCount);
	RUN_TEARDOWN_SUITE(suite);
	
	printf("\nSuccessful tests: %u/%u\n", successCount, suiteTestsCount);
	
	return 0;

// -- CLI END --

skip_cli:;

	printf("--- VTEST START ---\n");
	printf("Discovered %u tests for %lu suits\n", testsCount, suites->size);

	for (u32 i = 0; i < suites->capacity; i++)
	{
		if (suites->ctrl[i] == SWISS_DELETED || suites->ctrl[i] == SWISS_EMPTY) continue;
		suite                 = suites->values[i];
		u32   suiteTestsCount = suite->VtestTypeTest.size + suite->VtestTypeTestWithArg.size;

		RUN_SETUP_SUITE(suite, arena);
		RUN_SUITE(suite, suiteTestsCount, successCount);
		RUN_TEARDOWN_SUITE(suite);
	}

	printf("\nSuccessful tests: %u/%u\n", successCount, testsCount);
	printf("--- VTEST DONE ---\n");

	return 0;
}
