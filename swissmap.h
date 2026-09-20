#ifndef SWISSMAP_H
#define SWISSMAP_H

#include "c_types.h"

// Note: Vectorization based on architecture.
#if defined(__SSE2__)
#include <emmintrin.h>
#elif defined(__aarch64__)
#include <arm_neon.h>
#endif

/* Name: SWISS_EMPTY, SWISS_DELETED
 * Description: Control bits to check weather a slot in the map is empty or deleted.
 * Delation is not handled yet.
 * */
#define SWISS_EMPTY	0x80
#define SWISS_DELETED	0xFE

/* Name: SWISS_H2_MASK
 * Description: The seven first bits of a hash. They are used to efficiently prob.
 * */
#define SWISS_H2_MASK	0x7F

/* Name: SWISSMAP_GROUP_SIZE
 * Description: Size of a control bytes group.
 * */
#define SWISSMAP_GROUP_SIZE	16

/* Name: SWISSMAP_DECLARE
 * Description: The structure for the swissmap with the name, the key and value types decided by the user.
 * - ctrl	: control bytes array
 * - keys	: keys array
 * - values	: values array
 * - capacity	: user defined maximal capacity for each array
 * - size	: current number of elements in the map
 * */
#define SWISSMAP_DECLARE(name, K_t, V_t)	\
typedef struct name	name;			\
struct name					\
{						\
	u8*	ctrl;				\
	K_t*	keys;				\
	V_t*	values;				\
	u64	capacity;			\
	u64	size;				\
};

/* Name: swiss_h1
 * Description: first part of the hash (last bits).
 * h2 is popped and h1 is masked with the biggest index possible.
 * */
u64	swiss_h1(u64 hash, u64 cap);

/* Name: swiss_h2
 * Description: second part of the hash (first bits).
 * The hash is masked with SWISS_H2_MASK.
 * */
u8	swiss_h2(u64 hash);

/* Name: swiss_init_ctrl
 * Description: memset ctrl array with SWISS_EMPTY.
 * */
void	swiss_init_ctrl(u8* ctrl, u64 cap);

/* Name: swiss_match_h2
 * Description: h2 is matched with 8 control bytes of the crtl array starting at offset.
 * The nth index of the matching byte is returned under the form of a mask
 * with the nth bit set matching the nth index.
 * If SSE2 is available, this can be done for 16 bytes in 3 instructions.
 * */
u16	swiss_match_h2(const u8* ctrl, u8 h2, u64 offset);

/* Name: swiss_djb2_hash
 * Description: default hash when K_t is char*.
 * */
u64	swiss_djb2_hash(char* str);

/* Name: swiss_strcmp_wrapper
 * Description: default key comparison function when K_t is char*.
 * */
s32	swiss_strcmp_wrapper(char* s1, char* s2);

/* Name: SWISSMAP_DEFINE_NEW
 * Description: macro that defines the new function for a user defined swissmap.
 * Note: You need to #include <arena.h> to use this macro.
 * */
#define SWISSMAP_DEFINE_NEW(name, K_t, V_t)			\
name*								\
name##_new(arena_t* arena, u64 capacity)			\
{								\
	name*	map	= ARENA_PUSH_STRUCT(arena, name);	\
	u64	new_cap	= 16;					\
								\
	while (new_cap < capacity)				\
		new_cap <<= 1;					\
	map->ctrl	= ARENA_PUSH_ARRAY(arena, u8, new_cap);\
	map->keys	= ARENA_PUSH_ARRAY(arena, K_t, new_cap);\
	map->values	= ARENA_PUSH_ARRAY(arena, V_t, new_cap);\
	map->capacity	= new_cap;				\
	map->size	= 0;					\
								\
	swiss_init_ctrl(map->ctrl, new_cap);			\
								\
	return map;						\
}

/* Name: SWISSMAP_DEFINE_GET
 * Description: macro that defines the get function for a user defined swissmap.
 * Requires a hash function and a comparison function.
 * Checks each group starting by the one defined by h1 for a matching h2.
 * If match, returns a pointer to the value else returns a null pointer.
 * */
#define SWISSMAP_DEFINE_GET(name, K_t, V_t, hash_fn, eq_fn)	\
V_t*								\
name##_get(name* map, K_t key)					\
{								\
	u64	hash	= hash_fn(key);				\
	u8	h2	= swiss_h2(hash);			\
	u64	mask	= map->capacity - 1;			\
	u64	group	= swiss_h1(hash, map->capacity);	\
	u64	start	= group;				\
								\
	do							\
	{							\
		u16	m = swiss_match_h2(map->ctrl, h2, group & mask);\
								\
		while (m)					\
		{						\
			s32	i	= __builtin_ctz(m);	\
			u64	idx	= (group + i) & mask;	\
								\
			if (eq_fn(map->keys[idx], key))		\
				return &map->values[idx];	\
								\
			m &= m - 1;				\
		}						\
								\
		if (swiss_match_h2(map->ctrl, SWISS_EMPTY, group & mask))\
		if (swiss_match_h2(map->ctrl, SWISS_DELETED, group & mask))\
			return 0;				\
								\
		group = (group + SWISSMAP_GROUP_SIZE) & mask;	\
	}							\
	while (group != start);					\
								\
	return 0;						\
}

/* Name: SWISSMAP_DEFINE_PUT
 * Description: macro that defines the put function for a user defined swissmap.
 * Requires a hash function and a comparison function.
 * Checks each group starting by the one defined by h1 for a matching h2.
 * If match, updates the value else search for an empty space in group.
 * If match, creates a new entry with the key-value pair else search in next group.
 * If the table is full a new key wants to be added, does nothing.
 * Possible improvements: give a hint or split new entry with update entry.
 * */
#define SWISSMAP_DEFINE_PUT(name, K_t, V_t, hash_fn, eq_fn)	\
void								\
name##_put(name* map, K_t key, V_t value)			\
{								\
	u64	hash	= hash_fn(key);				\
	u8	h2	= swiss_h2(hash);			\
	u64	mask	= map->capacity - 1;			\
	u64	group	= swiss_h1(hash, map->capacity);	\
	u64	start	= group;				\
								\
	do							\
	{							\
		u16	m = swiss_match_h2(map->ctrl, h2, group & mask);\
								\
		while (m)					\
		{						\
			s32	i	= __builtin_ctz(m);	\
			u64	idx	= (group + i) & mask;	\
								\
			if (eq_fn(map->keys[idx], key))		\
			{					\
				map->values[idx] = value;	\
				return;				\
			}					\
								\
			m &= m - 1;				\
		}						\
								\
		u16	empty = swiss_match_h2(map->ctrl, SWISS_EMPTY, group & mask);\
								\
		if (!empty)					\
			empty = swiss_match_h2(map->ctrl, SWISS_DELETED, group & mask);\
								\
		if (empty)					\
		{						\
			s32	e	= __builtin_ctz(empty);	\
			u64	idx	= (group + e) & mask;	\
								\
			map->ctrl[idx]	= h2;			\
			map->keys[idx]	= key;			\
			map->values[idx]= value;		\
			map->size++;				\
								\
			return;					\
		}						\
								\
		group = (group + SWISSMAP_GROUP_SIZE) & mask;	\
	}							\
	while (group != start);					\
}

/* Name: SWISSMAP_DEFINE_DELETE
 * Description: macro that defines the delete function for a user defined swissmap.
 * Requires a hash function and a comparison function.
 * If match, sets the entry as SWISS_DELETED.
 * */
#define SWISSMAP_DEFINE_DELETE(name, K_t, V_t, hash_fn, eq_fn)	\
void								\
name##_delete(name* map, K_t key)				\
{								\
	u64	hash	= hash_fn(key);				\
	u8	h2	= swiss_h2(hash);			\
	u64	mask	= map->capacity - 1;			\
	u64	group	= swiss_h1(hash, map->capacity);	\
	u64	start	= group;				\
								\
	do							\
	{							\
		u16	m = swiss_match_h2(map->ctrl, h2, group & mask);\
								\
		while (m)					\
		{						\
			s32	i	= __builtin_ctz(m);	\
			u64	idx	= (group + i) & mask;	\
								\
			if (eq_fn(map->keys[idx], key))		\
			{					\
				map->ctrl[idx] = SWISS_DELETED;	\
				return;				\
			}					\
								\
			m &= m - 1;				\
		}						\
								\
		if (swiss_match_h2(map->ctrl, SWISS_EMPTY, group & mask))\
		if (swiss_match_h2(map->ctrl, SWISS_DELETED, group & mask))\
			return;					\
								\
		group = (group + SWISSMAP_GROUP_SIZE) & mask;	\
	}							\
	while (group != start);					\
}

/* Name: SWISSMAP_DEFINE_FUNCTIONS
 * Description: Single macro that defines all functions for a user defined swissmap.
 * */
#define SWISSMAP_DEFINE_FUNCTIONS(name, K_t, V_t, hash_fn, eq_fn)\
SWISSMAP_DEFINE_NEW(name, K_t, V_t)			\
SWISSMAP_DEFINE_GET(name, K_t, V_t, hash_fn, eq_fn)	\
SWISSMAP_DEFINE_PUT(name, K_t, V_t, hash_fn, eq_fn)	\
SWISSMAP_DEFINE_DELETE(name, K_t, V_t, hash_fn, eq_fn)

/* Name: SWISSMAP_DEFINE_FUNCTIONS_DEFAULT
 * Description: Single macro that defines all functions for the default swissmap.
 * K_t is char*, swiss_djb2_hash is the hash and swiss_strcmp_wrapper is the comparison function.
 * */
#define SWISSMAP_DEFINE_FUNCTIONS_DEFAULT(name, V_t)	\
SWISSMAP_DEFINE_FUNCTIONS(name, char*, V_t, swiss_djb2_hash, swiss_strcmp_wrapper)

/* Name: SWISSMAP_DECLARE_FUNCTIONS
 * Description: Single macro that declares all functions for a user defined swissmap.
 * */
#define SWISSMAP_DECLARE_FUNCTIONS(name, K_t, V_t)	\
name*	name##_new(arena_t* arena, u64 capacity);	\
void	name##_put(name* map, K_t key, V_t value);	\
V_t*	name##_get(name* map, K_t key);			\
void	name##_delete(name* map, K_t key);

/* Name: SWISSMAP_DECLARE_DEFAULT
 * Description: Single macro that declares the default swissmap and its functions.
 * */
#define SWISSMAP_DECLARE_DEFAULT(name, V_t)		\
SWISSMAP_DECLARE(name, char*, V_t)			\
SWISSMAP_DECLARE_FUNCTIONS(name, char*, V_t)

#ifdef TESTER

void	test_swissmap(void);

#endif // TESTER

#endif // SWISSMAP_H
