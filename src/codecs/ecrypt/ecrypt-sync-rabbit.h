/* ecrypt-sync.h */

/* 
 * Header file for synchronous stream ciphers without authentication
 * mechanism.
 * 
 * *** Please only edit parts marked with "[edit]". ***
 */

#ifndef Rabbit_SYNC
#define Rabbit_SYNC

#include "ecrypt-portable.h"

/* ------------------------------------------------------------------------- */

/* Cipher parameters */

/* 
 * The name of your cipher.
 */
#define Rabbit_NAME "Rabbit Stream Cipher"

/*
 * Specify which key and IV sizes are supported by your cipher. A user
 * should be able to enumerate the supported sizes by running the
 * following code:
 *
 * for (i = 0; Rabbit_KEYSIZE(i) <= Rabbit_MAXKEYSIZE; ++i)
 *   {
 *     keysize = Rabbit_KEYSIZE(i);
 *
 *     ...
 *   }
 *
 * All sizes are in bits.
 */

#define Rabbit_MAXKEYSIZE 128
#define Rabbit_KEYSIZE(i) (128 + (i)*32)

#define Rabbit_MAXIVSIZE 64
#define Rabbit_IVSIZE(i) (64 + (i)*64)

/* ------------------------------------------------------------------------- */

/* Data structures */

/* 
 * Rabbit_ctx is the structure containing the representation of the
 * internal state of your cipher. 
 */

typedef struct
{
   u32 x[8];
   u32 c[8];
   u32 carry;
} RABBIT_ctx;

typedef struct
{
  /* 
   * Put here all state variable needed during the encryption process.
   */
   RABBIT_ctx master_ctx;
   RABBIT_ctx work_ctx;
} Rabbit_ctx;

/* ------------------------------------------------------------------------- */

/* Mandatory functions */

/*
 * Key and message independent initialization. This function will be
 * called once when the program starts (e.g., to build expanded S-box
 * tables).
 */
void Rabbit_init(void);

/*
 * Key setup. It is the user's responsibility to select the values of
 * keysize and ivsize from the set of supported values specified
 * above.
 */
void Rabbit_keysetup(
  Rabbit_ctx* ctx, 
  const u8* key, 
  u32 keysize,                /* Key size in bits. */ 
  u32 ivsize);                /* IV size in bits. */ 

/*
 * IV setup. After having called Rabbit_keysetup(), the user is
 * allowed to call Rabbit_ivsetup() different times in order to
 * encrypt/decrypt different messages with the same key but different
 * IV's.
 */
void Rabbit_ivsetup(
  Rabbit_ctx* ctx, 
  const u8* iv);

/*
 * Encryption/decryption of arbitrary length messages.
 *
 * For efficiency reasons, the API provides two types of
 * encrypt/decrypt functions. The Rabbit_encrypt_bytes() function
 * (declared here) encrypts byte strings of arbitrary length, while
 * the Rabbit_encrypt_blocks() function (defined later) only accepts
 * lengths which are multiples of Rabbit_BLOCKLENGTH.
 * 
 * The user is allowed to make multiple calls to
 * Rabbit_encrypt_blocks() to incrementally encrypt a long message,
 * but he is NOT allowed to make additional encryption calls once he
 * has called Rabbit_encrypt_bytes() (unless he starts a new message
 * of course). For example, this sequence of calls is acceptable:
 *
 * Rabbit_keysetup();
 *
 * Rabbit_ivsetup();
 * Rabbit_encrypt_blocks();
 * Rabbit_encrypt_blocks();
 * Rabbit_encrypt_bytes();
 *
 * Rabbit_ivsetup();
 * Rabbit_encrypt_blocks();
 * Rabbit_encrypt_blocks();
 *
 * Rabbit_ivsetup();
 * Rabbit_encrypt_bytes();
 * 
 * The following sequence is not:
 *
 * Rabbit_keysetup();
 * Rabbit_ivsetup();
 * Rabbit_encrypt_blocks();
 * Rabbit_encrypt_bytes();
 * Rabbit_encrypt_blocks();
 */

/*
 * By default Rabbit_encrypt_bytes() and Rabbit_decrypt_bytes() are
 * defined as macros which redirect the call to a single function
 * Rabbit_process_bytes(). If you want to provide separate encryption
 * and decryption functions, please undef
 * Rabbit_HAS_SINGLE_BYTE_FUNCTION.
 */
#define Rabbit_HAS_SINGLE_BYTE_FUNCTION
#ifdef Rabbit_HAS_SINGLE_BYTE_FUNCTION

#define Rabbit_encrypt_bytes(ctx, plaintext, ciphertext, msglen)   \
  Rabbit_process_bytes(0, ctx, plaintext, ciphertext, msglen)

#define Rabbit_decrypt_bytes(ctx, ciphertext, plaintext, msglen)   \
  Rabbit_process_bytes(1, ctx, ciphertext, plaintext, msglen)

void Rabbit_process_bytes(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  Rabbit_ctx* ctx, 
  const u8* input, 
  u8* output, 
  u32 msglen);                /* Message length in bytes. */ 

#else

void Rabbit_encrypt_bytes(
  Rabbit_ctx* ctx, 
  const u8* plaintext, 
  u8* ciphertext, 
  u32 msglen);                /* Message length in bytes. */ 

void Rabbit_decrypt_bytes(
  Rabbit_ctx* ctx, 
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
 * reset the Rabbit_GENERATES_KEYSTREAM flag.
 */

#define Rabbit_GENERATES_KEYSTREAM
#ifdef Rabbit_GENERATES_KEYSTREAM

void Rabbit_keystream_bytes(
  Rabbit_ctx* ctx,
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
 * undef the Rabbit_USES_DEFAULT_ALL_IN_ONE flag.
 */
#define Rabbit_USES_DEFAULT_ALL_IN_ONE

/*
 * Undef Rabbit_HAS_SINGLE_PACKET_FUNCTION if you want to provide
 * separate packet encryption and decryption functions.
 */
#define Rabbit_HAS_SINGLE_PACKET_FUNCTION
#ifdef Rabbit_HAS_SINGLE_PACKET_FUNCTION

#define Rabbit_encrypt_packet(                                        \
    ctx, iv, plaintext, ciphertext, mglen)                            \
  Rabbit_process_packet(0,                                            \
    ctx, iv, plaintext, ciphertext, mglen)

#define Rabbit_decrypt_packet(                                        \
    ctx, iv, ciphertext, plaintext, mglen)                            \
  Rabbit_process_packet(1,                                            \
    ctx, iv, ciphertext, plaintext, mglen)

void Rabbit_process_packet(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  Rabbit_ctx* ctx, 
  const u8* iv,
  const u8* input, 
  u8* output, 
  u32 msglen);

#else

void Rabbit_encrypt_packet(
  Rabbit_ctx* ctx, 
  const u8* iv,
  const u8* plaintext, 
  u8* ciphertext, 
  u32 msglen);

void Rabbit_decrypt_packet(
  Rabbit_ctx* ctx, 
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
 * Rabbit_USES_DEFAULT_BLOCK_MACROS flag and implement the functions
 * declared below.
 */

#define Rabbit_BLOCKLENGTH 16

#undef Rabbit_USES_DEFAULT_BLOCK_MACROS
#ifdef Rabbit_USES_DEFAULT_BLOCK_MACROS

#define Rabbit_encrypt_blocks(ctx, plaintext, ciphertext, blocks)  \
  Rabbit_encrypt_bytes(ctx, plaintext, ciphertext,                 \
    (blocks) * Rabbit_BLOCKLENGTH)

#define Rabbit_decrypt_blocks(ctx, ciphertext, plaintext, blocks)  \
  Rabbit_decrypt_bytes(ctx, ciphertext, plaintext,                 \
    (blocks) * Rabbit_BLOCKLENGTH)

#ifdef Rabbit_GENERATES_KEYSTREAM

#define Rabbit_keystream_blocks(ctx, keystream, blocks)            \
  Rabbit_keystream_bytes(ctx, keystream,                           \
    (blocks) * Rabbit_BLOCKLENGTH)

#endif

#else

/*
 * Undef Rabbit_HAS_SINGLE_BLOCK_FUNCTION if you want to provide
 * separate block encryption and decryption functions.
 */
#define Rabbit_HAS_SINGLE_BLOCK_FUNCTION
#ifdef Rabbit_HAS_SINGLE_BLOCK_FUNCTION

#define Rabbit_encrypt_blocks(ctx, plaintext, ciphertext, blocks)     \
  Rabbit_process_blocks(0, ctx, plaintext, ciphertext, blocks)

#define Rabbit_decrypt_blocks(ctx, ciphertext, plaintext, blocks)     \
  Rabbit_process_blocks(1, ctx, ciphertext, plaintext, blocks)

void Rabbit_process_blocks(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  Rabbit_ctx* ctx, 
  const u8* input, 
  u8* output, 
  u32 blocks);                /* Message length in blocks. */

#else

void Rabbit_encrypt_blocks(
  Rabbit_ctx* ctx, 
  const u8* plaintext, 
  u8* ciphertext, 
  u32 blocks);                /* Message length in blocks. */ 

void Rabbit_decrypt_blocks(
  Rabbit_ctx* ctx, 
  const u8* ciphertext, 
  u8* plaintext, 
  u32 blocks);                /* Message length in blocks. */ 

#endif

#ifdef Rabbit_GENERATES_KEYSTREAM

void Rabbit_keystream_blocks(
  Rabbit_ctx* ctx,
  u8* keystream,
  u32 blocks);                /* Keystream length in blocks. */ 

#endif

#endif

/*
 * If your cipher can be implemented in different ways, you can use
 * the Rabbit_VARIANT parameter to allow the user to choose between
 * them at compile time (e.g., gcc -DRabbit_VARIANT=3 ...). Please
 * only use this possibility if you really think it could make a
 * significant difference and keep the number of variants
 * (Rabbit_MAXVARIANT) as small as possible (definitely not more than
 * 10). Note also that all variants should have exactly the same
 * external interface (i.e., the same Rabbit_BLOCKLENGTH, etc.). 
 */
#define Rabbit_MAXVARIANT 1

#ifndef Rabbit_VARIANT
#define Rabbit_VARIANT 1
#endif

#if (Rabbit_VARIANT > Rabbit_MAXVARIANT)
#error this variant does not exist
#endif

/* ------------------------------------------------------------------------- */

#endif
