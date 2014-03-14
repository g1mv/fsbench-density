///////////////////////////////////////////////////////////////////////////////////////////////
//
//  SWIFFTX ANSI C OPTIMIZED 64BIT IMPLEMENTATION FOR NIST SHA-3 COMPETITION
//
//  SHA3.h
//
//  October 2008
//
//	This file is the exact copy from the reference implementation.	
//
///////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __SHA_3__
#define __SHA_3__

// Remove these while using gcc:
#include "stdbool.h"
#include "stdint.h"

///////////////////////////////////////////////////////////////////////////////////////////////
// SWIFFTX-related constants portion
///////////////////////////////////////////////////////////////////////////////////////////////

// The size of SWIFFTX input in bytes.
#define SWIFFTX_INPUT_BLOCK_SIZE 256

// The size of output block in bytes. The compression function of SWIFFT outputs a block of 
// this size (i.e., this is the size of the resulting hash value).
#define SWIFFTX_OUTPUT_BLOCK_SIZE 65

///////////////////////////////////////////////////////////////////////////////////////////////
// HAIFA-related constants portion 1
///////////////////////////////////////////////////////////////////////////////////////////////

// The size in bytes of "salt" field.
#define HAIFA_SALT_SIZE 8

// The size in bytes of "#bits" field, as it is called in HAIFA paper.
#define HAIFA_NUM_OF_BITS_SIZE 8

// The size of the input block in bytes that we use in HAIFA (|M_i| in HAIFA paper).
#define HAIFA_INPUT_BLOCK_SIZE (SWIFFTX_INPUT_BLOCK_SIZE - SWIFFTX_OUTPUT_BLOCK_SIZE \
							  - HAIFA_NUM_OF_BITS_SIZE - HAIFA_SALT_SIZE)

///////////////////////////////////////////////////////////////////////////////////////////////
// NIST API definitions
///////////////////////////////////////////////////////////////////////////////////////////////

// The type of the input data as specified by NIST.
typedef unsigned char BitSequence;

// The data length type. Here we assume a typical modern machine with 64bit architecture.
typedef uint64_t DataLength;

// The success code values.
typedef enum 
{ 
	// Successfully computed the hash value.
	SUCCESS = 0, 

	// Failed to compute the hash value.
	FAIL = 1, 

	// Unsupported hash bit length.
	BAD_HASHBITLEN = 2,

	// The given salt is not of proper size.
	// The size shall be 'HAIFA_SALT_SIZE'.
	BAD_SALT_SIZE = 3,

	// Cannot change a salt value in the middle of a computation.
	SET_SALT_VALUE_FAILED = 4,

	// Will happen if 'Update()' is called after another 'Update()' with #bits not 0 mod 8.
	INPUT_DATA_NOT_ALIGNED = 5
} HashReturn;

// Here we specify the tables needed for SWIFFTX along with the intermediate values.
typedef struct hashState 
{
	// An integer value that indicates the length of the hash output in bits.
	// The supported values are:
	// - 224
	// - 256
	// - 384
	// - 512
	unsigned short hashbitlen;

	// The data remained after the recent call to 'Update()'. 
	BitSequence remaining[HAIFA_INPUT_BLOCK_SIZE + 1];

	// The size of the remaining data in bits.
	// Is 0 in case there is no remaning data at all.
	unsigned int remainingSize;

	// The current output of the compression function. At the end will contain the final digest
	// (which may be needed to be truncated, depending on hashbitlen).
	BitSequence currOutputBlock[SWIFFTX_OUTPUT_BLOCK_SIZE];

	// The value of '#bits hashed so far' field in HAIFA, in base 256.
	BitSequence numOfBitsChar[HAIFA_NUM_OF_BITS_SIZE];

	// The salt value currently in use:
	BitSequence salt[HAIFA_SALT_SIZE];

	// Indicates whether a single 'Update()' occured. 
	// Ater a call to 'Update()' the key and the salt values cannot be changed.
	bool wasUpdated;
} hashState;

// Initializes a hashState with the intended hash length of this particular instantiation.
// Additionally, any data independent setup is performed.
//
// Parameters:
// - state: a structure that holds the hashState information
// - hashbitlen: an integer value that indicates the length of the hash output in bits.
//
// Returns:
// - Success value.
HashReturn Init(hashState *state, int hashbitlen);

// Update() processes data using the algorithm’s compression function.
// Whatever integral amount of data the Update() routine can process through the compression
// function is handled. Any remaining data must be stored for future processing. 
//
// Parameters:
// - state: a structure that holds the hashState information
// - data: the input data to be hashed
// - databitlen: the length, in bits, of the input data to be hashed
//
// Returns:
// - Success value.
HashReturn Update(hashState *state, const BitSequence *data, DataLength databitlen);

// Final() processes any remaining partial block of the input data and performs any output
// filtering that may be needed to produce the final hash value.
// This function is called with pointers to the appropriate hashState structure and the storage
// for the final hash value to be returned (hashval). It performs any post processing that is 
// necessary, including the handling of any partial blocks, and places the final hash value in
// hashval. Lastly, an appropriate status value is returned.
//
// Parameters:
// - state: a structure that holds the hashState information
// - hashval: the storage for the final (output) hash value to be returned
//
// Returns:
// - Success value.
HashReturn Final(hashState *state, BitSequence *hashval);

// Hash() provides a method to perform all-at-once processing of the input data using SWIFFTX 
// and returns the resulting hash value. The Hash() function is called with a pointer to the 
// data to be processed, the length of the data to be processed (databitlen), a pointer to the 
// storage for the resulting hash value (hashval), and a length of the desired hash value
// (hashbitlen). This function utilizes the previous three function calls, namely Init(),
// Update(), and Final().
//
// Parameters:
// - hashbitlen: the length in bits of the desired hash value
// - data: the input data to be hashed
// - databitlen: the length, in bits, of the data to be hashed
// - hashval: the resulting hash value of the provided data
//
// Returns:
// - Success value.
HashReturn Hash(int hashbitlen, const BitSequence *data, DataLength databitlen, 
				BitSequence *hashval);

///////////////////////////////////////////////////////////////////////////////////////////////
// Additional API definition for salt that currently is not part of NIST's API.
///////////////////////////////////////////////////////////////////////////////////////////////

// Sets the salt.
//
// Parameters:
// - state: a structure that holds the hashState information.
// - salt: the salt to set.
// - saltLength: the length of the salt to set, in bytes.
//
// Returns:
// - Success value.
HashReturn SetSalt(hashState *state, BitSequence *salt, unsigned short saltLength);

///////////////////////////////////////////////////////////////////////////////////////////////
// HAIFA-related constants portion 2
///////////////////////////////////////////////////////////////////////////////////////////////

// The default salt value.
extern const DataLength SALT_VALUE;

// This is the initial value we choose. For each digest size m, IV_m is derived from this IV
// through SWIFFTX compression function, as specified in HAIFA paper.
#define HAIFA_IV 0

// The initial value for 224 digest size.
extern const BitSequence HAIFA_IV_224[SWIFFTX_OUTPUT_BLOCK_SIZE];

// The initial value for 256 digest size.
extern const BitSequence HAIFA_IV_256[SWIFFTX_OUTPUT_BLOCK_SIZE];

// The initial value for 384 digest size.
extern const BitSequence HAIFA_IV_384[SWIFFTX_OUTPUT_BLOCK_SIZE];

// The initial value for 512 digest size.
extern const BitSequence HAIFA_IV_512[SWIFFTX_OUTPUT_BLOCK_SIZE];

#endif // __SHA_3__