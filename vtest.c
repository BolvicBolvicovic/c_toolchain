#include "vtest.h"
#include "c_types.h"
#include <stdio.h>

jmp_buf g_assert_buffer;

s32
main(s32 argc, char** argv)
{
	(void)argc;
	(void)argv;

	s32 successCount = 0;
	s32 testsCount   = (__stop_vtest - __start_vtest);

	printf("--- VTEST START ---\n");
	printf("Discovered %d tests\n\n", testsCount);

	for (Vtest* test = __start_vtest; test < __stop_vtest; test++)
	{
		s32 jump = setjmp(g_assert_buffer);

		if (jump == 0)
		{
			test->func(test->args);
			successCount++;
		}
		else
		{
			printf("%s FAILED\n", test->name);
		}
	}

	printf("\nSuccessful tests: %u/%u\n", successCount, testsCount);
	printf("--- VTEST DONE ---\n");

	return 0;
}
