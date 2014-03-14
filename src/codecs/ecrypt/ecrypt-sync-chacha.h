/* ecrypt-sync.h */

/* 
 * Header file for synchronous stream ciphers without authentication
 * mechanism.
 * 
 * *** Please only edit parts marked with "[edit]". ***
 */

#ifndef ChaCha_SYNC_AE
#define ChaCha_SYNC_AE

#include "ecrypt-portable.h"

/* ------------------------------------------------------------------------- */

/* Cipher parameters */

/* 
 * The name of your cipher.
 */
#define ChaCha_NAME "Salsa20 stream cipher"    /* [edit] */ 

/*
 * Specify which key and IV sizes are supported by your cipher. A user
 * should be able to enumerate the supported sizes by running the
 * following code:
 *
 * for (i = 0; ChaCha_KEYSIZE(i) <= ChaCha_MAXKEYSIZE; ++i)
 *   {
 *     keysize = ChaCha_KEYSIZE(i);
 *
 *     ...
 *   }
 *
 * All sizes are in bits.
 */

#define ChaCha_MAXKEYSIZE 256                 /* [edit] */
#define ChaCha_KEYSIZE(i) (128 + (i)*128)     /* [edit] */

#define ChaCha_MAXIVSIZE 64                   /* [edit] */
#define ChaCha_IVSIZE(i) (64 + (i)*64)        /* [edit] */

/* ------------------------------------------------------------------------- */

/* Data structures */

/* 
 * ChaCha_ctx is the structure containing the representation of the
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
} ChaCha_ctx;

/* ------------------------------------------------------------------------- */

/* Mandatory functions */

/*
 * Key and message independent initialization. This function will be
 * called once when the program starts (e.g., to build expanded S-box
 * tables).
 */
void ChaCha_init();

/*
 * Key setup. It is the user's responsibility to select the values of
 * keysize and ivsize from the set of supported values specified
 * above.
 */
void ChaCha_keysetup(
  ChaCha_ctx* ctx, 
  const u8* key, 
  u32 keysize,                /* Key size in bits. */ 
  u32 ivsize);                /* IV size in bits. */ 

/*
 * IV setup. After having called ChaCha_keysetup(), the user is
 * allowed to call ChaCha_ivsetup() different times in order to
 * encrypt/decrypt different messages with the same key but different
 * IV's.
 */
void ChaCha_ivsetup(
  ChaCha_ctx* ctx, 
  const u8* iv);

/*
 * Encryption/decryption of arbitrary length messages.
 *
 * For efficiency reasons, the API provides two types of
 * encrypt/decrypt functions. The ChaCha_encrypt_bytes() function
 * (declared here) encrypts byte strings of arbitrary length, while
 * the ChaCha_encrypt_blocks() function (defined later) only accepts
 * lengths which are multiples of ChaCha_BLOCKLENGTH.
 * 
 * The user is allowed to make multiple calls to
 * ChaCha_encrypt_blocks() to incrementally encrypt a long message,
 * but he is NOT allowed to make additional encryption calls once he
 * has called ChaCha_encrypt_bytes() (unless he starts a new message
 * of course). For example, this sequence of calls is acceptable:
 *
 * ChaCha_keysetup();
 *
 * ChaCha_ivsetup();
 * ChaCha_encrypt_blocks();
 * ChaCha_encrypt_blocks();
 * ChaCha_encrypt_bytes();
 *
 * ChaCha_ivsetup();
 * ChaCha_encrypt_blocks();
 * ChaCha_encrypt_blocks();
 *
 * ChaCha_ivsetup();
 * ChaCha_encrypt_bytes();
 * 
 * The following sequence is not:
 *
 * ChaCha_keysetup();
 * ChaCha_ivsetup();
 * ChaCha_encrypt_blocks();
 * ChaCha_encrypt_bytes();
 * ChaCha_encrypt_blocks();
 */

void ChaCha_encrypt_bytes(
  ChaCha_ctx* ctx, 
  const u8* plaintext, 
  u8* ciphertext, 
  u32 msglen);                /* Message length in bytes. */ 

void ChaCha_decrypt_bytes(
  ChaCha_ctx* ctx, 
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
 * reset the ChaCha_GENERATES_KEYSTREAM flag.
 */

#define ChaCha_GENERATES_KEYSTREAM
#ifdef ChaCha_GENERATES_KEYSTREAM

void ChaCha_keystream_bytes(
  ChaCha_ctx* ctx,
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
 * undef the ChaCha_USES_DEFAULT_ALL_IN_ONE flag.
 */
#define ChaCha_USES_DEFAULT_ALL_IN_ONE        /* [edit] */

void ChaCha_encrypt_packet(
  ChaCha_ctx* ctx, 
  const u8* iv,
  const u8* plaintext, 
  u8* ciphertext, 
  u32 msglen);

void ChaCha_decrypt_packet(
  ChaCha_ctx* ctx, 
  const u8* iv,
  const u8* ciphertext, 
  u8* plaintext, 
  u32 msglen);

/*
 * Encryption/decryption of blocks.
 * 
 * By default, these functions are defined as macros. If you want to
 * provide a different implementation, please undef the
 * ChaCha_USES_DEFAULT_BLOCK_MACROS flag and implement the functions
 * declared below.
 */

#define ChaCha_BLOCKLENGTH 64                  /* [edit] */

#define ChaCha_USES_DEFAULT_BLOCK_MACROS      /* [edit] */
#ifdef ChaCha_USES_DEFAULT_BLOCK_MACROS

#define ChaCha_encrypt_blocks(ctx, plaintext, ciphertext, blocks)  \
  ChaCha_encrypt_bytes(ctx, plaintext, ciphertext,                 \
    (blocks) * ChaCha_BLOCKLENGTH)

#define ChaCha_decrypt_blocks(ctx, ciphertext, plaintext, blocks)  \
  ChaCha_decrypt_bytes(ctx, ciphertext, plaintext,                 \
    (blocks) * ChaCha_BLOCKLENGTH)

#ifdef ChaCha_GENERATES_KEYSTREAM

#define ChaCha_keystream_blocks(ctx, keystream, blocks)            \
  ChaCha_AE_keystream_bytes(ctx, keystream,                        \
    (blocks) * ChaCha_BLOCKLENGTH)

#endif

#else

void ChaCha_encrypt_blocks(
  ChaCha_ctx* ctx, 
  const u8* plaintext, 
  u8* ciphertext, 
  u32 blocks);                /* Message length in blocks. */ 

void ChaCha_decrypt_blocks(
  ChaCha_ctx* ctx, 
  const u8* ciphertext, 
  u8* plaintext, 
  u32 blocks);                /* Message length in blocks. */ 

#ifdef ChaCha_GENERATES_KEYSTREAM

void ChaCha_keystream_blocks(
  ChaCha_AE_ctx* ctx,
  const u8* keystream,
  u32 blocks);                /* Keystream length in blocks. */ 

#endif

#endif

/* ------------------------------------------------------------------------- */

#endif
