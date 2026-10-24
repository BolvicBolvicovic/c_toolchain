#include "c_types.h"
#include "vtest.h"
#include "vlist.h"

TEST(VListCycle)
{
	VList list     = { 0 };
	VNode nodes[2] = { 0 };

	ASSERT_FAILURE(VList_pop(&list));
	ASSERT_SUCCESS(VList_push_back(&list, &nodes[0]));
	ASSERT_SUCCESS(VList_push_back(&list, &nodes[1]));
	ASSERT_EQ(VList_pop(&list) == &nodes[0]);
	ASSERT_EQ(VList_pop(&list) == &nodes[1]);
	ASSERT_FAILURE(VList_pop(&list));
	ASSERT_SUCCESS(VList_push_front(&list, &nodes[0]));
	ASSERT_SUCCESS(VList_push_front(&list, &nodes[1]));
	ASSERT_EQ(VList_pop(&list) == &nodes[1]);
	ASSERT_EQ(VList_pop(&list) == &nodes[0]);
}

TEST(VNodeInsert)
{
	VList list     = { 0 };
	VNode nodes[3] = { 0 };
	
	ASSERT_SUCCESS(VList_push_back(&list, &nodes[0]));
	ASSERT_SUCCESS(VList_push_back(&list, &nodes[1]));
	ASSERT_SUCCESS(VNode_insert(&nodes[0], &nodes[2]));
	ASSERT_EQ(VList_pop(&list) == &nodes[0]);
	ASSERT_EQ(VList_pop(&list) == &nodes[2]);
	ASSERT_EQ(VList_pop(&list) == &nodes[1]);
}

TEST_WITH_ARG(VListInsert, _index)
{
	u32   index    = *(u32*)_index;
	VList list     = { 0 };
	VNode nodes[3] = { 0 };
	
	ASSERT_SUCCESS(VList_push_back(&list, &nodes[0]));
	ASSERT_SUCCESS(VList_push_back(&list, &nodes[1]));
	if (index <= list.size)
	{
		ASSERT_SUCCESS(VList_insert(&list, &nodes[2], index));
	}
	else
	{
		ASSERT_FAILURE(VList_insert(&list, &nodes[2], index));
	}

	switch (index)
	{
	case 0:
	{
		ASSERT_EQ(VList_pop(&list) == &nodes[2]);
		ASSERT_EQ(VList_pop(&list) == &nodes[0]);
		ASSERT_EQ(VList_pop(&list) == &nodes[1]);
	} break;
	case 1:
	{
		ASSERT_EQ(VList_pop(&list) == &nodes[0]);
		ASSERT_EQ(VList_pop(&list) == &nodes[2]);
		ASSERT_EQ(VList_pop(&list) == &nodes[1]);
	} break;
	case 2:
	{
		ASSERT_EQ(VList_pop(&list) == &nodes[0]);
		ASSERT_EQ(VList_pop(&list) == &nodes[1]);
		ASSERT_EQ(VList_pop(&list) == &nodes[2]);
	} break;
	default:
		ASSERT_EQ(VList_pop(&list) == &nodes[0]);
		ASSERT_EQ(VList_pop(&list) == &nodes[1]);
		ASSERT_FAILURE(VList_pop(&list));
	}
}

SETUP_SUITE()
{
	TEST_INPUT(VListInsert, u32, 0);
	TEST_INPUT(VListInsert, u32, 1);
	TEST_INPUT(VListInsert, u32, 2);
	TEST_INPUT(VListInsert, u32, 3);
}
