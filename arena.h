#ifndef ARENA_H
#define ARENA_H

#include <c_types.h>
#include <bits.h>
#include <sizes.h>

#define PAGE_SIZE	KB(4)
#define PAGE_LARGE_SIZE	MB(4)

typedef u32	arena_flags_t;
enum
{
	ARENA_FLAG_NO_CHAIN	= BIT0,
	ARENA_FLAG_LARGE_PAGES	= BIT1,
};

#define ARENA_FLAGS_DEFAULT		0
#define ARENA_COMMIT_SIZE_DEFAULT	KB(64)
#define ARENA_RESERVE_SIZE_DEFAULT	MB(64)
#define ARENA_HEADER_SIZE		128

/* Name: arena_parameters_t
 * Description: parameters to create a arena_t that are passed to arena_alloc.
 * */
typedef struct arena_parameters_t arena_parameters_t;
struct arena_parameters_t
{
	arena_flags_t	flags;
	u32		reserve_size;
	u32		commit_size;
	void*		optional_backbuffer;
	char*		allocation_site_file;
	s32		allocation_site_line;
};

/* Name: arena_t
 * Description: structure that contains the information related to a kernel arena (stack in the heap). 
 * */
typedef struct arena_t arena_t;
struct arena_t
{
	arena_t*	prev;
	arena_t*	current;
	arena_flags_t	flags;
	u32		position;
	u32		base_position;
	u32		reserve_size;
	u32		commit_size;
	u32		reserve;
	u32		commit;
	char*		allocation_site_file;
	s32		allocation_site_line;
};

/* Name: arena_temp_t
 * Description: structure for temporary scope in kernel arenas.
 * */
typedef struct arena_temp_t arena_temp_t;
struct arena_temp_t
{
	arena_t*	arena;
	u32		pos;
};

/* arena_t creation/destruction */
arena_t*	arena_alloc(arena_parameters_t*);
#define ARENA_ALLOC(...) arena_alloc(&(arena_parameters_t){		\
		.flags 			= ARENA_FLAGS_DEFAULT,		\
		.reserve_size 		= ARENA_RESERVE_SIZE_DEFAULT,	\
		.commit_size 		= ARENA_COMMIT_SIZE_DEFAULT,	\
		.allocation_site_file 	= __FILE__,			\
		.allocation_site_line 	= __LINE__,			\
	       	__VA_ARGS__})
void		arena_release(arena_t*);

/* arena_t push/pop/pos core funtions */
void*		arena_push(arena_t*, u32 size, u32 align, bool zero);
u32		arena_pos(arena_t*);
void		arena_pop_to(arena_t*, u32 pos);

/* arena_t push/pop helpers */
void		arena_clear(arena_t*);
void		arena_pop(arena_t*, u32 amount);

/* arena_temp_t scopes */
arena_temp_t	arena_temp_begin(arena_t*);
void		arena_temp_end(arena_temp_t);

/* arena_t push helper macros */
#define ARENA_PUSH_ARRAY_NO_ZERO_ALIGNED(a, T, c, align)\
	(T *)arena_push((a), sizeof(T)*(c), (align), 0)
#define ARENA_PUSH_ARRAY_ALIGNED(a, T, c, align)	\
	(T *)arena_push((a), sizeof(T)*(c), (align), 1)
#define ARENA_PUSH_ARRAY_NO_ZERO(a, T, c)		\
	ARENA_PUSH_ARRAY_NO_ZERO_ALIGNED(a, T, c, 4)
#define ARENA_PUSH_ARRAY(a, T, c)			\
	ARENA_PUSH_ARRAY_ALIGNED(a, T, c, 4)
#define ARENA_PUSH_STRUCT(a, T)				\
	ARENA_PUSH_ARRAY(a, T, 1)

#endif
