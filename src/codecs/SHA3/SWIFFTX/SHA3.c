///////////////////////////////////////////////////////////////////////////////////////////////
//
//  SWIFFTX ANSI C OPTIMIZED 64BIT IMPLEMENTATION FOR NIST SHA-3 COMPETITION
//
//  SHA3.c
//
//  October 2008
//
//	This file is the exact copy from the reference implementation.	
//
///////////////////////////////////////////////////////////////////////////////////////////////
#include "SHA3.h"
#include "SWIFFTX.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// The default salt value.
// This is the expansion of e (Euler's number) - the 19 digits after 2.71:
// 8281828459045235360.
// The above in base 256, from MSB to LSB:
BitSequence saltValueChar[HAIFA_SALT_SIZE] = {114, 238, 247, 26, 192, 28, 170, 160};

// All the IVs here below were produced from the decimal digits of e's expansion.
// The code can be found in 'ProduceRandomIV.c'.
// The initial value for 224 digest size.
const BitSequence HAIFA_IV_224[SWIFFTX_OUTPUT_BLOCK_SIZE] = 
{37, 242, 132,   2, 167,  81, 158, 237, 113,  77, 162,  60,  65, 236, 108, 246,
101,  72, 190, 109,  58, 205,  99,   6, 114, 169, 104, 114,  38, 146, 121, 142,
 59,  98, 233,  84,  72, 227,  22, 199,  17, 102, 198, 145,  24, 178,  37,   1,
215, 245,  66, 120, 230, 193, 113, 253, 165, 218,  66, 134,  49, 231, 124, 204,
  0};

// The initial value for 256 digest size.
const BitSequence HAIFA_IV_256[SWIFFTX_OUTPUT_BLOCK_SIZE] = 
{250,  50,  42,  40,  14, 233,  53,  48, 227,  42, 237, 187, 211, 120, 209, 234,
  27, 144,   4,  61, 243, 244,  29, 247,  37, 162,  70,  11, 231, 196,  53,   6,
 193, 240,  94, 126, 204, 132, 104,  46, 114,  29,   3, 104, 118, 184, 201,   3,
  57,  77,  91, 101,  31, 155,  84, 199, 228,  39, 198,  42, 248, 198, 201, 178,
   8};

// The initial value for 384 digest size.
const BitSequence HAIFA_IV_384[SWIFFTX_OUTPUT_BLOCK_SIZE] = 
{40, 145, 193, 100, 205, 171,  47,  76, 254,  10, 196,  41, 165, 207, 200,  79,
109,  13,  75, 201,  17, 172,  64, 162, 217,  22,  88,  39,  51,  30, 220, 151,
133,  73, 216, 233, 184, 203,  77,   0, 248,  13,  28, 199,  30, 147, 232, 242,
227, 124, 169, 174,  14,  45,  27,  87, 254,  73,  68, 136, 135, 159,  83, 152,
  0};

// The initial value for 512 digest size.
const BitSequence HAIFA_IV_512[SWIFFTX_OUTPUT_BLOCK_SIZE] = 
{195, 126, 197, 167, 157, 114,  99, 126, 208, 105, 200,  90,  71, 195, 144, 138,
 142, 122, 123, 116,  24, 214, 168, 173, 203, 183, 194, 210, 102, 117, 138,  42,
 114, 118, 132,  33,  35, 149, 143, 163, 163, 183, 243, 175,  72,  22, 201, 255,
 102, 243,  22, 187, 211, 167, 239,  76, 164,  70,  80, 182, 181, 212,   9, 185,
   0};

// A helper function.
// Adds the required number to #bits in base 256, in place.
//
// Parameters:
// - value: the current value of #bits.
// - toAdd: the value to add. 
static void AddToCurrInBase256(BitSequence value[HAIFA_NUM_OF_BITS_SIZE], unsigned short toAdd);

///////////////////////////////////////////////////////////////////////////////////////////////
// NIST API implementation portion.
///////////////////////////////////////////////////////////////////////////////////////////////

HashReturn Init(hashState *state, int hashbitlen)
{
	switch(hashbitlen)
	{
	case 224:
		state->hashbitlen = hashbitlen;
		// Initializes h_0 in HAIFA:
		memcpy(state->currOutputBlock, HAIFA_IV_224, SWIFFTX_OUTPUT_BLOCK_SIZE);
		break;
	case 256:
		state->hashbitlen = hashbitlen;
		memcpy(state->currOutputBlock, HAIFA_IV_256, SWIFFTX_OUTPUT_BLOCK_SIZE);
		break;
	case 384:
		state->hashbitlen = hashbitlen;
		memcpy(state->currOutputBlock, HAIFA_IV_384, SWIFFTX_OUTPUT_BLOCK_SIZE);
		break;
	case 512:
		state->hashbitlen = hashbitlen;
		memcpy(state->currOutputBlock, HAIFA_IV_512, SWIFFTX_OUTPUT_BLOCK_SIZE);
		break;
	default:
		return BAD_HASHBITLEN;
	}
	
	state->wasUpdated = false;
	state->remainingSize = 0;
	memset(state->remaining, 0, HAIFA_INPUT_BLOCK_SIZE);
	memset(state->numOfBitsChar, 0, HAIFA_NUM_OF_BITS_SIZE);
	// Initialize the salt with the default value.
	memcpy(state->salt, saltValueChar, HAIFA_SALT_SIZE);

	InitializeSWIFFTX();

	return SUCCESS;
}

HashReturn Update(hashState *state, const BitSequence *data, DataLength databitlen)
{
	// The size of input in bytes after putting the remaining data from previous invocation.
	int sizeOfInputAfterRemaining = 0;
	// The input block to compression function of SWIFFTX:
	BitSequence currInputBlock[SWIFFTX_INPUT_BLOCK_SIZE] = {0};
	// Whether we handled a single block.
	bool wasSingleBlockHandled = false;

	state->wasUpdated = true;

	// Handle an empty message as required by NIST. Since 'Final()' is oblivious to the input
	// (but of course uses the output of the compression function from the previous round, 
	// which is called h_{i-1} in HAIFA article), we have to do nothing here.
	if (databitlen == 0)
		return SUCCESS;

    // If we had before an input with unaligned length, return an error
    if (state->remainingSize % 8)
	{
    	return INPUT_DATA_NOT_ALIGNED;
    }

    // Convert remaining size to bytes.
    state->remainingSize /= 8;

	// As long as we have enough data combined from (remaining + data) to fill input block
	while (((databitlen / 8) + state->remainingSize) >= HAIFA_INPUT_BLOCK_SIZE)
	{
		// Fill the input block with data:
		// 1. The output of the previous block:
		memcpy(currInputBlock, state->currOutputBlock, SWIFFTX_OUTPUT_BLOCK_SIZE);
		// 2. The input part of the block:
		// 2a. The remaining data from the previous 'Update()' call:
		if (state->remainingSize)
			memcpy(currInputBlock + SWIFFTX_OUTPUT_BLOCK_SIZE, state->remaining, 
				   state->remainingSize);
		// 2b. The input data that we have place for after the 'remaining':
		sizeOfInputAfterRemaining = SWIFFTX_INPUT_BLOCK_SIZE - SWIFFTX_OUTPUT_BLOCK_SIZE 
								  - ((int) state->remainingSize) - HAIFA_NUM_OF_BITS_SIZE 
								  - HAIFA_SALT_SIZE;
		memcpy(currInputBlock + SWIFFTX_OUTPUT_BLOCK_SIZE + state->remainingSize, 
			   data, sizeOfInputAfterRemaining);

		// 3. The #bits part of the block:
		memcpy(currInputBlock + SWIFFTX_OUTPUT_BLOCK_SIZE + state->remainingSize 
			 + sizeOfInputAfterRemaining,
			   state->numOfBitsChar, HAIFA_NUM_OF_BITS_SIZE);
		// 4. The salt part of the block:
		memcpy(currInputBlock + SWIFFTX_OUTPUT_BLOCK_SIZE + state->remainingSize 
			 + sizeOfInputAfterRemaining + HAIFA_NUM_OF_BITS_SIZE,
			   state->salt, HAIFA_SALT_SIZE);

		ComputeSingleSWIFFTX(currInputBlock, state->currOutputBlock, false);

		// Update the #bits field with HAIFA_INPUT_BLOCK_SIZE.
		AddToCurrInBase256(state->numOfBitsChar, HAIFA_INPUT_BLOCK_SIZE * 8);
		wasSingleBlockHandled = true;
		data += sizeOfInputAfterRemaining;
		databitlen -= (sizeOfInputAfterRemaining * 8);
   		state->remainingSize = 0;
	}

	// Update the state->remaining and state->remainingSize.
    // remainingSize will be in bits after exiting 'Update()'.
	if (wasSingleBlockHandled)
	{		
		state->remainingSize = (unsigned int) databitlen; // now remaining size is in bits.
        if (state->remainingSize)
			memcpy(state->remaining, data, (state->remainingSize + 7) / 8);
	}
	else
	{
		memcpy(state->remaining + state->remainingSize, data, 
			   (size_t) (databitlen + 7) / 8);
		state->remainingSize = (state->remainingSize * 8) + (unsigned short) databitlen;
	}

	return SUCCESS;
}

HashReturn Final(hashState *state, BitSequence *hashval)
{
    int i;
    // Whether to add one last block. True if the padding appended to the last block overflows
	// the block size.
    bool toAddFinalBlock = false;
    bool toPutOneInFinalBlock = false;
    unsigned short oneShift = 0;
   	// The size of the last input block before the zeroes padding. We add 1 here because we
    // include the final '1' bit in the calculation and 7 as we round the length to bytes.
	unsigned short sizeOfLastInputBlock = (state->remainingSize + 1 + 7) / 8;
    // The number of bytes of zero in the padding part.
	// The padding contains:
	// 1. A single 1 bit.
	// 2. As many zeroes as needed.
	// 3. The message length in bits. Occupies HAIFA_NUM_OF_BITS_SIZE bytes.
	// 4. The digest size. Maximum is 512, so we need 2 bytes.
	// If the total number achieved is negative, add an additional block, as HAIFA specifies.
	short numOfZeroBytesInPadding = (short) SWIFFTX_INPUT_BLOCK_SIZE - SWIFFTX_OUTPUT_BLOCK_SIZE 
								  - sizeOfLastInputBlock - (2 * HAIFA_NUM_OF_BITS_SIZE) - 2 
								  - HAIFA_SALT_SIZE;
   	// The input block to compression function of SWIFFTX:
	BitSequence currInputBlock[SWIFFTX_INPUT_BLOCK_SIZE] = {0};
	// The message length in base 256.
	BitSequence messageLengthChar[HAIFA_NUM_OF_BITS_SIZE] = {0};
   	// The digest size used for padding:
	unsigned char digestSizeLSB = state->hashbitlen % 256;
	unsigned char digestSizeMSB = (state->hashbitlen - digestSizeLSB) / 256;

	if (numOfZeroBytesInPadding < 1)
		toAddFinalBlock = true;

	// Fill the input block with data:
	// 1. The output of the previous block:
	memcpy(currInputBlock, state->currOutputBlock, SWIFFTX_OUTPUT_BLOCK_SIZE);
	// 2a. The input part of the block, which is the remaining data from the previous 'Update()'
    //     call, if exists and an extra '1' bit (maybe all we have is this extra 1):

    // Add the last 1 in big-endian convention ...
    if (state->remainingSize % 8 == 0)
	{
       state->remaining[sizeOfLastInputBlock - 1] = 0x80;
    }
    else 
	{
       state->remaining[sizeOfLastInputBlock - 1] |= (1 << (7 - (state->remainingSize % 8)));
    }

	if (sizeOfLastInputBlock)
		memcpy(currInputBlock + SWIFFTX_OUTPUT_BLOCK_SIZE, state->remaining, 
			   sizeOfLastInputBlock);
    
   	// Compute the message length in base 256:
	for (i = 0; i < HAIFA_NUM_OF_BITS_SIZE; ++i)
        messageLengthChar[i] = state->numOfBitsChar[i];
    if (sizeOfLastInputBlock)
		AddToCurrInBase256(messageLengthChar, sizeOfLastInputBlock * 8);

	if (!toAddFinalBlock)
	{
		// 2b. Put the zeroes:
		memset(currInputBlock + SWIFFTX_OUTPUT_BLOCK_SIZE + sizeOfLastInputBlock,
			   0, numOfZeroBytesInPadding);
		// 2c. Pad the message length:
		for (i = 0; i < HAIFA_NUM_OF_BITS_SIZE; ++i)
			currInputBlock[SWIFFTX_OUTPUT_BLOCK_SIZE + sizeOfLastInputBlock 
						 + numOfZeroBytesInPadding + i] = messageLengthChar[i];
		// 2d. Pad the digest size:
		currInputBlock[SWIFFTX_OUTPUT_BLOCK_SIZE + sizeOfLastInputBlock 
					 + numOfZeroBytesInPadding + HAIFA_NUM_OF_BITS_SIZE] = digestSizeMSB;
		currInputBlock[SWIFFTX_OUTPUT_BLOCK_SIZE + sizeOfLastInputBlock 
					 + numOfZeroBytesInPadding + HAIFA_NUM_OF_BITS_SIZE + 1] = digestSizeLSB;
	}
	else
	{
		// 2b. Put the zeroes, if at all:
		if ((HAIFA_INPUT_BLOCK_SIZE - sizeOfLastInputBlock) > 0)
		{
			 memset(currInputBlock + SWIFFTX_OUTPUT_BLOCK_SIZE + sizeOfLastInputBlock,
					0, HAIFA_INPUT_BLOCK_SIZE - sizeOfLastInputBlock);
		}
	}

   	// 3. The #bits part of the block: 
	memcpy(currInputBlock + SWIFFTX_OUTPUT_BLOCK_SIZE + HAIFA_INPUT_BLOCK_SIZE, 
           state->numOfBitsChar, HAIFA_NUM_OF_BITS_SIZE);
	// 4. The salt part of the block:
	memcpy(currInputBlock + SWIFFTX_OUTPUT_BLOCK_SIZE + HAIFA_INPUT_BLOCK_SIZE 
		 + HAIFA_NUM_OF_BITS_SIZE, 
           state->salt, 
		   HAIFA_SALT_SIZE);

    ComputeSingleSWIFFTX(currInputBlock, state->currOutputBlock, !toAddFinalBlock); 

	// If we have to add one more block, it is now:
	if (toAddFinalBlock)
	{
		// 1. The previous output block, as usual.
		memcpy(currInputBlock, state->currOutputBlock, SWIFFTX_OUTPUT_BLOCK_SIZE);

		// 2a. Instead of the input, zeroes:
		memset(currInputBlock + SWIFFTX_OUTPUT_BLOCK_SIZE , 0, 
			   HAIFA_INPUT_BLOCK_SIZE - HAIFA_NUM_OF_BITS_SIZE - 2);
		// 2b. Instead of the input, the message length:
		memcpy(currInputBlock + SWIFFTX_OUTPUT_BLOCK_SIZE + HAIFA_INPUT_BLOCK_SIZE 
			 - HAIFA_NUM_OF_BITS_SIZE - 2,
			   messageLengthChar,
			   HAIFA_NUM_OF_BITS_SIZE);
		// 2c. Instead of the input, the digest size:
		currInputBlock[SWIFFTX_OUTPUT_BLOCK_SIZE + HAIFA_INPUT_BLOCK_SIZE - 2] = digestSizeMSB;
		currInputBlock[SWIFFTX_OUTPUT_BLOCK_SIZE + HAIFA_INPUT_BLOCK_SIZE - 1] = digestSizeLSB;
		// 3. The #bits part of the block, which is zero in case of additional block:
		memset(currInputBlock + SWIFFTX_OUTPUT_BLOCK_SIZE + HAIFA_INPUT_BLOCK_SIZE,
			   0,
			   HAIFA_NUM_OF_BITS_SIZE);
		// 4. The salt part of the block:
		memcpy(currInputBlock + SWIFFTX_OUTPUT_BLOCK_SIZE + HAIFA_INPUT_BLOCK_SIZE 
			 + HAIFA_NUM_OF_BITS_SIZE, 
               state->salt, 
			   HAIFA_SALT_SIZE);

        ComputeSingleSWIFFTX(currInputBlock, state->currOutputBlock, true); 
	}

	// Finally, copy the result into 'hashval'. In case the digest size is not 512bit, copy the
	// first hashbitlen of them:
    for (i = 0; i < (state->hashbitlen / 8); ++i)
		hashval[i] = state->currOutputBlock[i];

	return SUCCESS;
}

HashReturn Hash(int hashbitlen, const BitSequence *data, DataLength databitlen, 
				BitSequence *hashval)
{
	HashReturn result;
	hashState state;
   	// The pointer to the current place in the input we take into the compression function.
	DataLength currInputIndex = 0;

    result = Init(&state, hashbitlen);

	if (result != SUCCESS)
		return result;

	for ( ; (databitlen / 8) >  HAIFA_INPUT_BLOCK_SIZE; 
         currInputIndex += HAIFA_INPUT_BLOCK_SIZE, databitlen -= (HAIFA_INPUT_BLOCK_SIZE * 8))
	{
		result = Update(&state, data + currInputIndex, HAIFA_INPUT_BLOCK_SIZE * 8); 
		if (result != SUCCESS)
			return result;
	}

	// The length of the last block may be shorter than (HAIFA_INPUT_BLOCK_SIZE * 8)
	result = Update(&state, data + currInputIndex, databitlen); 
	if (result != SUCCESS)
	{
		return result;
	}

    return Final(&state, hashval);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Additional API implementations for salt and for keyed version that currently are not part of 
// NIST's API.
///////////////////////////////////////////////////////////////////////////////////////////////

HashReturn SetSalt(hashState *state, BitSequence *salt, unsigned short saltLength)
{
	if (saltLength != HAIFA_SALT_SIZE)
		return BAD_SALT_SIZE;

	if (state->wasUpdated)
		return SET_SALT_VALUE_FAILED;

	memcpy(saltValueChar, salt, HAIFA_SALT_SIZE);

	return SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Helper fuction implementation portion.
///////////////////////////////////////////////////////////////////////////////////////////////

static void AddToCurrInBase256(BitSequence value[HAIFA_NUM_OF_BITS_SIZE], 
							   unsigned short toAdd)
{
	unsigned char remainder = 0;
	short i;
	BitSequence currValueInBase256[8] = {0};
	unsigned short currIndex = 7;
	unsigned short temp = 0;

	do
	{
		remainder = toAdd % 256;
		currValueInBase256[currIndex--] = remainder;
		toAdd -= remainder;
		toAdd /= 256;
	}
	while(toAdd != 0);

	for (i = 7; i >= 0; --i)
	{
		temp = value[i] + currValueInBase256[i];
		if (temp > 255)
		{
			value[i] = temp % 256;
			currValueInBase256[i - 1]++;
		}
		else
			value[i] = (unsigned char) temp;
	}
}