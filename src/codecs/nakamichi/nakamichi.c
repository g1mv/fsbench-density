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

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <divsufsort.h>

#ifdef PLATFORM_XMM
#include <emmintrin.h> // SSE2 intrinsics
#include <smmintrin.h> // SSE4.1 intrinsics
#endif
#ifdef PLATFORM_YMM
#include <emmintrin.h> // SSE2 intrinsics
#include <smmintrin.h> // SSE4.1 intrinsics
#include <immintrin.h> // AVX intrinsics
#endif

typedef int64_t nssize_t;
#define MAX_NSIZE   2147483647L
#define MAX_SA_SIZE 2147483647L

#define MATCH_LENGTH 8
#define MAX_LITERAL_LENGTH 32
#define MATCH_COST 2
#define LITERAL_COST(len) ((len) + 1)
#define WINDOW_SIZE 65536
#define OFFSET_MASK 0xE0
//#define OFFSET_MASK 7
#define OFFSET_IS_VALID(offset) ((offset) <= WINDOW_SIZE && (offset) > 0 && ((offset) & OFFSET_MASK))
#define N_STREAM_ERROR     ((nssize_t)(-1ULL))
#define N_BUFFER_TOO_SMALL ((nssize_t)(-2ULL))
#define N_OUT_OF_MEMORY    ((nssize_t)(-3ULL))
#define N_INVALID_ARGUMENT ((nssize_t)(-4ULL))
#define N_GENERIC_ERROR    ((nssize_t)(-5ULL))
#define N_ERROR(code)      ((code) >= N_GENERIC_ERROR)


static inline uint_fast16_t load_2_bytes_le(const uint8_t * source)
{
#if !defined(PLATFORM_IS_ALIGNED) && defined (PLATFORM_LITTLE_ENDIAN)
	return *(uint16_t*)source;
#else
	return source[0] | (source[1] << CHAR_BIT);
#endif
}

static inline void copy_8_bytes(uint8_t * target, const uint8_t * source)
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

static inline void copy_at_most_32_bytes(uint8_t * target, const uint8_t * source, nssize_t size)
{
#if defined(PLATFORM_GENERIC_ALIGNED)
        memcpy(target, source, size);
#elif defined(PLATFORM_GENERIC_UNALIGNED)
        *(uint64_t*)(target + sizeof(uint64_t) * 0) = *(uint64_t*)(source + sizeof(uint64_t) * 0);
        *(uint64_t*)(target + sizeof(uint64_t) * 1) = *(uint64_t*)(source + sizeof(uint64_t) * 1);
        *(uint64_t*)(target + sizeof(uint64_t) * 2) = *(uint64_t*)(source + sizeof(uint64_t) * 2);
        *(uint64_t*)(target + sizeof(uint64_t) * 3) = *(uint64_t*)(source + sizeof(uint64_t) * 3);
	
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
// TODO: How about integer overflow in out_index +=  (two_bytes & 0xFF) >> 3; ?
// TODO: better ssize_t definition
nssize_t DecompressM(const void * restrict _in, nssize_t in_size, void * restrict _out, nssize_t out_size)
{
    const uint8_t * in = _in;
    uint8_t * out = _out;
	nssize_t in_index = 0;
	nssize_t out_index = 0;
	// main loop may go 32 bytes out of limit on input
	const size_t main_loop_in_limit = in_size < MAX_LITERAL_LENGTH ? 0 : in_size - MAX_LITERAL_LENGTH;
	// and 31 on output
	const size_t main_loop_out_limit = out_size < (MAX_LITERAL_LENGTH - 1) ? 0 : out_size - (MAX_LITERAL_LENGTH - 1);

	// The head loop
	while(in_index < main_loop_in_limit && out_index < main_loop_out_limit && out_index < (WINDOW_SIZE - 1))
	{
		// 16 bits, AAABBBBB CCCCCCCC
		// If AAA == 0, we have a literal.
		//     BBBBB is its length reduced by 1 and CCCCCCCC - its first byte
		// If AAA != 0, we have a match.
		//     It's always 8-byte long
		//     and the whole two bytes are a little endian match offset.
		// Note a funny side effect, we can't encode any match in the stream
		// because match distances can't have all 3 most significant bits zeroed.
		uint_fast16_t two_bytes = load_2_bytes_le(in + in_index);
		if ((two_bytes & OFFSET_MASK) == 0)
		{
			// literal
			const int_fast16_t literal_length = (two_bytes & 0xFF) + 1;
			// a wild copy, may write too much, but no more than 32 bytes
			// loop limits prevent us from getting out of bounds
			// TODO: on x86 aligned copy of 32 bytes is always faster than aligned copy of literal_length
			// x86 sucks as a testbed for aligned access performance, check it elsewhere
			copy_at_most_32_bytes(out + out_index, in + in_index + 1, MAX_LITERAL_LENGTH);
			out_index += literal_length;
			in_index  += literal_length + 1;
		}
		else
		{
			// match
			in_index += 2;
			nssize_t in_size_left = in_size - in_index - 2;
			if (out_index < two_bytes)
				return N_STREAM_ERROR;
			// match length is always MATCH_LENGTH bytes
			copy_8_bytes(out + out_index, out + out_index - two_bytes);
			out_index += MATCH_LENGTH; 
		}
	}
	// The main loop
	while(in_index < main_loop_in_limit && out_index < main_loop_out_limit)
	{
		uint_fast16_t two_bytes = load_2_bytes_le(in + in_index);
		if ((two_bytes & OFFSET_MASK) == 0)
		{
			// literal
			const int_fast16_t literal_length = (two_bytes & 0xFF) + 1;
			// a wild copy, may write too much, but no more than 32 bytes
			// loop limits prevent us from getting out of bounds
			copy_at_most_32_bytes(out + out_index, in + in_index + 1, MAX_LITERAL_LENGTH);
			out_index += literal_length;
			in_index  += literal_length + 1;
		}
		else
		{
			// match
			in_index += 2;
			// match length is always MATCH_LENGTH bytes
			copy_8_bytes(out + out_index, out + out_index - two_bytes);
			out_index += MATCH_LENGTH; 
		}
	}
	// The tail loop
	while(in_index < (in_size - 1) && out_index < out_size)
	{
		uint_fast16_t two_bytes = load_2_bytes_le(in + in_index);
		nssize_t out_size_left = out_size - out_index;
		nssize_t in_size_left  = in_size - in_index;
		if ((two_bytes & OFFSET_MASK) == 0)
		{
			// literal
			in_size_left -= 1;
			const nssize_t size_left = in_size_left > out_size_left ? out_size_left : in_size_left;
			const int_fast16_t literal_length = (two_bytes & 0xFF) + 1;
			if (literal_length > size_left)
				return N_STREAM_ERROR;
			memcpy(out + out_index, in + in_index + 1, literal_length);
			out_index += literal_length;
			in_index  += literal_length + 1;
		}
		else
		{
			// match
			in_index += 2;
			in_size_left -= 2;
			// match length is always MATCH_LENGTH bytes
			if (out_size_left < MATCH_LENGTH || out_index < two_bytes)
				return N_STREAM_ERROR;
			copy_8_bytes(out + out_index, out + out_index - two_bytes);
			out_index += MATCH_LENGTH; 
		}
	}
	return in_index == in_size ? out_index : N_BUFFER_TOO_SMALL;
}

// TODO: integer overflow of costs
//       Note: cost that is not an overflow, but is so large that it becomes an error code is a problem too
nssize_t CompressM(const void * _in, size_t isize, void * _out, size_t osize)
{
    const uint8_t * in = _in;
    uint8_t * out = _out;
    nssize_t ret = 0;
    if (isize > (size_t)MAX_SA_SIZE)
        return N_INVALID_ARGUMENT;
    size_t suffix_array_size = (size_t)isize * sizeof(saidx_t);
    if (suffix_array_size <= isize) // overflow, possible on systems with 32-bit size_t
        return N_INVALID_ARGUMENT;
    saidx_t * suffix_array =    (saidx_t*)malloc(suffix_array_size);
    saidx_t * costs =           (saidx_t*)malloc(isize * sizeof(saidx_t));
    uint8_t * literal_lengths = (uint8_t*)malloc(isize * sizeof(uint8_t)); // 0 length => match
    if (!suffix_array || !costs || !literal_lengths)
    {
        ret = N_OUT_OF_MEMORY;
        goto end;
    }
    // prepare a match finder
    if (divsufsort(in, suffix_array, isize) != 0)
    {
        ret = N_GENERIC_ERROR;
        goto end;
    }
    // parse matches
    nssize_t iidx, oidx, i;
    // at the end there can be no match
    for (iidx = isize - 1, i = 1; iidx != 0 && i != MATCH_LENGTH; --iidx, ++i)
    {
        costs[iidx] = i + 1;
        literal_lengths[iidx] = i;
    }
    for (; iidx >= 0; --iidx)
    {
        // is there a match?
        // 
        saidx_t match_idx;
        saidx_t no_of_matches = sa_search(in, isize,
                                          in + iidx, MATCH_LENGTH,
                                          suffix_array, isize,
                                          &match_idx);
        // check literal cost
        int literal_length;
        nssize_t min_cost = MAX_NSIZE;
        nssize_t best_literal_length = MAX_NSIZE;
        nssize_t literal_length_limit = isize - iidx >= MAX_LITERAL_LENGTH ? MAX_LITERAL_LENGTH : isize - iidx;
        for (literal_length = 1; literal_length <= literal_length_limit; ++literal_length)
        {
            saidx_t cost;
            if (literal_lengths[iidx + literal_length] == 0)
            {
                // at that position there's a match. We need to insert a new literal
                cost = costs[iidx + literal_length] + LITERAL_COST(literal_length);
                if (cost < min_cost)
                {
                    min_cost = cost;
                    best_literal_length = literal_length;
                }
            }
            else
            {
                int total_literal_length = literal_lengths[iidx + literal_length] + literal_length;
                int combine = 0;
                if (total_literal_length <= MAX_LITERAL_LENGTH)
                {
                    combine = 1;
                    // we can combine the literals into one
                    cost = costs[iidx + literal_length] + literal_length;
                }
                else
                {
                    // we need to insert another tag
                    cost = costs[iidx + literal_length] + LITERAL_COST(literal_length);
                }
                if (cost < min_cost)
                {
                    min_cost = cost;
                    if (combine)
                        best_literal_length = literal_lengths[iidx + literal_length] + literal_length;
                    else
                        best_literal_length = literal_length;
                }
                // we can skip further matches.
                break;
            }
        }
        if (no_of_matches > 0)
        {
            int found_a_match = 0;
            // check if there's a match that we can encode
            for (i=0; i<no_of_matches; ++i)
            {
                saidx_t offset = iidx - suffix_array[match_idx + i];
                if (OFFSET_IS_VALID(offset))
                {
                    found_a_match = 1;
                    saidx_t cost = MATCH_COST + costs[iidx + MATCH_LENGTH];
                    if (cost < min_cost)
                    {
                        min_cost = cost;
                        best_literal_length = 0;
                    }
                    break;
                }
            }
        }
        literal_lengths[iidx] = best_literal_length;
        costs[iidx] = min_cost;
    }
    // encode
    for (iidx=0, oidx=0; iidx<isize && oidx<osize; )
    {
        nssize_t ospace = osize - oidx;
        nssize_t ispace = isize - iidx;
        if (literal_lengths[iidx] == 0)
        {
            if (ospace < 2)
            {
                ret = N_BUFFER_TOO_SMALL;
                goto end;
            }
            assert(ispace >= MATCH_LENGTH);
            saidx_t match_idx;
            saidx_t no_of_matches = sa_search(in, isize,
                                              in + iidx, MATCH_LENGTH,
                                              suffix_array, isize,
                                              &match_idx);
            assert(no_of_matches > 0);
            nssize_t best_match = -1;
            // find the best match
            for (i=0; i<no_of_matches; ++i)
            {
                nssize_t offset = iidx - suffix_array[match_idx + i];
                if (OFFSET_IS_VALID(offset))
                {
                    if (best_match < 0 || offset < best_match)
                    {
                        best_match = offset;
                    }
                }
            }
            assert(best_match > 0);
            out[oidx++] = best_match & 0xFF;
            out[oidx++] = (best_match >> 8) & 0xFF;
            iidx += MATCH_LENGTH;
        }
        else
        {
            assert(literal_lengths[iidx] <= MAX_LITERAL_LENGTH);
            assert(ispace >= literal_lengths[iidx]);
            if (ospace < literal_lengths[iidx] + 1)
            {
                ret = N_BUFFER_TOO_SMALL;
                goto end;
            }
            out[oidx++] = literal_lengths[iidx] - 1;
            memcpy(out + oidx, in + iidx, literal_lengths[iidx]);
            oidx += literal_lengths[iidx];
            iidx += literal_lengths[iidx];
        }
    }
    assert(oidx == costs[0]);
    ret = oidx;
    end:
    free(suffix_array);
    free(costs);
    free(literal_lengths);
    return ret;
}

nssize_t DecompressSafe(const void * _in, nssize_t in_size, void * _out, nssize_t out_size)
{
    const uint8_t * in = _in;
    uint8_t * out = _out;
    nssize_t in_index = 0;
    nssize_t out_index = 0;
    // main loop may go 32 bytes out of limit on input
    const size_t main_loop_in_limit = in_size < 32 ? 0 : in_size - 32;
    // and 31 on output
    const size_t main_loop_out_limit = out_size < 31 ? 0 : out_size - 31;

    // The head loop
    while(in_index < main_loop_in_limit && out_index < main_loop_out_limit && out_index < 65535)
    {
        // 16 bits, AAAAABBB CCCCCCCC
        // If BBB == 0, we have a literal.
        //     AAAAA is its length and CCCCCCCC - its first byte
        // If BBB != 0, we have a match.
        //     It's always 8-byte long
        //     and the whole two bytes are a little endian match offset.
        // Note a funny side effect, we can't encode any match in the stream
        // because match distances can't have all 3 least significant bits zeroed.
        uint_fast16_t two_bytes = load_2_bytes_le(in + in_index);
        if ((two_bytes & 0x07) == 0)
        {
            // literal
            const int_fast16_t literal_length = (two_bytes & 0xFF) >> 3;
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
            nssize_t in_size_left = in_size - in_index - 2;
            if (out_index < two_bytes)
                return -1;
            // match length is always 8 bytes
            copy_8_bytes(out + out_index, out + out_index - two_bytes);
            out_index += 8; 
        }
    }
    // The main loop
    while(in_index < main_loop_in_limit && out_index < main_loop_out_limit)
    {
        uint_fast16_t two_bytes = load_2_bytes_le(in + in_index);
        if ((two_bytes & 0x07) == 0)
        {
            // literal
            const int_fast16_t literal_length = (two_bytes & 0xFF) >> 3;
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
            copy_8_bytes(out + out_index, out + out_index - two_bytes);
            out_index += 8; 
        }
    }
    // The tail loop
    while(in_index < (in_size - 1) && out_index < out_size)
    {
        uint_fast16_t two_bytes = load_2_bytes_le(in + in_index);
        nssize_t out_size_left = out_size - out_index;
        nssize_t in_size_left  = in_size - in_index;
        if ((two_bytes & 0x07) == 0)
        {
            // literal
            in_size_left -= 1;
            const nssize_t size_left = in_size_left > out_size_left ? out_size_left : in_size_left;
            const int_fast16_t literal_length = (two_bytes & 0xFF) >> 3;
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
            if (out_size_left < 8 || out_index < two_bytes)
                return -1;
            copy_8_bytes(out + out_index, out + out_index - two_bytes);
            out_index += 8; 
        }
    }
    return in_index == in_size ? out_index : -1;
}