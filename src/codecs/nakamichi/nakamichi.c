// Nakamichi is 100% FREE LZSS SUPERFAST decompressor.

// This code is based on Nakamichi Kaidanji by Kaze, but reworked for greater security and portability by m^2

//#define PLATFORM_GENERIC_ALIGNED
#define PLATFORM_GENERIC_UNALIGNED
//#define PLATFORM_XMM
//#define PLATFORM_YMM

//#define PLATFORM_BIG_ENDIAN
#define PLATFORM_LITTLE_ENDIAN

#ifdef PLATFORM_GENERIC_ALIGNED
#define PLATFORM_IS_ALIGNED
#endif

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef PLATFORM_XMM
#include <emmintrin.h> // SSE2 intrinsics
#include <smmintrin.h> // SSE4.1 intrinsics
#endif
#ifdef PLATFORM_YMM
#include <emmintrin.h> // SSE2 intrinsics
#include <smmintrin.h> // SSE4.1 intrinsics
#include <immintrin.h> // AVX intrinsics
#endif

typedef int64_t ssize_t;

static inline uint_fast16_t load_2_bytes_le(const uint8_t *source)
{
#if !defined(PLATFORM_IS_ALIGNED) && defined (PLATFORM_LITTLE_ENDIAN)
	return *(uint16_t*)source;
#else
	return source[0] | (source[1] << 8);
#endif
}

static inline void copy_8_bytes(char *target, const char *source)
{
#ifdef PLATFORM_IS_ALIGNED
        *(target + 0) = *(source + 0);
        *(target + 1) = *(source + 1);
        *(target + 2) = *(source + 2);
        *(target + 3) = *(source + 3);
        *(target + 4) = *(source + 4);
        *(target + 5) = *(source + 5);
        *(target + 6) = *(source + 6);
        *(target + 7) = *(source + 7);
#else
        *(uint64_t*)target = *(uint64_t*)source;
#endif
}

static inline void copy_at_most_32_bytes(char *target, const char *source, ssize_t size)
{
#if defined(PLATFORM_GENERIC_ALIGNED)
        memcpy(target, source, size);
#elif defined(PLATFORM_GENERIC_UNALIGNED)
        *(uint64_t*)(target + 8 * 0) = *(uint64_t*)(source + 8 * 0);
        *(uint64_t*)(target + 8 * 1) = *(uint64_t*)(source + 8 * 1);
        *(uint64_t*)(target + 8 * 2) = *(uint64_t*)(source + 8 * 2);
        *(uint64_t*)(target + 8 * 3) = *(uint64_t*)(source + 8 * 3);
	
#elif defined(PLATFORM_XMM)
	_mm_storeu_si128((__m128i *)(target), _mm_loadu_si128((const __m128i *)(source)));
	_mm_storeu_si128((__m128i *)(target), _mm_loadu_si128((const __m128i *)(source + 16)));
#elif defined(PLATFORM_YMM)
	_mm256_storeu_si256((__m256i *)target, _mm256_loadu_si256((const __m256i *)source));
#else
#error Define some platform
#endif
}


// A safe decompression function; should never go out of bounds
// However, it doesn't attempt to detect stream errors.
// On corrupted stream, it may return either success or error; then the contents of the output stream are undefined.
// The whole function consists of 3 loops:
// * tail loop, run when input or output are about to end
// * head loop, run for the first 64 KB of output or until we get near the end of one of the buffers
// * main loop, when both start and end are far
// They differ with amount of bounds checking they perform.
// * tail loop checks everything
// * head loop checks if buffers are about to end and if match doesn't send us to before the output buffer start
//   It's quite imprecise in its copy operations and may go slightly farther than the buffer it's given.
//   That's why we need to terminate it before the end and finish with the tail loop
// * main loop is the most relaxed with its assumptions. It's like the initial loop,
//   but it doesn't even check if match offset sends us out of bounds.
// The decompressor is optimised for CPUs that can perform unaligned memory accesses.
// Others are supported, but optimisation for them is to be done later
// TODO: How about integer overflow in out_index +=  (WORDpair & 0xFF) >> 3; ?
ssize_t DecompressSafe(const char * in, ssize_t in_size, char * out, ssize_t out_size)
{
	ssize_t in_index = 0;
	ssize_t out_index = 0;
	// main loop may go 32 bytes out of limit on input
	const size_t main_loop_in_limit = in_size < 32 ? 0 : in_size - 32;
	// and 31 on output
	const size_t main_loop_out_limit = out_size < 31 ? 0 : out_size - 31;

	// The head loop
	while(in_index < main_loop_in_limit && out_index < main_loop_out_limit && out_index < 65535)
	{
		uint_fast16_t WORDpair = load_2_bytes_le(in + in_index);
		if ((WORDpair & 0x07) == 0)
		{
			// literal
			const int_fast16_t literal_length = (WORDpair & 0xFF) >> 3;
			// a wild copy, at least 1 byte too large
			// (since max literal length is 31)
			// but loop limits prevent us from getting out of bounds
			// TODO: on x86 aligned copy of 32 bytes is always faster than aligned copy of literal_length
			// x86 sucks as a testbed for aligned access performance, check it elsewhere
			copy_at_most_32_bytes(out + out_index, in + in_index + 1, 32);
			out_index += literal_length;
			in_index  += literal_length + 1;
		}
		else
		{
			// match
			in_index += 2;
			ssize_t in_size_left = in_size - in_index - 2;
			if (out_index < WORDpair)
				return -1;
			// match length is always 8 bytes
			copy_8_bytes(out + out_index, out + out_index - WORDpair);
			out_index += 8; 
		}
	}
	// The main loop
	while(in_index < main_loop_in_limit && out_index < main_loop_out_limit)
	{
		uint_fast16_t WORDpair = load_2_bytes_le(in + in_index);
		if ((WORDpair & 0x07) == 0)
		{
			// literal
			const int_fast16_t literal_length = (WORDpair & 0xFF) >> 3;
			// a wild copy, at least 1 byte too large
			// (since max literal length is 31)
			// but loop limits prevent us from getting out of bounds
			copy_at_most_32_bytes(out + out_index, in + in_index + 1, 32);
			out_index += literal_length;
			in_index  += literal_length + 1;
		}
		else
		{
			// match
			in_index += sizeof(uint16_t);
			// match length is always 8 bytes
			copy_8_bytes(out + out_index, out + out_index - WORDpair);
			out_index += 8; 
		}
	}
	// The tail loop
	while(in_index < (in_size - 1) && out_index < out_size)
	{
		uint_fast16_t WORDpair = load_2_bytes_le(in + in_index);
		ssize_t out_size_left = out_size - out_index;
		ssize_t in_size_left  = in_size - in_index;
		if ((WORDpair & 0x07) == 0)
		{
			// literal
			in_size_left -= 1;
			const ssize_t size_left = in_size_left > out_size_left ? out_size_left : in_size_left;
			const int_fast16_t literal_length = (WORDpair & 0xFF) >> 3;
			if (literal_length > size_left)
				return -1;
			memcpy(out + out_index, in + in_index + 1, literal_length);
			out_index += literal_length;
			in_index  += literal_length + 1;
		}
		else
		{
			// match
			in_index += 2;
			in_size_left -= 2;
			// match length is always 8 bytes
			if (out_size_left < 8 || out_index < WORDpair)
				return -1;
			copy_8_bytes(out + out_index, out + out_index - WORDpair);
			out_index += 8; 
		}
	}
	return in_index == in_size ? out_index : -1;
}
