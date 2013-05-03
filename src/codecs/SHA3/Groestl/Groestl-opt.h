#ifndef __groestl_opt_h
#define __groestl_opt_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "brg_endian.h"
#define NEED_UINT_64T
//#include "brg_types.h"

#define u8 unsigned char
#define u32 unsigned int
#define u64 unsigned long long

/* some sizes (number of bytes) */
#define ROWS 8
#define LENGTHFIELDLEN ROWS
#define COLS512 8
#define COLS1024 16
#define SIZE512 (ROWS*COLS512)
#define SIZE1024 (ROWS*COLS1024)

#define ROUNDS512 10
#define ROUNDS1024 14

#define ROTL64(a,n) ((((a)<<(n))|((a)>>(64-(n))))&(0xffffffffffffffffULL))

#if (PLATFORM_BYTE_ORDER == IS_BIG_ENDIAN)
#define EXT_BYTE(var,n) ((u8)((u64)(var) >> (8*(7-(n)))))
#define U64BIG(a) (a)
#endif /* IS_BIG_ENDIAN */

#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
#define EXT_BYTE(var,n) ((u8)((u64)(var) >> (8*n)))
#define U64BIG(a) \
  ((ROTL64(a, 8) & (0x000000FF000000FFULL)) | \
   (ROTL64(a,24) & (0x0000FF000000FF00ULL)) | \
   (ROTL64(a,40) & (0x00FF000000FF0000ULL)) | \
   (ROTL64(a,56) & (0xFF000000FF000000ULL)))
#endif /* IS_LITTLE_ENDIAN */

enum { LONG = SIZE1024, SHORT = SIZE512 };


/* NIST API begin */
typedef unsigned char BitSequence;
typedef unsigned long long DataLength;
typedef enum { SUCCESS = 0, FAIL = 1, BAD_HASHLEN = 2 } HashReturn;
typedef struct {
  u64 *chaining __attribute__((aligned(16)));            /* actual state */
  BitSequence *buffer;      /* data buffer */
  u64 block_counter;        /* message block counter */
  int buf_ptr;              /* data buffer pointer */
  int bits_in_last_byte;    /* no. of message bits in last byte of
			       data buffer */
  int hashbitlen;           /* output length in bits */
  int size;                 /* LONG or SHORT */
} hashState;

HashReturn Init(hashState*, int);
HashReturn Update(hashState*, const BitSequence*, DataLength);
HashReturn Final(hashState*, BitSequence*);
HashReturn Hash(int, const BitSequence*, DataLength, BitSequence*);
/* NIST API end   */

/* helper functions */
void PrintHash(const BitSequence*, int);

#endif /* __groestl_opt_h */
