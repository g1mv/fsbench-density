/* Long Run RLE. A weak, but very fast compression algorithm
 * System requirements:
 *  - CPU with support for unaligned memory access
 *  - Compressor and decompressor run on machine with the same endianness
 * Written by m^2, based on RLE64 from Javier Guti�rrez Chamorro
 */
 
#include <stdint.h>
#include <string.h>

#define variant1

// Some random number that marks the beginning of a run in the output stream. It's meant to be unlikely to be found in data being compressed.
#define LRRLE_RUN_MARKER 0x372E934ACB2BF3D1ull

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint_fast64_t lrrle256_compress(const void * in_, void * out_, uint_fast64_t in_size)
{
	const int tail_size = in_size & 31;
	const uint64_t * in = (const uint64_t *) in_;
	const uint64_t * in_limit = (const uint64_t *)((const uint8_t *) in_ + in_size - tail_size);
	uint64_t * out = (uint64_t *) out_;

	if (in < in_limit)
	{
		uint_fast64_t previous1 = *in++;
		uint_fast64_t previous2 = *in++;
		uint_fast64_t previous3 = *in++;
		uint_fast64_t previous4 = *in++;
		uint_fast64_t repetitions = 0;
		while (in < in_limit)
		{
			uint_fast64_t current1 = *in++;
			uint_fast64_t current2 = *in++;
			uint_fast64_t current3 = *in++;
			uint_fast64_t current4 = *in++;
			if (current1 == previous1 &&
			    current2 == previous2 &&
			    current3 == previous3 &&
			    current4 == previous4)
			{
				repetitions++;
			}
#ifdef variant1
            // Optimize the most common case
            else if ((repetitions == 0) && (previous1 != LRRLE_RUN_MARKER))
            {
                *out++ = previous1;
                *out++ = previous2;
                *out++ = previous3;
                *out++ = previous4;
                previous1 = current1;
                previous2 = current2;
                previous3 = current3;
                previous4 = current4;
            }
            // Skip compressing short runs
            else if ((repetitions == 1) && (previous1 != LRRLE_RUN_MARKER))
            {
                // We have a 2-word run,
                // too short to be worth compressing
                *out++ = previous1;
                *out++ = previous2;
                *out++ = previous3;
                *out++ = previous4;
                *out++ = previous1;
                *out++ = previous2;
                *out++ = previous3;
                *out++ = previous4;
                repetitions = 0;
                previous1 = current1;
                previous2 = current2;
                previous3 = current3;
                previous4 = current4;
            }
            else
            {
                *out++ = LRRLE_RUN_MARKER;
                *out++ = repetitions;
                *out++ = previous1;
                *out++ = previous2;
                *out++ = previous3;
                *out++ = previous4;
                repetitions = 0;
                previous1 = current1;
                previous2 = current2;
                previous3 = current3;
                previous4 = current4;
            }
#else
			else if (previous1 != LRRLE_RUN_MARKER)
			{
				switch (repetitions)
				{
					case 0:
						// Faster code for the most common case,
						// no repetition
						*out++ = previous1;
						*out++ = previous2;
						*out++ = previous3;
						*out++ = previous4;
						break;
					case 1:
						// We have a 2-word run,
						// too short to be worth compressing
						*out++ = previous1;
						*out++ = previous2;
						*out++ = previous3;
						*out++ = previous4;
						*out++ = previous1;
						*out++ = previous2;
						*out++ = previous3;
						*out++ = previous4;
						repetitions = 0;
						break;
					default:
						*out++ = LRRLE_RUN_MARKER;
						*out++ = repetitions;
						*out++ = previous1;
						*out++ = previous2;
						*out++ = previous3;
						*out++ = previous4;
						repetitions = 0;
						break;
				}
				previous1 = current1;
				previous2 = current2;
				previous3 = current3;
				previous4 = current4;
			}
			else
			{
				*out++ = LRRLE_RUN_MARKER;
				*out++ = repetitions;
				*out++ = previous1;
				*out++ = previous2;
				*out++ = previous3;
				*out++ = previous4;
				repetitions = 0;
				previous1 = current1;
				previous2 = current2;
				previous3 = current3;
				previous4 = current4;
			}
#endif
		}
		// Write any pending words
		if (repetitions > 1)
		{
			*out++ = LRRLE_RUN_MARKER;
			*out++ = repetitions;
			*out++ = previous1;
			*out++ = previous2;
			*out++ = previous3;
			*out++ = previous4;
		}
		else
		{
			out[0] = previous1;
			out[1] = previous2;
			out[2] = previous3;
			out[3] = previous4;
			out[4] = previous1;
			out[5] = previous2;
			out[6] = previous3;
			out[7] = previous4;
			out += 4 * (repetitions + 1);
		}
	}
	// Write rest of non-word bytes
	memcpy(out, in, tail_size);
	return (out - (uint64_t *) out_) * sizeof(uint64_t) + tail_size;
}
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint_fast64_t lrrle256_decompress(const void * in_, void * out_, uint_fast64_t out_size)
{
	const int tail_size = out_size & 31;
	const uint64_t * in = (const uint64_t *) in_;
	const uint64_t * out_limit = (const uint64_t *)((const uint8_t *) out_ + out_size - tail_size);
	uint64_t * out = (uint64_t *) out_;

	while (out < out_limit)
	{
		uint_fast64_t current1 = *in++;
		uint_fast64_t current2 = *in++;
		uint_fast64_t current3 = *in++;
		uint_fast64_t current4 = *in++;
		if (current1 == LRRLE_RUN_MARKER)
		{
			uint_fast64_t repetitions = current2;
			current1 = current3;
			current2 = current4;
			current3 = *in++;
			current4 = *in++;
			while (repetitions--)
			{
				*out++ = current1;
				*out++ = current2;
				*out++ = current3;
				*out++ = current4;
			}
		}
		*out++ = current1;
		*out++ = current2;
		*out++ = current3;
		*out++ = current4;
	}
	// Write rest of non-word bytes
	memcpy(out, in, tail_size);
	return (out - (uint64_t *) out_ - 1) * sizeof(uint64_t) + tail_size;
}
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint_fast64_t lrrle192_compress(const void * in_, void * out_, uint_fast64_t in_size)
{
	const int tail_size = in_size % 24;
	const uint64_t * in = (const uint64_t *) in_;
	const uint64_t * in_limit = (const uint64_t *)((const uint8_t *) in_ + in_size - tail_size);
	uint64_t * out = (uint64_t *) out_;

	if (in < in_limit)
	{
		uint_fast64_t previous1 = *in++;
		uint_fast64_t previous2 = *in++;
		uint_fast64_t previous3 = *in++;
		uint_fast64_t repetitions = 0;
		while (in < in_limit)
		{
			uint_fast64_t current1 = *in++;
			uint_fast64_t current2 = *in++;
			uint_fast64_t current3 = *in++;
			if (current1 == previous1 &&
			    current2 == previous2 &&
			    current3 == previous3)
			{
				repetitions++;
			}
#ifdef variant1
            // Optimize the most common case
            else if ((repetitions == 0) && (previous1 != LRRLE_RUN_MARKER))
            {
                *out++ = previous1;
                *out++ = previous2;
                *out++ = previous3;
                previous1 = current1;
                previous2 = current2;
                previous3 = current3;
            }
            // Skip compressing short runs
            else if ((repetitions == 1) && (previous1 != LRRLE_RUN_MARKER))
            {
                // We have a 2-word run,
                // too short to be worth compressing
                *out++ = previous1;
                *out++ = previous2;
                *out++ = previous3;
                *out++ = previous1;
                *out++ = previous2;
                *out++ = previous3;
                repetitions = 0;
                previous1 = current1;
                previous2 = current2;
                previous3 = current3;
            }
            else
            {
                *out++ = LRRLE_RUN_MARKER;
                *out++ = repetitions;
                *out++ = previous1;
                *out++ = previous2;
                *out++ = previous3;
                repetitions = 0;
                previous1 = current1;
                previous2 = current2;
                previous3 = current3;
            }
#else
			else if (previous1 != LRRLE_RUN_MARKER)
			{
				switch (repetitions)
				{
					case 0:
						// Faster code for the most common case,
						// no repetition
						*out++ = previous1;
						*out++ = previous2;
						*out++ = previous3;
						break;
					case 1:
						// We have a 2-word run,
						// too short to be worth compressing
						*out++ = previous1;
						*out++ = previous2;
						*out++ = previous3;
						*out++ = previous1;
						*out++ = previous2;
						*out++ = previous3;
						repetitions = 0;
						break;
					default:
						*out++ = LRRLE_RUN_MARKER;
						*out++ = repetitions;
						*out++ = previous1;
						*out++ = previous2;
						*out++ = previous3;
						repetitions = 0;
						break;
				}
				previous1 = current1;
				previous2 = current2;
				previous3 = current3;
			}
			else
			{
				*out++ = LRRLE_RUN_MARKER;
				*out++ = repetitions;
				*out++ = previous1;
				*out++ = previous2;
				*out++ = previous3;
				repetitions = 0;
				previous1 = current1;
				previous2 = current2;
				previous3 = current3;
			}
#endif
		}
		// Write any pending words
		if (repetitions > 1)
		{
			*out++ = LRRLE_RUN_MARKER;
			*out++ = repetitions;
			*out++ = previous1;
			*out++ = previous2;
			*out++ = previous3;
		}
		else
		{
			out[0] = previous1;
			out[1] = previous2;
			out[2] = previous3;
			out[3] = previous1;
			out[4] = previous2;
			out[5] = previous3;
			out += 3 * (repetitions + 1);
		}
	}
	// Write rest of non-word bytes
	memcpy(out, in, tail_size);
	return (out - (uint64_t *) out_) * sizeof(uint64_t) + tail_size;
}
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint_fast64_t lrrle192_decompress(const void * in_, void * out_, uint_fast64_t out_size)
{
    const int tail_size = out_size % 24;
    const uint64_t * in = (const uint64_t *) in_;
    const uint64_t * out_limit = (const uint64_t *)((const uint8_t *) out_ + out_size - tail_size);
    uint64_t * out = (uint64_t *) out_;

    while (out < out_limit)
	{
		uint_fast64_t current1 = *in++;
		uint_fast64_t current2 = *in++;
		uint_fast64_t current3 = *in++;
		if (current1 == LRRLE_RUN_MARKER)
		{
			uint_fast64_t repetitions = current2;
			current1 = current3;
			current2 = *in++;
			current3 = *in++;
			while (repetitions--)
			{
				*out++ = current1;
				*out++ = current2;
				*out++ = current3;
			}
		}
		*out++ = current1;
		*out++ = current2;
		*out++ = current3;
	}
	// Write rest of non-word bytes
	memcpy(out, in, tail_size);
	return (out - (uint64_t *) out_) * sizeof(uint64_t) + tail_size;
}
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint_fast64_t lrrle128_compress(const void * in_, void * out_, uint_fast64_t in_size)
{
	const int tail_size = in_size & 15;
	const uint64_t * in = (const uint64_t *) in_;
	const uint64_t * in_limit = (const uint64_t *)((const uint8_t *) in_ + in_size - tail_size);
	uint64_t * out = (uint64_t *) out_;

	if (in < in_limit)
	{
		uint_fast64_t previous1 = *in++;
		uint_fast64_t previous2 = *in++;
		uint_fast64_t repetitions = 0;
		while (in < in_limit)
		{
			uint_fast64_t current1 = *in++;
			uint_fast64_t current2 = *in++;
			if (current1 == previous1 && current2 == previous2)
			{
				repetitions++;
			}
#ifdef variant1
            // Optimize the most common case
            else if ((repetitions == 0) && (previous1 != LRRLE_RUN_MARKER))
            {
                *out++ = previous1;
                *out++ = previous2;
                previous1 = current1;
                previous2 = current2;
            }
            // Skip compressing short runs
            else if ((repetitions == 1) && (previous1 != LRRLE_RUN_MARKER))
            {
                // We have a 2-word run,
                // too short to be worth compressing
                *out++ = previous1;
                *out++ = previous2;
                *out++ = previous1;
                *out++ = previous2;
                repetitions = 0;
                previous1 = current1;
                previous2 = current2;
            }
            else
            {
                *out++ = LRRLE_RUN_MARKER;
                *out++ = repetitions;
                *out++ = previous1;
                *out++ = previous2;
                repetitions = 0;
                previous1 = current1;
                previous2 = current2;
            }
#else
			else if (previous1 != LRRLE_RUN_MARKER)
			{
                //printf("%d\n", repetitions);
				switch (repetitions)
				{
					case 0:
						// Faster code for the most common case,
						// no repetition
						*out++ = previous1;
						*out++ = previous2;
						break;
					case 1:
						// We have a 2-word run,
						// too short to be worth compressing
						*out++ = previous1;
						*out++ = previous2;
						*out++ = previous1;
						*out++ = previous2;
						repetitions = 0;
						break;
					default:
						*out++ = LRRLE_RUN_MARKER;
						*out++ = repetitions;
						*out++ = previous1;
						*out++ = previous2;
						repetitions = 0;
						break;
				}
				previous1 = current1;
				previous2 = current2;
			}
			else
			{
                //printf("%d\n", repetitions);
				*out++ = LRRLE_RUN_MARKER;
				*out++ = repetitions;
				*out++ = previous1;
				*out++ = previous2;
				repetitions = 0;
				previous1 = current1;
				previous2 = current2;
			}
#endif
		}
        //printf("%d\n", repetitions);
		// Write any pending words
		if (repetitions > 1)
		{
			*out++ = LRRLE_RUN_MARKER;
			*out++ = repetitions;
			*out++ = previous1;
			*out++ = previous2;
		}
		else
		{
			out[0] = previous1;
			out[1] = previous2;
			out[2] = previous1;
			out[3] = previous2;
			out += 2 * (repetitions + 1);
		}
	}
	// Write rest of non-word bytes
	memcpy(out, in, tail_size);
	return (out - (uint64_t *) out_) * sizeof(uint64_t) + tail_size;
}
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint_fast64_t lrrle128_decompress(const void * in_, void * out_, uint_fast64_t out_size)
{
    const int tail_size = out_size & 15;
    const uint64_t * in = (const uint64_t *) in_;
    const uint64_t * out_limit = (const uint64_t *)((const uint8_t *) out_ + out_size - tail_size);
    uint64_t * out = (uint64_t *) out_;

    while (out < out_limit)
	{
		uint_fast64_t current1 = *in++;
		uint_fast64_t current2 = *in++;
		if (current1 == LRRLE_RUN_MARKER)
		{
			uint_fast64_t repetitions = current2;
            //printf("d%d\n", repetitions);
			current1 = *in++;
			current2 = *in++;
			while (repetitions--)
			{
				*out++ = current1;
				*out++ = current2;
			}
		}
		else
            ;//printf("d0\n");
		*out++ = current1;
		*out++ = current2;
	}
	// Write rest of non-word bytes
	memcpy(out, in, tail_size);
	return (out - (uint64_t *) out_) * sizeof(uint64_t) + tail_size;
}
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint_fast64_t lrrle64_compress(const void * in_, void * out_, uint_fast64_t in_size)
{
	const int tail_size = in_size & 7;
	const uint64_t * in = (const uint64_t *) in_;
	const uint64_t * in_limit = (const uint64_t *)((const uint8_t *) in_ + in_size - tail_size);
	uint64_t * out = (uint64_t *) out_;

	if (in < in_limit)
	{
		uint_fast64_t previous = *in++;
		uint_fast64_t repetitions = 0;
		while (in < in_limit)
		{
			uint_fast64_t current = *in++;
			if (current == previous)
			{
				repetitions++;
			}
#ifdef variant1
			// Optimize the most common case
			else if ((repetitions == 0) && (previous != LRRLE_RUN_MARKER))
			{
				*out++ = previous;
				previous = current;
			}
			// Skip compressing short runs
			else if ((repetitions < 3) && (previous != LRRLE_RUN_MARKER))
			{
				// We have 1- or 2- byte run.
				// Let's write 2 bytes unconditionally,
				// it's faster this way.
				out[0] = previous;
				out[1] = previous;
				out[2] = previous;
				out += repetitions + 1;
				previous = current;
				repetitions = 0;
			}
			else
			{
				*out++ = LRRLE_RUN_MARKER;
				*out++ = repetitions;
				*out++ = previous;
				repetitions = 0;
				previous = current;
			}
    #else
			else if (previous != LRRLE_RUN_MARKER)
			{
			#if 0
				switch (repetitions)
				{
					case 0:
						break;
					case 1:
					case 2:
						out[0] = previous;
						out[1] = previous;
						out += repetitions;
						repetitions = 0;
						break;
					default:
						*out++ = LRRLE_RUN_MARKER;
						*out++ = repetitions;
						repetitions = 0;
						break;
				}
				*out++ = previous;
				previous = current;
			#elif 0
				switch (repetitions)
				{
					case 0:
					case 1:
					case 2:
						out[0] = previous;
						out[1] = previous;
						out += repetitions;
						break;
					default:
						*out++ = LRRLE_RUN_MARKER;
						*out++ = repetitions;
						break;
				}
				repetitions = 0;
				*out++ = previous;
				previous = current;
			#else
				switch (repetitions)
				{
					case 0:
						// Faster code for the most common case,
						// no repetition
						*out++ = previous;
						break;
					case 1:
					case 2:
						// We have a 2- or 3- word run.
						// Let's write 3 words unconditionally,
						// it's faster this way.
						// Note: With 3 words we could encode either a run
						// or individual words. Individual words are faster.
						out[0] = previous;
						out[1] = previous;
						out[2] = previous;
						out += repetitions + 1;
						repetitions = 0;
						break;
					default:
						*out++ = LRRLE_RUN_MARKER;
						*out++ = repetitions;
						*out++ = previous;
						repetitions = 0;
						break;
				}
				previous = current;
#endif
			}
			else
			{
				*out++ = LRRLE_RUN_MARKER;
				*out++ = repetitions;
				*out++ = previous;
				repetitions = 0;
				previous = current;
			}
		#endif
		}
		// Write any pending words
		if (repetitions > 2)
		{
			*out++ = LRRLE_RUN_MARKER;
			*out++ = repetitions;
			*out++ = previous;
		}
		else
		{
			// We have a 1-, 2- or 3- word run.
			// Let's write 3 words unconditionally,
			// it's simpler this way.
			out[0] = previous;
			out[1] = previous;
			out[2] = previous;
			out += repetitions + 1;
		}
	}
	// Write rest of non-word bytes
	memcpy(out, in, tail_size);
	return (out - (uint64_t *) out_) * sizeof(uint64_t) + tail_size;
}
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint_fast64_t lrrle64_decompress(const void * in_, void * out_, uint_fast64_t out_size)
{
    const int tail_size = out_size & 7;
    const uint64_t * in = (const uint64_t *) in_;
    const uint64_t * out_limit = (const uint64_t *)((const uint8_t *) out_ + out_size - tail_size);
    uint64_t * out = (uint64_t *) out_;

    while (out < out_limit)
	{
		uint_fast64_t current = *in++;
		if (current == LRRLE_RUN_MARKER)
		{
			uint_fast64_t repetitions = *in++;
			current = *in++;
			while (repetitions--)
			{
				*out++ = current;
			}
		}
		*out++ = current;
	}
	// Write rest of non-word bytes
	memcpy(out, in, tail_size);
	return (out - (uint64_t *) out_ - 1) * sizeof(uint64_t) + tail_size;
}
