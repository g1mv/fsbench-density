// Nakamichi is 100% FREE LZSS SUPERFAST decompressor.

// This code is based on Nakamichi Kaidanji by Kaze, but heavily modified by m^2

//#define PLATFORM_GENERIC_ALIGNED
#define PLATFORM_GENERIC_UNALIGNED

//#define PLATFORM_BIG_ENDIAN
#define PLATFORM_LITTLE_ENDIAN

#ifdef PLATFORM_GENERIC_ALIGNED
#define PLATFORM_IS_ALIGNED
#endif

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <divsufsort.h>
#define MAX_SA_SIZE   2147483647L

#include "nakamichi.h"

//////////////
// TUNABLES //
//////////////


// MIN_MATCH_LENGTH: from 3 to MAX_MATCH_LENGTH
// Lowering this increases compression ratio at the cost of decompression speed
// Doesn't work well under 10 and gets really bad under 8
#define MIN_MATCH_LENGTH 12

// MIN_MATCH_LENGTH: no less than MIN_MATCH_LENGTH
//                   and no more than 31+MIN_MATCH_LENGTH
//                   also, no more than 48
// Good values are 31 / 32 / 40
#define MAX_MATCH_LENGTH 32

// Good values are 32 / 40 / 48
// The code doesn't support more than 48
#define MAX_LITERAL_LENGTH 40

// Only 262144 or 65536. 262144 seems better
#define WINDOW_SIZE 262144
//#define WINDOW_SIZE 65536

//////////////////
// OTHER MACROS //
//////////////////

#define MIN_MATCH_GAIN 1
#define MATCH_COST 3
#define LITERAL_COST(len) ((len) + 1)
// To enable copying of 8 bytes at the time, let's limit offsets to be large enough
#define MIN_OFFSET ((int)sizeof(uint64_t))
#define OFFSET_IS_VALID(offset) ((offset) < WINDOW_SIZE && (offset) > MIN_OFFSET)
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#if MAX_MATCH_LENGTH < MIN_MATCH_LENGTH || MAX_MATCH_LENGTH > MIN_MATCH_LENGTH + 31
# error "Invalid match lengths"
#endif

#if MAX_MATCH_LENGTH > 31
# define MIN_MATCH_LENGTH_SUBTRACT MIN_MATCH_LENGTH
#else
# define MIN_MATCH_LENGTH_SUBTRACT 0
#endif

#if WINDOW_SIZE > (1 << 16)
# if WINDOW_SIZE > (1 << 18)
#  error Too large window
# endif
# define WINDOW_EXTENSION(tag) (((tag) & 96) << 11)
#else
# define WINDOW_EXTENSION(tag) 0
#endif


static inline uint_fast16_t load_2_bytes_le(const uint8_t * source)
{
#if !defined(PLATFORM_IS_ALIGNED) && defined (PLATFORM_LITTLE_ENDIAN)
	return *(uint16_t*)source;
#else
	return source[0] | (source[1] << CHAR_BIT);
#endif
}

static inline void copy_a_match(uint8_t * target, const uint8_t * source, nssize_t size)
{
#if defined(PLATFORM_GENERIC_ALIGNED)
	// matches may overlap, though by at most MIN_OFFSET bytes
	memmove(target, source, size);
#elif defined(PLATFORM_GENERIC_UNALIGNED)
	
	*(uint64_t*)(target + sizeof(uint64_t) * 0) = *(uint64_t*)(source + sizeof(uint64_t) * 0);
	*(uint64_t*)(target + sizeof(uint64_t) * 1) = *(uint64_t*)(source + sizeof(uint64_t) * 1);
# if MAX_MATCH_LENGTH > 16
	*(uint64_t*)(target + sizeof(uint64_t) * 2) = *(uint64_t*)(source + sizeof(uint64_t) * 2);
# endif
# if MAX_MATCH_LENGTH > 24
	*(uint64_t*)(target + sizeof(uint64_t) * 3) = *(uint64_t*)(source + sizeof(uint64_t) * 3);
# endif
# if MAX_MATCH_LENGTH > 32
	*(uint64_t*)(target + sizeof(uint64_t) * 4) = *(uint64_t*)(source + sizeof(uint64_t) * 4);
# endif
# if MAX_MATCH_LENGTH > 40
	*(uint64_t*)(target + sizeof(uint64_t) * 5) = *(uint64_t*)(source + sizeof(uint64_t) * 5);
# endif
# if MAX_MATCH_LENGTH > 48
#  error MAX_MATCH_LENGTH it too long
# endif
#else
# error Define some platform
#endif
}

static inline void copy_literals(uint8_t * target, const uint8_t * source, nssize_t size)
{
#if defined(PLATFORM_GENERIC_ALIGNED)
	memcpy(target, source, size);
#elif defined(PLATFORM_GENERIC_UNALIGNED)
	*(uint64_t*)(target + sizeof(uint64_t) * 0) = *(uint64_t*)(source + sizeof(uint64_t) * 0);
	*(uint64_t*)(target + sizeof(uint64_t) * 1) = *(uint64_t*)(source + sizeof(uint64_t) * 1);
# if MAX_LITERAL_LENGTH > 16
	*(uint64_t*)(target + sizeof(uint64_t) * 2) = *(uint64_t*)(source + sizeof(uint64_t) * 2);
# endif
# if MAX_LITERAL_LENGTH > 24
	*(uint64_t*)(target + sizeof(uint64_t) * 3) = *(uint64_t*)(source + sizeof(uint64_t) * 3);
# endif
# if MAX_LITERAL_LENGTH > 32
	*(uint64_t*)(target + sizeof(uint64_t) * 4) = *(uint64_t*)(source + sizeof(uint64_t) * 4);
# endif
# if MAX_LITERAL_LENGTH > 40
	*(uint64_t*)(target + sizeof(uint64_t) * 5) = *(uint64_t*)(source + sizeof(uint64_t) * 5);
# endif
# if MAX_LITERAL_LENGTH > 48
#  error MAX_MATCH_LENGTH it too long
# endif
#else
# error Define some platform
#endif
}


// A safe decompression function; should never go out of bounds
// However, it doesn't attempt to detect stream errors.
// On corrupted stream, it may return either success or error; then the contents of the output stream are undefined.
// The whole function consists of 3 loops:
// * tail loop, run when input or output are about to end
// * head loop, run for the first WINDOW_SIZE KB of output or until we get near the end of one of the buffers
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
// TODO: Experiment: reverse loop conditions, so CPU predicts the other one
nssize_t DecompressM(const void * _in, nssize_t in_size, void * _out, nssize_t out_size)
{
	const uint8_t * in = _in;
	uint8_t * out = _out;
	nssize_t in_index = 0;
	nssize_t out_index = 0;
	// pointer movement / iteration limits
	const nssize_t max_main_loop_read = MAX(LITERAL_COST(MAX_LITERAL_LENGTH), MAX_MATCH_LENGTH + MATCH_COST);
	const nssize_t max_main_loop_write = MAX(MAX_LITERAL_LENGTH, MATCH_COST);
	// when to stop the main loop
	const size_t main_loop_in_limit  = in_size  < max_main_loop_read  ? 0 : in_size  - max_main_loop_read;
	const size_t main_loop_out_limit = out_size < max_main_loop_write ? 0 : out_size - max_main_loop_write;

	// The head loop
	while(in_index < main_loop_in_limit && out_index < main_loop_out_limit && out_index < (WINDOW_SIZE - 1))
	{
		uint_fast8_t tag = in[in_index];
		if ((tag & 128) == 0)
		{
			// literal
			const int_fast8_t literal_length = tag;
			copy_literals(out + out_index, in + in_index + 1, MAX_LITERAL_LENGTH);
			out_index += literal_length;
			in_index  += LITERAL_COST(literal_length);
		}
		else
		{
			// match
			const int_fast8_t match_length = (tag & 31) + MIN_MATCH_LENGTH_SUBTRACT;
			const uint_fast32_t offset = load_2_bytes_le(in + in_index + 1) | WINDOW_EXTENSION(tag);
			if (out_index < offset)
				return N_STREAM_ERROR;
			in_index += MATCH_COST;
			copy_a_match(out + out_index, out + out_index - offset, MAX_MATCH_LENGTH);
			out_index += match_length; 
		}
	}
	// The main loop
	while(in_index < main_loop_in_limit && out_index < main_loop_out_limit)
	{
		uint_fast8_t tag = in[in_index];
		if ((tag & 128) == 0)
		{
			// literal
			const int_fast8_t literal_length = tag;
			copy_literals(out + out_index, in + in_index + 1, MAX_LITERAL_LENGTH);
			out_index += literal_length;
			in_index  += LITERAL_COST(literal_length);
		}
		else
		{
			// match
			const int_fast8_t match_length = (tag & 31) + MIN_MATCH_LENGTH_SUBTRACT;
			const uint_fast32_t offset = load_2_bytes_le(in + in_index + 1) | WINDOW_EXTENSION(tag);
			in_index += MATCH_COST;
			copy_a_match(out + out_index, out + out_index - offset, MAX_MATCH_LENGTH);
			out_index += match_length; 
		}
	}
	// The tail loop
	while(in_index < (in_size - 1) && out_index < out_size)
	{
		nssize_t out_size_left = out_size - out_index;
		nssize_t in_size_left  = in_size - in_index;
		uint_fast8_t tag = in[in_index];
		if ((tag & 128) == 0)
		{
			// literal
			const int_fast8_t literal_length = tag;
			if (LITERAL_COST(literal_length) > in_size_left)
				return N_STREAM_ERROR;
			if (literal_length > out_size_left)
				return N_BUFFER_TOO_SMALL;
			memcpy(out + out_index, in + in_index + 1, literal_length);
			out_index += literal_length;
			in_index  += LITERAL_COST(literal_length);
		}
		else
		{
			// match
			if (in_size_left < MATCH_COST)
				return N_STREAM_ERROR;
			const int_fast8_t match_length = (tag & 31) + MIN_MATCH_LENGTH_SUBTRACT;
			const uint_fast32_t offset = load_2_bytes_le(in + in_index + 1) | WINDOW_EXTENSION(tag);
			if (out_index < offset || offset < MIN_OFFSET)
				return N_STREAM_ERROR;
			if (out_size_left < match_length)
				return N_BUFFER_TOO_SMALL;
			int i;
			for (i=0; i<match_length; ++i)
			{
				out[out_index + i] = out[out_index - offset + i];
			}
			// FIXME: for some reason memmove fails ??!!??
			//memmove(out + out_index, out + out_index - offset, MAX_MATCH_LENGTH);
			in_index += MATCH_COST;
			out_index += match_length; 
		}
	}
	return in_index == in_size ? out_index : N_BUFFER_TOO_SMALL;
}

// TODO: integer overflow of costs
//	   Note: cost that is not an overflow, but is so large that it becomes an error code is a problem too
// TODO: restrict
// TODO: remove assert macro redefinition


#undef assert
#define STRINGIFY(x) #x
//#define assert(x) do{if (!(x)) {printf("\nAssertion %s in line %d failed\n", STRINGIFY((x)), __LINE__); return 0;}} while(0)
#define assert(x)

nssize_t CompressM(const void * _in, nssize_t isize, void * _out, nssize_t osize)
{
	const uint8_t * in = _in;
	uint8_t * out = _out;
	nssize_t ret = 0;
	if (isize > MAX_SA_SIZE || isize < 0)
		return N_INVALID_ARGUMENT;
	size_t suffix_array_size = (size_t)isize * sizeof(saidx_t);
	if (suffix_array_size <= (size_t)isize) // overflow, possible on systems with 32-bit size_t
		return N_INVALID_ARGUMENT;
	saidx_t  * suffix_array = (saidx_t*) malloc(suffix_array_size);
	saidx_t  * costs =        (saidx_t*) malloc((isize+1) * sizeof(*costs));
	uint8_t  * tags =         (uint8_t*) malloc((isize+1) * sizeof(*tags));
	uint32_t * offsets =      (uint32_t*)malloc((isize+1) * sizeof(*offsets));
	if (!suffix_array || !costs || !tags || !offsets)
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
	costs[isize] = 0;
	for (iidx = isize - 1, i = 1; iidx != 0 && i != MIN_MATCH_LENGTH; --iidx, ++i)
	{
		costs[iidx] = i + 1;
		tags[iidx] = i;
		offsets[iidx] = 0;
	}
	for (; iidx >= 0; --iidx)
	{
		// check literal cost
		int_fast8_t literal_length;
		nssize_t min_cost = MAX_NSIZE;
		nssize_t best_tag = MAX_NSIZE;
		int best_offset = 0;
		int_fast8_t literal_length_limit = isize - iidx >= MAX_LITERAL_LENGTH ? MAX_LITERAL_LENGTH : isize - iidx;
		for (literal_length = 1; literal_length <= literal_length_limit; ++literal_length)
		{
			saidx_t cost;
			if (tags[iidx + literal_length] & 128)
			{
				// at that position there's a match. We need to insert a new literal
				cost = costs[iidx + literal_length] + LITERAL_COST(literal_length);
				if (cost < min_cost)
				{
					min_cost = cost;
					best_tag = literal_length;
				}
			}
			else
			{
				int total_literal_length = tags[iidx + literal_length] + literal_length;
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
						best_tag = tags[iidx + literal_length] + literal_length;
					else
						best_tag = literal_length;
				}
				// we can skip further literals.
				break;
			}
		}
		// is there a match?
		// 
		saidx_t match_idx;
		saidx_t no_of_matches = sa_search(in, isize,
		                                  in + iidx, MIN_MATCH_LENGTH,
		                                  suffix_array, isize,
						  &match_idx);
		if (no_of_matches > 0)
		{
			// check if there's a match that we can encode
			for (i=0; i<no_of_matches; ++i)
			{
				saidx_t offset = iidx - suffix_array[match_idx + i];
				if (OFFSET_IS_VALID(offset))
				{
					int match_length = MIN_MATCH_LENGTH;
					int max_match_length = MAX_MATCH_LENGTH;
					if (max_match_length > isize - iidx)
					max_match_length = isize - iidx ;
#ifndef PLATFORM_IS_ALIGNED
					// optimisation, a faster memcmp path
					int max_fast_match_length = ((max_match_length - MIN_MATCH_LENGTH) / sizeof(uint64_t)) * sizeof(uint64_t) + MIN_MATCH_LENGTH;
					for (; match_length<max_fast_match_length; match_length += sizeof(uint64_t))
					{
						if (*(uint64_t*)(in+iidx+match_length) != *(uint64_t*)(in+iidx+match_length-offset))
							break;
					}
#endif
					for (; match_length<max_match_length; ++match_length)
					{
						if (in[iidx+match_length] != in[iidx+match_length-offset])
							break;
					}
					//printf("match_length[%d, %d]: %d\n", iidx, i, match_length);
					assert(match_length >= MIN_MATCH_LENGTH);
					assert(match_length <= MAX_MATCH_LENGTH);
			
					saidx_t cost = MATCH_COST + costs[iidx + match_length];
					//printf("match_length[%d, %d]: %d, cost: %d\n", iidx, i, match_length, cost);
					// prefer literals; take a match only if cost is strictly lower (not equal)
					if ((best_offset == 0 && cost + MIN_MATCH_GAIN <= min_cost) || // literal is the best so far, we found a strong match
					    (best_offset != 0 && cost < min_cost) ||                   // match is the best so far, we found a stronger one
					    (cost == min_cost && best_offset > offset))                // match is the best so far, we found a more cache-efficient one
					{
						//printf("[%d]: best match[len=%d, offset=%d]\n", iidx, match_length, offset);
						min_cost = cost;
						best_offset = offset;
						best_tag = (match_length - MIN_MATCH_LENGTH_SUBTRACT) | 128;

#define INOPTIMAL
// Inoptimal mode is much faster, but weaker.
// It seems to produce code with longer average match size and therefore - one that decompresses faster
// Overall, as long as cost == size, it's superior to optimal in terms of decompression eficiency and also compresses much faster
// It would have been better to change the cost function, so it takes into account the time needed to decompress
#ifdef INOPTIMAL
						// don't consider all matches, only best 0 length ones
						if (match_length == MAX_MATCH_LENGTH)
							break;
#endif
					}
				}
			}
		}
		tags[iidx] = best_tag;
		costs[iidx] = min_cost;
		offsets[iidx] = best_offset;
	}
	/*for (i=0;i<isize;++i)
	{
		printf("idx=%d tag=%d cost=%d offset=%d\n", i, tags[i], costs[i], offsets[i]);
	}*/
	// encode
	for (iidx=0, oidx=0; iidx<isize && oidx<osize; )
	{
		nssize_t ospace = osize - oidx;
		nssize_t ispace = isize - iidx;
		if (tags[iidx] & 128)
		{
			const uint_fast8_t match_length = (tags[iidx] & 127) + MIN_MATCH_LENGTH_SUBTRACT;
			//printf("[%d]: match[len=%d, offset=%d, new_tag=%d]\n", iidx, match_length, offsets[iidx], tags[iidx] | ((offsets[iidx] >> 16) << 5));
			assert(match_length >= MIN_MATCH_LENGTH && match_length <= MAX_MATCH_LENGTH);
			assert(offsets[iidx] >= MIN_OFFSET);
			assert(memcmp(in + iidx, in + iidx - offsets[iidx], match_length) == 0);
			//printf("[%d]: match[len=%d, offset=%d]\n", iidx, match_length, offsets[iidx]);
			/*printf("[%d]: match[len=%d, offset=%d] [", iidx, match_length, offsets[iidx]);
			int i;
			for(i=0;i<match_length;++i)
			printf("%02X", in[iidx - offsets[iidx] + i]);
			printf("]\n");*/
			out[oidx++] = tags[iidx] | ((offsets[iidx] >> 16) << 5);
			out[oidx++] = offsets[iidx] & 0xFF;
			out[oidx++] = (offsets[iidx] >> 8) & 0xFF;
			iidx += match_length;
		}
		else
		{
			const int_fast8_t literal_length = tags[iidx];
			//printf("[%d]: literal[len=%d]\n", iidx, literal_length);
			/*printf("[%d]: literal[len=%d] [", iidx, literal_length);
			int i;
			for(i=0;i<literal_length;++i)
			printf("%02X", in[iidx + i]);
			printf("]\n");*/
			assert(literal_length <= MAX_LITERAL_LENGTH);
			assert(ispace >= literal_length);
			if (ospace < literal_length + 1)
			{
				ret = N_BUFFER_TOO_SMALL;
				goto end;
			}
			out[oidx++] = tags[iidx];
			memcpy(out + oidx, in + iidx, literal_length);
			oidx += literal_length;
			iidx += literal_length;
		}
	}
	assert(oidx == costs[0]);
	ret = oidx;
	end:
	free(suffix_array);
	free(costs);
	free(tags);
	free(offsets);
	//printf("ret=%d, error: %d\n", ret, N_ERROR(ret));
	return ret;
}
