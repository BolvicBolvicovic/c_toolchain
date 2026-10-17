#ifndef VTEST_H
#define VTEST_H

#ifdef _MSC_VER
#error "MSVC not supported. Consider GCC or CLANG instead."
#endif

#include "bits.h"
#include "c_types.h"
#include <setjmp.h>
#include <stdio.h> // IWYU pragma: keep

typedef enum   VtestType   VtestType;
typedef struct Vtest       Vtest;
typedef struct AssertCtx   AssertCtx;
typedef void (*VtestFn)(void*);

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
	void*       arg;
	VtestType   type;
	s32         idx;
	char*       file;
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

#define __TEST(name, arg_name, type)                                 \
	static void  _vtest_fn_##name(void*);                        \
	__attribute__((section("vtest"), used))                      \
	Vtest _vtest_##name =                                        \
	{ #name, _vtest_fn_##name, 0, type, __COUNTER__, __FILE__ }; \
	static void  _vtest_fn_##name(__attribute__((unused)) void* arg_name)

#define _TEST(name, type)                           __TEST(name, __unused, type)
#define TEST(name)                                   _TEST(name, VtestTypeTest)
#define PRE_TEST(name)                               _TEST(name, VtestTypePreTest)
#define POST_TEST(name)                              _TEST(name, VtestTypePostTest)
#define PRE_TRANS_UNIT(name)                         _TEST(name, VtestTypePreTransUnit)
#define POST_TRANS_UNIT(name)                        _TEST(name, VtestTypePostTransUnit)
#define TEST_WITH_ARG(name, arg_name)               __TEST(name, arg_name, VtestTypeTestWithArg)

#ifdef ASSERT
#warning "Are you sure to use <assert.h>?"
#else
#define ASSERT(eq)                                        \
	do {                                              \
		if (!eq) {                                \
			g_assert_ctx.line      = __LINE__;\
			g_assert_ctx.assertion = #eq;     \
			longjmp(g_assert_ctx.jmpbuffer,1);\
		}                                         \
	} while (0)
#endif

#endif
