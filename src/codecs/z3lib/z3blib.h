/*
 * z3lib (c)1993,2000,2005,2006/GPL,BSD Oskar Schirmer, schirmer@scara.com
 * combined huffman/lz compression library
 * now rfc1951 conformant
 */

#ifndef __Z3BLIB_H__
#define __Z3BLIB_H__

/* private stuff: */

#define Z3B_DICTBITS 15
#define Z3B_CODEENDOFBLOCK 256
#define Z3B_NBLENGTHCODES 29
#define Z3B_NBCODES (Z3B_CODEENDOFBLOCK+1+Z3B_NBLENGTHCODES)
#define Z3B_NBCODESPLUSUNUSED (Z3B_NBCODES+2)
#define Z3B_NBDISTANCECODES 30
#define Z3B_NBDISTANCECODESPLUSUNUSED (Z3B_NBDISTANCECODES+2)
#define Z3B_NBCODELENGTHSCODES (16+3)
#define Z3B_LZMINLENGTH 3
#define Z3B_LZMAXLENGTH 258

#define Z3B_BTYPE_NC            0
#define Z3B_BTYPE_FIXED         1
#define Z3B_BTYPE_DYNAMIC       2
#define Z3B_NB_BTYPE            3

#ifndef Z3LIB_DECODE_ONLY
#define Z3BE_HASHBITS   12
#define Z3BE_HASHSIZE   ((1 << Z3BE_HASHBITS) + (1 << 8))
#define Z3BE_REF3TOOLARGE_MAX     8

enum z3be_state {
  z3be_state_init,
  z3be_state_nc,
  z3be_state_ncdata,
  z3be_state_clc,
  z3be_state_ll,
  z3be_state_dc,
  z3be_state_data,
  z3be_state_distance,
  z3be_state_distance2,
  z3be_state_end
};

struct z3be_histogram {
  __u32 literal[Z3B_NBCODES];
  __u32 distance[Z3B_NBDISTANCECODES];
};

struct z3be_handle {
  __u8 *data; /* reference into 2nd half of mem */
  __u32 dsize; /* maximum number of uncompressed data */
  __u32 dpos; /* current position up to which data is filled */
  __u32 dshift; /* shift hashtab against uncompressed data */
  __u32 ipos; /* position of input bytes still to be processed */
  __u32 pbits;
  int pbits_nb;
  enum z3be_state state;
  __u32 match; /* index of longest match found so far */
  __u32 mlen; /* length of longest match found so far */
  __u32 mrunl; /* in case of runlength, length of it minus 1 (2..257), else 0 */
  __u32 limitwp; /* limit for subsequent put given by last tell */
  __u32 count;
  __u32 lcode;
  struct z3be_handle_huff {
    __u16 *code;
    __u8 *size;
  } ll, dc;
  struct z3be_histogram histogram;
  __u32 longermatch;
  __u32 longermlen;
  __u32 *histogram3; /* when checking short refs for usability: [8][256] */
  __u8 runlength;
  __u8 bfinal;
  __s16 hashtab[Z3BE_HASHSIZE]; /* hash chains root table */
  __s16 hashforw[1 << Z3B_DICTBITS]; /* hash chains buffer */
  __u8 huff_dc_size[Z3B_NBDISTANCECODES];
  __u16 huff_dc_code[Z3B_NBDISTANCECODES];
  __u8 huff_ll_size[Z3B_NBCODESPLUSUNUSED];
  __u16 huff_ll_code[Z3B_NBCODESPLUSUNUSED];
  __u16 temp_dynh_i[3][Z3B_NBCODESPLUSUNUSED];
  __u32 temp_dynh_cumul[Z3B_NBCODESPLUSUNUSED-1];
  __u32 temp_tell_clcnt[Z3B_NBCODELENGTHSCODES];
  __u8 mem[(2 << Z3B_DICTBITS) + (1 << 10)]; /* must be last field */
};

struct z3be_weighing {
  int btype;
  __u32 wpos; /* number of input bytes taken into account for weighing */
  __u32 len[Z3B_NB_BTYPE]; /* size expected for the three types of coding */
  __u32 limit3; /* maximum distance for ref:length=3, longer go literal */
  /* dh sizes: */
  __u16 hlit;
  __u8 hdist;
  __u8 hclen;
  /* dh tables: */
  __u8 codelen_ll[Z3B_NBCODESPLUSUNUSED];
  __u8 codelen_dc[Z3B_NBDISTANCECODES];
  __u8 codelen_clc[Z3B_NBCODELENGTHSCODES];
  struct z3be_histogram histogram;
};
#endif /* Z3LIB_DECODE_ONLY */

#ifndef Z3LIB_ENCODE_ONLY
enum z3bd_state {
  z3bd_state_init,
  z3bd_state_nc,
  z3bd_state_nclen,
  z3bd_state_ncdata,
  z3bd_state_dh,
  z3bd_state_codelengths,
  z3bd_state_codeliterals,
  z3bd_state_codeliterals2,
  z3bd_state_codedistances,
  z3bd_state_codedistances2,
  z3bd_state_data,
  z3bd_state_data1,
  z3bd_state_literal,
  z3bd_state_length,
  z3bd_state_length2,
  z3bd_state_distance,
  z3bd_state_distance1,
  z3bd_state_distance2,
  z3bd_state_end
};

struct z3bd_handle {
  __u32 pbits;
  int pbits_nb;
  __u32 din; /* current input position */
  __u32 dout; /* current output position */
  enum z3bd_state state;
  int bfinal;
  int error;
  __u16 count;
  __u8 clen;
  __u32 value;
  __u32 distance;
  __u32 alternate;
  struct z3bd_handle_huff {
    __u16 *bit1;
    __u16 *bit0;
    __u8 *length;
    __u8 size[1 << 8];
    __u16 code[1 << 8];
  } ll, dc, clc;
  __u16 huff_ll_bit1[Z3B_NBCODESPLUSUNUSED];
  __u16 huff_ll_bit0[Z3B_NBCODESPLUSUNUSED];
  __u16 huff_dc_bit1[Z3B_NBDISTANCECODESPLUSUNUSED];
  __u16 huff_dc_bit0[Z3B_NBDISTANCECODESPLUSUNUSED];
  __u16 huff_clc_bit1[Z3B_NBCODELENGTHSCODES];
  __u16 huff_clc_bit0[Z3B_NBCODELENGTHSCODES];
  __u8 huff_ll_length[Z3B_NBCODESPLUSUNUSED];
  __u8 huff_dc_length[Z3B_NBDISTANCECODESPLUSUNUSED];
  __u8 huff_clc_length[Z3B_NBCODELENGTHSCODES];
  __u8 data[1 << Z3B_DICTBITS]; /* 32k string ring buffer */
};
#endif /* Z3LIB_ENCODE_ONLY */

/* public stuff: */

#ifndef Z3LIB_DECODE_ONLY
#define Z3BE_MEMSIZE_MIN (sizeof(struct z3be_handle))
#define Z3BE_MEMSIZE_EXTRA3 (Z3BE_REF3TOOLARGE_MAX*sizeof(__u32)*((1<<8)+2))
/*
 * start the compression of one or more rfc1951 blocks
 * input:
 *    memory, memsize: memory reserved for the compressor, memsize must
 *      not be less than Z3BE_MEMSIZE_MIN (plus Z3BE_MEMSIZE_EXTRA3, when
 *      limitlength3 is non-zero)
 *    preferlonger: when non-zero, the compressor will try to find a longer
 *      match at n+1 and prefer it over a previous match at n
 *    limitlength3: when non-zero, then for codes with length 3 and large
 *      distance, check whether coding as literal is better
 * return: handle on success, NULL otherwise
 */
struct z3be_handle *z3be_start(void *memory, unsigned int memsize,
                        int preferlonger, int limitlength3);

/*
 * fill an amount of data into the compressor
 * input:
 *    data, datasize: a number of bytes that are available to compress
 * return:
 *    the number of bytes actually taken, in range 0..datasize
 *    when 0, z3be_tell should be called
 */
__u32 z3be_put(struct z3be_handle *zh, __u8 *data, __u32 datasize);

/*
 * flush the internal compressor pipe
 * should be called when the last byte has been delivered through z3be_put
 */
void z3be_push(struct z3be_handle *zh);

/*
 * calculate expected codesize for current position
 * output:
 *    weighing: description of the weight for the current position
 *    inpipe: number of bytes currently in the internal compressor pipe
 * return:
 *    the number of bytes that will be accepted with subsequent calls
 *    of z3be_put. note, that anyhow z3be_put might ask to call z3be_tell
 *    again at several positions.
 *    when 0, the internal buffer is full and z3be_get should be called
 */
__u32 z3be_tell(struct z3be_handle *zh, struct z3be_weighing *weighing,
                        __u32 *inpipe);

/*
 * decide for last or previous weighing and get some code
 * input:
 *    weighing: the weighing determined by the last or by the
 *      next to last call to z3be_tell
 *    codesize: space available in code buffer
 * output:
 *    code: a number of compressed bytes are stored hereto
 * return:
 *    the number of bytes stored into the code buffer, in range 0..codesize
 *    when 0, the block is empty and the next block may be started
 *    using z3be_put
 */
__u32 z3be_get(struct z3be_handle *zh, struct z3be_weighing *weighing,
                        __u8 *code, __u32 codesize);

/*
 * flush all internal pipes and return last code byte
 * output:
 *    code: the last code byte will be stored hereto
 * return:
 *    the number of valid bits in the last byte
 */
__u32 z3be_finish(struct z3be_handle *zh, __u8 *code);
#endif /* Z3LIB_DECODE_ONLY */

#ifndef Z3LIB_ENCODE_ONLY
#define Z3BD_MEMSIZE (sizeof(struct z3bd_handle))
/*
 * start the decompression of one or more rfc1951 blocks
 * input:
 *    pending_bits, pending_nb: a number of bits possibly left over from
 *      the previous block; (0, 0) for none
 *    memory, memsize: memory reserved for the decompressor, memsize shall
 *      be and must not be less than Z3BD_MEMSIZE
 * return: handle on success, NULL otherwise
 */
struct z3bd_handle *z3bd_start(__u32 pending_bits, int pending_nb,
                        void *memory, unsigned int memsize);

/*
 * fill an amount of code into the decompressor
 * input:
 *    code, codesize: a number of bytes that are available to decompress
 * return:
 *    the number of bytes actually taken, in range 0..codesize
 *    on error, 0 is returned
 */
__u32 z3bd_put(struct z3bd_handle *zh, __u8 *code, __u32 codesize);

/*
 * pick up decompressed data from the decompressor
 * output:
 *    data: a pointer to a number of bytes readily decompressed,
 *      NULL on error and on end-of-block
 * return:
 *    the number of bytes available at **data,
 *      0 on error and on end-of-block
 * note:
 *    possibly there are more bytes available than denoted by the return
 *    value. thus, this function must be called repeatedly, until it return 0,
 *    to ensure no data will be lost when calling z3bd_put again
 */
__u32 z3bd_get(struct z3bd_handle *zh, __u8 **data);

/*
 * finish the decompression of an rfc1951 block
 * output:
 *    pending_bits, pending_nb: a number of bits left over from this block
 * return:
 *    if the last block was decompressed completely and without error and
 *    it was not the last block, z3err_bd_notbfinal is returned;
 *    otherwise, the handle is invalidated and the last error code is
 *    returned (z3err_none for none)
 * note:
 *    when this function returns z3err_bd_notbfinal, the user has the choice
 *    to either start over with the next block without calling z3bd_start,
 *    or to abort decompression by calling z3bd_finish once again
 */
int z3bd_finish(struct z3bd_handle *zh,
                        __u32 *pending_bits, int *pending_nb);
#endif /* Z3LIB_ENCODE_ONLY */
#endif /* __Z3BLIB_H__ */
