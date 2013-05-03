///////////////////////////////////////////////////////////////////////////////////////////////
//
//  SWIFFTX ANSI C OPTIMIZED 64BIT IMPLEMENTATION FOR NIST SHA-3 COMPETITION
//
//  SWIFFTX.h
//
//  October 2008
//
//	This file is the exact copy from the reference implementation.	
//
///////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __SWIFFTX__
#define __SWIFFTX__

// See the remarks concerning compatibility issues inside stdint.h.
#include "stdint.h"
#include "SHA3.h"

// Computes the result of a single SWIFFT operation.
// This is the simple implementation, where our main concern is to show our design principles.
// It is made more efficient in the optimized version, by using FFT instead of DFT, and 
// through other speed-up techniques.
//
// Parameters:
// - input: the input string. Consists of 8*m input bytes, where each octet passes the DFT 
//   processing.
// - m: the length of the input in bytes.
// - output: the resulting hash value of SWIFFT, of size 65 bytes (520 bit). This is the 
//	 result of summing the dot products of the DFTS with the A's after applying the base
//	 change transformation
// - A: the A's coefficients to work with (since every SWIFFT in SWIFFTX uses different As).
//   A single application of SWIFFT uses 64*m A's.
void ComputeSingleSWIFFT(BitSequence *input, unsigned short m, 
					  	 BitSequence output[SWIFFTX_OUTPUT_BLOCK_SIZE],
						 const int16_t *a);

// Computes the result of a single SWIFFTX operation.
// NOTE: for simplicity we use 'ComputeSingleSWIFFT()' as a subroutine. This is only to show
// the design idea. In the optimized versions we don't do this for efficiency concerns, since
// there we compute the first part (which doesn't involve the A coefficients) only once for all
// of the 3 invocations of SWIFFT. This enables us to introduce a significant speedup.
//
// Parameters:
// - input: the input input of 256 bytes (2048 bit).
// - output: the resulting hash value of SWIFFT, of size 64 bytes (512 bit).
// - doSMooth: if true, a final smoothing stage is performed and the output is of size 512 bits.
//
// Returns:
// - Success value.
void ComputeSingleSWIFFTX(BitSequence input[SWIFFTX_INPUT_BLOCK_SIZE], 
			  	          BitSequence output[SWIFFTX_OUTPUT_BLOCK_SIZE],
						  bool doSmooth);

// Calculates the powers of OMEGA and generates the bit reversal permutation.
// You must call this function before doing SWIFFT/X, otherwise you will get zeroes everywhere.
void InitializeSWIFFTX();

#endif // __SWIFFTX__