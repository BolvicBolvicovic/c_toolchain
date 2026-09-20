#include "swissmap.h"
#include <string.h>

__always_inline u64
swiss_h1(u64 hash, u64 cap)
{
	return (hash >> 7) & (cap - 1);
}

/* Name: swiss_h2
 * Description: second part of the hash (first bits).
 * The hash is masked with SWISS_H2_MASK.
 * */
__always_inline u8
swiss_h2(u64 hash)
{
	return (u8)(hash & SWISS_H2_MASK);
}

/* Name: swiss_init_ctrl
 * Description: memset ctrl array with SWISS_EMPTY.
 * */
__always_inline void
swiss_init_ctrl(u8* ctrl, u64 cap)
{
	memset(ctrl, SWISS_EMPTY, cap);
}

/* Name: swiss_match_h2
 * Description: h2 is matched with 8 control bytes of the crtl array starting at offset.
 * The nth index of the matching byte is returned under the form of a mask
 * with the nth bit set matching the nth index.
 * If SSE2 is available, this can be done for 16 bytes in 3 instructions.
 * */
__always_inline u16
swiss_match_h2(const u8* ctrl, u8 h2, u64 offset)
{
#if defined(__SSE2__)
	__m128i	grp = _mm_loadu_si128((const __m128i*)(ctrl + offset));
	return _mm_movemask_epi8(_mm_cmpeq_epi8(_mm_set1_epi8(h2), grp));
#elif defined(__aarch64__)
	// TODO: Test this on aarch64.
	uint8x16_t	cmp = vceqq_u8(vdupq_n_u8(h2),vld1q_u8(crtl + offset));

	// Note: Equivalence to _mm_movemask_epi8 from x86.
	// Example input (half scale):
	// 0x89 FF 1D C0 00 10 99 33
	
	// Shift out everything but the sign bits
	// 0x01 01 00 01 00 00 01 00
	uint16x8_t	high_bits = vreinterpretq_u16_u8(vshrq_n_u8(cmp, 7));
	
	// Merge the even lanes together with vsra. The '??' bytes are garbage.
	// vsri could also be used, but it is slightly slower on aarch64.
	// 0x??03 ??02 ??00 ??01
	uint32x4_t	paired16 = vreinterpretq_u32_u16(
				vsraq_n_u16(high_bits, high_bits, 7));
	// Repeat with wider lanes.
	// 0x??????0B ??????04
	uint64x2_t	paired32 = vreinterpretq_u64_u32(
				vsraq_n_u32(paired16, paired16, 14));
	// 0x??????????????4B
	uint8x16_t	paired64 = vreinterpretq_u8_u64(
				vsraq_n_u64(paired32, paired32, 28));
	// Extract the low 8 bits from each lane and join.
	// 0x4B
	return vgetq_lane_u8(paired64, 0) | ((u16)vgetq_lane_u8(paired64, 8) << 8);
#else
	u16	mask = 0;

	for (u32 i = 0; i < SWISSMAP_GROUP_SIZE; i++)
		if (ctrl[offset +i] == h2)
			mask |= (1u << i);

	return mask;
#endif
}

/* Name: swiss_djb2_hash
 * Description: default hash when K_t is char*.
 * */
__always_inline u64
swiss_djb2_hash(char* str)
{
	u64	hash = 5381;
	s32	c;

	while ((c = *str++))
		hash = ((hash << 5) + hash) + c;

	return hash;
}

/* Name: swiss_strcmp_wrapper
 * Description: default key comparison function when K_t is char*.
 * */
__always_inline s32
swiss_strcmp_wrapper(char* s1, char* s2)
{
	return s1 && s2 ? strcmp(s1, s2) == 0 : 0;
}

#ifdef TESTER

#include <assert.h>
#include <arena.h>

SWISSMAP_DECLARE_DEFAULT(floatmap, float)
SWISSMAP_DEFINE_FUNCTIONS_DEFAULT(floatmap, float)

void
test_swissmap(void)
{
	arena_t*	arena	= ARENA_ALLOC();
	s32		sm_cap	= 1;
	floatmap*	sm	= floatmap_new(arena, sm_cap);
	char*		test	= "test\0";
	f64		value	= 42;

	assert(sm);

	assert(floatmap_get(sm, test) == 0);

	floatmap_put(sm, test, value);

	assert(floatmap_get(sm, test) != 0);
	assert(*floatmap_get(sm, test) == 42);
	
	floatmap_delete(sm, test);
	assert(floatmap_get(sm, test) == 0);
	
	char*	lit = ARENA_PUSH_ARRAY(arena, char, 5);

	sm_cap	= 16;
	sm	= floatmap_new(arena, sm_cap);

	memcpy(lit, test, 5);

	for (s32 i = 0; i <= sm_cap; i++)
	{
		lit[1] += i;

		char*	key = ARENA_PUSH_ARRAY(arena, char, 5);

		memcpy(key, lit, 5);
		floatmap_put(sm, key, (f64)i);

		if (i == sm_cap)
			assert(floatmap_get(sm, lit) == 0);
		else
		{
			assert(floatmap_get(sm, lit) != 0);
			assert(*floatmap_get(sm, lit) == i);
		}
	}

	arena_release(arena);
}

#endif // TESTER
