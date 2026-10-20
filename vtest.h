/*************************************************
 *         VTEST - Simple Test Framework         *
 *                     API                       *
 *************************************************/

#ifndef VTEST_H
#define VTEST_H

#ifdef _MSC_VER
#error "MSVC not supported. Consider GCC or CLANG instead."
#endif

#if !defined(__i386__) && !defined(__x86_64__)
#error "Architecture currently not supported"
#endif

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

/** struct Node!
 *  Node for a linked list.
 */
struct Node
{
	void* data;
	Node* next;
};

/** struct List!
 *  Linked list.
 */
struct List
{
	Node* head;
	Node* tail;
	u32   size;
};

/** struct Vtest!
 *  Contains the meta-data of each registered function
 *  whether it is a test or a meta-function.
 *  Vtest arrays are discriminated by section name.
 *  Each function is registered at compile-time in a dedicated section.
 *
 *  Because these sections are 32 bytes aligned, it is important to keep them 32 bytes long.
 *  Avoid adding the __attribute__((aligned(32))) else will silently double the size of the struct.
 *  Ta add more data in the future, consider playing with the pointers last bits.
 */
struct Vtest
{
	char*       name;
	VtestFn     func;
	void*       args;
	char*       file;
};

/** struct AssertCtx!
 *  Used by all ASSERT-kind macros to register the assertion and its line in the suite.
 *  The jmp_buf is used as the checkpoint before executing a test.
 *  If the test fails, the code will jump back to the checkpoint and take the failure path.
 */
struct AssertCtx
{
	jmp_buf jmpbuffer;
	s32     line;
	char*   assertion;
};

extern AssertCtx g_assert_ctx;

#define ASSERT_VERBOSE 1

/**
 *  Declare a global Vtest that will be stored in its dedicated section
 *  before proceding to define the test.
 */
#define __TEST(name, arg, Type)              \
	static void  name(void*);            \
	__attribute__((section(#Type), used))\
	Vtest _vtest_##name =                \
	{ #name, name, 0, __FILE__ };        \
	static void  name(__attribute__((unused)) void* arg)

#define _TEST(name, Type)        __TEST(name, __unused, Type)
/** Classic test with no argument!
 *  @param name
 */
#define TEST(name)                _TEST(name, VtestTypeTest)
/*! Meta function run before every test */
#define PRE_TEST()                _TEST(_vtest_pretest_##__COUNTER__, VtestTypePreTest)
/*! Meta function run after every test */
#define POST_TEST()               _TEST(_vtest_posttest_##__COUNTER__, VtestTypePostTest)
/** Meta function run at the start of the suite.
 *  There are registered inputs to tests with argument.
 *  Note that you can use a hidden arena_t* arena to push stuff on the heap.
 */
#define PRE_TRANS_UNIT()         __TEST(_vtest_pretransunit_##__COUNTER__, arena, VtestTypePreTransUnit)
/*! Meta function run at the end of the suite */
#define POST_TRANS_UNIT()         _TEST(_vtest_posttransunit_##__COUNTER__, VtestTypePostTransUnit)
/** Test with argument!
 *  @param name
 *  @param arg  Name of the argument. Assign it to a pointer to the user defined input struct.
 */
#define TEST_WITH_ARG(name, arg) __TEST(name, arg, VtestTypeTestWithArg)

/** Test input!
 *  Register an input of a user defined type for a test with argument.
 *  @param test        The name of the registered test.
 *  @param arg_type    The struct input type.
 *  @param __VA_ARGS__ The elements of the struct.
 */
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
/*! Assertion routine that use jmp_buf instead of crashing. */
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

/*! Assertion routine for running a test in a maximum amount of cycles start */
#define ASSERT_CLOCK_START(name)\
	u64 name##_start;       \
	u32 name##_shadow;      \
	name##_start = __rdtscp(&(name##_shadow))
/*! Assertion routine for running a test in a maximum amount of cycles end */
#define ASSERT_CLOCK_END(name, max_cycles)           \
	u64 name##_end;                              \
	name##_end = __rdtscp(&(name##_shadow));\
	__asm__ volatile("":::"memory");             \
	ASSERT((name##_end - name##_start) < max_cycles)
#endif
