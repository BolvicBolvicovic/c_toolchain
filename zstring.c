#include <zstring.h>
#include <string.h>
#include <unistd.h>

zstring*
zstring_on_heap(arena_t* arena, char* data, u64 size)
{
	zstring		zstr = zstring_on_stack(data, size);
	zstring*	zret = ARENA_PUSH_STRUCT(arena, zstring);

	memcpy(zret, &zstr, sizeof(zstring));
	return zret;
}

inline zstring
zstring_slice_on_stack(zstring* z, u64 start, u64 end)
{
	if (end > z->size)
		return  (zstring){ 0 };
	
	return zstring_on_stack(z->data + start, end - start);
}

zstring*
zstring_slice_on_heap(arena_t* arena, zstring* z, u64 start, u64 end)
{
	zstring		zstr = zstring_slice_on_stack(z, start, end);
	zstring*	zret = ARENA_PUSH_STRUCT(arena, zstring);

	memcpy(zret, &zstr, sizeof(zstring));
	return zret;
}

char*
zstring_as_cstring(arena_t* arena, zstring* z)
{
	char*	ret = ARENA_PUSH_ARRAY(arena, char, z->size + 1);

	memcpy(ret, z->data, z->size);
	return ret;
}

zstring*
zstring_cat(arena_t* arena, zstring* z1, zstring* z2)
{
	zstring*	zret = ARENA_PUSH_STRUCT(arena, zstring);
	u64		size = z1->size + z2->size;

	zret->data = ARENA_PUSH_ARRAY(arena, char, size);
	zret->size = size;
	memcpy(zret->data, z1->data, z1->size);
	memcpy(zret->data + z1->size, z2->data, z2->size);

	return zret;
}

static inline bool
char_in_list(char c, char* list)
{
	if (!list)
		return 0;

	while (*list)
	{
		if (c == *list)
			return 1;

		list++;
	}

	return 0;
}

zstring
zstring_trim(zstring* z, char* list)
{
	zstring	tend = zstring_trim_end(z, list);

	return zstring_trim_start(&tend, list);
}

zstring
zstring_trim_start(zstring* z, char* list)
{
	char*	data = z->data;
	u64	size = z->size;
	u64	i;

	for (i = 0; i < size && char_in_list(data[i], list); i++);

	return (zstring){ .data=data+i, .size=size-i };
}

zstring
zstring_trim_end(zstring* z, char* list)
{
	s64	size = (s64)z->size - 1;
	char*	data = z->data;

	for (; size >= 0 && char_in_list(data[size], list); size--);

	return (zstring){ .data=data, .size=(u64)size+1 };
}

zstring*
zstring_split(arena_t* arena, zstring* z, zstring* pattern, u64* out_count)
{
	*out_count = 0;

	if (pattern->size == 0)
		return zstring_slice_on_heap(arena, z, 0, z->size);

	if (z->size == 0)
		return zstring_on_heap(arena, 0, 0);
	
	u64	count = 1;
	
	if (pattern->size > 1)
		for (u64 i = 0; i <= z->size - pattern->size; )
		{
			if (memcmp(z->data + i, pattern->data, pattern->size) == 0)
			{
				count++;
				i += pattern->size;
			}
			else
				i++;
		}
	else
		for (u64 i = 0; i <= z->size - pattern->size; i++)
			if (z->data[i] == pattern->data[0])
				count++;
	
	zstring*	results = ARENA_PUSH_ARRAY(arena, zstring, count);

	*out_count = count;
	
	u64	current_segment	= 0;
	u64	start_index	= 0;
	
	if (pattern->size > 1)
	{
		for (u64 i = 0; i <= z->size - pattern->size; )
		{
			if (memcmp(z->data + i, pattern->data, pattern->size) == 0)
			{
				results[current_segment].data = z->data + start_index;
				results[current_segment].size = i - start_index;
				
				current_segment++;
				i += pattern->size;
				start_index = i;
			}
			else
				i++;
		}
	}
	else
	{
		for (u64 i = 0; i <= z->size - pattern->size; i++)
		{
			if (z->data[i] == pattern->data[0])
			{
				results[current_segment].data = z->data + start_index;
				results[current_segment].size = i - start_index;
				
				current_segment++;
				start_index = i + 1;
			}
		}
	}

	results[current_segment].data = z->data + start_index;
	results[current_segment].size = z->size - start_index;
	
	return results;
}

zstring*
zstring_join(arena_t* arena, zstring* zstrs, u64 size, zstring* pattern)
{
	u64	full_size = 0;

	for (u64 i = 0; i < size; i++)
		full_size += pattern->size + zstrs[i].size;

	full_size -= pattern->size;

	zstring*	zstr = ARENA_PUSH_STRUCT(arena, zstring);
	char*		data = ARENA_PUSH_ARRAY(arena, char, full_size);
	
	zstr->data = data;
	zstr->size = full_size;

	for (u64 i = 0; i < size; i++)
	{
		memcpy(data, zstrs[i].data, zstrs[i].size);
		data += zstrs[i].size;

		if (i >= size - 1)
			continue;

		memcpy(data, pattern->data, pattern->size);
		data += pattern->size;
	}

	return zstr;
}

bool
zstring_cmp(zstring* z1, zstring* z2)
{
	if (z1->size != z2->size)
		return 0;

	if (memcmp(z1->data, z2->data, z1->size))
		return 0;

	return 1;
}

s32
zstring_print(zstring* z)
{
	return write(1, z->data, z->size);
}

#ifdef TEST_MODE

#include <assert.h>

static void
test_slice(arena_t* arena)
{
	char		str[] = "Sea, sex and sun.";
	zstring*	zstr  = zstring_on_heap(arena, str, sizeof(str));
	zstring*	szstr = zstring_slice_on_heap(arena, zstr, 5, 8);

	assert(zstring_cmp(&zstring_on_stack(str + 5, 3), szstr));
	assert(strcmp(str, zstring_as_cstring(arena, zstr)) == 0);
}

static void
test_cat(arena_t* arena)
{
	zstring*	z1 = &zstring_on_stack("Sea, sex ", 9);
	zstring*	z2 = &zstring_on_stack("and sun.", 8);
	zstring*	z3 = zstring_cat(arena, z1, z2);

	assert(zstring_cmp(&zstring_on_stack("Sea, sex and sun.", 17), z3));
}

static void
test_split_join(arena_t* arena)
{
	u64		count = 0;
	char		str[] = "Sea, sex and sun.";
	zstring*	zstr  = &zstring_on_stack(str, sizeof(str));
	zstring*	sstr  = zstring_split(arena, zstr, &zstring_on_stack(", sex ", 6), &count);
	zstring*	jstr  = zstring_join(arena, sstr, count, &zstring_on_stack(", sex ", 6));

	assert(count == 2);
	assert(strcmp("Sea", zstring_as_cstring(arena, sstr)) == 0);
	assert(strcmp("and sun.", zstring_as_cstring(arena, sstr + 1)) == 0);
	assert(zstring_cmp(zstr, jstr));
}

static void
test_trim(arena_t* arena)
{
	char		str[] = "Sea, sex and sun.";
	zstring*	zstr  = &zstring_on_stack(str, sizeof(str) - 1);
	zstring		lstr  = zstring_trim_start(zstr, "Sea,");
	zstring		rstr  = zstring_trim_end(&lstr, "sun.");
	zstring		sstr  = zstring_trim(&rstr, " and");
	
	assert(strcmp(" sex and sun.", zstring_as_cstring(arena, &lstr)) == 0);
	assert(strcmp(" sex and ", zstring_as_cstring(arena, &rstr)) == 0);
	assert(strcmp("sex", zstring_as_cstring(arena, &sstr)) == 0);
}

int
main()
{
	arena_t*	arena = ARENA_ALLOC();

	test_slice(arena);
	test_cat(arena);
	test_split_join(arena);
	test_trim(arena);

	return 0;
}

#endif
