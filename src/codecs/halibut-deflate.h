/*
 * Header file for my independent implementation of Deflate
 * (RFC1951) compression.
 */

#ifndef DEFLATE_DEFLATE_H
#define DEFLATE_DEFLATE_H

/*
 * Types of Deflate data stream.
 * 
 * DEFLATE_TYPE_BARE represents the basic Deflate data format, as
 * defined in RFC 1951. It has no checksum to detect errors and no
 * magic-number header for ease of recognition, but it does have
 * internal EOF indication.
 * 
 * DEFLATE_TYPE_ZLIB represents the zlib container format, as
 * defined in RFC 1950. It has a two-byte header, and a four-byte
 * Adler32 checksum at the end to verify correct decoding, but
 * apart from those six bytes it's exactly equivalent to
 * DEFLATE_TYPE_BARE.
 * 
 * DEFLATE_TYPE_GZIP represents the gzip compressed file format, as
 * defined in RFC 1952. This is a more full-featured format, with a
 * magic number, a CRC checksum of the compressed data, and various
 * header features including storing the original filename. This
 * implementation accepts but ignores all of those features on
 * input except the checksum, and outputs them in the most trivial
 * fashion. Also, this implementation will not decode multiple
 * concatenated gzip members (permitted by the RFC).
 */
enum {
    DEFLATE_TYPE_BARE,
    DEFLATE_TYPE_ZLIB,
    DEFLATE_TYPE_GZIP
};

/* ----------------------------------------------------------------------
 * Compression functions. Create a compression context with
 * deflate_compress_new(); feed it data with repeated calls to
 * deflate_compress_data(); destroy it with
 * deflate_compress_free().
 */

typedef struct deflate_compress_ctx deflate_compress_ctx;

/*
 * Create a new compression context. `type' indicates whether it's
 * bare Deflate (as used in, say, zip files) or Zlib (as used in,
 * say, PDF).
 */
deflate_compress_ctx *deflate_compress_new(int type);

/*
 * Free a compression context previously allocated by
 * deflate_compress_new().
 */
void deflate_compress_free(deflate_compress_ctx *ctx);

/*
 * Give the compression context some data to compress. The input
 * data is passed in `inblock', and has length `inlen'. This
 * function may or may not produce some output data; if so, it is
 * written to a dynamically allocated chunk of memory, a pointer to
 * that memory is stored in `outblock', and the length of output
 * data is stored in `outlen'. It is common for no data to be
 * output, if the input data has merely been stored in internal
 * buffers.
 * 
 * `flushtype' indicates whether you want to force buffered data to
 * be output. It can be one of the following values:
 * 
 *  - DEFLATE_NO_FLUSH: nothing is output if the compressor would
 *    rather not. Use this when the best compression is desired
 *    (i.e. most of the time).
 *
 *  - DEFLATE_SYNC_FLUSH: all the buffered data is output, but the
 *    compressed data stream remains open and ready to continue
 *    compressing data. Use this in interactive protocols when a
 *    single compressed data stream is split across several network
 *    packets.
 * 
 *  - DEFLATE_END_OF_DATA: all the buffered data is output and the
 *    compressed data stream is cleaned up. Any checksums required
 *    at the end of the stream are also output.
 */
void deflate_compress_data(deflate_compress_ctx *ctx,
			   const void *inblock, int inlen, int flushtype,
			   void **outblock, int *outlen);

enum {
    DEFLATE_NO_FLUSH,
    DEFLATE_SYNC_FLUSH,
    DEFLATE_END_OF_DATA
};

/* ----------------------------------------------------------------------
 * Decompression functions. Create a decompression context with
 * deflate_decompress_new(); feed it data with repeated calls to
 * deflate_decompress_data(); destroy it with
 * deflate_decompress_free().
 */

typedef struct deflate_decompress_ctx deflate_decompress_ctx;

/*
 * Create a new decompression context. `type' means the same as it
 * does in deflate_compress_new().
 */
deflate_decompress_ctx *deflate_decompress_new(int type);

/*
 * Free a decompression context previously allocated by
 * deflate_decompress_new().
 */
void deflate_decompress_free(deflate_decompress_ctx *ctx);

/*
 * Give the decompression context some data to decompress. The
 * input data is passed in `inblock', and has length `inlen'. This
 * function may or may not produce some output data; if so, it is
 * written to a dynamically allocated chunk of memory, a pointer to
 * that memory is stored in `outblock', and the length of output
 * data is stored in `outlen'.
 *
 * Returns 0 on success, or a non-zero error code if there was a
 * decoding error. In case of an error return, the data decoded
 * before the error is still returned as well. The possible errors
 * are listed below.
 * 
 * If you want to check that the compressed data stream was
 * correctly terminated, you can call this function with inlen==0
 * to signal input EOF and see if an error comes back. If you don't
 * care, don't bother.
 */
int deflate_decompress_data(deflate_decompress_ctx *ctx,
			    const void *inblock, int inlen,
			    void **outblock, int *outlen);

/*
 * Enumeration of error codes. The strange macro is so that I can
 * define description arrays in the accompanying source.
 */
#define DEFLATE_ERRORLIST(A) \
    A(DEFLATE_NO_ERR, "success"), \
    A(DEFLATE_ERR_ZLIB_HEADER, "invalid zlib header"), \
    A(DEFLATE_ERR_ZLIB_WRONGCOMP, "zlib header specifies non-deflate compression"), \
    A(DEFLATE_ERR_GZIP_HEADER, "invalid gzip header"), \
    A(DEFLATE_ERR_GZIP_WRONGCOMP, "gzip header specifies non-deflate compression"), \
    A(DEFLATE_ERR_GZIP_FHCRC, "gzip header specifies disputed FHCRC flag"), \
    A(DEFLATE_ERR_SMALL_HUFTABLE, "under-committed Huffman code space"), \
    A(DEFLATE_ERR_LARGE_HUFTABLE, "over-committed Huffman code space"), \
    A(DEFLATE_ERR_UNCOMP_HDR, "wrongly formatted header in uncompressed block"), \
    A(DEFLATE_ERR_NODISTTABLE, "backward copy encoded in block without distances table"), \
    A(DEFLATE_ERR_CHECKSUM, "incorrect data checksum"), \
    A(DEFLATE_ERR_INLEN, "incorrect data length"), \
    A(DEFLATE_ERR_UNEXPECTED_EOF, "unexpected end of data")
#define DEFLATE_ENUM_DEF(x,y) x
enum { DEFLATE_ERRORLIST(DEFLATE_ENUM_DEF), DEFLATE_NUM_ERRORS };
#undef DEFLATE_ENUM_DEF

/*
 * Arrays mapping the above error codes to, respectively, a text
 * error string and a textual representation of the symbolic error
 * code.
 */
extern const char *const deflate_error_msg[DEFLATE_NUM_ERRORS];
extern const char *const deflate_error_sym[DEFLATE_NUM_ERRORS];

#endif /* DEFLATE_DEFLATE_H */
