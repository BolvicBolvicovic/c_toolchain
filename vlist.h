#ifndef VLIST_H
#define VLIST_H

#include "c_types.h"

typedef struct VNode VNode;
typedef struct VList VList;

/** VNode!
 *  Singly linked list node.
 */
struct VNode
{
	void*  data;
	VNode* next;
};

/** VList!
 *  Singly linked list with size.
 */
struct VList
{
	VNode* head;
	VNode* tail;
	u32    size;
};

_Bool   VNode_insert(VNode* head, VNode* tail);
_Bool   VList_push_back(VList* list, VNode* node);
_Bool   VList_push_front(VList* list, VNode* node);
VNode*  VList_pop(VList* list);
_Bool   VList_insert(VList* list, VNode* node, u32 idx);

#ifdef VLIST_IMPL

inline _Bool
VList_push_back(VList* list, VNode* node)
{
	if (list == 0)
	{
		return false;
	}

	if (list->head == 0)
	{
		list->head = node;
	}
	else
	{
		list->tail->next = node;
	}

	list->tail = node;
	list->size++;

	return true;
}

inline _Bool
VList_push_front(VList* list, VNode* node)
{
	if (list == 0)
	{
		return false;
	}

	if (list->head == 0)
	{
		list->tail = node;
	}
	else
	{
		node->next = list->head;
	}

	list->head = node;
	list->size++;

	return true;
}

inline VNode*
VList_pop(VList* list)
{
	if (list == 0)
	{
		return 0;
	}

	VNode* result = list->head;

	if (result == 0)
	{
		return result;
	}

	list->head = list->head->next;
	list->size--;

	return result;
}

inline _Bool
VNode_insert(VNode* head, VNode* tail)
{
	if (head == 0 || tail == 0)
	{
		return false;
	}

	tail->next = head->next;
	head->next = tail;
	return true;
}

inline _Bool
VList_insert(VList* list, VNode* node, u32 idx)
{
	if (!list || !node || idx > list->size)
	{
		return false;
	}

	if (idx == 0)
	{
		node->next = list->head;
		list->head = node;
		return true;
	}

	VNode* head = &(VNode){ 0, list->head};

	for (u32 i = 0; i < idx; i++)
	{
		head = head->next;
	}

	return VNode_insert(head, node);
}

#endif // VLIST_IMPL

#endif // VLIST_H
