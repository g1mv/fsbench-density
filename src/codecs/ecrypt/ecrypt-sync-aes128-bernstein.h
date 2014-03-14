/* ecrypt-sync.h */

/* 
 * Header file for synchronous stream ciphers without authentication
 * mechanism.
 * 
 * *** Please only edit parts marked with "[edit]". ***
 */

#ifndef AES128Bernstein_SYNC
#define AES128Bernstein_SYNC

#include "ecrypt-portable.h"

/* ------------------------------------------------------------------------- */

/* Cipher parameters */

/* 
 * The name of your cipher.
 */
#define AES128Bernstein_NAME "AES-CTR"                 /* [edit] */ 
#define AES128Bernstein_PROFILE "bench"

/*
 * Specify which key and IV sizes are supported by your cipher. A user
 * should be able to enumerate the supported sizes by running the
 * following code:
 *
 * for (i = 0; AES128Bernstein_KEYSIZE(i) <= AES128Bernstein_MAXKEYSIZE; ++i)
 *   {
 *     keysize = AES128Bernstein_KEYSIZE(i);
 *
 *     ...
 *   }
 *
 * All sizes are in bits.
 */

#define AES128Bernstein_MAXKEYSIZE 128                  /* [edit] */
#define AES128Bernstein_KEYSIZE(i) (128 + (i)*64)       /* [edit] */

#define AES128Bernstein_MAXIVSIZE 128                   /* [edit] */
#define AES128Bernstein_IVSIZE(i) (128 + (i)*32)        /* [edit] */

/* ------------------------------------------------------------------------- */

/* Data structures */

/* 
 * AES128Bernstein_ctx is the structure containing the representation of the
 * internal state of your cipher. 
 */

typedef struct {
  u32 key[14];
  u32 ctr[4];
} AES128Bernstein_ctx;

/* ------------------------------------------------------------------------- */

/* Mandatory functions */

/*
 * Key and message independent initialization. This function will be
 * called once when the program starts (e.g., to build expanded S-box
 * tables).
 */
void AES128Bernstein_init(void);

/*
 * Key setup. It is the user's responsibility to select the values of
 * keysize and ivsize from the set of supported values specified
 * above.
 */
void AES128Bernstein_keysetup(
  AES128Bernstein_ctx* ctx, 
  const u8* key, 
  u32 keysize,                /* Key size in bits. */ 
  u32 ivsize);                /* IV size in bits. */ 

/*
 * IV setup. After having called AES128Bernstein_keysetup(), the user is
 * allowed to call AES128Bernstein_ivsetup() different times in order to
 * encrypt/decrypt different messages with the same key but different
 * IV's.
 */
void AES128Bernstein_ivsetup(
  AES128Bernstein_ctx* ctx, 
  const u8* iv);

/*
 * Encryption/decryption of arbitrary length messages.
 *
 * For efficiency reasons, the API provides two types of
 * encrypt/decrypt functions. The AES128Bernstein_encrypt_bytes() function
 * (declared here) encrypts byte strings of arbitrary length, while
 * the AES128Bernstein_encrypt_blocks() function (defined later) only accepts
 * lengths which are multiples of AES128Bernstein_BLOCKLENGTH.
 * 
 * The user is allowed to make multiple calls to
 * AES128Bernstein_encrypt_blocks() to incrementally encrypt a long message,
 * but he is NOT allowed to make additional encryption calls once he
 * has called AES128Bernstein_encrypt_bytes() (unless he starts a new message
 * of course). For example, this sequence of calls is acceptable:
 *
 * AES128Bernstein_keysetup();
 *
 * AES128Bernstein_ivsetup();
 * AES128Bernstein_encrypt_blocks();
 * AES128Bernstein_encrypt_blocks();
 * AES128Bernstein_encrypt_bytes();
 *
 * AES128Bernstein_ivsetup();
 * AES128Bernstein_encrypt_blocks();
 * AES128Bernstein_encrypt_blocks();
 *
 * AES128Bernstein_ivsetup();
 * AES128Bernstein_encrypt_bytes();
 * 
 * The following sequence is not:
 *
 * AES128Bernstein_keysetup();
 * AES128Bernstein_ivsetup();
 * AES128Bernstein_encrypt_blocks();
 * AES128Bernstein_encrypt_bytes();
 * AES128Bernstein_encrypt_blocks();
 */

/*
 * By default AES128Bernstein_encrypt_bytes() and AES128Bernstein_decrypt_bytes() are
 * defined as macros which redirect the call to a single function
 * AES128Bernstein_process_bytes(). If you want to provide separate encryption
 * and decryption functions, please undef
 * AES128Bernstein_HAS_SINGLE_BYTE_FUNCTION.
 */
#define AES128Bernstein_HAS_SINGLE_BYTE_FUNCTION       /* [edit] */
#ifdef AES128Bernstein_HAS_SINGLE_BYTE_FUNCTION

#define AES128Bernstein_encrypt_bytes(ctx, plaintext, ciphertext, msglen)   \
  AES128Bernstein_process_bytes(0, ctx, plaintext, ciphertext, msglen)

#define AES128Bernstein_decrypt_bytes(ctx, ciphertext, plaintext, msglen)   \
  AES128Bernstein_process_bytes(1, ctx, ciphertext, plaintext, msglen)

void AES128Bernstein_process_bytes(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  AES128Bernstein_ctx* ctx, 
  const u8* input, 
  u8* output, 
  u32 msglen);                /* Message length in bytes. */ 

#else

void AES128Bernstein_encrypt_bytes(
  AES128Bernstein_ctx* ctx, 
  const u8* plaintext, 
  u8* ciphertext, 
  u32 msglen);                /* Message length in bytes. */ 

void AES128Bernstein_decrypt_bytes(
  AES128Bernstein_ctx* ctx, 
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
 * reset the AES128Bernstein_GENERATES_KEYSTREAM flag.
 */

#define AES128Bernstein_GENERATES_KEYSTREAM
#ifdef AES128Bernstein_GENERATES_KEYSTREAM

void AES128Bernstein_keystream_bytes(
  AES128Bernstein_ctx* ctx,
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
 * undef the AES128Bernstein_USES_DEFAULT_ALL_IN_ONE flag.
 */
#define AES128Bernstein_USES_DEFAULT_ALL_IN_ONE        /* [edit] */

/*
 * Undef AES128Bernstein_HAS_SINGLE_PACKET_FUNCTION if you want to provide
 * separate packet encryption and decryption functions.
 */
#define AES128Bernstein_HAS_SINGLE_PACKET_FUNCTION     /* [edit] */
#ifdef AES128Bernstein_HAS_SINGLE_PACKET_FUNCTION

#define AES128Bernstein_encrypt_packet(                                        \
    ctx, iv, plaintext, ciphertext, mglen)                            \
  AES128Bernstein_process_packet(0,                                            \
    ctx, iv, plaintext, ciphertext, mglen)

#define AES128Bernstein_decrypt_packet(                                        \
    ctx, iv, ciphertext, plaintext, mglen)                            \
  AES128Bernstein_process_packet(1,                                            \
    ctx, iv, ciphertext, plaintext, mglen)

void AES128Bernstein_process_packet(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  AES128Bernstein_ctx* ctx, 
  const u8* iv,
  const u8* input, 
  u8* output, 
  u32 msglen);

#else

void AES128Bernstein_encrypt_packet(
  AES128Bernstein_ctx* ctx, 
  const u8* iv,
  const u8* plaintext, 
  u8* ciphertext, 
  u32 msglen);

void AES128Bernstein_decrypt_packet(
  AES128Bernstein_ctx* ctx, 
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
 * AES128Bernstein_USES_DEFAULT_BLOCK_MACROS flag and implement the functions
 * declared below.
 */

#define AES128Bernstein_BLOCKLENGTH 16                 /* [edit] */

#define AES128Bernstein_USES_DEFAULT_BLOCK_MACROS      /* [edit] */
#ifdef AES128Bernstein_USES_DEFAULT_BLOCK_MACROS

#define AES128Bernstein_encrypt_blocks(ctx, plaintext, ciphertext, blocks)  \
  AES128Bernstein_encrypt_bytes(ctx, plaintext, ciphertext,                 \
    (blocks) * AES128Bernstein_BLOCKLENGTH)

#define AES128Bernstein_decrypt_blocks(ctx, ciphertext, plaintext, blocks)  \
  AES128Bernstein_decrypt_bytes(ctx, ciphertext, plaintext,                 \
    (blocks) * AES128Bernstein_BLOCKLENGTH)

#ifdef AES128Bernstein_GENERATES_KEYSTREAM

#define AES128Bernstein_keystream_blocks(ctx, keystream, blocks)            \
  AES128Bernstein_keystream_bytes(ctx, keystream,                           \
    (blocks) * AES128Bernstein_BLOCKLENGTH)

#endif

#else

/*
 * Undef AES128Bernstein_HAS_SINGLE_BLOCK_FUNCTION if you want to provide
 * separate block encryption and decryption functions.
 */
#define AES128Bernstein_HAS_SINGLE_BLOCK_FUNCTION      /* [edit] */
#ifdef AES128Bernstein_HAS_SINGLE_BLOCK_FUNCTION

#define AES128Bernstein_encrypt_blocks(ctx, plaintext, ciphertext, blocks)     \
  AES128Bernstein_process_blocks(0, ctx, plaintext, ciphertext, blocks)

#define AES128Bernstein_decrypt_blocks(ctx, ciphertext, plaintext, blocks)     \
  AES128Bernstein_process_blocks(1, ctx, ciphertext, plaintext, blocks)

void AES128Bernstein_process_blocks(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  AES128Bernstein_ctx* ctx, 
  const u8* input, 
  u8* output, 
  u32 blocks);                /* Message length in blocks. */

#else

void AES128Bernstein_encrypt_blocks(
  AES128Bernstein_ctx* ctx, 
  const u8* plaintext, 
  u8* ciphertext, 
  u32 blocks);                /* Message length in blocks. */ 

void AES128Bernstein_decrypt_blocks(
  AES128Bernstein_ctx* ctx, 
  const u8* ciphertext, 
  u8* plaintext, 
  u32 blocks);                /* Message length in blocks. */ 

#endif

#ifdef AES128Bernstein_GENERATES_KEYSTREAM

void AES128Bernstein_keystream_blocks(
  AES128Bernstein_ctx* ctx,
  u8* keystream,
  u32 blocks);                /* Keystream length in blocks. */ 

#endif

#endif

/*
 * If your cipher can be implemented in different ways, you can use
 * the AES128Bernstein_VARIANT parameter to allow the user to choose between
 * them at compile time (e.g., gcc -DAES128Bernstein_VARIANT=3 ...). Please
 * only use this possibility if you really think it could make a
 * significant difference and keep the number of variants
 * (AES128Bernstein_MAXVARIANT) as small as possible (definitely not more than
 * 10). Note also that all variants should have exactly the same
 * external interface (i.e., the same AES128Bernstein_BLOCKLENGTH, etc.). 
 */
#define AES128Bernstein_MAXVARIANT 1                   /* [edit] */

#ifndef AES128Bernstein_VARIANT
#define AES128Bernstein_VARIANT 1
#endif

#if (AES128Bernstein_VARIANT > AES128Bernstein_MAXVARIANT)
#error this variant does not exist
#endif

/* ------------------------------------------------------------------------- */

#endif
