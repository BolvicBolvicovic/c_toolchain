#ifndef VTEST_H
#define VTEST_H

#ifdef _MSC_VER
#error "MSVC not supported. Consider GCC or CLANG instead."
#endif

#include "bits.h"
#include <setjmp.h>
#include <stdio.h> // IWYU pragma: keep

typedef enum   VtestType   VtestType;
typedef enum   VtestResult VtestResult;
typedef struct Vtest       Vtest;
typedef void (*VtestFn)(void*);

enum VtestType
{
	VtestTypeError         = 0,
	VtestTypeTest          = BIT0,
	VtestTypePreTest       = BIT1,
	VtestTypePreTransUnit  = BIT2,
	VtestTypePostTest      = BIT3,
	VtestTypePostTransUnit = BIT4,
	VtestTypeRunner        = BIT5,
};

enum VtestResult
{
	VtestResultNone,
	VtestResultOk,
	VtestResultErr,
};

struct Vtest
{
	char*       name;
	VtestFn     func;
	void*       args;
	VtestType   type;
	VtestResult res;
};

extern Vtest   __start_vtest[];
extern Vtest   __stop_vtest[];
extern jmp_buf g_assert_buffer;

#define VTEST           1
#define VASSERT_VERBOSE 1

#define TEST(name)                                                                            \
	static void  _vtest_fn_##name(void*);                                                 \
	__attribute__((section("vtest"), used))                                               \
	Vtest _vtest_##name = { #name, _vtest_fn_##name, 0, VtestTypeTest, VtestResultNone }; \
	static void  _vtest_fn_##name(__attribute__((unused)) void* vtest_args)

#ifdef ASSERT
#warning "Are you sure to use <assert.h>?"
#else
#define ASSERT(eq) \
	do {                                                                               \
		if (!eq) {                                                                 \
			if (VASSERT_VERBOSE)                                               \
				printf("In file '%s' at line %u: Assertion '%s' failed\n", \
						__FILE__, __LINE__, #eq);                  \
			longjmp(g_assert_buffer,1);                                        \
		}                                                                          \
	} while (0)
#endif

#endif
