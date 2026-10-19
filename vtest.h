#ifndef VTEST_H
#define VTEST_H

#ifdef _MSC_VER
#error "MSVC not supported. Consider GCC or CLANG instead."
#endif

#if !defined(__i386__) && !defined(__x86_64__)
#error "Architecture currently not supported"
#endif

// TODO: Write the documentation for the testing framework.

#include "c_types.h"
#include "arena.h"
#include <setjmp.h>
#include <stdio.h>  // IWYU pragma: keep
#include <string.h> // IWYU pragma: keep
#include <immintrin.h>

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

// Node: keep it 32 bytes aligned.
// Do not use ((aligned(32))) else will silently double the size of the struct.
struct Vtest
{
	char*       name;
	VtestFn     func;
	void*       args;
	char*       file;
};

struct AssertCtx
{
	jmp_buf jmpbuffer;
	s32     line;
	char*   assertion;
};

extern AssertCtx g_assert_ctx;

#define ASSERT_VERBOSE 1

#define _TEST_LIST             \
	X(VtestTypeTestWithArg)\
	X(VtestTypeTest)

#define _META_TEST_LIST          \
	X(VtestTypePreTest)      \
	X(VtestTypePreTransUnit) \
	X(VtestTypePostTest)     \
	X(VtestTypePostTransUnit)

#define _SECTION_LIST             \
	_TEST_LIST                \
	_META_TEST_LIST

#define X(type)                       \
	extern Vtest __start_##type[];\
	extern Vtest __stop_##type[];
_SECTION_LIST
#undef X

#define __TEST(name, arg, type)              \
	static void  name(void*);            \
	__attribute__((section(#type), used))\
	Vtest _vtest_##name =                \
	{ #name, name, 0, __FILE__ };        \
	static void  name(__attribute__((unused)) void* arg)

#define _TEST(name, type)        __TEST(name, __unused, type)
#define TEST(name)                _TEST(name, VtestTypeTest)
#define PRE_TEST()                _TEST(_vtest_pretest_##__COUNTER__, VtestTypePreTest)
#define POST_TEST()               _TEST(_vtest_posttest_##__COUNTER__, VtestTypePostTest)
#define PRE_TRANS_UNIT()         __TEST(_vtest_pretransunit_##__COUNTER__, arena, VtestTypePreTransUnit)
#define POST_TRANS_UNIT()         _TEST(_vtest_posttransunit_##__COUNTER__, VtestTypePostTransUnit)
#define TEST_WITH_ARG(name, arg) __TEST(name, arg, VtestTypeTestWithArg)

#define TEST_INPUT(test, arg_type, ...)                                 \
	do                                                              \
	{                                                               \
		arena_t* ar   = arena;                                  \
		void*    ag   = ARENA_PUSH_STRUCT(ar, arg_type);        \
		Node*    node = ARENA_PUSH_STRUCT(ar, Node);            \
		memmove(ag, &(arg_type){__VA_ARGS__}, sizeof(arg_type));\
		node->data = ag;                                        \
		List* list = _vtest_##test.args;                        \
		if (list == 0)                                          \
		{                                                       \
			list = ARENA_PUSH_STRUCT(ar, List);             \
			list->head = node;                              \
			_vtest_##test.args = list;                      \
		}                                                       \
		else                                                    \
		{                                                       \
			list->tail->next = node;                        \
		}                                                       \
		list->tail = node;                                      \
		list->size++;                                           \
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

#define ASSERT_CLOCK_START(name)\
	u64 name##_start;       \
	u32 name##_shadow;      \
	name##_start = __rdtscp(&(name##_shadow))
#define ASSERT_CLOCK_END(name, max_cycles)           \
	u64 name##_end;                              \
	name##_end = __rdtscp(&(name##_shadow));\
	__asm__ volatile("":::"memory");             \
	ASSERT((name##_end - name##_start) < max_cycles)

#endif
