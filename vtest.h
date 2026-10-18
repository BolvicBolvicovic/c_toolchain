#ifndef VTEST_H
#define VTEST_H

#ifdef _MSC_VER
#error "MSVC not supported. Consider GCC or CLANG instead."
#endif

// TODO: Write the documentation for the testing framework.
// TODO: Write the ASSERT_CLOCK_START and ASSERT_CLOCK_END for testing performance requierment.

#include "bits.h"
#include "c_types.h"
#include "arena.h"
#include <setjmp.h>
#include <stdio.h>  // IWYU pragma: keep
#include <string.h> // IWYU pragma: keep

typedef struct Node Node;
typedef struct List List;

typedef enum   VtestType   VtestType;
typedef struct Vtest       Vtest;
typedef struct AssertCtx   AssertCtx;
typedef void (*VtestFn)(void*);

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

enum VtestType
{
	VtestTypeError         = 0,
	VtestTypeTest          = BIT0,
	VtestTypeTestWithArg   = BIT1,
	VtestTypePreTest       = BIT2,
	VtestTypePreTransUnit  = BIT3,
	VtestTypePostTest      = BIT4,
	VtestTypePostTransUnit = BIT5,
	VtestTypeRunner        = BIT6,
};

struct Vtest
{
	char*       name;
	VtestFn     func;
	void*       args;
	char*       file;
	VtestType   type; // TODO: if there is no more symbole, pack type in func pointer to have a 32 bytes struct (delete padding)
} __attribute__((aligned(32)));

struct AssertCtx
{
	jmp_buf jmpbuffer;
	s32     line;
	char*   assertion;
};

extern Vtest     __start_vtest[];
extern Vtest     __stop_vtest[];
extern AssertCtx g_assert_ctx;

#define ASSERT_VERBOSE 1

#define __TEST(name, arg, type)                \
	static void  name(void*);              \
	__attribute__((section("vtest"), used))\
	Vtest _vtest_##name =                  \
	{ #name, name, 0, __FILE__, type };    \
	static void  name(__attribute__((unused)) void* arg)

#define _TEST(name, type)        __TEST(name, __unused, type)
#define TEST(name)                _TEST(name, VtestTypeTest)
#define PRE_TEST()                _TEST(_vtest_pretest_##__COUNTER__, VtestTypePreTest)
#define POST_TEST()               _TEST(_vtest_posttest_##__COUNTER__, VtestTypePostTest)
#define PRE_TRANS_UNIT()          _TEST(_vtest_pretransunit_##__COUNTER__, VtestTypePreTransUnit)
#define POST_TRANS_UNIT()         _TEST(_vtest_posttransunit_##__COUNTER__, VtestTypePostTransUnit)
#define TEST_WITH_ARG(name, arg) __TEST(name, arg, VtestTypeTestWithArg)
#define RUNNER()                 __TEST(_vtest_runner, arena, VtestTypeRunner)

#define REGISTER_ARG(test, arg_type, ...) \
	do\
	{\
		arena_t* ar   = arena;\
		void*    ag   = ARENA_PUSH_STRUCT(ar, arg_type);\
		Node*    node = ARENA_PUSH_STRUCT(ar, Node);\
		memmove(ag, &(arg_type){__VA_ARGS__}, sizeof(arg_type));\
		node->data = ag;\
		List* list = _vtest_##test.args;\
		if (list == 0)\
		{\
			list = ARENA_PUSH_STRUCT(ar, List);\
			list->head = node;\
			_vtest_##test.args = list;\
		}\
		else\
		{\
			list->tail->next = node;\
		}\
		list->tail = node;\
		list->size++;\
	} while (0)

#ifdef ASSERT
#warning "Are you sure to use <assert.h>?"
#else
#define ASSERT(eq)                                        \
	do                                                \
	{                                                 \
		if (!(eq))                                \
		{                                         \
			g_assert_ctx.line      = __LINE__;\
			g_assert_ctx.assertion = #eq;     \
			longjmp(g_assert_ctx.jmpbuffer,1);\
		}                                         \
	} while (0)
#endif

#endif
