/* ecrypt-sync.h */

/* 
 * Header file for synchronous stream ciphers without authentication
 * mechanism.
 * 
 * *** Please only edit parts marked with "[edit]". ***
 */

#ifndef AES256Hongjun_SYNC
#define AES256Hongjun_SYNC

#include "ecrypt-portable.h"

/* ------------------------------------------------------------------------- */

/* Cipher parameters */

/* 
 * The name of your cipher.
 */
#define AES256Hongjun_NAME "AES-CTR"                 /* [edit] */ 
#define AES256Hongjun_PROFILE "bench"

/*
 * Specify which key and IV sizes are supported by your cipher. A user
 * should be able to enumerate the supported sizes by running the
 * following code:
 *
 * for (i = 0; AES256Hongjun_KEYSIZE(i) <= AES256Hongjun_MAXKEYSIZE; ++i)
 *   {
 *     keysize = AES256Hongjun_KEYSIZE(i);
 *
 *     ...
 *   }
 *
 * All sizes are in bits.
 */

#define AES256Hongjun_MAXKEYSIZE 256                  /* [edit] */
#define AES256Hongjun_KEYSIZE(i) (256 + (i)*128)      /* [edit] */

#define AES256Hongjun_MAXIVSIZE 128                   /* [edit] */
#define AES256Hongjun_IVSIZE(i) (128 + (i)*32)        /* [edit] */

/* ------------------------------------------------------------------------- */

/* Data structures */

/* 
 * AES256Hongjun_ctx is the structure containing the representation of the
 * internal state of your cipher. 
 */

#define Nr 14
#define Nk 8
#define Nb 4

typedef struct
{
  u32 keysize;
  u8 key[16];
  u8 IV[16];
  u32 round_key[Nr+1][4];
  u32 counter[4];
  u32 first_round_output_x0;
  u32 second_round_output[4];
} AES256Hongjun_ctx;

/* ------------------------------------------------------------------------- */

/* Mandatory functions */

/*
 * Key and message independent initialization. This function will be
 * called once when the program starts (e.g., to build expanded S-box
 * tables).
 */
void AES256Hongjun_init(void);

/*
 * Key setup. It is the user's responsibility to select the values of
 * keysize and ivsize from the set of supported values specified
 * above.
 */
void AES256Hongjun_keysetup(
  AES256Hongjun_ctx* ctx, 
  const u8* key, 
  u32 keysize,                /* Key size in bits. */ 
  u32 ivsize);                /* IV size in bits. */ 

/*
 * IV setup. After having called AES256Hongjun_keysetup(), the user is
 * allowed to call AES256Hongjun_ivsetup() different times in order to
 * encrypt/decrypt different messages with the same key but different
 * IV's.
 */
void AES256Hongjun_ivsetup(
  AES256Hongjun_ctx* ctx, 
  const u8* iv);

/*
 * Encryption/decryption of arbitrary length messages.
 *
 * For efficiency reasons, the API provides two types of
 * encrypt/decrypt functions. The AES256Hongjun_encrypt_bytes() function
 * (declared here) encrypts byte strings of arbitrary length, while
 * the AES256Hongjun_encrypt_blocks() function (defined later) only accepts
 * lengths which are multiples of AES256Hongjun_BLOCKLENGTH.
 * 
 * The user is allowed to make multiple calls to
 * AES256Hongjun_encrypt_blocks() to incrementally encrypt a long message,
 * but he is NOT allowed to make additional encryption calls once he
 * has called AES256Hongjun_encrypt_bytes() (unless he starts a new message
 * of course). For example, this sequence of calls is acceptable:
 *
 * AES256Hongjun_keysetup();
 *
 * AES256Hongjun_ivsetup();
 * AES256Hongjun_encrypt_blocks();
 * AES256Hongjun_encrypt_blocks();
 * AES256Hongjun_encrypt_bytes();
 *
 * AES256Hongjun_ivsetup();
 * AES256Hongjun_encrypt_blocks();
 * AES256Hongjun_encrypt_blocks();
 *
 * AES256Hongjun_ivsetup();
 * AES256Hongjun_encrypt_bytes();
 * 
 * The following sequence is not:
 *
 * AES256Hongjun_keysetup();
 * AES256Hongjun_ivsetup();
 * AES256Hongjun_encrypt_blocks();
 * AES256Hongjun_encrypt_bytes();
 * AES256Hongjun_encrypt_blocks();
 */

/*
 * By default AES256Hongjun_encrypt_bytes() and AES256Hongjun_decrypt_bytes() are
 * defined as macros which redirect the call to a single function
 * AES256Hongjun_process_bytes(). If you want to provide separate encryption
 * and decryption functions, please undef
 * AES256Hongjun_HAS_SINGLE_BYTE_FUNCTION.
 */
#define AES256Hongjun_HAS_SINGLE_BYTE_FUNCTION       /* [edit] */
#ifdef AES256Hongjun_HAS_SINGLE_BYTE_FUNCTION

#define AES256Hongjun_encrypt_bytes(ctx, plaintext, ciphertext, msglen)   \
  AES256Hongjun_process_bytes(0, ctx, plaintext, ciphertext, msglen)

#define AES256Hongjun_decrypt_bytes(ctx, ciphertext, plaintext, msglen)   \
  AES256Hongjun_process_bytes(1, ctx, ciphertext, plaintext, msglen)

void AES256Hongjun_process_bytes(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  AES256Hongjun_ctx* ctx, 
  const u8* input, 
  u8* output, 
  u32 msglen);                /* Message length in bytes. */ 

#else

void AES256Hongjun_encrypt_bytes(
  AES256Hongjun_ctx* ctx, 
  const u8* plaintext, 
  u8* ciphertext, 
  u32 msglen);                /* Message length in bytes. */ 

void AES256Hongjun_decrypt_bytes(
  AES256Hongjun_ctx* ctx, 
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
 * reset the AES256Hongjun_GENERATES_KEYSTREAM flag.
 */

#define AES256Hongjun_GENERATES_KEYSTREAM
#ifdef AES256Hongjun_GENERATES_KEYSTREAM

void AES256Hongjun_keystream_bytes(
  AES256Hongjun_ctx* ctx,
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
 * undef the AES256Hongjun_USES_DEFAULT_ALL_IN_ONE flag.
 */
#define AES256Hongjun_USES_DEFAULT_ALL_IN_ONE        /* [edit] */

/*
 * Undef AES256Hongjun_HAS_SINGLE_PACKET_FUNCTION if you want to provide
 * separate packet encryption and decryption functions.
 */
#define AES256Hongjun_HAS_SINGLE_PACKET_FUNCTION     /* [edit] */
#ifdef AES256Hongjun_HAS_SINGLE_PACKET_FUNCTION

#define AES256Hongjun_encrypt_packet(                                        \
    ctx, iv, plaintext, ciphertext, mglen)                            \
  AES256Hongjun_process_packet(0,                                            \
    ctx, iv, plaintext, ciphertext, mglen)

#define AES256Hongjun_decrypt_packet(                                        \
    ctx, iv, ciphertext, plaintext, mglen)                            \
  AES256Hongjun_process_packet(1,                                            \
    ctx, iv, ciphertext, plaintext, mglen)

void AES256Hongjun_process_packet(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  AES256Hongjun_ctx* ctx, 
  const u8* iv,
  const u8* input, 
  u8* output, 
  u32 msglen);

#else

void AES256Hongjun_encrypt_packet(
  AES256Hongjun_ctx* ctx, 
  const u8* iv,
  const u8* plaintext, 
  u8* ciphertext, 
  u32 msglen);

void AES256Hongjun_decrypt_packet(
  AES256Hongjun_ctx* ctx, 
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
 * AES256Hongjun_USES_DEFAULT_BLOCK_MACROS flag and implement the functions
 * declared below.
 */

#define AES256Hongjun_BLOCKLENGTH 64                 /* [edit] */

#define AES256Hongjun_USES_DEFAULT_BLOCK_MACROS      /* [edit] */
#ifdef AES256Hongjun_USES_DEFAULT_BLOCK_MACROS

#define AES256Hongjun_encrypt_blocks(ctx, plaintext, ciphertext, blocks)  \
  AES256Hongjun_encrypt_bytes(ctx, plaintext, ciphertext,                 \
    (blocks) * AES256Hongjun_BLOCKLENGTH)

#define AES256Hongjun_decrypt_blocks(ctx, ciphertext, plaintext, blocks)  \
  AES256Hongjun_decrypt_bytes(ctx, ciphertext, plaintext,                 \
    (blocks) * AES256Hongjun_BLOCKLENGTH)

#ifdef AES256Hongjun_GENERATES_KEYSTREAM

#define AES256Hongjun_keystream_blocks(ctx, keystream, blocks)            \
  AES256Hongjun_keystream_bytes(ctx, keystream,                           \
    (blocks) * AES256Hongjun_BLOCKLENGTH)

#endif

#else

/*
 * Undef AES256Hongjun_HAS_SINGLE_BLOCK_FUNCTION if you want to provide
 * separate block encryption and decryption functions.
 */
#define AES256Hongjun_HAS_SINGLE_BLOCK_FUNCTION      /* [edit] */
#ifdef AES256Hongjun_HAS_SINGLE_BLOCK_FUNCTION

#define AES256Hongjun_encrypt_blocks(ctx, plaintext, ciphertext, blocks)     \
  AES256Hongjun_process_blocks(0, ctx, plaintext, ciphertext, blocks)

#define AES256Hongjun_decrypt_blocks(ctx, ciphertext, plaintext, blocks)     \
  AES256Hongjun_process_blocks(1, ctx, ciphertext, plaintext, blocks)

void AES256Hongjun_process_blocks(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  AES256Hongjun_ctx* ctx, 
  const u8* input, 
  u8* output, 
  u32 blocks);                /* Message length in blocks. */

#else

void AES256Hongjun_encrypt_blocks(
  AES256Hongjun_ctx* ctx, 
  const u8* plaintext, 
  u8* ciphertext, 
  u32 blocks);                /* Message length in blocks. */ 

void AES256Hongjun_decrypt_blocks(
  AES256Hongjun_ctx* ctx, 
  const u8* ciphertext, 
  u8* plaintext, 
  u32 blocks);                /* Message length in blocks. */ 

#endif

#ifdef AES256Hongjun_GENERATES_KEYSTREAM

void AES256Hongjun_keystream_blocks(
  AES256Hongjun_ctx* ctx,
  u8* keystream,
  u32 blocks);                /* Keystream length in blocks. */ 

#endif

#endif

/*
 * If your cipher can be implemented in different ways, you can use
 * the AES256Hongjun_VARIANT parameter to allow the user to choose between
 * them at compile time (e.g., gcc -DAES256Hongjun_VARIANT=3 ...). Please
 * only use this possibility if you really think it could make a
 * significant difference and keep the number of variants
 * (AES256Hongjun_MAXVARIANT) as small as possible (definitely not more than
 * 10). Note also that all variants should have exactly the same
 * external interface (i.e., the same AES256Hongjun_BLOCKLENGTH, etc.). 
 */
#define AES256Hongjun_MAXVARIANT 1                   /* [edit] */

#ifndef AES256Hongjun_VARIANT
#define AES256Hongjun_VARIANT 1
#endif

#if (AES256Hongjun_VARIANT > AES256Hongjun_MAXVARIANT)
#error this variant does not exist
#endif

/* ------------------------------------------------------------------------- */

#endif
