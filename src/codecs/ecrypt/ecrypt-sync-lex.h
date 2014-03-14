/* ecrypt-sync.h */

/* 
 * Header file for synchronous stream ciphers without authentication
 * mechanism.
 * 
 * *** Please only edit parts marked with "[edit]". ***
 */

#ifndef Lex_SYNC
#define Lex_SYNC

#include "ecrypt-portable.h"

/* ------------------------------------------------------------------------- */

/* Cipher parameters */

/* 
 * The name of your cipher.
 */
#define Lex_NAME "LEX-v2"    /* [edit] */ 
#define Lex_PROFILE "S3___"

/*
 * Specify which key and IV sizes are supported by your cipher. A user
 * should be able to enumerate the supported sizes by running the
 * following code:
 *
 * for (i = 0; Lex_KEYSIZE(i) <= Lex_MAXKEYSIZE; ++i)
 *   {
 *     keysize = Lex_KEYSIZE(i);
 *
 *     ...
 *   }
 *
 * All sizes are in bits.
 */

#define Lex_MAXKEYSIZE 128                 /* [edit] */
#define Lex_KEYSIZE(i) (128 + (i)*64)      /* [edit] */

#define Lex_MAXIVSIZE 128                  /* [edit] */
#define Lex_IVSIZE(i) (128 + (i)*64)        /* [edit] */


/* ------------------------------------------------------------------------- */

/* Data structures */

/* 
 * Lex_ctx is the structure containing the representation of the
 * internal state of your cipher. 
 */

#define NUMBEROFROUNDS 10		/* Valid values are 10/12/14 for key sizes 128/192/256 */
#define NUMWORDS NUMBEROFROUNDS         

typedef struct
{
  u32 subkeys[4*(NUMBEROFROUNDS+1)];	/* Typically 10 round subkeys for 128-bit key Rijndael */
  u32 blockstate[4];			/* Intermediate state of a block-cipher, 128-bit block */
  u32 ks[NUMWORDS];			/* Keystream words collected from leaks */

} Lex_ctx;

/* ------------------------------------------------------------------------- */

/* Mandatory functions */

/*
 * Key and message independent initialization. This function will be
 * called once when the program starts (e.g., to build expanded S-box
 * tables).
 */
void Lex_init(void);

/*
 * Key setup. It is the user's responsibility to select the values of
 * keysize and ivsize from the set of supported values specified
 * above.
 */
void Lex_keysetup(
  Lex_ctx* ctx, 
  const u8* key, 
  u32 keysize,                /* Key size in bits. */ 
  u32 ivsize);                /* IV size in bits. */ 

/*
 * IV setup. After having called Lex_keysetup(), the user is
 * allowed to call Lex_ivsetup() different times in order to
 * encrypt/decrypt different messages with the same key but different
 * IV's.
 */
void Lex_ivsetup(
  Lex_ctx* ctx, 
  const u8* iv);

/*
 * Encryption/decryption of arbitrary length messages.
 *
 * For efficiency reasons, the API provides two types of
 * encrypt/decrypt functions. The Lex_encrypt_bytes() function
 * (declared here) encrypts byte strings of arbitrary length, while
 * the Lex_encrypt_blocks() function (defined later) only accepts
 * lengths which are multiples of Lex_BLOCKLENGTH.
 * 
 * The user is allowed to make multiple calls to
 * Lex_encrypt_blocks() to incrementally encrypt a long message,
 * but he is NOT allowed to make additional encryption calls once he
 * has called Lex_encrypt_bytes() (unless he starts a new message
 * of course). For example, this sequence of calls is acceptable:
 *
 * Lex_keysetup();
 *
 * Lex_ivsetup();
 * Lex_encrypt_blocks();
 * Lex_encrypt_blocks();
 * Lex_encrypt_bytes();
 *
 * Lex_ivsetup();
 * Lex_encrypt_blocks();
 * Lex_encrypt_blocks();
 *
 * Lex_ivsetup();
 * Lex_encrypt_bytes();
 * 
 * The following sequence is not:
 *
 * Lex_keysetup();
 * Lex_ivsetup();
 * Lex_encrypt_blocks();
 * Lex_encrypt_bytes();
 * Lex_encrypt_blocks();
 */

/*
 * By default Lex_encrypt_bytes() and Lex_decrypt_bytes() are
 * defined as macros which redirect the call to a single function
 * Lex_process_bytes(). If you want to provide separate encryption
 * and decryption functions, please undef
 * Lex_HAS_SINGLE_BYTE_FUNCTION.
 */
#define Lex_HAS_SINGLE_BYTE_FUNCTION       /* [edit] */
#ifdef Lex_HAS_SINGLE_BYTE_FUNCTION

#define Lex_encrypt_bytes(ctx, plaintext, ciphertext, msglen)   \
  Lex_process_bytes(0, ctx, plaintext, ciphertext, msglen)

#define Lex_decrypt_bytes(ctx, ciphertext, plaintext, msglen)   \
  Lex_process_bytes(1, ctx, ciphertext, plaintext, msglen)

void Lex_process_bytes(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  Lex_ctx* ctx, 
  const u8* input, 
  u8* output, 
  u32 msglen);                /* Message length in bytes. */ 

#else

void Lex_encrypt_bytes(
  Lex_ctx* ctx, 
  const u8* plaintext, 
  u8* ciphertext, 
  u32 msglen);                /* Message length in bytes. */ 

void Lex_decrypt_bytes(
  Lex_ctx* ctx, 
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
 * reset the Lex_GENERATES_KEYSTREAM flag.
 */

#define Lex_GENERATES_KEYSTREAM
#ifdef Lex_GENERATES_KEYSTREAM

void Lex_keystream_bytes(
  Lex_ctx* ctx,
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
 * undef the Lex_USES_DEFAULT_ALL_IN_ONE flag.
 */
#define Lex_USES_DEFAULT_ALL_IN_ONE        /* [edit] */

/*
 * Undef Lex_HAS_SINGLE_PACKET_FUNCTION if you want to provide
 * separate packet encryption and decryption functions.
 */
#define Lex_HAS_SINGLE_PACKET_FUNCTION     /* [edit] */
#ifdef Lex_HAS_SINGLE_PACKET_FUNCTION

#define Lex_encrypt_packet(                                        \
    ctx, iv, plaintext, ciphertext, mglen)                            \
  Lex_process_packet(0,                                            \
    ctx, iv, plaintext, ciphertext, mglen)

#define Lex_decrypt_packet(                                        \
    ctx, iv, ciphertext, plaintext, mglen)                            \
  Lex_process_packet(1,                                            \
    ctx, iv, ciphertext, plaintext, mglen)

void Lex_process_packet(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  Lex_ctx* ctx, 
  const u8* iv,
  const u8* input, 
  u8* output, 
  u32 msglen);

#else

void Lex_encrypt_packet(
  Lex_ctx* ctx, 
  const u8* iv,
  const u8* plaintext, 
  u8* ciphertext, 
  u32 msglen);

void Lex_decrypt_packet(
  Lex_ctx* ctx, 
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
 * Lex_USES_DEFAULT_BLOCK_MACROS flag and implement the functions
 * declared below.
 */

#define Lex_BLOCKLENGTH 40                  /* [edit] */

#define Lex_USES_DEFAULT_BLOCK_MACROS      /* [edit] */
#ifdef Lex_USES_DEFAULT_BLOCK_MACROS

#define Lex_encrypt_blocks(ctx, plaintext, ciphertext, blocks)  \
  Lex_encrypt_bytes(ctx, plaintext, ciphertext,                 \
    (blocks) * Lex_BLOCKLENGTH)

#define Lex_decrypt_blocks(ctx, ciphertext, plaintext, blocks)  \
  Lex_decrypt_bytes(ctx, ciphertext, plaintext,                 \
    (blocks) * Lex_BLOCKLENGTH)

#ifdef Lex_GENERATES_KEYSTREAM

#define Lex_keystream_blocks(ctx, keystream, blocks)            \
  Lex_keystream_bytes(ctx, keystream,                           \
    (blocks) * Lex_BLOCKLENGTH)

#endif

#else

/*
 * Undef Lex_HAS_SINGLE_BLOCK_FUNCTION if you want to provide
 * separate block encryption and decryption functions.
 */
#define Lex_HAS_SINGLE_BLOCK_FUNCTION      /* [edit] */
#ifdef Lex_HAS_SINGLE_BLOCK_FUNCTION

#define Lex_encrypt_blocks(ctx, plaintext, ciphertext, blocks)     \
  Lex_process_blocks(0, ctx, plaintext, ciphertext, blocks)

#define Lex_decrypt_blocks(ctx, ciphertext, plaintext, blocks)     \
  Lex_process_blocks(1, ctx, ciphertext, plaintext, blocks)

void Lex_process_blocks(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  Lex_ctx* ctx, 
  const u8* input, 
  u8* output, 
  u32 blocks);                /* Message length in blocks. */

#else

void Lex_encrypt_blocks(
  Lex_ctx* ctx, 
  const u8* plaintext, 
  u8* ciphertext, 
  u32 blocks);                /* Message length in blocks. */ 

void Lex_decrypt_blocks(
  Lex_ctx* ctx, 
  const u8* ciphertext, 
  u8* plaintext, 
  u32 blocks);                /* Message length in blocks. */ 

#endif

#ifdef Lex_GENERATES_KEYSTREAM

void Lex_keystream_blocks(
  Lex_ctx* ctx,
  u8* keystream,
  u32 blocks);                /* Keystream length in blocks. */ 

#endif

#endif

/*
 * If your cipher can be implemented in different ways, you can use
 * the Lex_VARIANT parameter to allow the user to choose between
 * them at compile time (e.g., gcc -DLex_VARIANT=3 ...). Please
 * only use this possibility if you really think it could make a
 * significant difference and keep the number of variants
 * (Lex_MAXVARIANT) as small as possible (definitely not more than
 * 10). Note also that all variants should have exactly the same
 * external interface (i.e., the same Lex_BLOCKLENGTH, etc.). 
 */
#define Lex_MAXVARIANT 1                   /* [edit] */

#ifndef Lex_VARIANT
#define Lex_VARIANT 1
#endif

#if (Lex_VARIANT > Lex_MAXVARIANT)
#error this variant does not exist
#endif

/* ------------------------------------------------------------------------- */

#endif
