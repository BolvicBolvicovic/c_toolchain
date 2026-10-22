/*************************************************
 *         VTEST - Simple Test Framework         *
 *                     API                       *
 *************************************************/

/*************************************************
 * Description:                                  *
 * VTEST is test framework aimed for small size  *
 * project. It is easy to use for it has a       *
 * minimal amount of features by design.         *
 *************************************************/

/*************************************************
 * Usage:                                        *
 * A suite is a .c file containing some tests.   *
 * A good practice is to use directories if one  *
 * needs to specify a kind of suite.             *
 * Example: in test/extensions/new_extensions.c  *
 * #include "vtest.h"                            *
 * #include "new_extensions.h"                   *
 *                                               *
 * TEST(FeatureFoo)                              *
 * {                                             *
 *	ASSERT_SUCCESS(foo("bar") == FeatureOk); *
 * }                                             *
 *                                               *
 * TEST_WITH_ARG(FeatureBar, _bar)               *
 * {                                             *
 *	Bar* bar = _bar;                         *
 *	ASSERT_FAILURE(bar->func("foo") == 0);   *
 * }                                             *
 *                                               *
 * SETUP_SUITE()                                 *
 * {                                             *
 *	TEST_INPUT(FeatureBar, Bar, .func=bAR);  *
 *	TEST_INPUT(FeatureBar, Bar, .func=baR);  *
 * }                                             *
 *                                               *
 * Compile and link all tests files with vtest.c *
 * and run the executable.                       *
 * `--help` shows the available options.         *
 *************************************************/

/** TODO Roadmap!
 *  - Add a logger which prints based on an enum.
 *  - Consider tagging tests that want to execute the pre/post meta-functions.
 *  - Improve ASSERT feedback.
 *  - Filter option which would run all suites containing the user input. Same for tests.
 *  - Runtime controle options (i.e. `--list`, `--repeat`, etc...).
 *  - Standard report of elapsed time. 
 *  - Wrap Vtest in the runner with a meta-Vtest which contains runtime information.
 *  - JSON output for CI.
 */

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

#define CAT_(a, b) a##b
#define CAT(a, b)  CAT_(a, b)
#define STR_(a)    #a
#define STR(a)     STR_(a)
#define UNIQUE(a)  CAT(a, __COUNTER__)

/**
 *  Declare a global Vtest that will be stored in its dedicated section
 *  before proceding to define the test.
 */
#define __TEST(name, arg, Type)                  \
	static void  name(void*);                \
	__attribute__((section(STR(Type)), used))\
	static Vtest CAT(_vtest_,name) =         \
	{ STR(name), name, 0, __FILE__ };        \
	static void  name(__attribute__((unused)) void* arg)

#define _TEST(name, Type)        __TEST(name, __unused, Type)
/** Classic test with no argument!
 *  @param name
 */
#define TEST(name)                _TEST(name, VtestTypeTest)
/*! Meta function run before every test */
#define PRE_TEST()                _TEST(UNIQUE(_vtest_pretest_), VtestTypePreTest)
/*! Meta function run after every test */
#define POST_TEST()               _TEST(UNIQUE(_vtest_posttest_), VtestTypePostTest)
/** Meta function run at the start of the suite.
 *  There are registered inputs to tests with argument.
 *  Note that you can use a hidden @param _ss_arena of type arena_t* to push stuff on the heap.
 *  That param can be passed as argument to further tests with argument.
 */
#define SETUP_SUITE()           __TEST(UNIQUE(_vtest_setupsuite_), _ss_arena, VtestTypeSetupSuite)
/*! Meta function run at the end of the suite */
#define TEARDOWN_SUITE()         _TEST(UNIQUE(_vtest_teardownsuite_), VtestTypeTeardownSuite)
/** Test with argument!
 *  @param name
 *  @param arg  Name of the argument. Assign it to a pointer to the user defined input struct.
 */
#define TEST_WITH_ARG(name, arg) __TEST(name, arg, VtestTypeTestWithArg)

/** Test input pointer!
 *  Register an input of a pointer to a user defined struct for a test with argument.
 *  @param test The name of the registered test.
 *  @param ptr  The pointer variable.
 */
#define TEST_INPUT_PTR(test, ptr)                                       \
	do                                                              \
	{                                                               \
		arena_t* ar   = _ss_arena;                              \
		Node*    node = ARENA_PUSH_STRUCT(ar, Node);            \
		node->data = ptr;                                       \
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

/** Test input!
 *  Register an input of a user defined type for a test with argument.
 *  @param test        The name of the registered test.
 *  @param arg_type    The struct input type.
 *  @param __VA_ARGS__ The elements of the struct.
 */
#define TEST_INPUT(test, arg_type, ...)                                 \
	do                                                              \
	{                                                               \
		arena_t* ar   = _ptu_arena;                             \
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

/*! More specific name for should successed assertions */
#define ASSERT_SUCCESS(eq) ASSERT(eq)

/*! More specific name for should fail assertions */
#define ASSERT_FAILURE(eq) ASSERT(!(eq))

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
