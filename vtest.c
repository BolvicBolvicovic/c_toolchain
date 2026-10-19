#include "vtest.h"
#include "c_types.h"
#include "arena.h"
#include "swissmap.h"
#include <stdio.h>
#include <stdbool.h>
#define _GNU_SOURCE
#include <getopt.h>

typedef struct Suite Suite;
struct Suite
{
#define X(type) List type;
_TEST_LIST
#undef X
#define X(type) Vtest* type;
_META_TEST_LIST
#undef X
	char*     file;
};

SWISSMAP_DECLARE_DEFAULT(SuiteHM, Suite*);
SWISSMAP_DEFINE_FUNCTIONS_DEFAULT(SuiteHM, Suite*);

#ifndef SUITES_CAPACITY
#define SUITES_CAPACITY 500
#endif

AssertCtx g_assert_ctx;

#define X(type) __attribute__((section(#type), used)) Vtest _guard_##type = {0};
_SECTION_LIST
#undef X

s32
main(s32 argc, char** argv)
{
#define X(Type) s32 count##Type = (__stop_##Type - __start_##Type);
_TEST_LIST
#undef X

	arena_t*  arena        = ARENA_ALLOC();
	SuiteHM*  suites       = SuiteHM_new(arena, SUITES_CAPACITY);
	s32       successCount = 0;
	const u32 testsCount   = countVtestTypeTest + countVtestTypeTestWithArg - 2; // -2 for guards
	char*     suiteFile    = 0;
	Suite*    suite        = 0;

// -- TEST DISCOVERY START --
#define NewSuite(Type)                                                                       \
	if (test == &_guard_##Type) continue;                                                \
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
	_TEST_LIST
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
				if (memcmp(t->name, optarg, strlen(t->name))) continue;

				test = t;
				break;
			}

			if (test !=0) break;

			for (Node* nodeTest = suite->VtestTypeTestWithArg.head; nodeTest; nodeTest=nodeTest->next)
			{
				Vtest* t = nodeTest->data;
				if (memcmp(t->name, optarg, strlen(t->name))) continue;

				test = t;
				break;
			}
			
			if (test == 0)
			{
				fprintf(stderr, "Test %s not found in suite %s\n", optarg, suite->file);
				return 1;
			}

			if (test->args == 0)
			{
				fprintf(stderr, "Test %s not setup in pre-trans-unit\n", optarg);
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
	
	s32   suiteTestsCount = suite->VtestTypeTest.size + suite->VtestTypeTestWithArg.size;

	if (test && (test < __start_VtestTypeTest || test > __stop_VtestTypeTest)
		 && suite->VtestTypePreTransUnit == 0)
	{
		fprintf(stderr, "Suite %s does not have pre-trans-unit to setup test with arg\n", suite->file);
		return 1;
	}
	
	if (suite->VtestTypePreTransUnit)
	{
		suite->VtestTypePreTransUnit->func(arena);
	}

	if (test)
	{
		s32    jump = setjmp(g_assert_ctx.jmpbuffer);

		if (jump == 0)
		{
			if (suite->VtestTypePreTest)
			{
				suite->VtestTypePreTest->func(0);
			}
			test->func(test->args);
			if (suite->VtestTypePostTest)
			{
				suite->VtestTypePostTest->func(0);
			}
			return 0;
		}
		else
		{
			printf("\n%s FAILED:\n", test->name);
			printf("In %s at line %d: '%s'\n", test->file, g_assert_ctx.line, g_assert_ctx.assertion);
			return 1;
		}
	}
	else
	{
		printf("\nRunning %d tests for suit %s\n", suiteTestsCount, suite->file);
		
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

	}

	if (suite->VtestTypePreTransUnit)
	{
		suite->VtestTypePreTransUnit->func(arena);
	}
	
	printf("\nSuccessful tests: %u/%u\n", successCount, suiteTestsCount);
	
	return 0;
// -- CLI END --
skip_cli:

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
