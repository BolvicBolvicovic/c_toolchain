#include "vtest.h"
#include "c_types.h"
#include "arena.h"
#include <stdio.h>
#include <stdbool.h>

typedef struct Suite Suite;

struct Suite
{
	VtestType kinds;
	List      listTest;
	List      listTestWithArg;
	Vtest*    preTest;
	Vtest*    preTransUnit;
	Vtest*    postTest;
	Vtest*    postTransUnit;
	Vtest*    runner;
	char*     file;
};

AssertCtx g_assert_ctx;

// TODO: Add command line to select test/suite to run
s32
main(s32 argc, char** argv)
{
	(void)argc;
	(void)argv;

	s32      successCount = 0;
	s32      testsCount   = 0;
	arena_t* arena        = ARENA_ALLOC();
	List     suites       = { 0 };
	char*    suiteFile    = 0;

	for (Vtest* test = __start_vtest; test < __stop_vtest; test++)
	{
		if (test->file != suiteFile)
		{
			Node* node       = ARENA_PUSH_STRUCT(arena, Node);
			node->data       = ARENA_PUSH_STRUCT(arena, Suite);
			Suite* suit       = node->data;

			suit->file = test->file;

			if (suites.size == 0)
			{
				suites.head = node;
			}
			else
			{
				suites.tail->next = node;
			}
			suites.tail = node;
			suites.size++;
			suiteFile = test->file;
		}

		Suite* suite  = suites.tail->data;
		suite->kinds |= test->type;

		switch (test->type)
		{
#define PushTestOnSuite(List)                             \
	Node* node       = ARENA_PUSH_STRUCT(arena, Node);\
	node->data       = test;                          \
                                                          \
	if (suite->List.size == 0)                        \
	{                                                 \
		suite->List.head = node;                  \
	}                                                 \
	else                                              \
	{                                                 \
		suite->List.tail->next = node;            \
	}                                                 \
	suite->List.tail = node;                          \
	suite->List.size++;                               \
	testsCount++;

#define SetMetaTest(Test)                                                         \
	if (suite->Test)                                                          \
	{                                                                         \
		printf("Updating already defined %s in %s\n", #Test, suite->file);\
	}                                                                         \
	suite->Test = test;

			case VtestTypeError: 
			{
				fprintf(stderr, "VtestTypeError %s registered in %s. Exiting...", test->name, test->file);
				return 1;
			} break;
			case VtestTypeTest:
			{
				PushTestOnSuite(listTest);
			} break;
			case VtestTypeTestWithArg:
			{
				PushTestOnSuite(listTestWithArg);
			} break;
			case VtestTypePreTest:
			{
				SetMetaTest(preTest);
			} break;
			case VtestTypePostTest:
			{
				SetMetaTest(postTest);
			} break;
			case VtestTypePreTransUnit:
			{
				SetMetaTest(preTransUnit);
			} break;
			case VtestTypePostTransUnit:
			{
				SetMetaTest(postTransUnit);
			} break;
			case VtestTypeRunner:
			{
				SetMetaTest(runner);
			} break;
#undef PushTestOnSuit
#undef SetMetaTest
		}
	}

	printf("--- VTEST START ---\n");
	printf("Discovered %d tests for %d suits\n", testsCount, suites.size);

	for (Node* node = suites.head; node; node=node->next)
	{
		Suite* suite          = node->data;
		s32   suiteTestsCount = suite->listTest.size + suite->listTestWithArg.size;

		printf("\nRunning %d tests for suit %s\n", suiteTestsCount, suite->file);

		if (suite->preTransUnit)
		{
			suite->preTransUnit->func(suite->preTransUnit->args);
		}

		if (suite->runner)
		{
			suite->runner->func(arena);
		}

		for (Node* nodeTest = suite->listTest.head; nodeTest; nodeTest=nodeTest->next)
		{
			Vtest* test = nodeTest->data;
			s32    jump = setjmp(g_assert_ctx.jmpbuffer);

			if (jump == 0)
			{
				if (suite->preTest)
				{
					suite->preTest->func(suite->preTest->args);
				}
				test->func(0);
				if (suite->postTest)
				{
					suite->postTest->func(suite->postTest->args);
				}
				successCount++;
			}
			else
			{
				printf("\n%s FAILED:\n", test->name);
				printf("In %s at line %d: '%s'\n", test->file, g_assert_ctx.line, g_assert_ctx.assertion);
			}
		}
		
		for (Node* nodeTest = suite->listTestWithArg.head; nodeTest; nodeTest=nodeTest->next)
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
					if (suite->preTest)
					{
						suite->preTest->func(suite->preTest->args);
					}
					test->func(nodeArg->data);
					if (suite->postTest)
					{
						suite->postTest->func(suite->postTest->args);
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

		if (suite->postTransUnit)
		{
			suite->postTransUnit->func(suite->postTransUnit->args);
		}
	}


	printf("\nSuccessful tests: %u/%u\n", successCount, testsCount);
	printf("--- VTEST DONE ---\n");

	return 0;
}
