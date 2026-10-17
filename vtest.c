#include "vtest.h"
#include "c_types.h"
#include "arena.h"
#include <stdio.h>

typedef struct Suit Suit;
typedef struct Node Node;
typedef struct List List;

struct Node
{
	void* data;
	Node* next;
};

struct List
{
	Node* head;
	Node* tail;
	u32   size;
};

struct Suit
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

s32
main(s32 argc, char** argv)
{
	(void)argc;
	(void)argv;

	s32      successCount = 0;
	s32      testsCount   = __stop_vtest - __start_vtest;
	arena_t* arena        = ARENA_ALLOC();
	List     suits        = { 0 };

	for (Vtest* test = __start_vtest; test < __stop_vtest; test++)
	{
		//TODO: find a safer way than using __COUNTER__
		if (test->idx == 0)
		{
			Node* node       = ARENA_PUSH_STRUCT(arena, Node);
			node->data       = ARENA_PUSH_STRUCT(arena, Suit);
			Suit* suit       = node->data;

			suit->file = test->file;

			if (suits.size == 0)
			{
				suits.head = node;
			}
			else
			{
				suits.tail->next = node;
			}
			suits.tail = node;
			suits.size++;
		}

		Suit* suit   = suits.tail->data;
		suit->kinds |= test->type;

		switch (test->type)
		{
#define PushTestOnSuit(List)                              \
	Node* node       = ARENA_PUSH_STRUCT(arena, Node);\
	node->data       = test;                          \
                                                          \
	if (suit->List.size == 0)                         \
	{                                                 \
		suit->List.head = node;                   \
	}                                                 \
	else                                              \
	{                                                 \
		suit->List.tail->next = node;             \
	}                                                 \
	suit->List.tail = node;                           \
	suit->List.size++;

#define SetMetaTest(Test)                                                        \
	if (suit->Test)                                                          \
	{                                                                        \
		printf("Updating already defined %s in %s\n", #Test, suit->file);\
	}                                                                        \
	suit->Test = test;

			// TODO: be more explicit about the error
			case VtestTypeError: perror("Invalide test"); return 1;
			case VtestTypeTest:
			{
				PushTestOnSuit(listTest);
			} break;
			case VtestTypeTestWithArg:
			{
				PushTestOnSuit(listTestWithArg);
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
	printf("Discovered %d tests for %d suits\n", testsCount, suits.size);

	for (Node* node = suits.head; node; node=node->next)
	{
		Suit* suit           = node->data;
		s32   suitTestsCount = suit->listTest.size + suit->listTestWithArg.size;

		printf("\nRunning %d tests for suit in file %s\n", suitTestsCount, suit->file);

		// TODO: handle tests with arg and the runner.
		if (suit->preTransUnit)
		{
			suit->preTransUnit->func(suit->preTransUnit->arg);
		}
		for (Node* nodeTest = suit->listTest.head; nodeTest; nodeTest=nodeTest->next)
		{
			Vtest* test = nodeTest->data;
			s32    jump = setjmp(g_assert_ctx.jmpbuffer);

			if (jump == 0)
			{
				if (suit->preTest)
				{
					suit->preTest->func(suit->preTest->arg);
				}
				test->func(0);
				if (suit->postTest)
				{
					suit->postTest->func(suit->postTest->arg);
				}
				successCount++;
			}
			else
			{
				printf("%s FAILED:\n", test->name);
				printf("In %s at line %d: '%s'\n", test->file, g_assert_ctx.line, g_assert_ctx.assertion);
			}
		}
		if (suit->postTransUnit)
		{
			suit->postTransUnit->func(suit->postTransUnit->arg);
		}
	}


	printf("\nSuccessful tests: %u/%u\n", successCount, testsCount);
	printf("--- VTEST DONE ---\n");

	return 0;
}
