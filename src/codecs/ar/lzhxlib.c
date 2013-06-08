/**
 *
 *  LZHXLib.C - Decompression module
 *
 *  Compression Library (LZH) from Haruhiko Okumura's "ar".
 *
 *  Copyright(c) 1996 Kerwin F. Medina
 *  Copyright(c) 1991 Haruhiko Okumura
 *
 *  In MSDOS, this MUST be compiled using compact, large,
 *      or huge memory models
 *
 *  To test, compile as below to create a stand-alone decompressor (will
 *      decompress standard input to standard output):
 *          bcc -O2 -mc -D__TEST__ lzhxlib.c
 *              or
 *          gcc -O2 -D__TEST__ lzhxlib.c
 *      then run as: lzhxlib <infile >outfile
 *
 *  To use as a library for your application, __TEST__ must
 *      not be defined.
 *
 */

#include "lzh.h"

#define BUFSIZE (1024 * 4)

typedef struct {
	type_fnc_read   fnc_read;
	type_fnc_write  fnc_write;
	type_fnc_malloc fnc_malloc;
	type_fnc_free   fnc_free;
	int with_error;
	int fillbufsize;
	uchar * buf;
	ushort  left [2 * NC - 1],  right[2 * NC - 1];
	BITBUFTYPE bitbuf;
	uint       subbitbuf;
	int        bitcount;
	uchar   c_len[NC],  pt_len[NPT];
	uint   blocksize;
	ushort  c_table[4096],  pt_table[256];
	int decode_j;
	void *p;
	/* Was Static Shit */
	uint fillbuff_i;
} lzh_globals_x;

/** Shift bitbuf n bits left, read n bits */
static void fillbuf (lzh_globals_x *G, int n)
{
    G->bitbuf = (G->bitbuf << n) & 0xffff;
    while (n > G->bitcount)
    {
        G->bitbuf |= G->subbitbuf << (n -= G->bitcount);
        if (G->fillbufsize == 0)
        {
            G->fillbuff_i = 0;
            G->fillbufsize = G->fnc_read(G->buf, BUFSIZE - 32, G->p);
	}
	if (G->fillbufsize > 0)
            G->fillbufsize--, G->subbitbuf = G->buf[G->fillbuff_i++];
        else
            G->subbitbuf = 0;
        G->bitcount = CHAR_BIT;
    }
    G->bitbuf |= G->subbitbuf >> (G->bitcount -= n);
}

static ushort getbits (lzh_globals_x *G, int n)
{
    ushort x;
    x = G->bitbuf >> (BITBUFSIZ - n);
    fillbuf (G, n);
    return x;
}

static void init_getbits (lzh_globals_x *G)
{
    G->bitbuf = 0;
    G->subbitbuf = 0;
    G->bitcount = 0;
    fillbuf (G, BITBUFSIZ);
}

static int make_table (lzh_globals_x *G, int nchar, uchar  *bitlen,
                       int tablebits, ushort  *table)
{
    ushort count[17], weight[17], start[18], *p;
    uint i, k, len, ch, jutbits, avail, nextcode, mask;

    for (i = 1; i <= 16; i++)
        count[i] = 0;
    for (i = 0; i < nchar; i++)
        count[bitlen[i]]++;

    start[1] = 0;
    for (i = 1; i <= 16; i++)
        start[i + 1] = start[i] + (count[i] << (16 - i));
    if (start[17] != (ushort) (1U << 16))
        return (1); /* error: bad table */

    jutbits = 16 - tablebits;
    for (i = 1; i <= tablebits; i++)
    {
        start[i] >>= jutbits;
        weight[i] = 1U << (tablebits - i);
    }
    while (i <= 16)
        weight[i++] = 1U << (16 - i);

    i = start[tablebits + 1] >> jutbits;
    if (i != (ushort) (1U << 16))
    {
        k = 1U << tablebits;
        while (i != k)
            table[i++] = 0;
    }

    avail = nchar;
    mask = 1U << (15 - tablebits);
    for (ch = 0; ch < nchar; ch++)
    {
        if ((len = bitlen[ch]) == 0)
            continue;
        nextcode = start[len] + weight[len];
        if (len <= tablebits)
        {
            for (i = start[len]; i < nextcode; i++)
                table[i] = ch;
        }
        else
        {
            k = start[len];
            p = &table[k >> jutbits];
            i = len - tablebits;
            while (i != 0)
            {
                if (*p == 0)
                {
                    G->right[avail] = G->left[avail] = 0;
                    *p = avail++;
                }
                if (k & mask)
                    p = &G->right[*p];
                else
                    p = &G->left[*p];
                k <<= 1;
                i--;
            }
            *p = ch;
        }
        start[len] = nextcode;
    }
    return (0);
}

static void read_pt_len (lzh_globals_x *G, int nn, int nbit, int i_special)
{
    int i, n;
    short c;
    ushort mask;

    n = getbits (G, nbit);
    if (n == 0)
    {
        c = getbits (G, nbit);
        for (i = 0; i < nn; i++)
            G->pt_len[i] = 0;
        for (i = 0; i < 256; i++)
            G->pt_table[i] = c;
    }
    else
    {
        i = 0;
        while (i < n)
        {
            c = G->bitbuf >> (BITBUFSIZ - 3);
            if (c == 7)
            {
                mask = 1U << (BITBUFSIZ - 1 - 3);
                while (mask & G->bitbuf)
                {
                    mask >>= 1;
                    c++;
                }
            }
            fillbuf (G, (c < 7) ? 3 : c - 3);
            G->pt_len[i++] = c;
            if (i == i_special)
            {
                c = getbits (G, 2);
                while (--c >= 0)
                    G->pt_len[i++] = 0;
            }
        }
        while (i < nn)
            G->pt_len[i++] = 0;
        make_table (G, nn, G->pt_len, 8, G->pt_table);
    }
}

static void read_c_len (lzh_globals_x *G)
{
    short i, c, n;
    ushort mask;

    n = getbits (G, CBIT);
    if (n == 0)
    {
        c = getbits (G, CBIT);
        for (i = 0; i < NC; i++)
            G->c_len[i] = 0;
        for (i = 0; i < 4096; i++)
            G->c_table[i] = c;
    }
    else
    {
        i = 0;
        while (i < n)
        {
            c = G->pt_table[G->bitbuf >> (BITBUFSIZ - 8)];
            if (c >= NT)
            {
                mask = 1U << (BITBUFSIZ - 1 - 8);
                do
                {
                    if (G->bitbuf & mask)
			c = G->right[c];
                    else
			c = G->left[c];
                    mask >>= 1;
                } while (c >= NT);
            }
            fillbuf (G, G->pt_len[c]);
            if (c <= 2)
            {
                if (c == 0)
                    c = 1;
                else if (c == 1)
                    c = getbits (G, 4) + 3;
                else
                    c = getbits (G, CBIT) + 20;
                while (--c >= 0)
                    G->c_len[i++] = 0;
            }
            else
                G->c_len[i++] = c - 2;
        }
        while (i < NC)
            G->c_len[i++] = 0;
        make_table (G, NC, G->c_len, 12, G->c_table);
    }
}

ushort decode_c (lzh_globals_x *G)
{
    ushort j, mask;

    if (G->blocksize == 0)
    {
        G->blocksize = getbits (G, 16);
        read_pt_len (G, NT, TBIT, 3);
        read_c_len (G);
        read_pt_len (G, NP, PBIT, -1);
    }
    G->blocksize--;
    j = G->c_table[G->bitbuf >> (BITBUFSIZ - 12)];
    if (j >= NC)
    {
        mask = 1U << (BITBUFSIZ - 1 - 12);
        do
        {
            if (G->bitbuf & mask)
		j = G->right[j];
            else
		j = G->left[j];
            mask >>= 1;
        } while (j >= NC);
    }
    fillbuf (G, G->c_len[j]);
    return j;
}

ushort decode_p (lzh_globals_x *G)
{
    ushort j, mask;

    j = G->pt_table[G->bitbuf >> (BITBUFSIZ - 8)];
    if (j >= NP)
    {
        mask = 1U << (BITBUFSIZ - 1 - 8);
        do
        {
            if (G->bitbuf & mask)
		j = G->right[j];
            else
		j = G->left[j];
            mask >>= 1;
        } while (j >= NP);
    }
    fillbuf (G, G->pt_len[j]);
    if (j != 0)
        j = (1U << (j - 1)) + getbits (G, j - 1);
    return j;
}

void huf_decode_start (lzh_globals_x *G)
{
    init_getbits (G);
    G->blocksize = 0;
}

static void decode_start (lzh_globals_x *G)
{
    G->fillbufsize = 0;
    huf_decode_start (G);
    G->decode_j = 0;
}

/*
 * The calling function must keep the number of bytes to be processed.  This
 * function decodes either 'count' bytes or 'DICSIZ' bytes, whichever is
 * smaller, into the array 'buffer[]' of size 'DICSIZ' or more. Call
 * decode_start() once for each new file before calling this function.
 */
static void decode (lzh_globals_x *G, uint count, uchar buffer[])
{
    static uint i;
    uint r, c;

    r = 0;
    while (--G->decode_j >= 0)
    {
        buffer[r] = buffer[i];
        i = (i + 1) & (DICSIZ - 1);
        if (++r == count)
            return;
    }
    for (;;)
    {
        c = decode_c (G);
        if (c <= UCHAR_MAX)
        {
            buffer[r] = c;
            if (++r == count)
                return;
        }
        else
        {
            G->decode_j = c - (UCHAR_MAX + 1 - THRESHOLD);
            i = (r - decode_p (G) - 1) & (DICSIZ - 1);
            while (--G->decode_j >= 0)
            {
                buffer[r] = buffer[i];
                i = (i + 1) & (DICSIZ - 1);
                if (++r == count)
                    return;
            }
        }
    }
}

int lzh_melt (type_fnc_write  pfnc_read,
	      type_fnc_read   pfnc_write,
	      type_fnc_malloc pfnc_malloc,
	      type_fnc_free   pfnc_free,
	      ulong origsize,
	      void *p)
{
    int n;
    uchar *outbuf;
    lzh_globals_x *G=pfnc_malloc(sizeof(lzh_globals_x));
    if(!G)
	    return 1;

    G->fnc_write  = pfnc_write;
    G->fnc_read	  = pfnc_read;
    G->fnc_malloc = pfnc_malloc;
    G->fnc_free	  = pfnc_free;
    G->p          = p;

    G->with_error = 0;

    if ((G->buf = G->fnc_malloc (BUFSIZE)) == 0L)
	return (1);
    if ((outbuf = G->fnc_malloc (DICSIZ)) == 0L)
    {
	G->fnc_free (G->buf);
	return (1);
    }

    decode_start (G);
    while (origsize != 0)
    {
        n = (uint) ((origsize > DICSIZ) ? DICSIZ : origsize);
        decode (G, n, outbuf);
	if (G->with_error)
	    return (1);
	G->fnc_write (outbuf, n, G->p);
        origsize -= n;
	if (G->with_error)
	    return (1);
    }
    return (0);
}

/* end of LZHXLib.C */
