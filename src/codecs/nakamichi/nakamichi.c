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

static inline uint_fast16_t load_2_bytes_le(const uint8_t * source)
{
#if !defined(PLATFORM_IS_ALIGNED) && defined (PLATFORM_LITTLE_ENDIAN)
	return *(uint16_t*)source;
#else
	return source[0] | (source[1] << 8);
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

static inline void copy_at_most_32_bytes(uint8_t * target, const uint8_t * source, ssize_t size)
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
// TODO: How about integer overflow in out_index +=  (two_bytes & 0xFF) >> 3; ?
ssize_t DecompressM(const void * _in, ssize_t in_size, void * _out, ssize_t out_size)
{
    const uint8_t * in = _in;
    uint8_t * out = _out;
	ssize_t in_index = 0;
	ssize_t out_index = 0;
	// main loop may go 32 bytes out of limit on input
	const size_t main_loop_in_limit = in_size < 32 ? 0 : in_size - 32;
	// and 31 on output
	const size_t main_loop_out_limit = out_size < 31 ? 0 : out_size - 31;

	// The head loop
	while(in_index < main_loop_in_limit && out_index < main_loop_out_limit && out_index < 65535)
	{
		// 16 bits, AAAAABBB CCCCCCCC
		// If BBB == 0, we have a literal.
		//     AAAAA is its length reduced by 1 and CCCCCCCC - its first byte
		// If BBB != 0, we have a match.
		//     It's always 8-byte long
		//     and the whole two bytes are a little endian match offset.
		// Note a funny side effect, we can't encode any match in the stream
		// because match distances can't have all 3 least significant bits zeroed.
		uint_fast16_t two_bytes = load_2_bytes_le(in + in_index);
		if ((two_bytes & 0x07) == 0)
		{
			// literal
			const int_fast16_t literal_length = ((two_bytes & 0xFF) >> 3) + 1;
			// a wild copy, may write too much, but no more than 32 bytes
			// loop limits prevent us from getting out of bounds
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
			const int_fast16_t literal_length = ((two_bytes & 0xFF) >> 3) + 1;
			// a wild copy, may write too much, but no more than 32 bytes
			// loop limits prevent us from getting out of bounds
			copy_at_most_32_bytes(out + out_index, in + in_index + 1, 32);
			out_index += literal_length;
			in_index  += literal_length + 1;
		}
		else
		{
			// match
			in_index += 2;
			// match length is always 8 bytes
			copy_8_bytes(out + out_index, out + out_index - two_bytes);
			out_index += 8; 
		}
	}
	// The tail loop
	while(in_index < (in_size - 1) && out_index < out_size)
	{
		uint_fast16_t two_bytes = load_2_bytes_le(in + in_index);
		ssize_t out_size_left = out_size - out_index;
		ssize_t in_size_left  = in_size - in_index;
		if ((two_bytes & 0x07) == 0)
		{
			// literal
			in_size_left -= 1;
			const ssize_t size_left = in_size_left > out_size_left ? out_size_left : in_size_left;
			const int_fast16_t literal_length = ((two_bytes & 0xFF) >> 3) + 1;
			if (literal_length > size_left)
				return -1;
			memcpy(out + out_index, in + in_index + 1, literal_length);
			out_index += literal_length;
			in_index  += literal_length + 1;
			/*ssize_t i;
			++in_index;
			for(i=0; i<literal_length; ++i)
				out[out_index++] = in[in_index++];*/
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
ssize_t DecompressSafe(const void * _in, ssize_t in_size, void * _out, ssize_t out_size)
{
    const uint8_t * in = _in;
    uint8_t * out = _out;
	ssize_t in_index = 0;
	ssize_t out_index = 0;
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
			ssize_t in_size_left = in_size - in_index - 2;
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
		ssize_t out_size_left = out_size - out_index;
		ssize_t in_size_left  = in_size - in_index;
		if ((two_bytes & 0x07) == 0)
		{
			// literal
			in_size_left -= 1;
			const ssize_t size_left = in_size_left > out_size_left ? out_size_left : in_size_left;
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

static void SearchIntoSlidingWindow(unsigned int * retIndex, unsigned int * retMatch, const uint8_t * refStart, const uint8_t * refEnd, const uint8_t * encStart);
static const uint8_t * Railgun_Swampshine_BailOut(const uint8_t * pbTarget, const uint8_t * pbPattern, uint32_t cbTarget, uint32_t cbPattern);

#define Match_Length (8)
#define OffsetBITS (16)
#define LengthBITS (1)

//12bit
#define REF_SIZE ( ((1<<OffsetBITS)-1) )
//3bit
#define ENC_SIZE ( ((1<<LengthBITS)-1) + Match_Length )

static void SearchIntoSlidingWindow(unsigned int * retIndex, unsigned int * retMatch, const uint8_t * refStart, const uint8_t * refEnd, const uint8_t * encStart)
{
    const uint8_t* FoundAtPosition;
    const uint8_t* split  = refStart + 64*1024 - 16*1024; // last 16 KB
    const uint8_t* split2 = refStart + 64*1024 - 32*1024; // last 32 KB

    *retIndex=0;
    *retMatch=0;

    const uint8_t * start[] = {split, split2, refStart};
    const uint8_t * end[] = {refEnd, split + Match_Length, split2 + Match_Length}; // TODO: is + Match_Length needed?
	int i;
	for(i=0; i<3; ++i)
	{
	    const uint8_t *current_start = start[i];
	    const uint8_t *current_end = end[i];
		while (current_start < current_end && current_start < refEnd)
		{
			FoundAtPosition = Railgun_Swampshine_BailOut(current_start, encStart, (uint32_t)(refEnd-current_start), Match_Length);
			if (FoundAtPosition!=NULL)
			{
				// Stupid sanity check, in next revision I will discard 'Match_Length' additions/subtractions altogether:
				if ( (refEnd-FoundAtPosition) & 0x07 )
				{
					// Discard matches that have OFFSET with lower 3bits ALL zero.
					// Having all 3 lower bits 0 is an indicator of a literal
					*retMatch=Match_Length;
					*retIndex=refEnd-FoundAtPosition;
					return;
				}
				current_start=FoundAtPosition+1; // Exhaust the pool.
			}
			else
			    break;
		}
	}
}

unsigned int CompressM(void * _ret, const void * _src, unsigned int srcSize)
{
    const uint8_t * src = _src;
    uint8_t * ret = _ret;
	unsigned int srcIndex=0;
	unsigned int retIndex=0;
	unsigned int index=0;
	unsigned int match=0;
	unsigned int notMatch=0;
	uint8_t * notMatchStart=NULL;
	const uint8_t * refStart=NULL;
	const uint8_t * encEnd=NULL;

	while(srcIndex < srcSize){
		if(srcIndex>=REF_SIZE)
			refStart=&src[srcIndex-REF_SIZE];
		else
			refStart=src;
		if(srcIndex>=srcSize-ENC_SIZE)
			encEnd=&src[srcSize];
		else
			encEnd=&src[srcIndex+ENC_SIZE];
		if(srcIndex+ENC_SIZE < srcSize)
			SearchIntoSlidingWindow(&index,&match,refStart,&src[srcIndex],&src[srcIndex]);
		else
			match=0; // Nothing to find.
  		if ( match==0 ) {
			if(notMatch==0){
				notMatchStart=&ret[retIndex];
				retIndex++;
			}
			else if (notMatch==32) {
				*notMatchStart=(uint8_t)31<<3;
				notMatch=0;
				notMatchStart=&ret[retIndex];
				retIndex++;
			}
			ret[retIndex]=src[srcIndex];
			retIndex++;
			notMatch++;
			srcIndex++;
		} else {
			if(notMatch > 0){
				*notMatchStart=(uint8_t)((notMatch-1)<<3);
				notMatch=0;
			}
			// sanity check
			if (index>0xFFFF) {return 0;}
			 // copy lower 2 bytes
			ret[retIndex++] = index & 0xFF;
			ret[retIndex++] = (index >> 8) & 0xFF;
			srcIndex+=match;
		}
	}
	if(notMatch > 0){
		*notMatchStart=(uint8_t)((notMatch-1)<<3);
	}
	return retIndex;
}


// Railgun_Swampshine_BailOut, copyleft 2014-Jan-31, Kaze.
// Caution: For better speed the case 'if (cbPattern==1)' was removed, so Pattern must be longer than 1 char.
#define NeedleThreshold2vs4swampLITE 9+10 // Should be bigger than 9. BMH2 works up to this value (inclusive), if bigger then BMH4 takes over.
static const uint8_t * Railgun_Swampshine_BailOut(const uint8_t * pbTarget, const uint8_t * pbPattern, uint32_t cbTarget, uint32_t cbPattern)
{
    const uint8_t * pbTargetMax = pbTarget + cbTarget;
	register uint32_t ulHashPattern;
	signed long count;

	uint8_t bm_Horspool_Order2[256*256]; // Bitwise soon...
	uint32_t i, Gulliver;

	uint32_t PRIMALposition, PRIMALpositionCANDIDATE;
	uint32_t PRIMALlength, PRIMALlengthCANDIDATE;
	uint32_t j, FoundAtPosition;

	if (cbPattern > cbTarget) return(NULL);

	if ( cbPattern<4 ) { 
		// SSE2 i.e. 128bit Assembly rules here:
		// ...
        	pbTarget = pbTarget+cbPattern;
		ulHashPattern = ( (*(uint8_t *)(pbPattern))<<8 ) + *(pbPattern+(cbPattern-1));
		if ( cbPattern==3 ) {
			for ( ;; ) {
				if ( ulHashPattern == ( (*(uint8_t *)(pbTarget-3))<<8 ) + *(pbTarget-1) ) {
					if ( *(uint8_t *)(pbPattern+1) == *(uint8_t *)(pbTarget-2) ) return((pbTarget-3));
				}
				if ( (uint8_t)(ulHashPattern>>8) != *(pbTarget-2) ) { 
					pbTarget++;
					if ( (uint8_t)(ulHashPattern>>8) != *(pbTarget-2) ) pbTarget++;
				}
				pbTarget++;
				if (pbTarget > pbTargetMax) return(NULL);
			}
		} else {
		}
		for ( ;; ) {
			if ( ulHashPattern == ( (*(uint8_t *)(pbTarget-2))<<8 ) + *(pbTarget-1) ) return((pbTarget-2));
			if ( (uint8_t)(ulHashPattern>>8) != *(pbTarget-1) ) pbTarget++;
			pbTarget++;
			if (pbTarget > pbTargetMax) return(NULL);
		}
	} else { //if ( cbPattern<4 )
		if ( cbPattern<=NeedleThreshold2vs4swampLITE ) { 
			// BMH order 2, needle should be >=4:
			ulHashPattern = *(uint32_t *)(pbPattern); // First four bytes
			for (i=0; i < 256*256; i++) {bm_Horspool_Order2[i]=0;}
			for (i=0; i < cbPattern-1; i++) bm_Horspool_Order2[*(unsigned short *)(pbPattern+i)]=1;
			i=0;
			while (i <= cbTarget-cbPattern) {
				Gulliver = 1; // 'Gulliver' is the skip
				if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1]] != 0 ) {
					if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1-2]] == 0 ) Gulliver = cbPattern-(2-1)-2; else {
						if ( *(uint32_t *)&pbTarget[i] == ulHashPattern) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
							count = cbPattern-4+1; 
							while ( count > 0 && *(uint32_t *)(pbPattern+count-1) == *(uint32_t *)(&pbTarget[i]+(count-1)) )
								count = count-4;
							if ( count <= 0 ) return(pbTarget+i);
						}
					}
				} else Gulliver = cbPattern-(2-1);
				i = i + Gulliver;
				//GlobalI++; // Comment it, it is only for stats.
			}
			return(NULL);
		} else { // if ( cbPattern<=NeedleThreshold2vs4swampLITE )

// Swampwalker_BAILOUT heuristic order 4 (Needle should be bigger than 4) [
// Needle: 1234567890qwertyuiopasdfghjklzxcv            PRIMALposition=01 PRIMALlength=33  '1234567890qwertyuiopasdfghjklzxcv'
// Needle: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv            PRIMALposition=29 PRIMALlength=04  'vvvv'
// Needle: vvvvvvvvvvBOOMSHAKALAKAvvvvvvvvvv            PRIMALposition=08 PRIMALlength=20  'vvvBOOMSHAKALAKAvvvv'
// Needle: Trollland                                    PRIMALposition=01 PRIMALlength=09  'Trollland'
// Needle: Swampwalker                                  PRIMALposition=01 PRIMALlength=11  'Swampwalker'
// Needle: licenselessness                              PRIMALposition=01 PRIMALlength=15  'licenselessness'
// Needle: alfalfa                                      PRIMALposition=02 PRIMALlength=06  'lfalfa'
// Needle: Sandokan                                     PRIMALposition=01 PRIMALlength=08  'Sandokan'
// Needle: shazamish                                    PRIMALposition=01 PRIMALlength=09  'shazamish'
// Needle: Simplicius Simplicissimus                    PRIMALposition=06 PRIMALlength=20  'icius Simplicissimus'
// Needle: domilliaquadringenquattuorquinquagintillion  PRIMALposition=01 PRIMALlength=32  'domilliaquadringenquattuorquinqu'
// Needle: boom-boom                                    PRIMALposition=02 PRIMALlength=08  'oom-boom'
// Needle: vvvvv                                        PRIMALposition=01 PRIMALlength=04  'vvvv'
// Needle: 12345                                        PRIMALposition=01 PRIMALlength=05  '12345'
// Needle: likey-likey                                  PRIMALposition=03 PRIMALlength=09  'key-likey'
// Needle: BOOOOOM                                      PRIMALposition=03 PRIMALlength=05  'OOOOM'
// Needle: aaaaaBOOOOOM                                 PRIMALposition=02 PRIMALlength=09  'aaaaBOOOO'
// Needle: BOOOOOMaaaaa                                 PRIMALposition=03 PRIMALlength=09  'OOOOMaaaa'
PRIMALlength=0;
for (i=0+(1); i < cbPattern-((4)-1)+(1)-(1); i++) { // -(1) because the last BB order 4 has no counterpart(s)
	FoundAtPosition = cbPattern - ((4)-1) + 1;
	PRIMALpositionCANDIDATE=i;
	while ( PRIMALpositionCANDIDATE <= (FoundAtPosition-1) ) {
		j = PRIMALpositionCANDIDATE + 1;
		while ( j <= (FoundAtPosition-1) ) {
			if ( *(uint32_t *)(pbPattern+PRIMALpositionCANDIDATE-(1)) == *(uint32_t *)(pbPattern+j-(1)) ) FoundAtPosition = j;
			j++;
		}
		PRIMALpositionCANDIDATE++;
	}
	PRIMALlengthCANDIDATE = (FoundAtPosition-1)-i+1 +((4)-1);
	if (PRIMALlengthCANDIDATE >= PRIMALlength) {PRIMALposition=i; PRIMALlength = PRIMALlengthCANDIDATE;}
	if (cbPattern-i+1 <= PRIMALlength) break;
	if (PRIMALlength > 128) break; // Bail Out for 129[+]
}
// Swampwalker_BAILOUT heuristic order 4 (Needle should be bigger than 4) ]

// Here we have 4 or bigger NewNeedle, apply order 2 for pbPattern[i+(PRIMALposition-1)] with length 'PRIMALlength' and compare the pbPattern[i] with length 'cbPattern':
PRIMALlengthCANDIDATE = cbPattern;
cbPattern = PRIMALlength;
pbPattern = pbPattern + (PRIMALposition-1);

// Revision 2 commented section ]

		if ( cbPattern<=NeedleThreshold2vs4swampLITE ) { 

			// BMH order 2, needle should be >=4:
			ulHashPattern = *(uint32_t *)(pbPattern); // First four bytes
			for (i=0; i < 256*256; i++) {bm_Horspool_Order2[i]=0;}
			for (i=0; i < cbPattern-1; i++) bm_Horspool_Order2[*(unsigned short *)(pbPattern+i)]=1;
			i=0;
			while (i <= cbTarget-cbPattern) {
				Gulliver = 1; // 'Gulliver' is the skip
				if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1]] != 0 ) {
					if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1-2]] == 0 ) Gulliver = cbPattern-(2-1)-2; else {
						if ( *(uint32_t *)&pbTarget[i] == ulHashPattern) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
							count = cbPattern-4+1; 
							while ( count > 0 && *(uint32_t *)(pbPattern+count-1) == *(uint32_t *)(&pbTarget[i]+(count-1)) )
								count = count-4;
// If we miss to hit then no need to compare the original: Needle
if ( count <= 0 ) {
// I have to add out-of-range checks...
// i-(PRIMALposition-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4

// FIX from 2014-Apr-27:
// Because (count-1) is negative, above fours are reduced to next twos:
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
	// The line below is BUGGY:
	//if ( (i-(PRIMALposition-1) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) && (&pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4) ) {
	// The line below is OKAY:
	if ( ((signed int)(i-(PRIMALposition-1)+(count-1)) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) ) {

		if ( *(uint32_t *)&pbTarget[i-(PRIMALposition-1)] == *(uint32_t *)(pbPattern-(PRIMALposition-1))) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
			count = PRIMALlengthCANDIDATE-4+1; 
			while ( count > 0 && *(uint32_t *)(pbPattern-(PRIMALposition-1)+count-1) == *(uint32_t *)(&pbTarget[i-(PRIMALposition-1)]+(count-1)) )
				count = count-4;
			if ( count <= 0 ) return(pbTarget+i-(PRIMALposition-1));	
		}
	}
}
						}
					}
				} else Gulliver = cbPattern-(2-1);
				i = i + Gulliver;
				//GlobalI++; // Comment it, it is only for stats.
			}
			return(NULL);

		} else { // if ( cbPattern<=NeedleThreshold2vs4swampLITE )

			// BMH pseudo-order 4, needle should be >=8+2:
			ulHashPattern = *(uint32_t *)(pbPattern); // First four bytes
			for (i=0; i < 256*256; i++) {bm_Horspool_Order2[i]=0;}
			// In line below we "hash" 4bytes to 2bytes i.e. 16bit table, how to compute TOTAL number of BBs, 'cbPattern - Order + 1' is the number of BBs for text 'cbPattern' bytes long, for example, for cbPattern=11 'fastest fox' and Order=4 we have BBs = 11-4+1=8:
			//"fast"
			//"aste"
			//"stes"
			//"test"
			//"est "
			//"st f"
			//"t fo"
			//" fox"
			//for (i=0; i < cbPattern-4+1; i++) bm_Horspool_Order2[( *(unsigned short *)(pbPattern+i+0) + *(unsigned short *)(pbPattern+i+2) ) & ( (1<<16)-1 )]=1;
			//for (i=0; i < cbPattern-4+1; i++) bm_Horspool_Order2[( (*(uint32_t *)(pbPattern+i+0)>>16)+(*(uint32_t *)(pbPattern+i+0)&0xFFFF) ) & ( (1<<16)-1 )]=1;
			// Above line is replaced by next one with better hashing:
			for (i=0; i < cbPattern-4+1; i++) bm_Horspool_Order2[( (*(uint32_t *)(pbPattern+i+0)>>(16-1))+(*(uint32_t *)(pbPattern+i+0)&0xFFFF) ) & ( (1<<16)-1 )]=1;
			i=0;
			while (i <= cbTarget-cbPattern) {
				Gulliver = 1;
				//if ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2]&0xFFFF) ) & ( (1<<16)-1 )] != 0 ) { // DWORD #1
				// Above line is replaced by next one with better hashing:
				if ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2]>>(16-1))+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2]&0xFFFF) ) & ( (1<<16)-1 )] != 0 ) { // DWORD #1
					//if ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) ) & ( (1<<16)-1 )] == 0 ) Gulliver = cbPattern-(2-1)-2-4; else {
					// Above line is replaced in order to strengthen the skip by checking the middle DWORD,if the two DWORDs are 'ab' and 'cd' i.e. [2x][2a][2b][2c][2d] then the middle DWORD is 'bc'.
					// The respective offsets (backwards) are: -10/-8/-6/-4 for 'xa'/'ab'/'bc'/'cd'.
					//if ( ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]&0xFFFF) ) & ( (1<<16)-1 )] ) < 3 ) Gulliver = cbPattern-(2-1)-2-4-2; else {
					// Above line is replaced by next one with better hashing:
					// When using (16-1) right shifting instead of 16 we will have two different pairs (if they are equal), the highest bit being lost do the job especialy for ASCII texts with no symbols in range 128-255.
					// Example for genomesque pair TT+TT being shifted by (16-1):
					// T            = 01010100
					// TT           = 01010100 01010100
					// TTTT         = 01010100 01010100 01010100 01010100
					// TTTT>>16     = 00000000 00000000 01010100 01010100
					// TTTT>>(16-1) = 00000000 00000000 10101000 10101000 <--- Due to the left shift by 1, the 8th bits of 1st and 2nd bytes are populated - usually they are 0 for English texts & 'ACGT' data.
					//if ( ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]>>(16-1))+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]>>(16-1))+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]>>(16-1))+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]&0xFFFF) ) & ( (1<<16)-1 )] ) < 3 ) Gulliver = cbPattern-(2-1)-2-4-2; else {
					// 'Maximus' uses branched 'if', again.
					if ( \
					( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6 +1]>>(16-1))+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6 +1]&0xFFFF) ) & ( (1<<16)-1 )] ) == 0 \
					|| ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4 +1]>>(16-1))+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4 +1]&0xFFFF) ) & ( (1<<16)-1 )] ) == 0 \
					) Gulliver = cbPattern-(2-1)-2-4-2 +1; else {
					// Above line is not optimized (several a SHR are used), we have 5 non-overlapping WORDs, or 3 overlapping WORDs, within 4 overlapping DWORDs so:

					//if ( ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-8]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-8]&0xFFFF) ) & ( (1<<16)-1 )] ) < 2 ) Gulliver = cbPattern-(2-1)-2-8; else {
						if ( *(uint32_t *)&pbTarget[i] == ulHashPattern) {
							// Order 4 [
						// Let's try something "outrageous" like comparing with[out] overlap BBs 4bytes long instead of 1 byte back-to-back:
						// Inhere we are using order 4, 'cbPattern - Order + 1' is the number of BBs for text 'cbPattern' bytes long, for example, for cbPattern=11 'fastest fox' and Order=4 we have BBs = 11-4+1=8:
						//0:"fast" if the comparison failed here, 'count' is 1; 'Gulliver' is cbPattern-(4-1)-7
						//1:"aste" if the comparison failed here, 'count' is 2; 'Gulliver' is cbPattern-(4-1)-6
						//2:"stes" if the comparison failed here, 'count' is 3; 'Gulliver' is cbPattern-(4-1)-5
						//3:"test" if the comparison failed here, 'count' is 4; 'Gulliver' is cbPattern-(4-1)-4
						//4:"est " if the comparison failed here, 'count' is 5; 'Gulliver' is cbPattern-(4-1)-3
						//5:"st f" if the comparison failed here, 'count' is 6; 'Gulliver' is cbPattern-(4-1)-2
						//6:"t fo" if the comparison failed here, 'count' is 7; 'Gulliver' is cbPattern-(4-1)-1
						//7:" fox" if the comparison failed here, 'count' is 8; 'Gulliver' is cbPattern-(4-1)
							count = cbPattern-4+1; 
							// Below comparison is UNIdirectional:
							while ( count > 0 && *(uint32_t *)(pbPattern+count-1) == *(uint32_t *)(&pbTarget[i]+(count-1)) )
								count = count-4;
// count = cbPattern-4+1 = 23-4+1 = 20
// boomshakalakaZZZZZZ[ZZZZ] 20
// boomshakalakaZZ[ZZZZ]ZZZZ 20-4
// boomshakala[kaZZ]ZZZZZZZZ 20-8 = 12
// boomsha[kala]kaZZZZZZZZZZ 20-12 = 8
// boo[msha]kalakaZZZZZZZZZZ 20-16 = 4

// If we miss to hit then no need to compare the original: Needle
if ( count <= 0 ) {
// I have to add out-of-range checks...
// i-(PRIMALposition-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4

// FIX from 2014-Apr-27:
// Because (count-1) is negative, above fours are reduced to next twos:
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
	// The line below is BUGGY:
	//if ( (i-(PRIMALposition-1) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) && (&pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4) ) {
	// The line below is OKAY:
	if ( ((signed int)(i-(PRIMALposition-1)+(count-1)) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) ) {

		if ( *(uint32_t *)&pbTarget[i-(PRIMALposition-1)] == *(uint32_t *)(pbPattern-(PRIMALposition-1))) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
			count = PRIMALlengthCANDIDATE-4+1; 
			while ( count > 0 && *(uint32_t *)(pbPattern-(PRIMALposition-1)+count-1) == *(uint32_t *)(&pbTarget[i-(PRIMALposition-1)]+(count-1)) )
				count = count-4;
			if ( count <= 0 ) return(pbTarget+i-(PRIMALposition-1));	
		}
	}
}

							// In order to avoid only-left or only-right WCS the memcmp should be done as left-to-right and right-to-left AT THE SAME TIME.
							// Below comparison is BIdirectional. It pays off when needle is 8+++ long:
//							for (count = cbPattern-4+1; count > 0; count = count-4) {
//								if ( *(uint32_t *)(pbPattern+count-1) != *(uint32_t *)(&pbTarget[i]+(count-1)) ) {break;};
//								if ( *(uint32_t *)(pbPattern+(cbPattern-4+1)-count) != *(uint32_t *)(&pbTarget[i]+(cbPattern-4+1)-count) ) {count = (cbPattern-4+1)-count +(1); break;} // +(1) because two lookups are implemented as one, also no danger of 'count' being 0 because of the fast check outwith the 'while': if ( *(uint32_t *)&pbTarget[i] == ulHashPattern)
//							}
//							if ( count <= 0 ) return(pbTarget+i);
								// Checking the order 2 pairs in mismatched DWORD, all the 3:
								//if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+count-1]] == 0 ) Gulliver = count; // 1 or bigger, as it should
								//if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+count-1+1]] == 0 ) Gulliver = count+1; // 1 or bigger, as it should
								//if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+count-1+1+1]] == 0 ) Gulliver = count+1+1; // 1 or bigger, as it should
							//	if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+count-1]] + bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+count-1+1]] + bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+count-1+1+1]] < 3 ) Gulliver = count; // 1 or bigger, as it should, THE MIN(count,count+1,count+1+1)
								// Above compound 'if' guarantees not that Gulliver > 1, an example:
								// Needle:    fastest tax
								// Window: ...fastast tax...
								// After matching ' tax' vs ' tax' and 'fast' vs 'fast' the mismathced DWORD is 'test' vs 'tast':
								// 'tast' when factorized down to order 2 yields: 'ta','as','st' - all the three when summed give 1+1+1=3 i.e. Gulliver remains 1.
								// Roughly speaking, this attempt maybe has its place in worst-case scenarios but not in English text and even not in ACGT data, that's why I commented it in original 'Shockeroo'.
								//if ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+count-1]>>16)+(*(uint32_t *)&pbTarget[i+count-1]&0xFFFF) ) & ( (1<<16)-1 )] == 0 ) Gulliver = count; // 1 or bigger, as it should
								// Above line is replaced by next one with better hashing:
//								if ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+count-1]>>(16-1))+(*(uint32_t *)&pbTarget[i+count-1]&0xFFFF) ) & ( (1<<16)-1 )] == 0 ) Gulliver = count; // 1 or bigger, as it should
							// Order 4 ]
						}
					}
				} else Gulliver = cbPattern-(2-1)-2; // -2 because we check the 4 rightmost bytes not 2.
				i = i + Gulliver;
				//GlobalI++; // Comment it, it is only for stats.
			}
			return(NULL);

		} // if ( cbPattern<=NeedleThreshold2vs4swampLITE )
		} // if ( cbPattern<=NeedleThreshold2vs4swampLITE )
	} //if ( cbPattern<4 )
}