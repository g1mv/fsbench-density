/**
 *
 *  LZHCLib.C - Compression module
 *
 *  Compression Library (LZH) from Haruhiko Okumura's "ar".
 *
 *  Copyright(c) 1996 Kerwin F. Medina
 *  Copyright(c) 1991 Haruhiko Okumura
 *
 *  In MSDOS, this MUST be compiled using compact, large,
 *      or huge memory models
 *
 *  To test, compile as below to create a stand-alone compressor (will
 *      compress standard input to standard output):
 *          bcc -O2 -mc -D__TEST__ lzhclib.c
 *              or
 *          gcc -O2 -D__TEST__ lzhclib.c
 *      then run as: lzhclib <infile >outfile
 *
 *  To use as a library for your application, __TEST__ must
 *      not be defined.
 *
 */

#include "lzh.h"

#define BUFSIZ (1024 * 16)

#define OUTBUFSIZE (1024 * 4)
#define PERCOLATE  1
#define NIL        0
#define MAX_HASH_VAL (3 * DICSIZ + (DICSIZ / 512 + 1) * UCHAR_MAX)
#define PERC_FLAG 0x8000U

#define MAX_HASH_VAL (3 * DICSIZ + (DICSIZ / 512 + 1) * UCHAR_MAX)
#define HASH(p, c) ((p) + ((c) << (DICBIT - 9)) + DICSIZ * 2)

struct struct_mem_list {
    void_pointer *p;
    int size;
};

/* Globals */
typedef struct {
	uchar  *outbuf;
	int outbufpos;
	type_fnc_read   fnc_read;
	type_fnc_write  fnc_write;
	type_fnc_malloc fnc_malloc;
	type_fnc_free   fnc_free;
	int error_write;
	uint subbitbuf;
	int bitcount;
	void *p;
	/* maketree */
	int maketree_n, maketree_heapsize;
	short maketree_heap[NC + 1];
	ushort  *maketree_freq,  *maketree_sortptr, maketree_len_cnt[17];
	uchar  *maketree_len;
	short left[2 * NC - 1], right[2 * NC - 1];
	/* huffman */
	uchar  *buf,  c_len[NC],  pt_len[NPT];
	ushort  c_freq[2 * NC - 1],  c_code[NC],
               p_freq[2 * NP - 1],  pt_code[NPT],
              t_freq[2 * NT - 1];
	uint output_pos, output_mask;
	/* encode */
	uchar  *text,  *childcount;
	node pos, matchpos, avail,
             *position,  *parent,
             *prev,  *next;
	int remainder, matchlen;
	/* Was Static Shit */
	uint cpos;
	int depth;
	#if MAXMATCH <= (UCHAR_MAX + 1)
	uchar *level;
	#else
	ushort *level;
	#endif
} lzh_globals_c;


/*
 * Write rightmost n bits of x
 */
void putbits (lzh_globals_c *G, int n, uint x)
{
    if (n < G->bitcount)
    {
         G->subbitbuf |= x << (G->bitcount -= n);
    }
    else
    {
        G->outbuf[G->outbufpos++] = G->subbitbuf | (x >> (n -= G->bitcount));
        if (G->outbufpos >= OUTBUFSIZE)
        {
	    if (G->fnc_write (G->outbuf, G->outbufpos, G->p) != G->outbufpos)
                G->error_write = 1;
            G->outbufpos = 0;
        }
        if (n < CHAR_BIT)
        {
            G->subbitbuf = x << (G->bitcount = CHAR_BIT - n);
        }
        else
        {
            G->outbuf[G->outbufpos++] = x >> (n - CHAR_BIT);
            if (G->outbufpos >= OUTBUFSIZE)
            {
		if (G->fnc_write (G->outbuf, G->outbufpos, G->p) != G->outbufpos)
                    G->error_write = 1;
                G->outbufpos = 0;
            }
            G->subbitbuf = x << (G->bitcount = 2 * CHAR_BIT - n);
        }
    }
}

void init_putbits (lzh_globals_c *G)
{
    G->bitcount = CHAR_BIT;
    G->subbitbuf = 0;
}

/*
 * maketree.c
 */

void count_len (lzh_globals_c *G, int i)          /* call with i = root */
{
    if (i < G->maketree_n)
        G->maketree_len_cnt[(G->depth < 16) ? G->depth : 16]++;
    else
    {
        G->depth++;
        count_len (G, G->left[i]);
        count_len (G, G->right[i]);
        G->depth--;
    }
}

void make_len (lzh_globals_c *G, int root)
{
    int i, k;
    uint cum;

    for (i = 0; i <= 16; i++)
        G->maketree_len_cnt[i] = 0;
    count_len (G, root);
    cum = 0;
    for (i = 16; i > 0; i--)
        cum += G->maketree_len_cnt[i] << (16 - i);
    while (cum != (1U << 16))
    {
      #if 0
        fflush (stderr);
        fprintf (stderr, "17");
        fflush (stderr);
      #endif
        G->maketree_len_cnt[16]--;
        for (i = 15; i > 0; i--)
        {
            if (G->maketree_len_cnt[i] != 0)
            {
                G->maketree_len_cnt[i]--;
                G->maketree_len_cnt[i + 1] += 2;
                break;
            }
        }
        cum--;
    }
    for (i = 16; i > 0; i--)
    {
        k = G->maketree_len_cnt[i];
        while (--k >= 0)
            G->maketree_len[*G->maketree_sortptr++] = i;
    }
}

/*
 * priority queue; send i-th entry down maketree_heap
 */
void downheap (lzh_globals_c *G, int i)
{
    int j, k;

    k = G->maketree_heap[i];
    while ((j = 2 * i) <= G->maketree_heapsize)
    {
        if (j < G->maketree_heapsize && G->maketree_freq[G->maketree_heap[j]] > G->maketree_freq[G->maketree_heap[j + 1]])
            j++;
        if (G->maketree_freq[k] <= G->maketree_freq[G->maketree_heap[j]])
            break;
        G->maketree_heap[i] = G->maketree_heap[j];
        i = j;
    }
    G->maketree_heap[i] = k;
}

void make_code (lzh_globals_c *G, int maketree_n, uchar  *maketree_len, ushort  *code)
{
    int i;
    ushort start[18];

    start[1] = 0;
    for (i = 1; i <= 16; i++)
        start[i + 1] = (start[i] + G->maketree_len_cnt[i]) << 1;
    for (i = 0; i < maketree_n; i++)
        code[i] = start[maketree_len[i]]++;
}

/*
 * make tree, calculate maketree_len[], return root
 */
int make_tree (lzh_globals_c *G, int nparm, ushort  *freqparm,
		uchar  *lenparm, ushort  *codeparm)
{
    int i, j, k, avail;

    G->maketree_n = nparm;
    G->maketree_freq = freqparm;
    G->maketree_len = lenparm;
    avail = G->maketree_n;
    G->maketree_heapsize = 0;
    G->maketree_heap[1] = 0;
    for (i = 0; i < G->maketree_n; i++)
    {
        G->maketree_len[i] = 0;
        if (G->maketree_freq[i])
            G->maketree_heap[++G->maketree_heapsize] = i;
    }
    if (G->maketree_heapsize < 2)
    {
        codeparm[G->maketree_heap[1]] = 0;
        return G->maketree_heap[1];
    }
    for (i = G->maketree_heapsize / 2; i >= 1; i--)
        downheap (G, i);                  /* make priority queue */
    G->maketree_sortptr = codeparm;
    do
    {   /* while queue has at least two entries */
        i = G->maketree_heap[1];   /* take out least-maketree_freq entry */
        if (i < G->maketree_n)
            *G->maketree_sortptr++ = i;
        G->maketree_heap[1] = G->maketree_heap[G->maketree_heapsize--];
        downheap (G, 1);
        j = G->maketree_heap[1];   /* next least-maketree_freq entry */
        if (j < G->maketree_n)
            *G->maketree_sortptr++ = j;
        k = avail++;                   /* generate new node */
        G->maketree_freq[k] = G->maketree_freq[i] + G->maketree_freq[j];
        G->maketree_heap[1] = k;
        downheap (G, 1);                  /* put into queue */
        G->left[k] = i;
        G->right[k] = j;
    } while (G->maketree_heapsize > 1);
    G->maketree_sortptr = codeparm;
    make_len (G, k);
    make_code (G, nparm, lenparm, codeparm);
    return k;                          /* return root */
}

/*
 * huf.c
 */

void count_t_freq (lzh_globals_c *G)
{
    int i, k, n, count;

    for (i = 0; i < NT; i++)
        G->t_freq[i] = 0;
    n = NC;
    while (n > 0 && G->c_len[n - 1] == 0)
        n--;
    i = 0;
    while (i < n)
    {
        k = G->c_len[i++];
        if (k == 0)
        {
            count = 1;
            while (i < n && G->c_len[i] == 0)
            {
                i++;
                count++;
            }
            if (count <= 2)
                G->t_freq[0] += count;
            else if (count <= 18)
                G->t_freq[1]++;
            else if (count == 19)
            {
                G->t_freq[0]++;
                G->t_freq[1]++;
            }
            else
                G->t_freq[2]++;
        }
        else
            G->t_freq[k + 2]++;
    }
}

void write_pt_len (lzh_globals_c *G, int n, int nbit, int i_special)
{
    int i, k;

    while (n > 0 && G->pt_len[n - 1] == 0)
        n--;
    putbits (G, nbit, n);
    i = 0;
    while (i < n)
    {
        k = G->pt_len[i++];
        if (k <= 6)
            putbits (G, 3, k);
        else
            putbits (G, k - 3, (1U << (k - 3)) - 2);
        if (i == i_special)
        {
            while (i < 6 && G->pt_len[i] == 0)
                i++;
            putbits (G, 2, (i - 3) & 3);
        }
    }
}

void write_c_len (lzh_globals_c *G)
{
    int i, k, n, count;

    n = NC;
    while (n > 0 && G->c_len[n - 1] == 0)
        n--;
    putbits (G, CBIT, n);
    i = 0;
    while (i < n)
    {
        k = G->c_len[i++];
        if (k == 0)
        {
            count = 1;
            while (i < n && G->c_len[i] == 0)
            {
                i++;
                count++;
            }
            if (count <= 2)
            {
                for (k = 0; k < count; k++)
                    putbits (G, G->pt_len[0], G->pt_code[0]);
            }
            else if (count <= 18)
            {
                putbits (G, G->pt_len[1], G->pt_code[1]);
                putbits (G, 4, count - 3);
            }
            else if (count == 19)
            {
                putbits (G, G->pt_len[0], G->pt_code[0]);
                putbits (G, G->pt_len[1], G->pt_code[1]);
                putbits (G, 4, 15);
            }
            else
            {
                putbits (G, G->pt_len[2], G->pt_code[2]);
                putbits (G, CBIT, count - 20);
            }
        }
        else
            putbits (G, G->pt_len[k + 2], G->pt_code[k + 2]);
    }
}

void encode_c (lzh_globals_c *G, int c)
{
     putbits (G, G->c_len[c], G->c_code[c]);
}

void encode_p (lzh_globals_c *G, uint p)
{
    uint c, q;

    c = 0;
    q = p;
    while (q)
    {
        q >>= 1;
        c++;
    }
    putbits (G, G->pt_len[c], G->pt_code[c]);
    if (c > 1)
        putbits (G, c - 1, p & (0xFFFFU >> (17 - c)));
}

void send_block (lzh_globals_c *G)
{
    uint i, k, flags, root, pos, size;

    root = make_tree (G, NC, G->c_freq, G->c_len, G->c_code);
    size = G->c_freq[root];
    putbits (G, 16, size);
    if (root >= NC)
    {
        count_t_freq (G);
        root = make_tree (G, NT, G->t_freq, G->pt_len, G->pt_code);
        if (root >= NT)
        {
            write_pt_len (G, NT, TBIT, 3);
        }
        else
        {
            putbits (G, TBIT, 0);
            putbits (G, TBIT, root);
        }
        write_c_len (G);
    }
    else
    {
        putbits (G, TBIT, 0);
        putbits (G, TBIT, 0);
        putbits (G, CBIT, 0);
        putbits (G, CBIT, root);
    }
    root = make_tree (G, NP, G->p_freq, G->pt_len, G->pt_code);
    if (root >= NP)
    {
        write_pt_len (G, NP, PBIT, -1);
    }
    else
    {
        putbits (G, PBIT, 0);
        putbits (G, PBIT, root);
    }
    pos = 0;
    for (i = 0; i < size; i++)
    {
        if (i % CHAR_BIT == 0)
            flags = G->buf[pos++];
        else
            flags <<= 1;
        if (flags & (1U << (CHAR_BIT - 1)))
        {
            encode_c (G, G->buf[pos++] + (1U << CHAR_BIT));
            k = G->buf[pos++] << CHAR_BIT;
            k += G->buf[pos++];
            encode_p (G, k);
        }
        else
            encode_c (G, G->buf[pos++]);
        if (G->error_write)
            return;
    }
    for (i = 0; i < NC; i++)
        G->c_freq[i] = 0;
    for (i = 0; i < NP; i++)
        G->p_freq[i] = 0;
}

void output (lzh_globals_c *G, uint c, uint p)
{
    if ((G->output_mask >>= 1) == 0)
    {
        G->output_mask = 1U << (CHAR_BIT - 1);
	if (G->output_pos >= BUFSIZ - 3 * CHAR_BIT)
        {
            send_block (G);
            if (G->error_write)
                return;
            G->output_pos = 0;
        }
        G->cpos = G->output_pos++;
        G->buf[G->cpos] = 0;
    }
    G->buf[G->output_pos++] = (uchar) c;
    G->c_freq[c]++;
    if (c >= (1U << CHAR_BIT))
    {
        G->buf[G->cpos] |= G->output_mask;
        G->buf[G->output_pos++] = (uchar) (p >> CHAR_BIT);
        G->buf[G->output_pos++] = (uchar) p;
        c = 0;
        while (p)
        {
            p >>= 1;
            c++;
        }
        G->p_freq[c]++;
    }
}

void huf_encode_start (lzh_globals_c *G)
{
    int i;

    G->buf[0] = 0;
    for (i = 0; i < NC; i++)
        G->c_freq[i] = 0;
    for (i = 0; i < NP; i++)
        G->p_freq[i] = 0;
    G->output_pos = G->output_mask = 0;
    init_putbits (G);
}

void huf_encode_end (lzh_globals_c *G)
{
    if (!G->error_write)
    {
        send_block (G);
        putbits (G, CHAR_BIT - 1, 0);     /* flush remaining bits */
    }
    if (G->outbufpos > 0)
	G->fnc_write (G->outbuf, G->outbufpos, G->p);
}

/*
 * encode.c
 */

void init_slide (lzh_globals_c *G)
{
    node i;

    for (i = DICSIZ; i <= DICSIZ + UCHAR_MAX; i++)
    {
        G->level[i] = 1;
#if PERCOLATE
        G->position[i] = NIL;             /* sentinel */
#endif
    }
    for (i = DICSIZ; i < DICSIZ * 2; i++)
        G->parent[i] = NIL;
    G->avail = 1;
    for (i = 1; i < DICSIZ - 1; i++)
        G->next[i] = i + 1;
    G->next[DICSIZ - 1] = NIL;
    for (i = DICSIZ * 2; i <= MAX_HASH_VAL; i++)
        G->next[i] = NIL;
}



node child (lzh_globals_c *G, node q, uchar c)
/* q's child for character c (NIL if not found) */
{
    node r;

    r = G->next[HASH (q, c)];
    G->parent[NIL] = q;                   /* sentinel */
    while (G->parent[r] != q)
        r = G->next[r];
    return r;
}

void makechild (lzh_globals_c *G, node q, uchar c, node r)
/* Let r be q's child for character c. */
{
    node h, t;

    h = HASH (q, c);
    t = G->next[h];
    G->next[h] = r;
    G->next[r] = t;
    G->prev[t] = r;
    G->prev[r] = h;
    G->parent[r] = q;
    G->childcount[q]++;
}

void split (lzh_globals_c *G, node old)
{
    node new, t;

    new = G->avail;
    G->avail = G->next[new];
    G->childcount[new] = 0;
    t = G->prev[old];
    G->prev[new] = t;
    G->next[t] = new;
    t = G->next[old];
    G->next[new] = t;
    G->prev[t] = new;
    G->parent[new] = G->parent[old];
    G->level[new] = G->matchlen;
    G->position[new] = G->pos;
    makechild (G, new, G->text[G->matchpos + G->matchlen], old);
    makechild (G, new, G->text[G->pos + G->matchlen], G->pos);
}

void insert_node (lzh_globals_c *G)
{
    node q, r, j, t;
    uchar c,  *t1,  *t2;

    if (G->matchlen >= 4)
    {
        G->matchlen--;
        r = (G->matchpos + 1) | DICSIZ;
        while ((q = G->parent[r]) == NIL)
            r = G->next[r];
        while (G->level[q] >= G->matchlen)
        {
            r = q;
            q = G->parent[q];
        }
#if PERCOLATE
        t = q;
        while (G->position[t] < 0)
        {
            G->position[t] = G->pos;
            t = G->parent[t];
        }
        if (t < DICSIZ)
            G->position[t] = G->pos | PERC_FLAG;
#else
        t = q;
        while (t < DICSIZ)
        {
            G->position[t] = G->pos;
            t = G->parent[t];
        }
#endif
    }
    else
    {
        q = G->text[G->pos] + DICSIZ;
        c = G->text[G->pos + 1];
        if ((r = child (G, q, c)) == NIL)
        {
            makechild (G, q, c, G->pos);
            G->matchlen = 1;
            return;
        }
        G->matchlen = 2;
    }
    for (;;)
    {
        if (r >= DICSIZ)
        {
            j = MAXMATCH;
            G->matchpos = r;
        }
        else
        {
            j = G->level[r];
            G->matchpos = G->position[r] & ~PERC_FLAG;
        }
        if (G->matchpos >= G->pos)
            G->matchpos -= DICSIZ;
        t1 = &G->text[G->pos + G->matchlen];
        t2 = &G->text[G->matchpos + G->matchlen];
        while (G->matchlen < j)
        {
            if (*t1 != *t2)
            {
                split (G, r);
                return;
            }
            G->matchlen++;
            t1++;
            t2++;
        }
        if (G->matchlen >= MAXMATCH)
            break;
        G->position[r] = G->pos;
        q = r;
        if ((r = child (G, q, *t1)) == NIL)
        {
            makechild (G, q, *t1, G->pos);
            return;
        }
        G->matchlen++;
    }
    t = G->prev[r];
    G->prev[G->pos] = t;
    G->next[t] = G->pos;
    t = G->next[r];
    G->next[G->pos] = t;
    G->prev[t] = G->pos;
    G->parent[G->pos] = q;
    G->parent[r] = NIL;
    G->next[r] = G->pos;                     /* special use of next[] */
}

void delete_node (lzh_globals_c *G)
{
#if PERCOLATE
    node q, r, s, t, u;
#else
    node r, s, t, u;
#endif

    if (G->parent[G->pos] == NIL)
        return;
    r = G->prev[G->pos];
    s = G->next[G->pos];
    G->next[r] = s;
    G->prev[s] = r;
    r = G->parent[G->pos];
    G->parent[G->pos] = NIL;
    if (r >= DICSIZ || --G->childcount[r] > 1)
        return;
#if PERCOLATE
    t = G->position[r] & ~PERC_FLAG;
#else
    t = G->position[r];
#endif
    if (t >= G->pos)
        t -= DICSIZ;
#if PERCOLATE
    s = t;
    q = G->parent[r];
    while ((u = G->position[q]) & PERC_FLAG)
    {
        u &= ~PERC_FLAG;
        if (u >= G->pos)
            u -= DICSIZ;
        if (u > s)
            s = u;
        G->position[q] = (s | DICSIZ);
        q = G->parent[q];
    }
    if (q < DICSIZ)
    {
        if (u >= G->pos)
            u -= DICSIZ;
        if (u > s)
            s = u;
        G->position[q] = s | DICSIZ | PERC_FLAG;
    }
#endif
    s = child (G, r, G->text[t + G->level[r]]);
    t = G->prev[s];
    u = G->next[s];
    G->next[t] = u;
    G->prev[u] = t;
    t = G->prev[r];
    G->next[t] = s;
    G->prev[s] = t;
    t = G->next[r];
    G->prev[t] = s;
    G->next[s] = t;
    G->parent[s] = G->parent[r];
    G->parent[r] = NIL;
    G->next[r] = G->avail;
    G->avail = r;
}

void get_next_match (lzh_globals_c *G)
{
    int n;

    G->remainder--;
    if (++G->pos == DICSIZ * 2)
    {
	memcpy (&G->text[0], &G->text[DICSIZ], DICSIZ + MAXMATCH);
	n = G->fnc_read (&G->text[DICSIZ + MAXMATCH], DICSIZ, G->p);
	if (n < 0)
            n = 0;
        G->remainder += n;
        G->pos = DICSIZ;
    }
    delete_node (G);
    insert_node (G);
}

/*
 * Additions
 */


int allocate_memory (lzh_globals_c *G)
{
    struct struct_mem_list mem_list[] = {
    	{ (void *)&G->text      , DICSIZ * 2 + MAXMATCH                              },
    	{ (void *)&G->level     , (DICSIZ + UCHAR_MAX + 1) * sizeof (*G->level)      },
    	{ (void *)&G->childcount, (DICSIZ + UCHAR_MAX + 1) * sizeof (*G->childcount) },
    	{ (void *)&G->position  , (DICSIZ + UCHAR_MAX + 1) * sizeof (*G->position)   },
    	{ (void *)&G->parent    , DICSIZ * 2 * sizeof (*G->parent)                   },
    	{ (void *)&G->prev      , DICSIZ * 2 * sizeof (*G->prev)                     },
    	{ (void *)&G->next      , (MAX_HASH_VAL + 1) * sizeof (*G->next)             },
    	{ (void *)&G->buf       , BUFSIZ                                             },
    	{ (void *)&G->outbuf    , OUTBUFSIZE                                         },
    	{ 0L         , 0                                                             }
    };
    struct struct_mem_list * p = mem_list, *q = mem_list;
    while (p->size)
    {
	if ((*(p->p) = G->fnc_malloc (p->size)) == 0L)
	{
	    while (q != p)
		G->fnc_free (*(q->p)), q++;
	    return (1);
	}
	p++;
    }
    return (0);
}

void release_memory (lzh_globals_c *G)
{
    struct struct_mem_list mem_list[] = {
    	{ (void *)&G->text      , DICSIZ * 2 + MAXMATCH                              },
    	{ (void *)&G->level     , (DICSIZ + UCHAR_MAX + 1) * sizeof (*G->level)      },
    	{ (void *)&G->childcount, (DICSIZ + UCHAR_MAX + 1) * sizeof (*G->childcount) },
    	{ (void *)&G->position  , (DICSIZ + UCHAR_MAX + 1) * sizeof (*G->position)   },
    	{ (void *)&G->parent    , DICSIZ * 2 * sizeof (*G->parent)                   },
    	{ (void *)&G->prev      , DICSIZ * 2 * sizeof (*G->prev)                     },
    	{ (void *)&G->next      , (MAX_HASH_VAL + 1) * sizeof (*G->next)             },
    	{ (void *)&G->buf       , BUFSIZ                                             },
    	{ (void *)&G->outbuf    , OUTBUFSIZE                                         },
    	{ 0L         , 0                                                             }
    };
    struct struct_mem_list * p = mem_list;
    while (p->size)
	G->fnc_free (*(p->p)), p++;
}

int lzh_freeze (type_fnc_write  pfnc_read,
		type_fnc_read   pfnc_write,
		type_fnc_malloc pfnc_malloc,
		type_fnc_free   pfnc_free,
		void *p)
{
    int lastmatchlen;
    node lastmatchpos;
    lzh_globals_c *G=pfnc_malloc(sizeof(lzh_globals_c));
    if(!G)
	    return 1;
    /* Additions */
    G->fnc_write  = pfnc_write;
    G->fnc_read	  = pfnc_read;
    G->fnc_malloc = pfnc_malloc;
    G->fnc_free	  = pfnc_free;
    G->p          = p;
    G->depth      = 0;

    if (allocate_memory(G) != 0)
	return (1);
    G->error_write = 0;
    G->outbufpos = 0;

    /* encode.c :: void encode (void) */

    init_slide (G);
    huf_encode_start (G);
    G->remainder = G->fnc_read (&G->text[DICSIZ], DICSIZ + MAXMATCH, G->p);
    if (G->remainder < 0)
	G->remainder = 0;
    G->matchlen = 0;
    G->pos = DICSIZ;
    insert_node (G);
    if (G->matchlen > G->remainder)
        G->matchlen = G->remainder;
    while (G->remainder > 0 && !G->error_write)
    {
        lastmatchlen = G->matchlen;
        lastmatchpos = G->matchpos;
        get_next_match (G);
        if (G->matchlen > G->remainder)
            G->matchlen = G->remainder;
        if (G->matchlen > lastmatchlen || lastmatchlen < THRESHOLD)
            output (G, G->text[G->pos - 1], 0);
        else
        {
            output (G, lastmatchlen + (UCHAR_MAX + 1 - THRESHOLD),
                    (G->pos - lastmatchpos - 2) & (DICSIZ - 1));
            while (--lastmatchlen > 0)
                get_next_match (G);
            if (G->matchlen > G->remainder)
                G->matchlen = G->remainder;
        }
    }
    huf_encode_end (G);

    release_memory (G);
    return (0);
}

/* end of LZHCLib.C */
