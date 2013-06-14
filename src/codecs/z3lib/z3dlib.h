/*
 * z3lib (c)1993,2000,2005,2006/GPL,BSD Oskar Schirmer, schirmer@scara.com
 * combined huffman/lz compression library
 * now rfc1951 conformant
 */

#ifndef __Z3DLIB_H__
#define __Z3DLIB_H__

/* private stuff: */

#ifndef Z3LIB_DECODE_ONLY
#define Z3DE_SLICE_MIN 128
#define Z3DE_THRESHOLD_ONE 255

enum z3de_state {
  z3de_state_fill,
  z3de_state_drain,
  z3de_state_butlast,
  z3de_state_finish
};

struct z3de_handle {
  enum z3de_state state;
  __u32 tellfreq;
  __u32 tellnext;
  __u32 nclimit;
  __u32 leftover;
  __u32 lastlimit;
  int weights;
  struct z3be_weighing weight[2];
  __u32 srcmin, srcmax, trgmin, trgmax;
  __u32 initialgrant;
  __u8 thresholdmin, thresholdmax;
  struct z3be_handle z3be; /* must be last field */
};
#endif /* Z3LIB_DECODE_ONLY */

#ifndef Z3LIB_ENCODE_ONLY
struct z3dd_handle {
  struct z3bd_handle z3bd;
};
#endif /* Z3LIB_ENCODE_ONLY */

/* public stuff: */

#define Z3D_ALGORITHM_GNUZIP (1 << 1)

#ifndef Z3LIB_DECODE_ONLY
#define Z3DE_MEMSIZE_MIN (sizeof(struct z3de_handle))

/*
 * initialise compression
 * input:
 *    memory, memsize: memory reserved for the compressor, memsize must not
 *      be less then Z3DE_MEMSIZE_MIN (e.g. Z3DE_MEMSIZE_MIN + 65402)
 *    tellwhen: frequency for code size estimation and block close
 *      decision. 0 for none (i.e. full block usage always), minimum
 *      value is Z3DE_SLICE_MIN (e.g. 4096, but values larger
 *      than (memsize - Z3DE_MEMSIZE_MIN)/2 are not useful)
 *    thrmin: threshold for block close decision, in range
 *      0..Z3DE_THRESHOLD_ONE (on behalf of rational range 0..1); a block
 *      will be closed when compression ratio is less than all previously
 *      determined ratios times thrmin/Z3DE_THRESHOLD_ONE (e.g. 127)
 *    thrmax: similar to thrmin, a block will be closed, when compression ratio
 *      is greater than all previously determined ratios times
 *      Z3DE_THRESHOLD_ONE/thrmax (e.g. 153)
 *    initialgrant: for threshold comparision, all but the first slice are
 *      asumed to be incremented by this value to roughly compensate
 *      rfc 1951 block dynamic table size (e.g. 30)
 *    preferlonger: when non-zero, the compressor will try to find a longer
 *      match at n+1 and prefer it over a previous match at n
 *    limitlength3: when non-zero, then for codes with length 3 and large
 *      distance, check whether coding as literal is better
 */
struct z3de_handle *z3d_encode_init(void *memory, unsigned int memsize,
                        __u32 tellwhen,
                        __u8 thrmin, __u8 thrmax, __u32 initialgrant,
                        int preferlonger, int limitlength3);

/*
 * call z3d_encode repeatedly, until it returns NULL
 * input:
 *    data, datasize: a number of bytes that are available to compress
 *      datasize must be greater than 0 unless end of data is reached
 *    codesize: space available in code buffer
 * output:
 *    taken: number of bytes actually taken from data buffer,
 *      in range 0..datasize
 *    code: a number of compressed bytes are stored hereto
 *    given: the number of bytes stored into the code buffer,
 *      in range 0..codesize
 * return:
 *    NULL upon end of compression, whereupon all output variables are
 *    still valid
 */
struct z3de_handle *z3d_encode(struct z3de_handle *zh,
                        __u8 *data, __u32 datasize, __u32 *taken,
                        __u8 *code, __u32 codesize, __u32 *given);
#endif /* Z3LIB_DECODE_ONLY */

#ifndef Z3LIB_ENCODE_ONLY
#define Z3DD_MEMSIZE (sizeof(struct z3dd_handle))

/*
 * initialise decompression
 * input:
 *    pending_bits, pending_nb: a number of bits pending from previous
 *      decoding activity, and its number. Initially 0.
 *    memory, memsize: memory reserved for the decompressor, memsize shall
 *      be and must not be less than Z3DD_MEMSIZE
 * return:
 *    NULL upon error, a valid pointer otherwise
 */
struct z3dd_handle *z3d_decode_init(__u32 pending_bits, int pending_nb,
                        void *memory, unsigned int memsize);

/*
 * call z3d_decode repeatedly, until it returns NULL
 * input:
 *    code, codesize: a number of bytes that are available to decompress
 * output:
 *    error: error code, see z3lib.h
 *    pending_bits, pending_nb: a number of bits finally pending
 *    taken: the number of bytes actually taken from code, in range 0..codesize
 *    data: a pointer to a number of bytes readily decompressed
 *    given: the number of bytes available at the data pointer
 * return:
 *    NULL upon error or at end of compression, whereupon taken, data and given
 *    are not valid
 */
struct z3dd_handle *z3d_decode(struct z3dd_handle *zh, int *error,
                        __u32 *pending_bits, int *pending_nb,
                        __u8 *code, __u32 codesize, __u32 *taken,
                        __u8 **data, __u32 *given);

/*
 * for use with garbage collectable systems, one may want to use the
 * following decode function which returns a relative index instead of the
 * absolute data pointer.
 * output:
 *    index: an index into (char *)zh, where to find the output bytes.
 */
static inline struct z3dd_handle *z3d_decode_relative(struct z3dd_handle *zh,
                        int *error,
                        __u32 *pending_bits, int *pending_nb,
                        __u8 *code, __u32 codesize, __u32 *taken,
                        __u32 *index, __u32 *given)
{
  __u8 *data;
  struct z3dd_handle *r;
  r = z3d_decode(zh, error, pending_bits, pending_nb,
        code, codesize, taken, &data, given);
  if (r != NULL) {
    *index = data - (__u8*)zh;
  }
  return r;
}

#endif /* Z3LIB_ENCODE_ONLY */
#endif /* __Z3DLIB_H__ */
