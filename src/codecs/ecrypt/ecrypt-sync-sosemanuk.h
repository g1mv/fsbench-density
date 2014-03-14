/* ecrypt-sync.h */

/* 
 * Header file for synchronous stream ciphers without authentication
 * mechanism.
 * 
 * *** Please only edit parts marked with "[edit]". ***
 */

#ifndef Sosemanuk_SYNC
#define Sosemanuk_SYNC

#include "ecrypt-portable.h"

/* ------------------------------------------------------------------------- */

/* Cipher parameters */

/* 
 * The name of your cipher.
 */
#define Sosemanuk_NAME "Sosemanuk"    /* [edit] */ 

/*
 * Specify which key and IV sizes are supported by your cipher. A user
 * should be able to enumerate the supported sizes by running the
 * following code:
 *
 * for (i = 0; Sosemanuk_KEYSIZE(i) <= Sosemanuk_MAXKEYSIZE; ++i)
 *   {
 *     keysize = Sosemanuk_KEYSIZE(i);
 *
 *     ...
 *   }
 *
 * All sizes are in bits.
 */

#define Sosemanuk_MAXKEYSIZE 256                 /* [edit] */
#define Sosemanuk_KEYSIZE(i) (8 + (i)*8)         /* [edit] */

#define Sosemanuk_MAXIVSIZE 128                  /* [edit] */
#define Sosemanuk_IVSIZE(i) (8 + (i)*8)          /* [edit] */

/* ------------------------------------------------------------------------- */

/* Data structures */

/* 
 * Sosemanuk_ctx is the structure containing the representation of the
 * internal state of your cipher. 
 */

typedef struct
{
  /* 
   * [edit]
   *
   * Put here all state variable needed during the encryption process.
   */

	/*
	 * Sub-keys (computed from the key).
	 */
	u32 sk[100];

	/*
	 * IV length (in bytes).
	 */
	size_t ivlen;

	/*
	 * Internal state.
	 */
	u32 s00, s01, s02, s03, s04, s05, s06, s07, s08, s09;
	u32 r1, r2;

} Sosemanuk_ctx;

/* ------------------------------------------------------------------------- */

/* Mandatory functions */

/*
 * Key and message independent initialization. This function will be
 * called once when the program starts (e.g., to build expanded S-box
 * tables).
 */
void Sosemanuk_init(void);

/*
 * Key setup. It is the user's responsibility to select the values of
 * keysize and ivsize from the set of supported values specified
 * above.
 */
void Sosemanuk_keysetup(
  Sosemanuk_ctx* ctx, 
  const u8* key, 
  u32 keysize,                /* Key size in bits. */ 
  u32 ivsize);                /* IV size in bits. */ 

/*
 * IV setup. After having called Sosemanuk_keysetup(), the user is
 * allowed to call Sosemanuk_ivsetup() different times in order to
 * encrypt/decrypt different messages with the same key but different
 * IV's.
 */
void Sosemanuk_ivsetup(
  Sosemanuk_ctx* ctx, 
  const u8* iv);

/*
 * Encryption/decryption of arbitrary length messages.
 *
 * For efficiency reasons, the API provides two types of
 * encrypt/decrypt functions. The Sosemanuk_encrypt_bytes() function
 * (declared here) encrypts byte strings of arbitrary length, while
 * the Sosemanuk_encrypt_blocks() function (defined later) only accepts
 * lengths which are multiples of Sosemanuk_BLOCKLENGTH.
 * 
 * The user is allowed to make multiple calls to
 * Sosemanuk_encrypt_blocks() to incrementally encrypt a long message,
 * but he is NOT allowed to make additional encryption calls once he
 * has called Sosemanuk_encrypt_bytes() (unless he starts a new message
 * of course). For example, this sequence of calls is acceptable:
 *
 * Sosemanuk_keysetup();
 *
 * Sosemanuk_ivsetup();
 * Sosemanuk_encrypt_blocks();
 * Sosemanuk_encrypt_blocks();
 * Sosemanuk_encrypt_bytes();
 *
 * Sosemanuk_ivsetup();
 * Sosemanuk_encrypt_blocks();
 * Sosemanuk_encrypt_blocks();
 *
 * Sosemanuk_ivsetup();
 * Sosemanuk_encrypt_bytes();
 * 
 * The following sequence is not:
 *
 * Sosemanuk_keysetup();
 * Sosemanuk_ivsetup();
 * Sosemanuk_encrypt_blocks();
 * Sosemanuk_encrypt_bytes();
 * Sosemanuk_encrypt_blocks();
 */

/*
 * By default Sosemanuk_encrypt_bytes() and Sosemanuk_decrypt_bytes() are
 * defined as macros which redirect the call to a single function
 * Sosemanuk_process_bytes(). If you want to provide separate encryption
 * and decryption functions, please undef
 * Sosemanuk_HAS_SINGLE_BYTE_FUNCTION.
 */
#define Sosemanuk_HAS_SINGLE_BYTE_FUNCTION       /* [edit] */
#ifdef Sosemanuk_HAS_SINGLE_BYTE_FUNCTION

#define Sosemanuk_encrypt_bytes(ctx, plaintext, ciphertext, msglen)   \
  Sosemanuk_process_bytes(0, ctx, plaintext, ciphertext, msglen)

#define Sosemanuk_decrypt_bytes(ctx, ciphertext, plaintext, msglen)   \
  Sosemanuk_process_bytes(1, ctx, ciphertext, plaintext, msglen)

void Sosemanuk_process_bytes(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  Sosemanuk_ctx* ctx, 
  const u8* input, 
  u8* output, 
  u32 msglen);                /* Message length in bytes. */ 

#else

void Sosemanuk_encrypt_bytes(
  Sosemanuk_ctx* ctx, 
  const u8* plaintext, 
  u8* ciphertext, 
  u32 msglen);                /* Message length in bytes. */ 

void Sosemanuk_decrypt_bytes(
  Sosemanuk_ctx* ctx, 
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
 * reset the Sosemanuk_GENERATES_KEYSTREAM flag.
 */

#define Sosemanuk_GENERATES_KEYSTREAM
#ifdef Sosemanuk_GENERATES_KEYSTREAM

void Sosemanuk_keystream_bytes(
  Sosemanuk_ctx* ctx,
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
 * undef the Sosemanuk_USES_DEFAULT_ALL_IN_ONE flag.
 */
#define Sosemanuk_USES_DEFAULT_ALL_IN_ONE        /* [edit] */

/*
 * Undef Sosemanuk_HAS_SINGLE_PACKET_FUNCTION if you want to provide
 * separate packet encryption and decryption functions.
 */
#define Sosemanuk_HAS_SINGLE_PACKET_FUNCTION     /* [edit] */
#ifdef Sosemanuk_HAS_SINGLE_PACKET_FUNCTION

#define Sosemanuk_encrypt_packet(                                        \
    ctx, iv, plaintext, ciphertext, mglen)                            \
  Sosemanuk_process_packet(0,                                            \
    ctx, iv, plaintext, ciphertext, mglen)

#define Sosemanuk_decrypt_packet(                                        \
    ctx, iv, ciphertext, plaintext, mglen)                            \
  Sosemanuk_process_packet(1,                                            \
    ctx, iv, ciphertext, plaintext, mglen)

void Sosemanuk_process_packet(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  Sosemanuk_ctx* ctx, 
  const u8* iv,
  const u8* input, 
  u8* output, 
  u32 msglen);

#else

void Sosemanuk_encrypt_packet(
  Sosemanuk_ctx* ctx, 
  const u8* iv,
  const u8* plaintext, 
  u8* ciphertext, 
  u32 msglen);

void Sosemanuk_decrypt_packet(
  Sosemanuk_ctx* ctx, 
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
 * Sosemanuk_USES_DEFAULT_BLOCK_MACROS flag and implement the functions
 * declared below.
 */

#define Sosemanuk_BLOCKLENGTH 80                /* [edit] */

#undef Sosemanuk_USES_DEFAULT_BLOCK_MACROS      /* [edit] */
#ifdef Sosemanuk_USES_DEFAULT_BLOCK_MACROS

#define Sosemanuk_encrypt_blocks(ctx, plaintext, ciphertext, blocks)  \
  Sosemanuk_encrypt_bytes(ctx, plaintext, ciphertext,                 \
    (blocks) * Sosemanuk_BLOCKLENGTH)

#define Sosemanuk_decrypt_blocks(ctx, ciphertext, plaintext, blocks)  \
  Sosemanuk_decrypt_bytes(ctx, ciphertext, plaintext,                 \
    (blocks) * Sosemanuk_BLOCKLENGTH)

#ifdef Sosemanuk_GENERATES_KEYSTREAM

#define Sosemanuk_keystream_blocks(ctx, keystream, blocks)            \
  Sosemanuk_keystream_bytes(ctx, keystream,                           \
    (blocks) * Sosemanuk_BLOCKLENGTH)

#endif

#else

/*
 * Undef Sosemanuk_HAS_SINGLE_BLOCK_FUNCTION if you want to provide
 * separate block encryption and decryption functions.
 */
#define Sosemanuk_HAS_SINGLE_BLOCK_FUNCTION      /* [edit] */
#ifdef Sosemanuk_HAS_SINGLE_BLOCK_FUNCTION

#define Sosemanuk_encrypt_blocks(ctx, plaintext, ciphertext, blocks)     \
  Sosemanuk_process_blocks(0, ctx, plaintext, ciphertext, blocks)

#define Sosemanuk_decrypt_blocks(ctx, ciphertext, plaintext, blocks)     \
  Sosemanuk_process_blocks(1, ctx, ciphertext, plaintext, blocks)

void Sosemanuk_process_blocks(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  Sosemanuk_ctx* ctx, 
  const u8* input, 
  u8* output, 
  u32 blocks);                /* Message length in blocks. */

#else

void Sosemanuk_encrypt_blocks(
  Sosemanuk_ctx* ctx, 
  const u8* plaintext, 
  u8* ciphertext, 
  u32 blocks);                /* Message length in blocks. */ 

void Sosemanuk_decrypt_blocks(
  Sosemanuk_ctx* ctx, 
  const u8* ciphertext, 
  u8* plaintext, 
  u32 blocks);                /* Message length in blocks. */ 

#endif

#ifdef Sosemanuk_GENERATES_KEYSTREAM

void Sosemanuk_keystream_blocks(
  Sosemanuk_ctx* ctx,
  u8* keystream,
  u32 blocks);                /* Keystream length in blocks. */ 

#endif

#endif

/*
 * If your cipher can be implemented in different ways, you can use
 * the Sosemanuk_VARIANT parameter to allow the user to choose between
 * them at compile time (e.g., gcc -DSosemanuk_VARIANT=3 ...). Please
 * only use this possibility if you really think it could make a
 * significant difference and keep the number of variants
 * (Sosemanuk_MAXVARIANT) as small as possible (definitely not more than
 * 10). Note also that all variants should have exactly the same
 * external interface (i.e., the same Sosemanuk_BLOCKLENGTH, etc.). 
 */
#define Sosemanuk_MAXVARIANT 1                   /* [edit] */

#ifndef Sosemanuk_VARIANT
#define Sosemanuk_VARIANT 1
#endif

#if (Sosemanuk_VARIANT > Sosemanuk_MAXVARIANT)
#error this variant does not exist
#endif

/* ------------------------------------------------------------------------- */

#endif
