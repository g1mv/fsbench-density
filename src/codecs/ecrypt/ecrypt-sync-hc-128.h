/* ecrypt-sync.h */

/* 
 * Header file for synchronous stream ciphers without authentication
 * mechanism.
 * 
 * *** Please only edit parts marked with "[edit]". ***
 */
#ifndef HC128_SYNC
#define HC128_SYNC

#include "ecrypt-portable.h"

/* ------------------------------------------------------------------------- */

/* Cipher parameters */

/* 
 * The name of your cipher.
 */
#define HC128_NAME "HC-128"    /* [edit] */ 
#define HC128_PROFILE "SW"

/*
 * Specify which key and IV sizes are supported by your cipher. A user
 * should be able to enumerate the supported sizes by running the
 * following code:
 *
 * for (i = 0; HC128_KEYSIZE(i) <= HC128_MAXKEYSIZE; ++i)
 *   {
 *     keysize = HC128_KEYSIZE(i);
 *
 *     ...
 *   }
 *
 * All sizes are in bits.
 */

/*
*Remarks:  One key size is supported: 128 bits 
*          One IV size is supported: 128 bits 
*          
*          The other key, IV sizes can also be used in HC-128, 
*          but not recommended:   
*          1) For any key with size not equal to 128, 
*          the key needs to be concatenated to a 128-bit key
*          before being used in HC-128. 
*          2) For any IV with size not equal to 128,
*          the IV needs to be concatenated to a 128-bit IV
*          before being used in HC-128
*
*Caution:  Two keys with different sizes should be independently generated 
*          Two IVs with different sizes should not be used with the same key
*  
*Recommended: 128-bit IV for 128-bit key;  
*/

#define HC128_MAXKEYSIZE 128                  /* [edit] */
#define HC128_KEYSIZE(i) (128 + (i)*128)      /* [edit] */

#define HC128_MAXIVSIZE 128                   /* [edit] */
#define HC128_IVSIZE(i) (128 + (i)*128)       /* [edit] */



/* ------------------------------------------------------------------------- */

/* Data structures */

/* 
 * HC128_ctx is the structure containing the representation of the
 * internal state of your cipher. 
 */

typedef struct
{
  /* 
   * [edit]
   *
   * Put here all state variable needed during the encryption process.
  */
  u32 T[1024];       /* P[i] = T[i]; Q[i] = T[1024+i];*/
  u32 X[16];
  u32 Y[16];
  u32 counter1024;   /*counter1024 = i mod 1024 at the i-th step */ 
  u32 key[8];
  u32 iv[8];
  u32 keysize;       /* key size in bits */
  u32 ivsize;        /* iv size in bits*/ 
} HC128_ctx;

/*-------------------------------------
Added functions
---------------------------------------*/

void generate_keystream(HC128_ctx* ctx, u32* keystream);

void setup_update(HC128_ctx* ctx);

/* ------------------------------------------------------------------------- */

/* Mandatory functions */

/*
 * Key and message independent initialization. This function will be
 * called once when the program starts (e.g., to build expanded S-box
 * tables).
 */
void HC128_init(void);

/*
 * Key setup. It is the user's responsibility to select the values of
 * keysize and ivsize from the set of supported values specified
 * above.
 */
void HC128_keysetup(
  HC128_ctx* ctx, 
  const u8* key, 
  u32 keysize,                /* Key size in bits. */ 
  u32 ivsize);                /* IV size in bits. */ 

/*
 * IV setup. After having called HC128_keysetup(), the user is
 * allowed to call HC128_ivsetup() different times in order to
 * encrypt/decrypt different messages with the same key but different
 * IV's.
 */
void HC128_ivsetup(
  HC128_ctx* ctx, 
  const u8* iv);

/*
 * Encryption/decryption of arbitrary length messages.
 *
 * For efficiency reasons, the API provides two types of
 * encrypt/decrypt functions. The HC128_encrypt_bytes() function
 * (declared here) encrypts byte strings of arbitrary length, while
 * the HC128_encrypt_blocks() function (defined later) only accepts
 * lengths which are multiples of HC128_BLOCKLENGTH.
 * 
 * The user is allowed to make multiple calls to
 * HC128_encrypt_blocks() to incrementally encrypt a long message,
 * but he is NOT allowed to make additional encryption calls once he
 * has called HC128_encrypt_bytes() (unless he starts a new message
 * of course). For example, this sequence of calls is acceptable:
 *
 * HC128_keysetup();
 *
 * HC128_ivsetup();
 * HC128_encrypt_blocks();
 * HC128_encrypt_blocks();
 * HC128_encrypt_bytes();
 *
 * HC128_ivsetup();
 * HC128_encrypt_blocks();
 * HC128_encrypt_blocks();
 *
 * HC128_ivsetup();
 * HC128_encrypt_bytes();
 * 
 * The following sequence is not:
 *
 * HC128_keysetup();
 * HC128_ivsetup();
 * HC128_encrypt_blocks();
 * HC128_encrypt_bytes();
 * HC128_encrypt_blocks();
 */

/*
 * By default HC128_encrypt_bytes() and HC128_decrypt_bytes() are
 * defined as macros which redirect the call to a single function
 * HC128_process_bytes(). If you want to provide separate encryption
 * and decryption functions, please undef
 * HC128_HAS_SINGLE_BYTE_FUNCTION.
 */
#define HC128_HAS_SINGLE_BYTE_FUNCTION       /* [edit] */
#ifdef HC128_HAS_SINGLE_BYTE_FUNCTION

#define HC128_encrypt_bytes(ctx, plaintext, ciphertext, msglen)   \
  HC128_process_bytes(0, ctx, plaintext, ciphertext, msglen)

#define HC128_decrypt_bytes(ctx, ciphertext, plaintext, msglen)   \
  HC128_process_bytes(1, ctx, ciphertext, plaintext, msglen)

void HC128_process_bytes(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  HC128_ctx* ctx, 
  const u8* input, 
  u8* output, 
  u32 msglen);                /* Message length in bytes. */ 

#else

void HC128_encrypt_bytes(
  HC128_ctx* ctx, 
  const u8* plaintext, 
  u8* ciphertext, 
  u32 msglen);                /* Message length in bytes. */ 

void HC128_decrypt_bytes(
  HC128_ctx* ctx, 
  const u8* ciphertext, 
  u8* plaintext, 
  u32 msglen);                /* Message length in bytes. */ 

#endif

/* ------------------------------------------------------------------------- */

/* Optional features */

/* 
 * For testing purposes it can sometimes be useful to have a function
 * which immediately generates keystream without having to provide it
 * with a zero plaintext. If your cipher cannot provide this function
 * (e.g., because it is not strictly a synchronous cipher), please
 * reset the HC128_GENERATES_KEYSTREAM flag.
 */

#define HC128_GENERATES_KEYSTREAM
#ifdef HC128_GENERATES_KEYSTREAM

void HC128_keystream_bytes(
  HC128_ctx* ctx,
  u8* keystream,
  u32 length);                /* Length of keystream in bytes. */

#endif

/* ------------------------------------------------------------------------- */

/* Optional optimizations */

/* 
 * By default, the functions in this section are implemented using
 * calls to functions declared above. However, you might want to
 * implement them differently for performance reasons.
 */

/*
 * All-in-one encryption/decryption of (short) packets.
 *
 * The default definitions of these functions can be found in
 * "ecrypt-sync.c". If you want to implement them differently, please
 * undef the HC128_USES_DEFAULT_ALL_IN_ONE flag.
 */
#define HC128_USES_DEFAULT_ALL_IN_ONE        /* [edit] */

/*
 * Undef HC128_HAS_SINGLE_PACKET_FUNCTION if you want to provide
 * separate packet encryption and decryption functions.
 */
#define HC128_HAS_SINGLE_PACKET_FUNCTION     /* [edit] */
#ifdef HC128_HAS_SINGLE_PACKET_FUNCTION

#define HC128_encrypt_packet(                                        \
    ctx, iv, plaintext, ciphertext, mglen)                            \
  HC128_process_packet(0,                                            \
    ctx, iv, plaintext, ciphertext, mglen)

#define HC128_decrypt_packet(                                        \
    ctx, iv, ciphertext, plaintext, mglen)                            \
  HC128_process_packet(1,                                            \
    ctx, iv, ciphertext, plaintext, mglen)

void HC128_process_packet(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  HC128_ctx* ctx, 
  const u8* iv,
  const u8* input, 
  u8* output, 
  u32 msglen);

#else

void HC128_encrypt_packet(
  HC128_ctx* ctx, 
  const u8* iv,
  const u8* plaintext, 
  u8* ciphertext, 
  u32 msglen);

void HC128_decrypt_packet(
  HC128_ctx* ctx, 
  const u8* iv,
  const u8* ciphertext, 
  u8* plaintext, 
  u32 msglen);

#endif

/*
 * Encryption/decryption of blocks.
 * 
 * By default, these functions are defined as macros. If you want to
 * provide a different implementation, please undef the
 * HC128_USES_DEFAULT_BLOCK_MACROS flag and implement the functions
 * declared below.
 */

#define HC128_BLOCKLENGTH 64                 /* [edit] */

#define HC128_USES_DEFAULT_BLOCK_MACROS      /* [edit] */
#ifdef HC128_USES_DEFAULT_BLOCK_MACROS

#define HC128_encrypt_blocks(ctx, plaintext, ciphertext, blocks)  \
  HC128_encrypt_bytes(ctx, plaintext, ciphertext,                 \
    (blocks) * HC128_BLOCKLENGTH)

#define HC128_decrypt_blocks(ctx, ciphertext, plaintext, blocks)  \
  HC128_decrypt_bytes(ctx, ciphertext, plaintext,                 \
    (blocks) * HC128_BLOCKLENGTH)

#ifdef HC128_GENERATES_KEYSTREAM

#define HC128_keystream_blocks(ctx, keystream, blocks)            \
  HC128_keystream_bytes(ctx, keystream,                           \
    (blocks) * HC128_BLOCKLENGTH)

#endif

#else

/*
 * Undef HC128_HAS_SINGLE_BLOCK_FUNCTION if you want to provide
 * separate block encryption and decryption functions.
 */
#define HC128_HAS_SINGLE_BLOCK_FUNCTION      /* [edit] */
#ifdef HC128_HAS_SINGLE_BLOCK_FUNCTION

#define HC128_encrypt_blocks(ctx, plaintext, ciphertext, blocks)     \
  HC128_process_blocks(0, ctx, plaintext, ciphertext, blocks)

#define HC128_decrypt_blocks(ctx, ciphertext, plaintext, blocks)     \
  HC128_process_blocks(1, ctx, ciphertext, plaintext, blocks)

void HC128_process_blocks(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  HC128_ctx* ctx, 
  const u8* input, 
  u8* output, 
  u32 blocks);                /* Message length in blocks. */

#else

void HC128_encrypt_blocks(
  HC128_ctx* ctx, 
  const u8* plaintext, 
  u8* ciphertext, 
  u32 blocks);                /* Message length in blocks. */ 

void HC128_decrypt_blocks(
  HC128_ctx* ctx, 
  const u8* ciphertext, 
  u8* plaintext, 
  u32 blocks);                /* Message length in blocks. */ 

#endif

#ifdef HC128_GENERATES_KEYSTREAM

void HC128_keystream_blocks(
  HC128_ctx* ctx,
  u8* keystream,
  u32 blocks);                /* Keystream length in blocks. */ 

#endif

#endif

/*
 * If your cipher can be implemented in different ways, you can use
 * the HC128_VARIANT parameter to allow the user to choose between
 * them at compile time (e.g., gcc -DHC128_VARIANT=3 ...). Please
 * only use this possibility if you really think it could make a
 * significant difference and keep the number of variants
 * (HC128_MAXVARIANT) as small as possible (definitely not more than
 * 10). Note also that all variants should have exactly the same
 * external interface (i.e., the same HC128_BLOCKLENGTH, etc.). 
 */
#define HC128_MAXVARIANT 1                   /* [edit] */

#ifndef HC128_VARIANT
#define HC128_VARIANT 1
#endif

#if (HC128_VARIANT > HC128_MAXVARIANT)
#error this variant does not exist
#endif

/* ------------------------------------------------------------------------- */

#endif


