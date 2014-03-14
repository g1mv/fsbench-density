/* ecrypt-sync.h */

/* 
 * Header file for synchronous stream ciphers without authentication
 * mechanism.
 * 
 * *** Please only edit parts marked with "[edit]". ***
 */

#ifndef Salsa20_SYNC
#define Salsa20_SYNC

#include "ecrypt-portable.h"

/* ------------------------------------------------------------------------- */

/* Cipher parameters */

/* 
 * The name of your cipher.
 */
#define Salsa20_NAME "Salsa20"    /* [edit] */ 
#define Salsa20_PROFILE "S3___"

/*
 * Specify which key and IV sizes are supported by your cipher. A user
 * should be able to enumerate the supported sizes by running the
 * following code:
 *
 * for (i = 0; Salsa20_KEYSIZE(i) <= Salsa20_MAXKEYSIZE; ++i)
 *   {
 *     keysize = Salsa20_KEYSIZE(i);
 *
 *     ...
 *   }
 *
 * All sizes are in bits.
 */

#define Salsa20_MAXKEYSIZE 256                 /* [edit] */
#define Salsa20_KEYSIZE(i) (128 + (i)*128)     /* [edit] */

#define Salsa20_MAXIVSIZE 64                   /* [edit] */
#define Salsa20_IVSIZE(i) (64 + (i)*64)        /* [edit] */

/* ------------------------------------------------------------------------- */

/* Data structures */

/* 
 * Salsa20_ctx is the structure containing the representation of the
 * internal state of your cipher. 
 */

typedef struct
{
  u32 input[16]; /* could be compressed */
  /* 
   * [edit]
   *
   * Put here all state variable needed during the encryption process.
   */
} Salsa20_ctx;

/* ------------------------------------------------------------------------- */

/* Mandatory functions */

/*
 * Key and message independent initialization. This function will be
 * called once when the program starts (e.g., to build expanded S-box
 * tables).
 */
void Salsa20_init();

/*
 * Key setup. It is the user's responsibility to select the values of
 * keysize and ivsize from the set of supported values specified
 * above.
 */
void Salsa20_keysetup(
  Salsa20_ctx* ctx, 
  const u8* key, 
  u32 keysize,                /* Key size in bits. */ 
  u32 ivsize);                /* IV size in bits. */ 

/*
 * IV setup. After having called Salsa20_keysetup(), the user is
 * allowed to call Salsa20_ivsetup() different times in order to
 * encrypt/decrypt different messages with the same key but different
 * IV's.
 */
void Salsa20_ivsetup(
  Salsa20_ctx* ctx, 
  const u8* iv);

/*
 * Encryption/decryption of arbitrary length messages.
 *
 * For efficiency reasons, the API provides two types of
 * encrypt/decrypt functions. The Salsa20_encrypt_bytes() function
 * (declared here) encrypts byte strings of arbitrary length, while
 * the Salsa20_encrypt_blocks() function (defined later) only accepts
 * lengths which are multiples of Salsa20_BLOCKLENGTH.
 * 
 * The user is allowed to make multiple calls to
 * Salsa20_encrypt_blocks() to incrementally encrypt a long message,
 * but he is NOT allowed to make additional encryption calls once he
 * has called Salsa20_encrypt_bytes() (unless he starts a new message
 * of course). For example, this sequence of calls is acceptable:
 *
 * Salsa20_keysetup();
 *
 * Salsa20_ivsetup();
 * Salsa20_encrypt_blocks();
 * Salsa20_encrypt_blocks();
 * Salsa20_encrypt_bytes();
 *
 * Salsa20_ivsetup();
 * Salsa20_encrypt_blocks();
 * Salsa20_encrypt_blocks();
 *
 * Salsa20_ivsetup();
 * Salsa20_encrypt_bytes();
 * 
 * The following sequence is not:
 *
 * Salsa20_keysetup();
 * Salsa20_ivsetup();
 * Salsa20_encrypt_blocks();
 * Salsa20_encrypt_bytes();
 * Salsa20_encrypt_blocks();
 */

void Salsa20_encrypt_bytes(
  Salsa20_ctx* ctx, 
  const u8* plaintext, 
  u8* ciphertext, 
  u32 msglen);                /* Message length in bytes. */ 

void Salsa20_decrypt_bytes(
  Salsa20_ctx* ctx, 
  const u8* ciphertext, 
  u8* plaintext, 
  u32 msglen);                /* Message length in bytes. */ 

/* ------------------------------------------------------------------------- */

/* Optional features */

/* 
 * For testing purposes it can sometimes be useful to have a function
 * which immediately generates keystream without having to provide it
 * with a zero plaintext. If your cipher cannot provide this function
 * (e.g., because it is not strictly a synchronous cipher), please
 * reset the Salsa20_GENERATES_KEYSTREAM flag.
 */

#define Salsa20_GENERATES_KEYSTREAM
#ifdef Salsa20_GENERATES_KEYSTREAM

void Salsa20_keystream_bytes(
  Salsa20_ctx* ctx,
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
 * undef the Salsa20_USES_DEFAULT_ALL_IN_ONE flag.
 */
#define Salsa20_USES_DEFAULT_ALL_IN_ONE        /* [edit] */

void Salsa20_encrypt_packet(
  Salsa20_ctx* ctx, 
  const u8* iv,
  const u8* plaintext, 
  u8* ciphertext, 
  u32 msglen);

void Salsa20_decrypt_packet(
  Salsa20_ctx* ctx, 
  const u8* iv,
  const u8* ciphertext, 
  u8* plaintext, 
  u32 msglen);

/*
 * Encryption/decryption of blocks.
 * 
 * By default, these functions are defined as macros. If you want to
 * provide a different implementation, please undef the
 * Salsa20_USES_DEFAULT_BLOCK_MACROS flag and implement the functions
 * declared below.
 */

#define Salsa20_BLOCKLENGTH 64                  /* [edit] */

#define Salsa20_USES_DEFAULT_BLOCK_MACROS      /* [edit] */
#ifdef Salsa20_USES_DEFAULT_BLOCK_MACROS

#define Salsa20_encrypt_blocks(ctx, plaintext, ciphertext, blocks)  \
  Salsa20_encrypt_bytes(ctx, plaintext, ciphertext,                 \
    (blocks) * Salsa20_BLOCKLENGTH)

#define Salsa20_decrypt_blocks(ctx, ciphertext, plaintext, blocks)  \
  Salsa20_decrypt_bytes(ctx, ciphertext, plaintext,                 \
    (blocks) * Salsa20_BLOCKLENGTH)

#ifdef Salsa20_GENERATES_KEYSTREAM

#define Salsa20_keystream_blocks(ctx, keystream, blocks)            \
  Salsa20_keystream_bytes(ctx, keystream,                        \
    (blocks) * Salsa20_BLOCKLENGTH)

#endif

#else

void Salsa20_encrypt_blocks(
  Salsa20_ctx* ctx, 
  const u8* plaintext, 
  u8* ciphertext, 
  u32 blocks);                /* Message length in blocks. */ 

void Salsa20_decrypt_blocks(
  Salsa20_ctx* ctx, 
  const u8* ciphertext, 
  u8* plaintext, 
  u32 blocks);                /* Message length in blocks. */ 

#ifdef Salsa20_GENERATES_KEYSTREAM

void Salsa20_keystream_blocks(
  Salsa20_ctx* ctx,
  const u8* keystream,
  u32 blocks);                /* Keystream length in blocks. */ 

#endif

#endif

/*
 * If your cipher can be implemented in different ways, you can use
 * the Salsa20_VARIANT parameter to allow the user to choose between
 * them at compile time (e.g., gcc -DSalsa20_VARIANT=3 ...). Please
 * only use this possibility if you really think it could make a
 * significant difference and keep the number of variants
 * (Salsa20_MAXVARIANT) as small as possible (definitely not more than
 * 10). Note also that all variants should have exactly the same
 * external interface (i.e., the same Salsa20_BLOCKLENGTH, etc.). 
 */
#define Salsa20_MAXVARIANT 1                   /* [edit] */

#ifndef Salsa20_VARIANT
#define Salsa20_VARIANT 1
#endif

#if (Salsa20_VARIANT > Salsa20_MAXVARIANT)
#error this variant does not exist
#endif

/* ------------------------------------------------------------------------- */

#endif
