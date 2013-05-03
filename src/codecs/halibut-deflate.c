/*
 * Reimplementation of Deflate (RFC1951) compression. Adapted from
 * the version in PuTTY, and extended to write dynamic Huffman
 * trees and choose block boundaries usefully.
 */

/*
 * TODO:
 * 
 *  - Feature: could do with forms of flush other than SYNC_FLUSH.
 *    I'm not sure exactly how those work when you don't know in
 *    advance that your next block will be static (as we did in
 *    PuTTY). And remember the 9-bit limitation of zlib.
 *     + also, zlib has FULL_FLUSH which clears the LZ77 state as
 * 	 well, for random access.
 *
 *  - Compression quality: chooseblock() appears to be computing
 *    wildly inaccurate block size estimates. Possible resolutions:
 *     + find and fix some trivial bug I haven't spotted yet
 *     + abandon the entropic approximation and go with trial
 * 	 Huffman runs
 *
 *  - Compression quality: see if increasing SYMLIMIT causes
 *    dynamic blocks to start being consistently smaller than it.
 *     + actually we seem to be there already, but check on a
 * 	 larger corpus.
 *
 *  - Compression quality: we ought to be able to fall right back
 *    to actual uncompressed blocks if really necessary, though
 *    it's not clear what the criterion for doing so would be.
 */

/*
 * This software is copyright 2000-2006 Simon Tatham.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "halibut-deflate.h"

#define snew(type) ( (type *) malloc(sizeof(type)) )
#define snewn(n, type) ( (type *) malloc((n) * sizeof(type)) )
#define sresize(x, n, type) ( (type *) realloc((x), (n) * sizeof(type)) )
#define sfree(x) ( free((x)) )

#define lenof(x) (sizeof((x)) / sizeof(*(x)))

#ifndef FALSE
#define FALSE 0
#define TRUE (!FALSE)
#endif

/* ----------------------------------------------------------------------
 * This file can be compiled in a number of modes.
 * 
 * With -DSTANDALONE, it builds a self-contained deflate tool which
 * can compress, decompress, and also analyse a deflated file to
 * print out the sequence of literals and copy commands it
 * contains.
 * 
 * With -DTESTMODE, it builds a test application which is given a
 * file on standard input, both compresses and decompresses it, and
 * outputs the re-decompressed result so it can be conveniently
 * diffed against the original. Define -DTESTDBG as well for lots
 * of diagnostics.
 */

#if defined TESTDBG
/* gcc-specific diagnostic macro */
#define debug_int(x...) ( fprintf(stderr, x) )
#define debug(x) ( debug_int x )
#else
#define debug(x) ((void)0)
#endif

#ifdef STANDALONE
#define ANALYSIS
#endif

#ifdef ANALYSIS
int analyse_level = 0;
#endif

/* ----------------------------------------------------------------------
 * Basic LZ77 code. This bit is designed modularly, so it could be
 * ripped out and used in a different LZ77 compressor. Go to it,
 * and good luck :-)
 */

struct LZ77InternalContext;
struct LZ77Context {
    struct LZ77InternalContext *ictx;
    void *userdata;
    void (*literal) (struct LZ77Context * ctx, unsigned char c);
    void (*match) (struct LZ77Context * ctx, int distance, int len);
};

/*
 * Initialise the private fields of an LZ77Context. It's up to the
 * user to initialise the public fields.
 */
static int lz77_init(struct LZ77Context *ctx);

/*
 * Supply data to be compressed. Will update the private fields of
 * the LZ77Context, and will call literal() and match() to output.
 * If `compress' is FALSE, it will never emit a match, but will
 * instead call literal() for everything.
 */
static void lz77_compress(struct LZ77Context *ctx,
			  const unsigned char *data, int len, int compress);

/*
 * Modifiable parameters.
 */
#define WINSIZE 32768		       /* window size. Must be power of 2! */
#define HASHMAX 2039		       /* one more than max hash value */
#define MAXMATCH 32		       /* how many matches we track */
#define HASHCHARS 3		       /* how many chars make a hash */

/*
 * This compressor takes a less slapdash approach than the
 * gzip/zlib one. Rather than allowing our hash chains to fall into
 * disuse near the far end, we keep them doubly linked so we can
 * _find_ the far end, and then every time we add a new byte to the
 * window (thus rolling round by one and removing the previous
 * byte), we can carefully remove the hash chain entry.
 */

#define INVALID -1		       /* invalid hash _and_ invalid offset */
struct WindowEntry {
    short next, prev;		       /* array indices within the window */
    short hashval;
};

struct HashEntry {
    short first;		       /* window index of first in chain */
};

struct Match {
    int distance, len;
};

struct LZ77InternalContext {
    struct WindowEntry win[WINSIZE];
    unsigned char data[WINSIZE];
    int winpos;
    struct HashEntry hashtab[HASHMAX];
    unsigned char pending[HASHCHARS];
    int npending;
};

static int lz77_hash(const unsigned char *data)
{
    return (257 * data[0] + 263 * data[1] + 269 * data[2]) % HASHMAX;
}

static int lz77_init(struct LZ77Context *ctx)
{
    struct LZ77InternalContext *st;
    int i;

    st = snew(struct LZ77InternalContext);
    if (!st)
	return 0;

    ctx->ictx = st;

    for (i = 0; i < WINSIZE; i++)
	st->win[i].next = st->win[i].prev = st->win[i].hashval = INVALID;
    for (i = 0; i < HASHMAX; i++)
	st->hashtab[i].first = INVALID;
    st->winpos = 0;

    st->npending = 0;

    return 1;
}

static void lz77_advance(struct LZ77InternalContext *st,
			 unsigned char c, int hash)
{
    int off;

    /*
     * Remove the hash entry at winpos from the tail of its chain,
     * or empty the chain if it's the only thing on the chain.
     */
    if (st->win[st->winpos].prev != INVALID) {
	st->win[st->win[st->winpos].prev].next = INVALID;
    } else if (st->win[st->winpos].hashval != INVALID) {
	st->hashtab[st->win[st->winpos].hashval].first = INVALID;
    }

    /*
     * Create a new entry at winpos and add it to the head of its
     * hash chain.
     */
    st->win[st->winpos].hashval = hash;
    st->win[st->winpos].prev = INVALID;
    off = st->win[st->winpos].next = st->hashtab[hash].first;
    st->hashtab[hash].first = st->winpos;
    if (off != INVALID)
	st->win[off].prev = st->winpos;
    st->data[st->winpos] = c;

    /*
     * Advance the window pointer.
     */
    st->winpos = (st->winpos + 1) & (WINSIZE - 1);
}

#define CHARAT(k) ( (k)<0 ? st->data[(st->winpos+k)&(WINSIZE-1)] : data[k] )

static void lz77_compress(struct LZ77Context *ctx,
			  const unsigned char *data, int len, int compress)
{
    struct LZ77InternalContext *st = ctx->ictx;
    int i, hash, distance, off, nmatch, matchlen, advance;
    struct Match defermatch, matches[MAXMATCH];
    int deferchr;

    /*
     * Add any pending characters from last time to the window. (We
     * might not be able to.)
     */
    for (i = 0; i < st->npending; i++) {
	unsigned char foo[HASHCHARS];
	int j;
	if (len + st->npending - i < HASHCHARS) {
	    /* Update the pending array. */
	    for (j = i; j < st->npending; j++)
		st->pending[j - i] = st->pending[j];
	    break;
	}
	for (j = 0; j < HASHCHARS; j++)
	    foo[j] = (i + j < st->npending ? st->pending[i + j] :
		      data[i + j - st->npending]);
	lz77_advance(st, foo[0], lz77_hash(foo));
    }
    st->npending -= i;

    defermatch.len = 0;
    deferchr = '\0';
    while (len > 0) {

	/* Don't even look for a match, if we're not compressing. */
	if (compress && len >= HASHCHARS) {
	    /*
	     * Hash the next few characters.
	     */
	    hash = lz77_hash(data);

	    /*
	     * Look the hash up in the corresponding hash chain and see
	     * what we can find.
	     */
	    nmatch = 0;
	    for (off = st->hashtab[hash].first;
		 off != INVALID; off = st->win[off].next) {
		/* distance = 1       if off == st->winpos-1 */
		/* distance = WINSIZE if off == st->winpos   */
		distance =
		    WINSIZE - (off + WINSIZE - st->winpos) % WINSIZE;
		for (i = 0; i < HASHCHARS; i++)
		    if (CHARAT(i) != CHARAT(i - distance))
			break;
		if (i == HASHCHARS) {
		    matches[nmatch].distance = distance;
		    matches[nmatch].len = 3;
		    if (++nmatch >= MAXMATCH)
			break;
		}
	    }
	} else {
	    nmatch = 0;
	    hash = INVALID;
	}

	if (nmatch > 0) {
	    /*
	     * We've now filled up matches[] with nmatch potential
	     * matches. Follow them down to find the longest. (We
	     * assume here that it's always worth favouring a
	     * longer match over a shorter one.)
	     */
	    matchlen = HASHCHARS;
	    while (matchlen < len) {
		int j;
		for (i = j = 0; i < nmatch; i++) {
		    if (CHARAT(matchlen) ==
			CHARAT(matchlen - matches[i].distance)) {
			matches[j++] = matches[i];
		    }
		}
		if (j == 0)
		    break;
		matchlen++;
		nmatch = j;
	    }

	    /*
	     * We've now got all the longest matches. We favour the
	     * shorter distances, which means we go with matches[0].
	     * So see if we want to defer it or throw it away.
	     */
	    matches[0].len = matchlen;
	    if (defermatch.len > 0) {
		if (matches[0].len > defermatch.len + 1) {
		    /* We have a better match. Emit the deferred char,
		     * and defer this match. */
		    ctx->literal(ctx, (unsigned char) deferchr);
		    defermatch = matches[0];
		    deferchr = data[0];
		    advance = 1;
		} else {
		    /* We don't have a better match. Do the deferred one. */
		    ctx->match(ctx, defermatch.distance, defermatch.len);
		    advance = defermatch.len - 1;
		    defermatch.len = 0;
		}
	    } else {
		/* There was no deferred match. Defer this one. */
		defermatch = matches[0];
		deferchr = data[0];
		advance = 1;
	    }
	} else {
	    /*
	     * We found no matches. Emit the deferred match, if
	     * any; otherwise emit a literal.
	     */
	    if (defermatch.len > 0) {
		ctx->match(ctx, defermatch.distance, defermatch.len);
		advance = defermatch.len - 1;
		defermatch.len = 0;
	    } else {
		ctx->literal(ctx, data[0]);
		advance = 1;
	    }
	}

	/*
	 * Now advance the position by `advance' characters,
	 * keeping the window and hash chains consistent.
	 */
	while (advance > 0) {
	    if (len >= HASHCHARS) {
		lz77_advance(st, *data, lz77_hash(data));
	    } else {
		st->pending[st->npending++] = *data;
	    }
	    data++;
	    len--;
	    advance--;
	}
    }
}

/* ----------------------------------------------------------------------
 * Deflate functionality common to both compression and decompression.
 */

static const unsigned char lenlenmap[] = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

#define MAXCODELEN 16

/*
 * Given a sequence of Huffman code lengths, compute the actual
 * codes, in the final form suitable for feeding to outbits (i.e.
 * already bit-mirrored).
 *
 * Returns the maximum code length found. Can also return -1 to
 * indicate the table was overcommitted (too many or too short
 * codes to exactly cover the possible space), or -2 to indicate it
 * was undercommitted (too few or too long codes).
 */
static int hufcodes(const unsigned char *lengths, int *codes, int nsyms)
{
    int count[MAXCODELEN], startcode[MAXCODELEN];
    int code, maxlen;
    int i, j;

    /* Count the codes of each length. */
    maxlen = 0;
    for (i = 1; i < MAXCODELEN; i++)
	count[i] = 0;
    for (i = 0; i < nsyms; i++) {
	count[lengths[i]]++;
	if (maxlen < lengths[i])
	    maxlen = lengths[i];
    }
    /* Determine the starting code for each length block. */
    code = 0;
    for (i = 1; i < MAXCODELEN; i++) {
	startcode[i] = code;
	code += count[i];
	if (code > (1 << i))
	    maxlen = -1;	       /* overcommitted */
	code <<= 1;
    }
    if (code < (1 << MAXCODELEN))
	maxlen = -2;		       /* undercommitted */
    /* Determine the code for each symbol. Mirrored, of course. */
    for (i = 0; i < nsyms; i++) {
	code = startcode[lengths[i]]++;
	codes[i] = 0;
	for (j = 0; j < lengths[i]; j++) {
	    codes[i] = (codes[i] << 1) | (code & 1);
	    code >>= 1;
	}
    }

    return maxlen;
}

/*
 * Adler32 checksum function.
 */
static unsigned long adler32_update(unsigned long s,
				    const unsigned char *data, int len)
{
    unsigned s1 = s & 0xFFFF, s2 = (s >> 16) & 0xFFFF;
    int i;

    for (i = 0; i < len; i++) {
	s1 += data[i];
	s2 += s1;
	if (!(i & 0xFFF)) {
	    s1 %= 65521;
	    s2 %= 65521;
	}
    }

    return ((s2 % 65521) << 16) | (s1 % 65521);
}

/*
 * CRC32 checksum function.
 */

static unsigned long crc32_update(unsigned long crcword,
				  const unsigned char *data, int len)
{
    static const unsigned long crc32_table[256] = {
	0x00000000L, 0x77073096L, 0xEE0E612CL, 0x990951BAL,
	0x076DC419L, 0x706AF48FL, 0xE963A535L, 0x9E6495A3L,
	0x0EDB8832L, 0x79DCB8A4L, 0xE0D5E91EL, 0x97D2D988L,
	0x09B64C2BL, 0x7EB17CBDL, 0xE7B82D07L, 0x90BF1D91L,
	0x1DB71064L, 0x6AB020F2L, 0xF3B97148L, 0x84BE41DEL,
	0x1ADAD47DL, 0x6DDDE4EBL, 0xF4D4B551L, 0x83D385C7L,
	0x136C9856L, 0x646BA8C0L, 0xFD62F97AL, 0x8A65C9ECL,
	0x14015C4FL, 0x63066CD9L, 0xFA0F3D63L, 0x8D080DF5L,
	0x3B6E20C8L, 0x4C69105EL, 0xD56041E4L, 0xA2677172L,
	0x3C03E4D1L, 0x4B04D447L, 0xD20D85FDL, 0xA50AB56BL,
	0x35B5A8FAL, 0x42B2986CL, 0xDBBBC9D6L, 0xACBCF940L,
	0x32D86CE3L, 0x45DF5C75L, 0xDCD60DCFL, 0xABD13D59L,
	0x26D930ACL, 0x51DE003AL, 0xC8D75180L, 0xBFD06116L,
	0x21B4F4B5L, 0x56B3C423L, 0xCFBA9599L, 0xB8BDA50FL,
	0x2802B89EL, 0x5F058808L, 0xC60CD9B2L, 0xB10BE924L,
	0x2F6F7C87L, 0x58684C11L, 0xC1611DABL, 0xB6662D3DL,
	0x76DC4190L, 0x01DB7106L, 0x98D220BCL, 0xEFD5102AL,
	0x71B18589L, 0x06B6B51FL, 0x9FBFE4A5L, 0xE8B8D433L,
	0x7807C9A2L, 0x0F00F934L, 0x9609A88EL, 0xE10E9818L,
	0x7F6A0DBBL, 0x086D3D2DL, 0x91646C97L, 0xE6635C01L,
	0x6B6B51F4L, 0x1C6C6162L, 0x856530D8L, 0xF262004EL,
	0x6C0695EDL, 0x1B01A57BL, 0x8208F4C1L, 0xF50FC457L,
	0x65B0D9C6L, 0x12B7E950L, 0x8BBEB8EAL, 0xFCB9887CL,
	0x62DD1DDFL, 0x15DA2D49L, 0x8CD37CF3L, 0xFBD44C65L,
	0x4DB26158L, 0x3AB551CEL, 0xA3BC0074L, 0xD4BB30E2L,
	0x4ADFA541L, 0x3DD895D7L, 0xA4D1C46DL, 0xD3D6F4FBL,
	0x4369E96AL, 0x346ED9FCL, 0xAD678846L, 0xDA60B8D0L,
	0x44042D73L, 0x33031DE5L, 0xAA0A4C5FL, 0xDD0D7CC9L,
	0x5005713CL, 0x270241AAL, 0xBE0B1010L, 0xC90C2086L,
	0x5768B525L, 0x206F85B3L, 0xB966D409L, 0xCE61E49FL,
	0x5EDEF90EL, 0x29D9C998L, 0xB0D09822L, 0xC7D7A8B4L,
	0x59B33D17L, 0x2EB40D81L, 0xB7BD5C3BL, 0xC0BA6CADL,
	0xEDB88320L, 0x9ABFB3B6L, 0x03B6E20CL, 0x74B1D29AL,
	0xEAD54739L, 0x9DD277AFL, 0x04DB2615L, 0x73DC1683L,
	0xE3630B12L, 0x94643B84L, 0x0D6D6A3EL, 0x7A6A5AA8L,
	0xE40ECF0BL, 0x9309FF9DL, 0x0A00AE27L, 0x7D079EB1L,
	0xF00F9344L, 0x8708A3D2L, 0x1E01F268L, 0x6906C2FEL,
	0xF762575DL, 0x806567CBL, 0x196C3671L, 0x6E6B06E7L,
	0xFED41B76L, 0x89D32BE0L, 0x10DA7A5AL, 0x67DD4ACCL,
	0xF9B9DF6FL, 0x8EBEEFF9L, 0x17B7BE43L, 0x60B08ED5L,
	0xD6D6A3E8L, 0xA1D1937EL, 0x38D8C2C4L, 0x4FDFF252L,
	0xD1BB67F1L, 0xA6BC5767L, 0x3FB506DDL, 0x48B2364BL,
	0xD80D2BDAL, 0xAF0A1B4CL, 0x36034AF6L, 0x41047A60L,
	0xDF60EFC3L, 0xA867DF55L, 0x316E8EEFL, 0x4669BE79L,
	0xCB61B38CL, 0xBC66831AL, 0x256FD2A0L, 0x5268E236L,
	0xCC0C7795L, 0xBB0B4703L, 0x220216B9L, 0x5505262FL,
	0xC5BA3BBEL, 0xB2BD0B28L, 0x2BB45A92L, 0x5CB36A04L,
	0xC2D7FFA7L, 0xB5D0CF31L, 0x2CD99E8BL, 0x5BDEAE1DL,
	0x9B64C2B0L, 0xEC63F226L, 0x756AA39CL, 0x026D930AL,
	0x9C0906A9L, 0xEB0E363FL, 0x72076785L, 0x05005713L,
	0x95BF4A82L, 0xE2B87A14L, 0x7BB12BAEL, 0x0CB61B38L,
	0x92D28E9BL, 0xE5D5BE0DL, 0x7CDCEFB7L, 0x0BDBDF21L,
	0x86D3D2D4L, 0xF1D4E242L, 0x68DDB3F8L, 0x1FDA836EL,
	0x81BE16CDL, 0xF6B9265BL, 0x6FB077E1L, 0x18B74777L,
	0x88085AE6L, 0xFF0F6A70L, 0x66063BCAL, 0x11010B5CL,
	0x8F659EFFL, 0xF862AE69L, 0x616BFFD3L, 0x166CCF45L,
	0xA00AE278L, 0xD70DD2EEL, 0x4E048354L, 0x3903B3C2L,
	0xA7672661L, 0xD06016F7L, 0x4969474DL, 0x3E6E77DBL,
	0xAED16A4AL, 0xD9D65ADCL, 0x40DF0B66L, 0x37D83BF0L,
	0xA9BCAE53L, 0xDEBB9EC5L, 0x47B2CF7FL, 0x30B5FFE9L,
	0xBDBDF21CL, 0xCABAC28AL, 0x53B39330L, 0x24B4A3A6L,
	0xBAD03605L, 0xCDD70693L, 0x54DE5729L, 0x23D967BFL,
	0xB3667A2EL, 0xC4614AB8L, 0x5D681B02L, 0x2A6F2B94L,
	0xB40BBE37L, 0xC30C8EA1L, 0x5A05DF1BL, 0x2D02EF8DL
    };
    crcword ^= 0xFFFFFFFFL;
    while (len--) {
	unsigned long newbyte = *data++;
	newbyte ^= crcword & 0xFFL;
	crcword = (crcword >> 8) ^ crc32_table[newbyte];
    }
    return crcword ^ 0xFFFFFFFFL;
}

typedef struct {
    short code, extrabits;
    int min, max;
} coderecord;

static const coderecord lencodes[] = {
    {257, 0, 3, 3},
    {258, 0, 4, 4},
    {259, 0, 5, 5},
    {260, 0, 6, 6},
    {261, 0, 7, 7},
    {262, 0, 8, 8},
    {263, 0, 9, 9},
    {264, 0, 10, 10},
    {265, 1, 11, 12},
    {266, 1, 13, 14},
    {267, 1, 15, 16},
    {268, 1, 17, 18},
    {269, 2, 19, 22},
    {270, 2, 23, 26},
    {271, 2, 27, 30},
    {272, 2, 31, 34},
    {273, 3, 35, 42},
    {274, 3, 43, 50},
    {275, 3, 51, 58},
    {276, 3, 59, 66},
    {277, 4, 67, 82},
    {278, 4, 83, 98},
    {279, 4, 99, 114},
    {280, 4, 115, 130},
    {281, 5, 131, 162},
    {282, 5, 163, 194},
    {283, 5, 195, 226},
    {284, 5, 227, 257},
    {285, 0, 258, 258},
};

static const coderecord distcodes[] = {
    {0, 0, 1, 1},
    {1, 0, 2, 2},
    {2, 0, 3, 3},
    {3, 0, 4, 4},
    {4, 1, 5, 6},
    {5, 1, 7, 8},
    {6, 2, 9, 12},
    {7, 2, 13, 16},
    {8, 3, 17, 24},
    {9, 3, 25, 32},
    {10, 4, 33, 48},
    {11, 4, 49, 64},
    {12, 5, 65, 96},
    {13, 5, 97, 128},
    {14, 6, 129, 192},
    {15, 6, 193, 256},
    {16, 7, 257, 384},
    {17, 7, 385, 512},
    {18, 8, 513, 768},
    {19, 8, 769, 1024},
    {20, 9, 1025, 1536},
    {21, 9, 1537, 2048},
    {22, 10, 2049, 3072},
    {23, 10, 3073, 4096},
    {24, 11, 4097, 6144},
    {25, 11, 6145, 8192},
    {26, 12, 8193, 12288},
    {27, 12, 12289, 16384},
    {28, 13, 16385, 24576},
    {29, 13, 24577, 32768},
};

/* ----------------------------------------------------------------------
 * Deflate compression.
 */

#define SYMLIMIT 65536
#define SYMPFX_LITLEN    0x00000000U
#define SYMPFX_DIST      0x40000000U
#define SYMPFX_EXTRABITS 0x80000000U
#define SYMPFX_CODELEN   0xC0000000U
#define SYMPFX_MASK      0xC0000000U

#define SYM_EXTRABITS_MASK 0x3C000000U
#define SYM_EXTRABITS_SHIFT 26

struct huftrees {
    unsigned char *len_litlen;
    int *code_litlen;
    unsigned char *len_dist;
    int *code_dist;
    unsigned char *len_codelen;
    int *code_codelen;
};

struct deflate_compress_ctx {
    struct LZ77Context *lzc;
    unsigned char *outbuf;
    int outlen, outsize;
    unsigned long outbits;
    int noutbits;
    int firstblock;
    unsigned long *syms;
    int symstart, nsyms;
    int type;
    unsigned long checksum;
    unsigned long datasize;
    int lastblock;
    int finished;
    unsigned char static_len1[288], static_len2[30];
    int static_code1[288], static_code2[30];
    struct huftrees sht;
#ifdef STATISTICS
    unsigned long bitcount;
#endif
};

static void outbits(deflate_compress_ctx *out,
		    unsigned long bits, int nbits)
{
    assert(out->noutbits + nbits <= 32);
    out->outbits |= bits << out->noutbits;
    out->noutbits += nbits;
    while (out->noutbits >= 8) {
	if (out->outlen >= out->outsize) {
	    out->outsize = out->outlen + 64;
	    out->outbuf = sresize(out->outbuf, out->outsize, unsigned char);
	}
	out->outbuf[out->outlen++] = (unsigned char) (out->outbits & 0xFF);
	out->outbits >>= 8;
	out->noutbits -= 8;
    }
#ifdef STATISTICS
    out->bitcount += nbits;
#endif
}

/*
 * Binary heap functions used by buildhuf(). Each one assumes the
 * heap to be stored in an array of ints, with two ints per node
 * (user data and key). They take in the old heap length, and
 * return the new one.
 */
#define HEAPPARENT(x) (((x)-2)/4*2)
#define HEAPLEFT(x) ((x)*2+2)
#define HEAPRIGHT(x) ((x)*2+4)
static int addheap(int *heap, int len, int userdata, int key)
{
    int me, dad, tmp;

    me = len;
    heap[len++] = userdata;
    heap[len++] = key;

    while (me > 0) {
	dad = HEAPPARENT(me);
	if (heap[me+1] < heap[dad+1]) {
	    tmp = heap[me]; heap[me] = heap[dad]; heap[dad] = tmp;
	    tmp = heap[me+1]; heap[me+1] = heap[dad+1]; heap[dad+1] = tmp;
	    me = dad;
	} else
	    break;
    }

    return len;
}
static int rmheap(int *heap, int len, int *userdata, int *key)
{
    int me, lc, rc, c, tmp;

    len -= 2;
    *userdata = heap[0];
    *key = heap[1];
    heap[0] = heap[len];
    heap[1] = heap[len+1];

    me = 0;

    while (1) {
	lc = HEAPLEFT(me);
	rc = HEAPRIGHT(me);
	if (lc >= len)
	    break;
	else if (rc >= len || heap[lc+1] < heap[rc+1])
	    c = lc;
	else
	    c = rc;
	if (heap[me+1] > heap[c+1]) {
	    tmp = heap[me]; heap[me] = heap[c]; heap[c] = tmp;
	    tmp = heap[me+1]; heap[me+1] = heap[c+1]; heap[c+1] = tmp;
	} else
	    break;
	me = c;
    }

    return len;
}

/*
 * The core of the Huffman algorithm: takes an input array of
 * symbol frequencies, and produces an output array of code
 * lengths.
 *
 * This is basically a generic Huffman implementation, but it has
 * one zlib-related quirk which is that it caps the output code
 * lengths to fit in an unsigned char (which is safe since Deflate
 * will reject anything longer than 15 anyway). Anyone wanting to
 * rip it out and use it in another context should find that easy
 * to remove.
 */
#define HUFMAX 286
static void buildhuf(const int *freqs, unsigned char *lengths, int nsyms)
{
    int parent[2*HUFMAX-1];
    int length[2*HUFMAX-1];
    int heap[2*HUFMAX];
    int heapsize;
    int i, j, n;
    int si, sj;

    assert(nsyms <= HUFMAX);

    memset(parent, 0, sizeof(parent));

    /*
     * Begin by building the heap.
     */
    heapsize = 0;
    for (i = 0; i < nsyms; i++)
	if (freqs[i] > 0)	       /* leave unused symbols out totally */
	    heapsize = addheap(heap, heapsize, i, freqs[i]);

    /*
     * Now repeatedly take two elements off the heap and merge
     * them.
     */
    n = HUFMAX;
    while (heapsize > 2) {
	heapsize = rmheap(heap, heapsize, &i, &si);
	heapsize = rmheap(heap, heapsize, &j, &sj);
	parent[i] = n;
	parent[j] = n;
	heapsize = addheap(heap, heapsize, n, si + sj);
	n++;
    }

    /*
     * Now we have our tree, in the form of a link from each node
     * to the index of its parent. Count back down the tree to
     * determine the code lengths.
     */
    memset(length, 0, sizeof(length));
    /* The tree root has length 0 after that, which is correct. */
    for (i = n-1; i-- ;)
	if (parent[i] > 0)
	    length[i] = 1 + length[parent[i]];

    /*
     * And that's it. (Simple, wasn't it?) Copy the lengths into
     * the output array and leave.
     * 
     * Here we cap lengths to fit in unsigned char.
     */
    for (i = 0; i < nsyms; i++)
	lengths[i] = (length[i] > 255 ? 255 : length[i]);
}

/*
 * Wrapper around buildhuf() which enforces the Deflate restriction
 * that no code length may exceed 15 bits, or 7 for the auxiliary
 * code length alphabet. This function has the same calling
 * semantics as buildhuf(), except that it might modify the freqs
 * array.
 */
static void deflate_buildhuf(int *freqs, unsigned char *lengths,
			     int nsyms, int limit)
{
    int smallestfreq, totalfreq, nactivesyms;
    int num, denom, adjust;
    int i;
    int maxprob;

    /*
     * Nasty special case: if the frequency table has fewer than
     * two non-zero elements, we must invent some, because we can't
     * have fewer than one bit encoding a symbol.
     */
    assert(nsyms >= 2);
    {
	int count = 0;
	for (i = 0; i < nsyms; i++)
	    if (freqs[i] > 0)
		count++;
	if (count < 2) {
	    for (i = 0; i < nsyms && count > 0; i++)
		if (freqs[i] == 0) {
		    freqs[i] = 1;
		    count--;
		}
	}
    }

    /*
     * First, try building the Huffman table the normal way. If
     * this works, it's optimal, so we don't want to mess with it.
     */
    buildhuf(freqs, lengths, nsyms);

    for (i = 0; i < nsyms; i++)
	if (lengths[i] > limit)
	    break;

    if (i == nsyms)
	return;			       /* OK */

    /*
     * The Huffman algorithm can only ever generate a code length
     * of N bits or more if there is a symbol whose probability is
     * less than the reciprocal of the (N+2)th Fibonacci number
     * (counting from F_0=0 and F_1=1), i.e. 1/2584 for N=16, or
     * 1/55 for N=8. (This is a necessary though not sufficient
     * condition.)
     *
     * Why is this? Well, consider the input symbol with the
     * smallest probability. Let that probability be x. In order
     * for this symbol to have a code length of at least 1, the
     * Huffman algorithm will have to merge it with some other
     * node; and since x is the smallest probability, the node it
     * gets merged with must be at least x. Thus, the probability
     * of the resulting combined node will be at least 2x. Now in
     * order for our node to reach depth 2, this 2x-node must be
     * merged again. But what with? We can't assume the node it
     * merges with is at least 2x, because this one might only be
     * the _second_ smallest remaining node. But we do know the
     * node it merges with must be at least x, so our order-2
     * internal node is at least 3x.
     *
     * How small a node can merge with _that_ to get an order-3
     * internal node? Well, it must be at least 2x, because if it
     * was smaller than that then it would have been one of the two
     * smallest nodes in the previous step and been merged at that
     * point. So at least 3x, plus at least 2x, comes to at least
     * 5x for an order-3 node.
     *
     * And so it goes on: at every stage we must merge our current
     * node with a node at least as big as the bigger of this one's
     * two parents, and from this starting point that gives rise to
     * the Fibonacci sequence. So we find that in order to have a
     * node n levels deep (i.e. a maximum code length of n), the
     * overall probability of the root of the entire tree must be
     * at least F_{n+2} times the probability of the rarest symbol.
     * In other words, since the overall probability is 1, it is a
     * necessary condition for a code length of 16 or more that
     * there must be at least one symbol with probability <=
     * 1/F_18.
     *
     * (To demonstrate that a probability this big really can give
     * rise to a code length of 16, consider the set of input
     * frequencies { 1-epsilon, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55,
     * 89, 144, 233, 377, 610, 987 }, for arbitrarily small
     * epsilon.)
     *
     * So here buildhuf() has returned us an overlong code. So to
     * ensure it doesn't do it again, we add a constant to all the
     * (non-zero) symbol frequencies, causing them to become more
     * balanced and removing the danger. We can then feed the
     * results back to the standard buildhuf() and be
     * assert()-level confident that the resulting code lengths
     * contain nothing outside the permitted range.
     */
    assert(limit == 15 || limit == 7);
    maxprob = (limit == 15 ? 2584 : 55);   /* no point in computing full F_n */
    totalfreq = nactivesyms = 0;
    smallestfreq = -1;
    for (i = 0; i < nsyms; i++) {
	if (freqs[i] == 0)
	    continue;
	if (smallestfreq < 0 || smallestfreq > freqs[i])
	    smallestfreq = freqs[i];
	totalfreq += freqs[i];
	nactivesyms++;
    }
    assert(smallestfreq <= totalfreq / maxprob);

    /*
     * We want to find the smallest integer `adjust' such that
     * (totalfreq + nactivesyms * adjust) / (smallestfreq +
     * adjust) is less than maxprob. A bit of algebra tells us
     * that the threshold value is equal to
     *
     *   totalfreq - maxprob * smallestfreq
     *   ----------------------------------
     *          maxprob - nactivesyms
     *
     * rounded up, of course. And we'll only even be trying
     * this if
     */
    num = totalfreq - smallestfreq * maxprob;
    denom = maxprob - nactivesyms;
    adjust = (num + denom - 1) / denom;

    /*
     * Now add `adjust' to all the input symbol frequencies.
     */
    for (i = 0; i < nsyms; i++)
	if (freqs[i] != 0)
	    freqs[i] += adjust;

    /*
     * Rebuild the Huffman tree...
     */
    buildhuf(freqs, lengths, nsyms);

    /*
     * ... and this time it ought to be OK.
     */
    for (i = 0; i < nsyms; i++)
	assert(lengths[i] <= limit);
}

/*
 * Compute the bit length of a symbol, given the three Huffman
 * trees.
 */
static int symsize(unsigned sym, const struct huftrees *trees)
{
    unsigned basesym = sym &~ SYMPFX_MASK;

    switch (sym & SYMPFX_MASK) {
      case SYMPFX_LITLEN:
	return trees->len_litlen[basesym];
      case SYMPFX_DIST:
	return trees->len_dist[basesym];
      case SYMPFX_CODELEN:
	return trees->len_codelen[basesym];
      default /*case SYMPFX_EXTRABITS*/:
	return basesym >> SYM_EXTRABITS_SHIFT;
    }
}

/*
 * Write out a single symbol, given the three Huffman trees.
 */
static void writesym(deflate_compress_ctx *out,
		     unsigned sym, const struct huftrees *trees)
{
    unsigned basesym = sym &~ SYMPFX_MASK;
    int i;

    switch (sym & SYMPFX_MASK) {
      case SYMPFX_LITLEN:
	debug(("send: litlen %d\n", basesym));
	outbits(out, trees->code_litlen[basesym], trees->len_litlen[basesym]);
	break;
      case SYMPFX_DIST:
	debug(("send: dist %d\n", basesym));
	outbits(out, trees->code_dist[basesym], trees->len_dist[basesym]);
	break;
      case SYMPFX_CODELEN:
	debug(("send: codelen %d\n", basesym));
	outbits(out, trees->code_codelen[basesym],trees->len_codelen[basesym]);
	break;
      case SYMPFX_EXTRABITS:
	i = basesym >> SYM_EXTRABITS_SHIFT;
	basesym &= ~SYM_EXTRABITS_MASK;
	debug(("send: extrabits %d/%d\n", basesym, i));
	outbits(out, basesym, i);
	break;
    }
}

/*
 * outblock() must output _either_ a dynamic block of length
 * `dynamic_len', _or_ a static block of length `static_len', but
 * it gets to choose which.
 */
static void outblock(deflate_compress_ctx *out,
		     int dynamic_len, int static_len)
{
    int freqs1[286], freqs2[30], freqs3[19];
    unsigned char len1[286], len2[30], len3[19];
    int code1[286], code2[30], code3[19];
    int hlit, hdist, hclen, bfinal, btype;
    int treesrc[286 + 30];
    int treesyms[286 + 30];
    int codelen[19];
    int i, ntreesrc, ntreesyms;
    int dynamic, blklen;
    struct huftrees dht;
    const struct huftrees *ht;
#ifdef STATISTICS
    unsigned long bitcount_before;
#endif

    dht.len_litlen = len1;
    dht.len_dist = len2;
    dht.len_codelen = len3;
    dht.code_litlen = code1;
    dht.code_dist = code2;
    dht.code_codelen = code3;

    /*
     * We make our choice of block to output by doing all the
     * detailed work to determine the exact length of each possible
     * block. Then we choose the one which has fewest output bits
     * per symbol.
     */

    /*
     * First build the two main Huffman trees for the dynamic
     * block.
     */

    /*
     * Count up the frequency tables.
     */
    memset(freqs1, 0, sizeof(freqs1));
    memset(freqs2, 0, sizeof(freqs2));
    freqs1[256] = 1;	       /* we're bound to need one EOB */
    for (i = 0; i < dynamic_len; i++) {
	unsigned sym = out->syms[(out->symstart + i) % SYMLIMIT];

	/*
	 * Increment the occurrence counter for this symbol, if
	 * it's in one of the Huffman alphabets and isn't extra
	 * bits.
	 */
	if ((sym & SYMPFX_MASK) == SYMPFX_LITLEN) {
	    sym &= ~SYMPFX_MASK;
	    assert(sym < lenof(freqs1));
	    freqs1[sym]++;
	} else if ((sym & SYMPFX_MASK) == SYMPFX_DIST) {
	    sym &= ~SYMPFX_MASK;
	    assert(sym < lenof(freqs2));
	    freqs2[sym]++;
	}
    }
    deflate_buildhuf(freqs1, len1, lenof(freqs1), 15);
    deflate_buildhuf(freqs2, len2, lenof(freqs2), 15);
    hufcodes(len1, code1, lenof(freqs1));
    hufcodes(len2, code2, lenof(freqs2));

    /*
     * Determine HLIT and HDIST.
     */
    for (hlit = 286; hlit > 257 && len1[hlit-1] == 0; hlit--);
    for (hdist = 30; hdist > 1 && len2[hdist-1] == 0; hdist--);

    /*
     * Write out the list of symbols used to transmit the
     * trees.
     */
    ntreesrc = 0;
    for (i = 0; i < hlit; i++)
	treesrc[ntreesrc++] = len1[i];
    for (i = 0; i < hdist; i++)
	treesrc[ntreesrc++] = len2[i];
    ntreesyms = 0;
    for (i = 0; i < ntreesrc ;) {
	int j = 1;
	int k;

	/* Find length of run of the same length code. */
	while (i+j < ntreesrc && treesrc[i+j] == treesrc[i])
	    j++;

	/* Encode that run as economically as we can. */
	k = j;
	if (treesrc[i] == 0) {
	    /*
	     * Zero code length: we can output run codes for
	     * 3-138 zeroes. So if we have fewer than 3 zeroes,
	     * we just output literals. Otherwise, we output
	     * nothing but run codes, and tweak their lengths
	     * to make sure we aren't left with under 3 at the
	     * end.
	     */
	    if (k < 3) {
		while (k--)
		    treesyms[ntreesyms++] = 0 | SYMPFX_CODELEN;
	    } else {
		while (k > 0) {
		    int rpt = (k < 138 ? k : 138);
		    if (rpt > k-3 && rpt < k)
			rpt = k-3;
		    assert(rpt >= 3 && rpt <= 138);
		    if (rpt < 11) {
			treesyms[ntreesyms++] = 17 | SYMPFX_CODELEN;
			treesyms[ntreesyms++] =
			    (SYMPFX_EXTRABITS | (rpt - 3) |
			     (3 << SYM_EXTRABITS_SHIFT));
		    } else {
			treesyms[ntreesyms++] = 18 | SYMPFX_CODELEN;
			treesyms[ntreesyms++] =
			    (SYMPFX_EXTRABITS | (rpt - 11) |
			     (7 << SYM_EXTRABITS_SHIFT));
		    }
		    k -= rpt;
		}
	    }
	} else {
	    /*
	     * Non-zero code length: we must output the first
	     * one explicitly, then we can output a copy code
	     * for 3-6 repeats. So if we have fewer than 4
	     * repeats, we _just_ output literals. Otherwise,
	     * we output one literal plus at least one copy
	     * code, and tweak the copy codes to make sure we
	     * aren't left with under 3 at the end.
	     */
	    assert(treesrc[i] < 16);
	    treesyms[ntreesyms++] = treesrc[i] | SYMPFX_CODELEN;
	    k--;
	    if (k < 3) {
		while (k--)
		    treesyms[ntreesyms++] = treesrc[i] | SYMPFX_CODELEN;
	    } else {
		while (k > 0) {
		    int rpt = (k < 6 ? k : 6);
		    if (rpt > k-3 && rpt < k)
			rpt = k-3;
		    assert(rpt >= 3 && rpt <= 6);
		    treesyms[ntreesyms++] = 16 | SYMPFX_CODELEN;
		    treesyms[ntreesyms++] = (SYMPFX_EXTRABITS | (rpt - 3) |
					     (2 << SYM_EXTRABITS_SHIFT));
		    k -= rpt;
		}
	    }
	}

	i += j;
    }
    assert((unsigned)ntreesyms < lenof(treesyms));

    /*
     * Count up the frequency table for the tree-transmission
     * symbols, and build the auxiliary Huffman tree for that.
     */
    memset(freqs3, 0, sizeof(freqs3));
    for (i = 0; i < ntreesyms; i++) {
	unsigned sym = treesyms[i];

	/*
	 * Increment the occurrence counter for this symbol, if
	 * it's the Huffman alphabet and isn't extra bits.
	 */
	if ((sym & SYMPFX_MASK) == SYMPFX_CODELEN) {
	    sym &= ~SYMPFX_MASK;
	    assert(sym < lenof(freqs3));
	    freqs3[sym]++;
	}
    }
    deflate_buildhuf(freqs3, len3, lenof(freqs3), 7);
    hufcodes(len3, code3, lenof(freqs3));

    /*
     * Reorder the code length codes into transmission order, and
     * determine HCLEN.
     */
    for (i = 0; i < 19; i++)
	codelen[i] = len3[lenlenmap[i]];
    for (hclen = 19; hclen > 4 && codelen[hclen-1] == 0; hclen--);

    /*
     * Now work out the exact size of both the dynamic and the
     * static block, in bits.
     */
    {
	int ssize, dsize;

	/*
	 * First the dynamic block.
	 */
	dsize = 3 + 5 + 5 + 4;	       /* 3-bit header, HLIT, HDIST, HCLEN */
	dsize += 3 * hclen;	       /* code-length-alphabet code lengths */
	/* Code lengths */
	for (i = 0; i < ntreesyms; i++)
	    dsize += symsize(treesyms[i], &dht);
	/* The actual block data */
	for (i = 0; i < dynamic_len; i++) {
	    unsigned sym = out->syms[(out->symstart + i) % SYMLIMIT];
	    dsize += symsize(sym, &dht);
	}
	/* And the end-of-data symbol. */
	dsize += symsize(SYMPFX_LITLEN | 256, &dht);

	/*
	 * Now the static block.
	 */
	ssize = 3;		       /* 3-bit block header */
	/* The actual block data */
	for (i = 0; i < static_len; i++) {
	    unsigned sym = out->syms[(out->symstart + i) % SYMLIMIT];
	    ssize += symsize(sym, &out->sht);
	}
	/* And the end-of-data symbol. */
	ssize += symsize(SYMPFX_LITLEN | 256, &out->sht);

	/*
	 * Compare the two and decide which to output. We break
	 * exact ties in favour of the static block, because of the
	 * special case in which that block has zero length.
	 */
	dynamic = ((double)ssize * dynamic_len > (double)dsize * static_len);
	ht = dynamic ? &dht : &out->sht;
	blklen = dynamic ? dynamic_len : static_len;
    }

    /*
     * Actually transmit the block.
     */

    /* 3-bit block header */
    bfinal = (out->lastblock ? 1 : 0);
    btype = dynamic ? 2 : 1;
    debug(("send: bfinal=%d btype=%d\n", bfinal, btype));
    outbits(out, bfinal, 1);
    outbits(out, btype, 2);

#ifdef STATISTICS
    bitcount_before = out->bitcount;
#endif

    if (dynamic) {
	/* HLIT, HDIST and HCLEN */
	debug(("send: hlit=%d hdist=%d hclen=%d\n", hlit, hdist, hclen));
	outbits(out, hlit - 257, 5);
	outbits(out, hdist - 1, 5);
	outbits(out, hclen - 4, 4);

	/* Code lengths for the auxiliary tree */
	for (i = 0; i < hclen; i++) {
	    debug(("send: lenlen %d\n", codelen[i]));
	    outbits(out, codelen[i], 3);
	}

	/* Code lengths for the literal/length and distance trees */
	for (i = 0; i < ntreesyms; i++)
	    writesym(out, treesyms[i], ht);
#ifdef STATISTICS
	fprintf(stderr, "total tree size %lu bits\n",
		out->bitcount - bitcount_before);
#endif
    }

    /* Output the actual symbols from the buffer */
    for (i = 0; i < blklen; i++) {
	unsigned sym = out->syms[(out->symstart + i) % SYMLIMIT];
	writesym(out, sym, ht);
    }

    /* Output the end-of-data symbol */
    writesym(out, SYMPFX_LITLEN | 256, ht);

    /*
     * Remove all the just-output symbols from the symbol buffer by
     * adjusting symstart and nsyms.
     */
    out->symstart = (out->symstart + blklen) % SYMLIMIT;
    out->nsyms -= blklen;
}

/*
 * Give the approximate log-base-2 of an input integer, measured in
 * 8ths of a bit. (I.e. this computes an integer approximation to
 * 8*logbase2(x).)
 */
static int approxlog2(unsigned x)
{
    int ret = 31*8;

    /*
     * Binary-search to get the top bit of x up to bit 31.
     */
    if (x < 0x00010000U) x <<= 16, ret -= 16*8;
    if (x < 0x01000000U) x <<=  8, ret -=  8*8;
    if (x < 0x10000000U) x <<=  4, ret -=  4*8;
    if (x < 0x40000000U) x <<=  2, ret -=  2*8;
    if (x < 0x80000000U) x <<=  1, ret -=  1*8;

    /*
     * Now we know the logarithm we want is in [ret,ret+1).
     * Determine the bottom three bits by checking against
     * threshold values.
     * 
     * (Each of these threshold values is 0x80000000 times an odd
     * power of 2^(1/16). Therefore, this function rounds to
     * nearest.)
     */
    if (x <= 0xAD583EEAU) {
	if (x <= 0x91C3D373U)
	    ret += (x <= 0x85AAC367U ? 0 : 1);
	else
	    ret += (x <= 0x9EF53260U ? 2 : 3);
    } else {
	if (x <= 0xCE248C15U)
	    ret += (x <= 0xBD08A39FU ? 4 : 5);
	else
	    ret += (x <= 0xE0CCDEECU ? 6 : x <= 0xF5257D15L ? 7 : 8);
    }

    return ret;
}

static void chooseblock(deflate_compress_ctx *out)
{
    int freqs1[286], freqs2[30];
    int i, len, bestlen, longestlen = 0;
    int total1, total2;
    int bestvfm;

    memset(freqs1, 0, sizeof(freqs1));
    memset(freqs2, 0, sizeof(freqs2));
    freqs1[256] = 1;		       /* we're bound to need one EOB */
    total1 = 1;
    total2 = 0;

    /*
     * Iterate over all possible block lengths, computing the
     * entropic coding approximation to the final length at every
     * stage. We divide the result by the number of symbols
     * encoded, to determine the `value for money' (overall
     * bits-per-symbol count) of a block of that length.
     */
    bestlen = -1;
    bestvfm = 0;

    len = 300 * 8;	      /* very approximate size of the Huffman trees */

    for (i = 0; i < out->nsyms; i++) {
	unsigned sym = out->syms[(out->symstart + i) % SYMLIMIT];

	if (i > 0 && (sym & SYMPFX_MASK) == SYMPFX_LITLEN) {
	    /*
	     * This is a viable point at which to end the block.
	     * Compute the value for money.
	     */
	    int vfm = i * 32768 / len;      /* symbols encoded per bit */

	    if (bestlen < 0 || vfm > bestvfm) {
		bestlen = i;
		bestvfm = vfm;
	    }

	    longestlen = i;
	}

	/*
	 * Increment the occurrence counter for this symbol, if
	 * it's in one of the Huffman alphabets and isn't extra
	 * bits.
	 */
	if ((sym & SYMPFX_MASK) == SYMPFX_LITLEN) {
	    sym &= ~SYMPFX_MASK;
	    assert(sym < lenof(freqs1));
	    len += freqs1[sym] * approxlog2(freqs1[sym]);
	    len -= total1 * approxlog2(total1);
	    freqs1[sym]++;
	    total1++;
	    len -= freqs1[sym] * approxlog2(freqs1[sym]);
	    len += total1 * approxlog2(total1);
	} else if ((sym & SYMPFX_MASK) == SYMPFX_DIST) {
	    sym &= ~SYMPFX_MASK;
	    assert(sym < lenof(freqs2));
	    len += freqs2[sym] * approxlog2(freqs2[sym]);
	    len -= total2 * approxlog2(total2);
	    freqs2[sym]++;
	    total2++;
	    len -= freqs2[sym] * approxlog2(freqs2[sym]);
	    len += total2 * approxlog2(total2);
	} else if ((sym & SYMPFX_MASK) == SYMPFX_EXTRABITS) {
	    len += 8 * ((sym &~ SYMPFX_MASK) >> SYM_EXTRABITS_SHIFT);
	}
    }

    assert(bestlen > 0);

    outblock(out, bestlen, longestlen);
}

/*
 * Force the current symbol buffer to be flushed out as a single
 * block.
 */
static void flushblock(deflate_compress_ctx *out)
{
    /*
     * No need to check that out->nsyms is a valid block length: we
     * know it has to be, because flushblock() is called in between
     * two matches/literals.
     */
    outblock(out, out->nsyms, out->nsyms);
    assert(out->nsyms == 0);
}

/*
 * Place a symbol into the symbols buffer.
 */
static void outsym(deflate_compress_ctx *out, unsigned long sym)
{
    assert(out->nsyms < SYMLIMIT);
    out->syms[(out->symstart + out->nsyms++) % SYMLIMIT] = sym;

    if (out->nsyms == SYMLIMIT)
	chooseblock(out);
}

static void literal(struct LZ77Context *ectx, unsigned char c)
{
    deflate_compress_ctx *out = (deflate_compress_ctx *) ectx->userdata;

    outsym(out, SYMPFX_LITLEN | c);
}

static void match(struct LZ77Context *ectx, int distance, int len)
{
    const coderecord *d, *l;
    int i, j, k;
    deflate_compress_ctx *out = (deflate_compress_ctx *) ectx->userdata;

    while (len > 0) {
        int thislen;

        /*
         * We can transmit matches of lengths 3 through 258
         * inclusive. So if len exceeds 258, we must transmit in
         * several steps, with 258 or less in each step.
         *
         * Specifically: if len >= 261, we can transmit 258 and be
         * sure of having at least 3 left for the next step. And if
         * len <= 258, we can just transmit len. But if len == 259
         * or 260, we must transmit len-3.
         */
        thislen = (len > 260 ? 258 : len <= 258 ? len : len - 3);
        len -= thislen;

        /*
         * Binary-search to find which length code we're
         * transmitting.
         */
        i = -1;
        j = sizeof(lencodes) / sizeof(*lencodes);
        while (1) {
            assert(j - i >= 2);
            k = (j + i) / 2;
            if (thislen < lencodes[k].min)
                j = k;
            else if (thislen > lencodes[k].max)
                i = k;
            else {
                l = &lencodes[k];
                break;                 /* found it! */
            }
        }

        /*
         * Transmit the length code.
         */
        outsym(out, SYMPFX_LITLEN | l->code);

        /*
         * Transmit the extra bits.
         */
        if (l->extrabits) {
            outsym(out, (SYMPFX_EXTRABITS | (thislen - l->min) |
                         (l->extrabits << SYM_EXTRABITS_SHIFT)));
        }

        /*
         * Binary-search to find which distance code we're
         * transmitting.
         */
        i = -1;
        j = sizeof(distcodes) / sizeof(*distcodes);
        while (1) {
            assert(j - i >= 2);
            k = (j + i) / 2;
            if (distance < distcodes[k].min)
                j = k;
            else if (distance > distcodes[k].max)
                i = k;
            else {
                d = &distcodes[k];
                break;                 /* found it! */
            }
        }

        /*
         * Write the distance code.
         */
        outsym(out, SYMPFX_DIST | d->code);

        /*
         * Transmit the extra bits.
         */
        if (d->extrabits) {
            outsym(out, (SYMPFX_EXTRABITS | (distance - d->min) |
                         (d->extrabits << SYM_EXTRABITS_SHIFT)));
        }
    }
}

deflate_compress_ctx *deflate_compress_new(int type)
{
    deflate_compress_ctx *out;
    struct LZ77Context *ectx = snew(struct LZ77Context);

    lz77_init(ectx);
    ectx->literal = literal;
    ectx->match = match;

    out = snew(deflate_compress_ctx);
    out->type = type;
    out->outbits = out->noutbits = 0;
    out->firstblock = TRUE;
#ifdef STATISTICS
    out->bitcount = 0;
#endif

    out->syms = snewn(SYMLIMIT, unsigned long);
    out->symstart = out->nsyms = 0;

    out->checksum = (type == DEFLATE_TYPE_ZLIB ? 1 : 0);
    out->datasize = 0;
    out->lastblock = FALSE;
    out->finished = FALSE;

    /*
     * Build the static Huffman tables now, so we'll have them
     * available every time outblock() is called.
     */
    {
	int i;

	for (i = 0; i < (int)lenof(out->static_len1); i++)
	    out->static_len1[i] = (i < 144 ? 8 :
				   i < 256 ? 9 :
				   i < 280 ? 7 : 8);
	for (i = 0; i < (int)lenof(out->static_len2); i++)
	    out->static_len2[i] = 5;
    }
    hufcodes(out->static_len1, out->static_code1, lenof(out->static_code1));
    hufcodes(out->static_len2, out->static_code2, lenof(out->static_code2));
    out->sht.len_litlen = out->static_len1;
    out->sht.len_dist = out->static_len2;
    out->sht.len_codelen = NULL;
    out->sht.code_litlen = out->static_code1;
    out->sht.code_dist = out->static_code2;
    out->sht.code_codelen = NULL;

    ectx->userdata = out;
    out->lzc = ectx;

    return out;
}

void deflate_compress_free(deflate_compress_ctx *out)
{
    struct LZ77Context *ectx = out->lzc;

    sfree(out->syms);
    sfree(ectx->ictx);
    sfree(ectx);
    sfree(out);
}

void deflate_compress_data(deflate_compress_ctx *out,
			   const void *vblock, int len, int flushtype,
			   void **outblock, int *outlen)
{
    struct LZ77Context *ectx = out->lzc;
    const unsigned char *block = (const unsigned char *)vblock;

    assert(!out->finished);

    out->outbuf = NULL;
    out->outlen = out->outsize = 0;

    /*
     * If this is the first block, output the header.
     */
    if (out->firstblock) {
	switch (out->type) {
	  case DEFLATE_TYPE_BARE:
	    break;		       /* no header */
	  case DEFLATE_TYPE_ZLIB:
	    /*
	     * zlib (RFC1950) header bytes: 78 9C. (Deflate
	     * compression, 32K window size, default algorithm.)
	     */
	    outbits(out, 0x9C78, 16);
	    break;
	  case DEFLATE_TYPE_GZIP:
	    /*
	     * Minimal gzip (RFC1952) header:
	     * 
	     *  - basic header of 1F 8B
	     *  - compression method byte (8 = deflate)
	     *  - flags byte (zero: we use no optional features)
	     *  - modification time (zero: no time stamp available)
	     * 	- extra flags byte (2: we use maximum compression
	     * 	  always)
	     *  - operating system byte (255: we do not specify)
	     */
	    outbits(out, 0x00088B1F, 32);   /* header, CM, flags */
	    outbits(out, 0, 32);       /* mtime */
	    outbits(out, 0xFF02, 16);  /* xflags, OS */
	    break;
	}
	out->firstblock = FALSE;
    }

    /*
     * Feed our data to the LZ77 compression phase.
     */
    lz77_compress(ectx, block, len, TRUE);

    /*
     * Update checksums and counters.
     */
    switch (out->type) {
      case DEFLATE_TYPE_ZLIB:
	out->checksum = adler32_update(out->checksum, block, len);
	break;
      case DEFLATE_TYPE_GZIP:
	out->checksum = crc32_update(out->checksum, block, len);
	break;
    }
    out->datasize += len;

    switch (flushtype) {
	/*
	 * FIXME: what other flush types are available and useful?
	 * In PuTTY, it was clear that we generally wanted to be in
	 * a static block so it was safe to open one. Here, we
	 * probably prefer to be _outside_ a block if we can. Think
	 * about this.
	 */
      case DEFLATE_NO_FLUSH:
	break;			       /* don't flush any data at all (duh) */
      case DEFLATE_SYNC_FLUSH:
	/*
	 * Close the current block.
	 */
	flushblock(out);

	/*
	 * Then output an empty _uncompressed_ block: send 000,
	 * then sync to byte boundary, then send bytes 00 00 FF
	 * FF.
	 */
	outbits(out, 0, 3);
	if (out->noutbits)
	    outbits(out, 0, 8 - out->noutbits);
	outbits(out, 0, 16);
	outbits(out, 0xFFFF, 16);
	break;
      case DEFLATE_END_OF_DATA:
	/*
	 * Output a block with BFINAL set.
	 */
	out->lastblock = TRUE;
	flushblock(out);

	/*
	 * Sync to byte boundary, flushing out the final byte.
	 */
	if (out->noutbits)
	    outbits(out, 0, 8 - out->noutbits);

	/*
	 * Format-specific trailer data.
	 */
	switch (out->type) {
	  case DEFLATE_TYPE_ZLIB:
	    /*
	     * Just write out the Adler32 checksum.
	     */
	    outbits(out, (out->checksum >> 24) & 0xFF, 8);
	    outbits(out, (out->checksum >> 16) & 0xFF, 8);
	    outbits(out, (out->checksum >>  8) & 0xFF, 8);
	    outbits(out, (out->checksum >>  0) & 0xFF, 8);
	    break;
	  case DEFLATE_TYPE_GZIP:
	    /*
	     * Write out the CRC32 checksum and the data length.
	     */
	    outbits(out, out->checksum, 32);
	    outbits(out, out->datasize, 32);
	    break;
	}

	out->finished = TRUE;
	break;
    }

    /*
     * Return any data that we've generated.
     */
    *outblock = (void *)out->outbuf;
    *outlen = out->outlen;
}

/* ----------------------------------------------------------------------
 * Deflate decompression.
 */

/*
 * The way we work the Huffman decode is to have a table lookup on
 * the first N bits of the input stream (in the order they arrive,
 * of course, i.e. the first bit of the Huffman code is in bit 0).
 * Each table entry lists the number of bits to consume, plus
 * either an output code or a pointer to a secondary table.
 */
struct table;
struct tableentry;

struct tableentry {
    unsigned char nbits;
    short code;
    struct table *nexttable;
};

struct table {
    int mask;			       /* mask applied to input bit stream */
    struct tableentry *table;
};

#define MAXSYMS 288

#define DWINSIZE 32768

/*
 * Build a single-level decode table for elements
 * [minlength,maxlength) of the provided code/length tables, and
 * recurse to build subtables.
 */
static struct table *mkonetab(int *codes, unsigned char *lengths, int nsyms,
			      int pfx, int pfxbits, int bits)
{
    struct table *tab = snew(struct table);
    int pfxmask = (1 << pfxbits) - 1;
    int nbits, i, j, code;

    tab->table = snewn(1 << bits, struct tableentry);
    tab->mask = (1 << bits) - 1;

    for (code = 0; code <= tab->mask; code++) {
	tab->table[code].code = -1;
	tab->table[code].nbits = 0;
	tab->table[code].nexttable = NULL;
    }

    for (i = 0; i < nsyms; i++) {
	if (lengths[i] <= pfxbits || (codes[i] & pfxmask) != pfx)
	    continue;
	code = (codes[i] >> pfxbits) & tab->mask;
	for (j = code; j <= tab->mask; j += 1 << (lengths[i] - pfxbits)) {
	    tab->table[j].code = i;
	    nbits = lengths[i] - pfxbits;
	    if (tab->table[j].nbits < nbits)
		tab->table[j].nbits = nbits;
	}
    }
    for (code = 0; code <= tab->mask; code++) {
	if (tab->table[code].nbits <= bits)
	    continue;
	/* Generate a subtable. */
	tab->table[code].code = -1;
	nbits = tab->table[code].nbits - bits;
	if (nbits > 7)
	    nbits = 7;
	tab->table[code].nbits = bits;
	tab->table[code].nexttable = mkonetab(codes, lengths, nsyms,
					      pfx | (code << pfxbits),
					      pfxbits + bits, nbits);
    }

    return tab;
}

/*
 * Build a decode table, given a set of Huffman tree lengths.
 */
static struct table *mktable(unsigned char *lengths, int nlengths,
#ifdef ANALYSIS
			     const char *alphabet,
#endif
			     int *error)
{
    int codes[MAXSYMS];
    int maxlen;

#ifdef ANALYSIS
    if (alphabet && analyse_level > 1) {
	int i, col = 0;
	printf("code lengths for %s alphabet:\n", alphabet);
	for (i = 0; i < nlengths; i++) {
	    col += printf("%3d", lengths[i]);
	    if (col > 72) {
		putchar('\n');
		col = 0;
	    }
	}
	if (col > 0)
	    putchar('\n');
    }
#endif

    maxlen = hufcodes(lengths, codes, nlengths);

    if (maxlen < 0) {
	*error = (maxlen == -1 ? DEFLATE_ERR_LARGE_HUFTABLE :
		  DEFLATE_ERR_SMALL_HUFTABLE);
	return NULL;
    }

    /*
     * Now we have the complete list of Huffman codes. Build a
     * table.
     */
    return mkonetab(codes, lengths, nlengths, 0, 0, maxlen < 9 ? maxlen : 9);
}

static int freetable(struct table **ztab)
{
    struct table *tab;
    int code;

    if (ztab == NULL)
	return -1;

    if (*ztab == NULL)
	return 0;

    tab = *ztab;

    for (code = 0; code <= tab->mask; code++)
	if (tab->table[code].nexttable != NULL)
	    freetable(&tab->table[code].nexttable);

    sfree(tab->table);
    tab->table = NULL;

    sfree(tab);
    *ztab = NULL;

    return (0);
}

struct deflate_decompress_ctx {
    struct table *staticlentable, *staticdisttable;
    struct table *currlentable, *currdisttable, *lenlentable;
    enum {
	ZLIBSTART,
	GZIPSTART, GZIPMETHFLAGS, GZIPIGNORE1, GZIPIGNORE2, GZIPIGNORE3,
	GZIPEXTRA, GZIPFNAME, GZIPCOMMENT,
	OUTSIDEBLK, TREES_HDR, TREES_LENLEN, TREES_LEN, TREES_LENREP,
	INBLK, GOTLENSYM, GOTLEN, GOTDISTSYM,
	UNCOMP_LEN, UNCOMP_NLEN, UNCOMP_DATA,
	END,
	ADLER1, ADLER2,
	CRC1, CRC2, ILEN1, ILEN2,
	FINALSPIN
    } state;
    int sym, hlit, hdist, hclen, lenptr, lenextrabits, lenaddon, len,
	lenrep, lastblock;
    int uncomplen;
    unsigned char lenlen[19];
    unsigned char lengths[286 + 32];
    unsigned long bits;
    int nbits;
    unsigned char window[DWINSIZE];
    int winpos;
    unsigned char *outblk;
    int outlen, outsize;
    int type;
    unsigned long checksum;
    unsigned long bytesout;
    int gzflags, gzextralen;
#ifdef ANALYSIS
    int bytesread;
    int bitcount_before;
#define BITCOUNT(dctx) ( (dctx)->bytesread * 8 - (dctx)->nbits )
#endif
};

deflate_decompress_ctx *deflate_decompress_new(int type)
{
    deflate_decompress_ctx *dctx = snew(deflate_decompress_ctx);
    unsigned char lengths[288];

    memset(lengths, 8, 144);
    memset(lengths + 144, 9, 256 - 144);
    memset(lengths + 256, 7, 280 - 256);
    memset(lengths + 280, 8, 288 - 280);
    dctx->staticlentable = mktable(lengths, 288,
#ifdef ANALYSIS
				   NULL,
#endif
				   NULL);
    assert(dctx->staticlentable);
    memset(lengths, 5, 32);
    dctx->staticdisttable = mktable(lengths, 32,
#ifdef ANALYSIS
				    NULL,
#endif
				    NULL);
    assert(dctx->staticdisttable);
    dctx->state = (type == DEFLATE_TYPE_ZLIB ? ZLIBSTART :
		   type == DEFLATE_TYPE_GZIP ? GZIPSTART :
		   OUTSIDEBLK);
    dctx->currlentable = dctx->currdisttable = dctx->lenlentable = NULL;
    dctx->bits = 0;
    dctx->nbits = 0;
    dctx->winpos = 0;
    dctx->type = type;
    dctx->lastblock = FALSE;
    dctx->checksum = (type == DEFLATE_TYPE_ZLIB ? 1 : 0);
    dctx->bytesout = 0;
    dctx->gzflags = dctx->gzextralen = 0;
#ifdef ANALYSIS
    dctx->bytesread = dctx->bitcount_before = 0;
#endif

    return dctx;
}

void deflate_decompress_free(deflate_decompress_ctx *dctx)
{
    if (dctx->currlentable && dctx->currlentable != dctx->staticlentable)
	freetable(&dctx->currlentable);
    if (dctx->currdisttable && dctx->currdisttable != dctx->staticdisttable)
	freetable(&dctx->currdisttable);
    if (dctx->lenlentable)
	freetable(&dctx->lenlentable);
    freetable(&dctx->staticlentable);
    freetable(&dctx->staticdisttable);
    sfree(dctx);
}

static int huflookup(unsigned long *bitsp, int *nbitsp, struct table *tab)
{
    unsigned long bits = *bitsp;
    int nbits = *nbitsp;
    while (1) {
	struct tableentry *ent;
	ent = &tab->table[bits & tab->mask];
	if (ent->nbits > nbits)
	    return -1;		       /* not enough data */
	bits >>= ent->nbits;
	nbits -= ent->nbits;
	if (ent->code == -1)
	    tab = ent->nexttable;
	else {
	    *bitsp = bits;
	    *nbitsp = nbits;
	    return ent->code;
	}

	/*
	 * If we reach here with `tab' null, it can only be because
	 * there was a missing entry in the Huffman table. This
	 * should never occur even with invalid input data, because
	 * we enforce at mktable time that the Huffman codes should
	 * precisely cover the code space; so we can enforce this
	 * by assertion.
	 */
	assert(tab);
    }
}

static void emit_char(deflate_decompress_ctx *dctx, int c)
{
    dctx->window[dctx->winpos] = c;
    dctx->winpos = (dctx->winpos + 1) & (DWINSIZE - 1);
    if (dctx->outlen >= dctx->outsize) {
	dctx->outsize = dctx->outlen * 3 / 2 + 512;
	dctx->outblk = sresize(dctx->outblk, dctx->outsize, unsigned char);
    }
    if (dctx->type == DEFLATE_TYPE_ZLIB) {
	unsigned char uc = c;
	dctx->checksum = adler32_update(dctx->checksum, &uc, 1);
    } else if (dctx->type == DEFLATE_TYPE_GZIP) {
	unsigned char uc = c;
	dctx->checksum = crc32_update(dctx->checksum, &uc, 1);
    }
    dctx->outblk[dctx->outlen++] = c;
    dctx->bytesout++;
}

#define EATBITS(n) ( dctx->nbits -= (n), dctx->bits >>= (n) )

int deflate_decompress_data(deflate_decompress_ctx *dctx,
			    const void *vblock, int len,
			    void **outblock, int *outlen)
{
    const coderecord *rec;
    const unsigned char *block = (const unsigned char *)vblock;
    int code, bfinal, btype, rep, dist, nlen, header;
    unsigned long cksum;
    int error = 0;

    if (len == 0) {
	*outblock = NULL;
	*outlen = 0;
	if (dctx->state != FINALSPIN)
	    return DEFLATE_ERR_UNEXPECTED_EOF;
	else
	    return 0;
    }

    dctx->outblk = NULL;
    dctx->outsize = 0;
    dctx->outlen = 0;

    while (len > 0 || dctx->nbits > 0) {
	while (dctx->nbits < 24 && len > 0) {
	    dctx->bits |= (*block++) << dctx->nbits;
	    dctx->nbits += 8;
	    len--;
#ifdef ANALYSIS
	    dctx->bytesread++;
#endif
	}
	switch (dctx->state) {
	  case ZLIBSTART:
	    /* Expect 16-bit zlib header. */
	    if (dctx->nbits < 16)
		goto finished;	       /* done all we can */

            /*
             * The header is stored as a big-endian 16-bit integer,
             * in contrast to the general little-endian policy in
             * the rest of the format :-(
             */
            header = (((dctx->bits & 0xFF00) >> 8) |
                      ((dctx->bits & 0x00FF) << 8));
            EATBITS(16);

            /*
             * Check the header:
             *
             *  - bits 8-11 should be 1000 (Deflate/RFC1951)
             *  - bits 12-15 should be at most 0111 (window size)
             *  - bit 5 should be zero (no dictionary present)
             *  - we don't care about bits 6-7 (compression rate)
             *  - bits 0-4 should be set up to make the whole thing
             *    a multiple of 31 (checksum).
             */
	    if ((header & 0xF000) >  0x7000 ||
                (header & 0x0020) != 0x0000 ||
                (header % 31) != 0) {
		error = DEFLATE_ERR_ZLIB_HEADER;
                goto finished;
	    }
            if ((header & 0x0F00) != 0x0800) {
		error = DEFLATE_ERR_ZLIB_WRONGCOMP;
                goto finished;
	    }
	    dctx->state = OUTSIDEBLK;
	    break;
	  case GZIPSTART:
	    /* Expect 16-bit gzip header. */
	    if (dctx->nbits < 16)
		goto finished;
	    header = dctx->bits & 0xFFFF;
	    EATBITS(16);
	    if (header != 0x8B1F) {
		error = DEFLATE_ERR_GZIP_HEADER;
		goto finished;
	    }
	    dctx->state = GZIPMETHFLAGS;
	    break;
	  case GZIPMETHFLAGS:
	    /* Expect gzip compression method and flags bytes. */
	    if (dctx->nbits < 16)
		goto finished;
	    header = dctx->bits & 0xFF;
	    EATBITS(8);
	    if (header != 8) {
		error = DEFLATE_ERR_GZIP_WRONGCOMP;
		goto finished;
	    }
	    dctx->gzflags = dctx->bits & 0xFF;
	    if (dctx->gzflags & 2) {
		/*
		 * The FHCRC flag is slightly confusing. RFC1952
		 * documents it as indicating the presence of a
		 * two-byte CRC16 of the gzip header, occurring
		 * just before the beginning of the Deflate stream.
		 * However, gzip itself (as of 1.3.5) appears to
		 * believe it indicates that the current gzip
		 * `member' is not the final one, i.e. that the
		 * stream is composed of multiple gzip members
		 * concatenated together, and furthermore gzip will
		 * refuse to decode any file that has it set.
		 * 
		 * For this reason, I label it as `disputed' and
		 * also refuse to decode anything that has it set.
		 * I don't expect this to be a problem in practice.
		 */
		error = DEFLATE_ERR_GZIP_FHCRC;
		goto finished;
	    }
	    EATBITS(8);
	    dctx->state = GZIPIGNORE1;
	    break;
	  case GZIPIGNORE1:
	  case GZIPIGNORE2:
	  case GZIPIGNORE3:
	    /* Expect two bytes of gzip timestamp/XFL/OS, which we ignore. */
	    if (dctx->nbits < 16)
		goto finished;
	    EATBITS(16);
	    if (dctx->state == GZIPIGNORE3) {
		dctx->state = GZIPEXTRA;
	    } else
		dctx->state++;	       /* maps IGNORE1 -> IGNORE2 -> IGNORE3 */
	    break;
	  case GZIPEXTRA:
	    if (dctx->gzflags & 4) {
		/* Expect two bytes of extra-length count, then that many
		 * extra bytes of header data, all of which we ignore. */
		if (!dctx->gzextralen) {
		    if (dctx->nbits < 16)
			goto finished;
		    dctx->gzextralen = dctx->bits & 0xFFFF;
		    EATBITS(16);
		    break;
		} else if (dctx->gzextralen > 0) {
		    if (dctx->nbits < 8)
			goto finished;
		    EATBITS(8);
		    if (--dctx->gzextralen > 0)
			break;
		}
	    }
	    dctx->state = GZIPFNAME;
	    break;
	  case GZIPFNAME:
	    if (dctx->gzflags & 8) {
		/*
		 * Expect a NUL-terminated filename.
		 */
		if (dctx->nbits < 8)
		    goto finished;
		code = dctx->bits & 0xFF;
		EATBITS(8);
	    } else
		code = 0;
	    if (code == 0)
		dctx->state = GZIPCOMMENT;
	    break;
	  case GZIPCOMMENT:
	    if (dctx->gzflags & 16) {
		/*
		 * Expect a NUL-terminated filename.
		 */
		if (dctx->nbits < 8)
		    goto finished;
		code = dctx->bits & 0xFF;
		EATBITS(8);
	    } else
		code = 0;
	    if (code == 0)
		dctx->state = OUTSIDEBLK;
	    break;
	  case OUTSIDEBLK:
	    /* Expect 3-bit block header. */
	    if (dctx->nbits < 3)
		goto finished;	       /* done all we can */
	    bfinal = dctx->bits & 1;
	    if (bfinal)
		dctx->lastblock = TRUE;
	    EATBITS(1);
	    btype = dctx->bits & 3;
	    EATBITS(2);
	    if (btype == 0) {
		int to_eat = dctx->nbits & 7;
		dctx->state = UNCOMP_LEN;
		EATBITS(to_eat);       /* align to byte boundary */
	    } else if (btype == 1) {
		dctx->currlentable = dctx->staticlentable;
		dctx->currdisttable = dctx->staticdisttable;
		dctx->state = INBLK;
	    } else if (btype == 2) {
		dctx->state = TREES_HDR;
	    }
	    debug(("recv: bfinal=%d btype=%d\n", bfinal, btype));
#ifdef ANALYSIS
	    if (analyse_level > 1) {
		static const char *const btypes[] = {
		    "uncompressed", "static", "dynamic", "type 3 (unknown)"
		};
		printf("new block, %sfinal, %s\n",
		       bfinal ? "" : "not ",
		       btypes[btype]);
	    }
#endif
	    break;
	  case TREES_HDR:
	    /*
	     * Dynamic block header. Five bits of HLIT, five of
	     * HDIST, four of HCLEN.
	     */
	    if (dctx->nbits < 5 + 5 + 4)
		goto finished;	       /* done all we can */
	    dctx->hlit = 257 + (dctx->bits & 31);
	    EATBITS(5);
	    dctx->hdist = 1 + (dctx->bits & 31);
	    EATBITS(5);
	    dctx->hclen = 4 + (dctx->bits & 15);
	    EATBITS(4);
	    debug(("recv: hlit=%d hdist=%d hclen=%d\n", dctx->hlit,
		   dctx->hdist, dctx->hclen));
#ifdef ANALYSIS
	    if (analyse_level > 1)
		printf("hlit=%d, hdist=%d, hclen=%d\n",
		        dctx->hlit, dctx->hdist, dctx->hclen);
#endif
	    dctx->lenptr = 0;
	    dctx->state = TREES_LENLEN;
	    memset(dctx->lenlen, 0, sizeof(dctx->lenlen));
	    break;
	  case TREES_LENLEN:
	    if (dctx->nbits < 3)
		goto finished;
	    while (dctx->lenptr < dctx->hclen && dctx->nbits >= 3) {
		dctx->lenlen[lenlenmap[dctx->lenptr++]] =
		    (unsigned char) (dctx->bits & 7);
		debug(("recv: lenlen %d\n", (unsigned char) (dctx->bits & 7)));
		EATBITS(3);
	    }
	    if (dctx->lenptr == dctx->hclen) {
		dctx->lenlentable = mktable(dctx->lenlen, 19,
#ifdef ANALYSIS
					    "code length",
#endif
					    &error);
		if (!dctx->lenlentable)
		    goto finished;     /* error code set up by mktable */
		dctx->state = TREES_LEN;
		dctx->lenptr = 0;
	    }
	    break;
	  case TREES_LEN:
	    if (dctx->lenptr >= dctx->hlit + dctx->hdist) {
		dctx->currlentable = mktable(dctx->lengths, dctx->hlit,
#ifdef ANALYSIS
					     "literal/length",
#endif
					     &error);
		if (!dctx->currlentable)
		    goto finished;     /* error code set up by mktable */
                if (dctx->hdist == 1 && dctx->lengths[dctx->hlit] == 0) {
                    /*
                     * Special case: if the code length list for the
                     * backward-distance table contains a single zero
                     * entry, it means this block will never encode a
                     * backward distance at all (i.e. it's all
                     * literals).
                     */
                    dctx->currdisttable = NULL;
                } else {
                    dctx->currdisttable = mktable(dctx->lengths + dctx->hlit,
                                                  dctx->hdist,
#ifdef ANALYSIS
                                                  "distance",
#endif
                                                  &error);
                    if (!dctx->currdisttable)
                        goto finished;     /* error code set up by mktable */
                }
		freetable(&dctx->lenlentable);
		dctx->lenlentable = NULL;
		dctx->state = INBLK;
		break;
	    }
	    code = huflookup(&dctx->bits, &dctx->nbits, dctx->lenlentable);
	    debug(("recv: codelen %d\n", code));
	    if (code == -1)
		goto finished;
	    if (code < 16) {
#ifdef ANALYSIS
		if (analyse_level > 1)
		    printf("code-length %d\n", code);
#endif
		dctx->lengths[dctx->lenptr++] = code;
	    } else {
		dctx->lenextrabits = (code == 16 ? 2 : code == 17 ? 3 : 7);
		dctx->lenaddon = (code == 18 ? 11 : 3);
		dctx->lenrep = (code == 16 && dctx->lenptr > 0 ?
				dctx->lengths[dctx->lenptr - 1] : 0);
		dctx->state = TREES_LENREP;
	    }
	    break;
	  case TREES_LENREP:
	    if (dctx->nbits < dctx->lenextrabits)
		goto finished;
	    rep =
		dctx->lenaddon +
		(dctx->bits & ((1 << dctx->lenextrabits) - 1));
	    EATBITS(dctx->lenextrabits);
	    if (dctx->lenextrabits)
		debug(("recv: codelen-extrabits %d/%d\n", rep - dctx->lenaddon,
		       dctx->lenextrabits));
#ifdef ANALYSIS
	    if (analyse_level > 1)
		printf("code-length-repeat: %d copies of %d\n", rep,
		       dctx->lenrep);
#endif
	    while (rep > 0 && dctx->lenptr < dctx->hlit + dctx->hdist) {
		dctx->lengths[dctx->lenptr] = dctx->lenrep;
		dctx->lenptr++;
		rep--;
	    }
	    dctx->state = TREES_LEN;
	    break;
	  case INBLK:
#ifdef ANALYSIS
	    dctx->bitcount_before = BITCOUNT(dctx);
#endif
	    code = huflookup(&dctx->bits, &dctx->nbits, dctx->currlentable);
	    debug(("recv: litlen %d\n", code));
	    if (code == -1)
		goto finished;
	    if (code < 256) {
#ifdef ANALYSIS
		if (analyse_level > 0)
		    printf("%lu: literal %d [%d]\n", dctx->bytesout, code,
			   BITCOUNT(dctx) - dctx->bitcount_before);
#endif
		emit_char(dctx, code);
	    } else if (code == 256) {
		if (dctx->lastblock)
		    dctx->state = END;
		else
		    dctx->state = OUTSIDEBLK;
		if (dctx->currlentable != dctx->staticlentable) {
		    freetable(&dctx->currlentable);
		    dctx->currlentable = NULL;
		}
		if (dctx->currdisttable &&
                    dctx->currdisttable != dctx->staticdisttable) {
		    freetable(&dctx->currdisttable);
		    dctx->currdisttable = NULL;
		}
	    } else if (code < 286) {   /* static tree can give >285; ignore */
		dctx->state = GOTLENSYM;
		dctx->sym = code;
	    }
	    break;
	  case GOTLENSYM:
	    rec = &lencodes[dctx->sym - 257];
	    if (dctx->nbits < rec->extrabits)
		goto finished;
	    dctx->len =
		rec->min + (dctx->bits & ((1 << rec->extrabits) - 1));
	    if (rec->extrabits)
		debug(("recv: litlen-extrabits %d/%d\n",
		       dctx->len - rec->min, rec->extrabits));
	    EATBITS(rec->extrabits);
	    dctx->state = GOTLEN;
	    break;
	  case GOTLEN:
            if (!dctx->currdisttable) {
		error = DEFLATE_ERR_NODISTTABLE;
                goto finished;
            }
	    code = huflookup(&dctx->bits, &dctx->nbits, dctx->currdisttable);
	    debug(("recv: dist %d\n", code));
	    if (code == -1)
		goto finished;
	    dctx->state = GOTDISTSYM;
	    dctx->sym = code;
	    break;
	  case GOTDISTSYM:
	    rec = &distcodes[dctx->sym];
	    if (dctx->nbits < rec->extrabits)
		goto finished;
	    dist = rec->min + (dctx->bits & ((1 << rec->extrabits) - 1));
	    if (rec->extrabits)
		debug(("recv: dist-extrabits %d/%d\n",
		       dist - rec->min, rec->extrabits));
	    EATBITS(rec->extrabits);
	    dctx->state = INBLK;
#ifdef ANALYSIS
	    if (analyse_level > 0)
		printf("%lu: copy len=%d dist=%d [%d]\n", dctx->bytesout,
		       dctx->len, dist,
		       BITCOUNT(dctx) - dctx->bitcount_before);
#endif
	    while (dctx->len--)
		emit_char(dctx, dctx->window[(dctx->winpos - dist) &
					     (DWINSIZE - 1)]);
	    break;
	  case UNCOMP_LEN:
	    /*
	     * Uncompressed block. We expect to see a 16-bit LEN.
	     */
	    if (dctx->nbits < 16)
		goto finished;
	    dctx->uncomplen = dctx->bits & 0xFFFF;
	    EATBITS(16);
	    dctx->state = UNCOMP_NLEN;
	    break;
	  case UNCOMP_NLEN:
	    /*
	     * Uncompressed block. We expect to see a 16-bit NLEN,
	     * which should be the one's complement of the previous
	     * LEN.
	     */
	    if (dctx->nbits < 16)
		goto finished;
	    nlen = dctx->bits & 0xFFFF;
	    EATBITS(16);
	    if (dctx->uncomplen != (nlen ^ 0xFFFF)) {
                error = DEFLATE_ERR_UNCOMP_HDR;
                goto finished;
            }
	    if (dctx->uncomplen == 0) {/* block is empty */
		if (dctx->lastblock)
		    dctx->state = END;
		else
		    dctx->state = OUTSIDEBLK;
	    } else
		dctx->state = UNCOMP_DATA;
	    break;
	  case UNCOMP_DATA:
	    if (dctx->nbits < 8)
		goto finished;
#ifdef ANALYSIS
	    if (analyse_level > 0)
		printf("%lu: uncompressed %d [8]\n", dctx->bytesout,
		       (int)(dctx->bits & 0xFF));
#endif
	    emit_char(dctx, dctx->bits & 0xFF);
	    EATBITS(8);
	    if (--dctx->uncomplen == 0) {	/* end of uncompressed block */
		if (dctx->lastblock)
		    dctx->state = END;
		else
		    dctx->state = OUTSIDEBLK;
	    }
	    break;
	  case END:
	    /*
	     * End of compressed data. We align to a byte boundary,
	     * and then look for format-specific trailer data.
	     */
	    {
		int to_eat = dctx->nbits & 7;
		EATBITS(to_eat);
	    }
	    if (dctx->type == DEFLATE_TYPE_ZLIB)
		dctx->state = ADLER1;
	    else if (dctx->type == DEFLATE_TYPE_GZIP)
		dctx->state = CRC1;
	    else
		dctx->state = FINALSPIN;
	    break;
	  case ADLER1:
	    if (dctx->nbits < 16)
		goto finished;
	    cksum = (dctx->bits & 0xFF) << 8;
	    EATBITS(8);
	    cksum |= (dctx->bits & 0xFF);
	    EATBITS(8);
	    if (cksum != ((dctx->checksum >> 16) & 0xFFFF)) {
		error = DEFLATE_ERR_CHECKSUM;
		goto finished;
	    }
	    dctx->state = ADLER2;
	    break;
	  case ADLER2:
	    if (dctx->nbits < 16)
		goto finished;
	    cksum = (dctx->bits & 0xFF) << 8;
	    EATBITS(8);
	    cksum |= (dctx->bits & 0xFF);
	    EATBITS(8);
	    if (cksum != (dctx->checksum & 0xFFFF)) {
		error = DEFLATE_ERR_CHECKSUM;
		goto finished;
	    }
	    dctx->state = FINALSPIN;
	    break;
	  case CRC1:
	    if (dctx->nbits < 16)
		goto finished;
	    cksum = dctx->bits & 0xFFFF;
	    EATBITS(16);
	    if (cksum != (dctx->checksum & 0xFFFF)) {
		error = DEFLATE_ERR_CHECKSUM;
		goto finished;
	    }
	    dctx->state = CRC2;
	    break;
	  case CRC2:
	    if (dctx->nbits < 16)
		goto finished;
	    cksum = dctx->bits & 0xFFFF;
	    EATBITS(16);
	    if (cksum != ((dctx->checksum >> 16) & 0xFFFF)) {
		error = DEFLATE_ERR_CHECKSUM;
		goto finished;
	    }
	    dctx->state = ILEN1;
	    break;
	  case ILEN1:
	    if (dctx->nbits < 16)
		goto finished;
	    cksum = dctx->bits & 0xFFFF;
	    EATBITS(16);
	    if (cksum != (dctx->bytesout & 0xFFFF)) {
		error = DEFLATE_ERR_INLEN;
		goto finished;
	    }
	    dctx->state = ILEN2;
	    break;
	  case ILEN2:
	    if (dctx->nbits < 16)
		goto finished;
	    cksum = dctx->bits & 0xFFFF;
	    EATBITS(16);
	    if (cksum != ((dctx->bytesout >> 16) & 0xFFFF)) {
		error = DEFLATE_ERR_INLEN;
		goto finished;
	    }
	    dctx->state = FINALSPIN;
	    break;
	  case FINALSPIN:
	    /* Just ignore any trailing garbage on the data stream. */
	    /* (We could alternatively throw an error here, if we wanted
	     * to detect and complain about trailing garbage.) */
	    EATBITS(dctx->nbits);
	    break;
	}
    }

    finished:
    *outblock = dctx->outblk;
    *outlen = dctx->outlen;
    return error;
}

#define A(code,str) str
const char *const deflate_error_msg[DEFLATE_NUM_ERRORS] = {
    DEFLATE_ERRORLIST(A)
};
#undef A

#define A(code,str) #code
const char *const deflate_error_sym[DEFLATE_NUM_ERRORS] = {
    DEFLATE_ERRORLIST(A)
};
#undef A

#ifdef STANDALONE

int main(int argc, char **argv)
{
    unsigned char buf[65536];
    void *outbuf;
    int ret, err, outlen;
    deflate_decompress_ctx *dhandle;
    deflate_compress_ctx *chandle;
    int type = DEFLATE_TYPE_ZLIB, opts = TRUE;
    int compress = FALSE, decompress = FALSE;
    int got_arg = FALSE;
    char *filename = NULL;
    FILE *fp;

    while (--argc) {
        char *p = *++argv;

	got_arg = TRUE;

        if (p[0] == '-' && opts) {
            if (!strcmp(p, "-b"))
                type = DEFLATE_TYPE_BARE;
            else if (!strcmp(p, "-g"))
                type = DEFLATE_TYPE_GZIP;
            else if (!strcmp(p, "-c"))
                compress = TRUE;
            else if (!strcmp(p, "-d"))
                decompress = TRUE;
            else if (!strcmp(p, "-a"))
                analyse_level++, decompress = TRUE;
            else if (!strcmp(p, "--"))
                opts = FALSE;          /* next thing is filename */
            else {
                fprintf(stderr, "unknown command line option '%s'\n", p);
                return 1;
            }
        } else if (!filename) {
            filename = p;
        } else {
            fprintf(stderr, "can only handle one filename\n");
            return 1;
        }
    }

    if (!compress && !decompress) {
	fprintf(stderr, "usage: deflate [ -c | -d | -a ] [ -b | -g ]"
		" [filename]\n");
	return (got_arg ? 1 : 0);
    }

    if (compress && decompress) {
	fprintf(stderr, "please do not specify both compression and"
		" decompression\n");
	return (got_arg ? 1 : 0);
    }

    if (compress) {
	chandle = deflate_compress_new(type);
	dhandle = NULL;
    } else {
	dhandle = deflate_decompress_new(type);
	chandle = NULL;
    }

    if (filename)
        fp = fopen(filename, "rb");
    else
        fp = stdin;

    if (!fp) {
        assert(filename);
        fprintf(stderr, "unable to open '%s'\n", filename);
        return 1;
    }

    do {
	ret = fread(buf, 1, sizeof(buf), fp);
	outbuf = NULL;
	if (dhandle) {
	    if (ret > 0)
		err = deflate_decompress_data(dhandle, buf, ret,
					      (void **)&outbuf, &outlen);
	    else
		err = deflate_decompress_data(dhandle, NULL, 0,
					      (void **)&outbuf, &outlen);
	} else {
	    if (ret > 0)
		deflate_compress_data(chandle, buf, ret, DEFLATE_NO_FLUSH,
				      (void **)&outbuf, &outlen);
	    else
		deflate_compress_data(chandle, buf, ret, DEFLATE_END_OF_DATA,
				      (void **)&outbuf, &outlen);
	    err = 0;
	}
        if (outbuf) {
            if (!analyse_level && outlen)
                fwrite(outbuf, 1, outlen, stdout);
            sfree(outbuf);
        }
	if (err > 0) {
            fprintf(stderr, "decoding error: %s\n", deflate_error_msg[err]);
            return 1;
        }
    } while (ret > 0);

    if (dhandle)
	deflate_decompress_free(dhandle);
    if (chandle)
	deflate_compress_free(chandle);

    if (filename)
        fclose(fp);

    return 0;
}

#endif

#ifdef TESTMODE

int main(int argc, char **argv)
{
    char *filename = NULL;
    FILE *fp;
    deflate_compress_ctx *chandle;
    deflate_decompress_ctx *dhandle;
    unsigned char buf[65536], *outbuf, *outbuf2;
    int ret, err, outlen, outlen2;
    int dlen = 0, clen = 0;
    int opts = TRUE;

    while (--argc) {
        char *p = *++argv;

        if (p[0] == '-' && opts) {
            if (!strcmp(p, "--"))
                opts = FALSE;          /* next thing is filename */
            else {
                fprintf(stderr, "unknown command line option '%s'\n", p);
                return 1;
            }
        } else if (!filename) {
            filename = p;
        } else {
            fprintf(stderr, "can only handle one filename\n");
            return 1;
        }
    }

    if (filename)
        fp = fopen(filename, "rb");
    else
        fp = stdin;

    if (!fp) {
        assert(filename);
        fprintf(stderr, "unable to open '%s'\n", filename);
        return 1;
    }

    chandle = deflate_compress_new(DEFLATE_TYPE_ZLIB);
    dhandle = deflate_decompress_new(DEFLATE_TYPE_ZLIB);

    do {
	ret = fread(buf, 1, sizeof(buf), fp);
	if (ret <= 0) {
	    deflate_compress_data(chandle, NULL, 0, DEFLATE_END_OF_DATA,
				  (void **)&outbuf, &outlen);
	} else {
	    dlen += ret;
	    deflate_compress_data(chandle, buf, ret, DEFLATE_NO_FLUSH,
				  (void **)&outbuf, &outlen);
	}
	if (outbuf) {
	    clen += outlen;
	    err = deflate_decompress_data(dhandle, outbuf, outlen,
					  (void **)&outbuf2, &outlen2);
	    sfree(outbuf);
	    if (outbuf2) {
		if (outlen2)
		    fwrite(outbuf2, 1, outlen2, stdout);
		sfree(outbuf2);
	    }
	    if (!err && ret <= 0) {
		/*
		 * signal EOF
		 */
		err = deflate_decompress_data(dhandle, NULL, 0,
					      (void **)&outbuf2, &outlen2);
		assert(outbuf2 == NULL);
	    }
	    if (err) {
		fprintf(stderr, "decoding error: %s\n",
			deflate_error_msg[err]);
		return 1;
	    }
	}
    } while (ret > 0);

    fprintf(stderr, "%d plaintext -> %d compressed\n", dlen, clen);

    return 0;
}

#endif
