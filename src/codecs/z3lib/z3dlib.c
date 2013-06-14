/*
 * z3lib (c)1993,2000,2005,2006/GPL,BSD Oskar Schirmer, schirmer@scara.com
 * combined huffman/lz compression library
 * now rfc1951 conformant
 */

// #include <stdio.h>
// #define D(x) x

#include "z3lib.h"
#include "z3liblib.h"
#include "z3crc32.h"
#include "z3blib.h"
#include "z3dlib.h"

#ifndef D
#define D(x)
#endif

#ifndef Z3LIB_DECODE_ONLY
#define Z3DE_NCLIMIT ((1UL << 16) - 1 - Z3B_LZMINLENGTH)

static void z3de_setnclimit(struct z3de_handle *zh)
{
  __u32 n;
  if (zh->nclimit > 0) {
    n = zh->lastlimit - zh->tellnext;
    if (n < zh->nclimit) {
      zh->nclimit -= n;
    } else {
      zh->nclimit = 0;
    }
  }
}

static void z3de_settellnext(struct z3de_handle *zh, __u32 leftover)
{
  __u32 n;
  if ((zh->nclimit > 0) && (zh->nclimit < (zh->tellfreq*5/4))) {
    n = zh->nclimit;
  } else {
    n = zh->tellfreq;
  }
  zh->lastlimit = n;
  if (n > leftover) {
    n -= leftover;
  } else {
    n = 0;
  }
  zh->tellnext = n;
}

//leftover: when worse, the bytes taken no more, go into next block.
//ito: the bytes in pipe in z3blib.

struct z3de_handle *z3d_encode_init(void *memory, unsigned int memsize,
                        __u32 tellwhen,
                        __u8 thrmin, __u8 thrmax, __u32 initialgrant,
                        int preferlonger, int limitlength3)
{
  struct z3de_handle *zh;
  __u32 t;
  if ((memory == NULL) || (memsize < (limitlength3 ?
        (Z3DE_MEMSIZE_MIN + Z3BE_MEMSIZE_EXTRA3) : Z3DE_MEMSIZE_MIN))) {
    return NULL;
  }
  zh = (struct z3de_handle *)memory;
  if (z3be_start(&zh->z3be,
        memsize - sizeof(struct z3de_handle) + sizeof(struct z3be_handle),
        preferlonger, limitlength3)
      != &zh->z3be) {
    return NULL;
  }
  if (tellwhen == 0) {
    t = (__u32)-1;
  } else if (tellwhen < Z3DE_SLICE_MIN) {
    t = Z3DE_SLICE_MIN;
  } else {
    t = tellwhen;
  }
  zh->tellfreq = t;
  zh->nclimit = Z3DE_NCLIMIT;
  z3de_settellnext(zh, 0);
  zh->state = z3de_state_fill;
  zh->weights = 0;
  zh->thresholdmin = thrmin;
  zh->thresholdmax = thrmax;
  zh->initialgrant = initialgrant;
  return zh;
}

static void z3de_trimquotient(__u32 *src, __u32 *trg)
{
#define Z3DE_LIMITQ (1UL << ((32-8)/2))
  __u32 s, t;
  s = *src;
  t = *trg;
  if ((t > (Z3DE_LIMITQ*2)) || (s > (Z3DE_LIMITQ/2))) {
D(fprintf(stderr,"TRIM(%lu) %lu/%lu --> ", Z3DE_LIMITQ, t, s);)
    do {
      s >>= 1;
      t >>= 1;
    } while ((t > (Z3DE_LIMITQ*2)) || (s > (Z3DE_LIMITQ/2)));
    *src = s;
    *trg = t;
D(fprintf(stderr," %lu/%lu\n", t, s);)
  }
}

static int z3de_isworseestimate(struct z3de_handle *zh,
                        struct z3be_weighing *v, struct z3be_weighing *w)
{
  __u32 src, trg, thr;
  src = w->wpos - v->wpos;
  trg = w->len[w->btype] - v->len[v->btype] + 8 * zh->initialgrant;
D({int d=0;char c[999];sprintf(&c[0],"  min:%5d/%5d %%c now:%5d/%5d %%c max:%5d/%5d,%%9.5f [%4.2f;%4.2f]\n", zh->trgmin/8, zh->srcmin, trg/8, src, zh->trgmax/8, zh->srcmax, ((double)zh->thresholdmin)/Z3DE_THRESHOLD_ONE, ((double)zh->thresholdmax)/Z3DE_THRESHOLD_ONE);)
  z3de_trimquotient(&src, &trg);
  if ((trg * zh->srcmin) < (src * zh->trgmin)) {
    zh->trgmin = trg;
    zh->srcmin = src;
    thr = zh->thresholdmin;
D(fprintf(stderr,&c[0],'>','<',((((double)zh->trgmin)/(double)zh->srcmin)/(((double)zh->trgmax)/(double)zh->srcmax)));)
  } else if ((trg * zh->srcmax) > (src * zh->trgmax)) {
    zh->trgmax = trg;
    zh->srcmax = src;
    thr = zh->thresholdmax;
D(fprintf(stderr,&c[0],'<','>',((((double)zh->trgmin)/(double)zh->srcmin)/(((double)zh->trgmax)/(double)zh->srcmax)));)
  } else {
D(fprintf(stderr,&c[0],'<','<',((((double)zh->trgmin)/(double)zh->srcmin)/(((double)zh->trgmax)/(double)zh->srcmax)));)
    return 0;
  }
D(})
  return (Z3DE_THRESHOLD_ONE * zh->trgmin * zh->srcmax)
       < (thr * zh->trgmax * zh->srcmin);
}

/* >= 0: is not worse, returns result of tell
 * < 0: is worse. */
static int z3de_isworse(struct z3de_handle *zh)
{
  int worse;
  __u32 t, src, trg, ito;
  struct z3be_weighing *w, *v;
  w = &zh->weight[zh->weights & 1];
  t = z3be_tell(&zh->z3be, w, &ito);
D({fprintf(stderr,"tell(%3d) b=%d s=%5d s/w:%6.2f%% (w=%5d, nc:%5d, y=%7.5f)\n", zh->weights, w->btype, w->len[w->btype]/8, ((double)w->len[w->btype])/((double)w->wpos)*12.5, w->wpos, zh->nclimit, (((double)zh->trgmin)/(double)zh->srcmin)/(((double)zh->trgmax)/(double)zh->srcmax));})
  if (zh->weights <= 0) {
    src = w->wpos;
    trg = w->len[w->btype];
    z3de_trimquotient(&src, &trg);
    zh->srcmin = zh->srcmax = src;
    zh->trgmin = zh->trgmax = trg;
    zh->weights += 1;
    zh->leftover = ito;
D(fprintf(stderr,"z3de_isworse: w=%2d, final:%d, tell=%u\n", zh->weights, zh->z3be.bfinal, t);)
    return t;
  }
  v = &zh->weight[(zh->weights-1) & 1];
  if (w->btype == Z3B_BTYPE_NC) {
    worse = (v->btype != Z3B_BTYPE_NC);
  } else {
    worse = z3de_isworseestimate(zh, v, w);
  }
  if (worse) {
    zh->leftover = ito + w->wpos - v->wpos;
D(fprintf(stderr,"z3de_isworse: w=%2d, final:%d, yes, leftover=%lu\n", zh->weights, zh->z3be.bfinal, zh->leftover);)
    return -1;
  }
  zh->weights += 1;
  zh->leftover = ito;
D(fprintf(stderr,"z3de_isworse: w=%2d, final:%d, tell=%u, no\n", zh->weights, zh->z3be.bfinal, t);)
  return t;
}

static int z3de_compressnow(struct z3de_handle *zh)
{
D(fprintf(stderr,"z3de_compressnow: limit=%d, w[%d].btype: %d\n", zh->nclimit, (zh->weights-1) & 1, zh->weight[(zh->weights-1) & 1].btype);)
  if ((zh->nclimit > 0)
   && (zh->nclimit <= zh->lastlimit)
   && (zh->weight[(zh->weights-1) & 1].btype == Z3B_BTYPE_NC)) {
    return 1;
  }
  return 0;
}

struct z3de_handle *z3d_encode(struct z3de_handle *zh,
                        __u8 *data, __u32 datasize, __u32 *taken,
                        __u8 *code, __u32 codesize, __u32 *given)
{
  __u32 n, f, ito;
D(fprintf(stderr,"Z3BE state=%d, Z3DE state=%d, datasize=%d, tellnext=%d\n", zh->z3be.state, zh->state, datasize, zh->tellnext);)
  if (zh->state == z3de_state_fill) {
    if (datasize == 0) {
      z3be_push(&zh->z3be);
      if (z3de_isworse(zh) < 0) {
D(fprintf(stderr,"STATE: butlast\n");)
        zh->state = z3de_state_butlast;
      } else {
D(fprintf(stderr,"STATE: finish\n");)
        zh->state = z3de_state_finish;
      }
    } else if (zh->tellnext == 0) {
      if ((z3de_isworse(zh) >= 0) && (!z3de_compressnow(zh))) {
        z3de_setnclimit(zh);
        z3de_settellnext(zh, 0);
        *given = 0;
        *taken = 0;
        return zh;
      }
D(fprintf(stderr,"STATE: drain (W)\n");)
      zh->state = z3de_state_drain;
    } else {
      f = datasize;
      if (f > zh->tellnext) {
        f = zh->tellnext;
      }
      n = z3be_put(&zh->z3be, data, f);
D(fprintf(stderr,"  put %d\n", n);)
      if (n > 0) {
        zh->tellnext -= n;
        *given = 0;
        *taken = n;
        return zh;
      }
      if (z3de_isworse(zh) > 0) {
        z3de_setnclimit(zh);
        z3de_settellnext(zh, 0);
        *given = 0;
        *taken = 0;
        return zh;
      }
D(fprintf(stderr,"STATE: drain (P)\n");)
      zh->state = z3de_state_drain;
    }
    zh->weights = (zh->weights - 1) & 1;
  }
  *taken = 0;
  if (codesize > 0) {
    n = z3be_get(&zh->z3be, &zh->weight[zh->weights], code, codesize);
D(fprintf(stderr,"    get %d\n", n);)
    if (n == 0) {
      switch (zh->state) {
      case z3de_state_finish:
        *given = (z3be_finish(&zh->z3be, code) + 7) / 8;
D(fprintf(stderr,"END\n");)
        return NULL;
        break;
      case z3de_state_butlast:
        z3be_tell(&zh->z3be, &zh->weight[0], &ito);
D(fprintf(stderr,"STATE: finish\n");)
        zh->state = z3de_state_finish;
        break;
      default:
        zh->nclimit = Z3DE_NCLIMIT;
        z3de_settellnext(zh, zh->leftover);
D(fprintf(stderr,"STATE: fill\n");)
        zh->state = z3de_state_fill;
        break;
      }
      zh->weights = 0;
    }
    *given = n;
  } else {
    *given = 0;
  }
  return zh;
}
#endif /* Z3LIB_DECODE_ONLY */

#ifndef Z3LIB_ENCODE_ONLY
struct z3dd_handle *z3d_decode_init(__u32 pending_bits, int pending_nb,
                        void *memory, unsigned int memsize)
{
  struct z3dd_handle *zh;
  if ((memory == NULL) || (memsize < Z3DD_MEMSIZE)) {
    return NULL;
  }
  zh = (struct z3dd_handle *)memory;
  if (z3bd_start(pending_bits, pending_nb, &zh->z3bd,
        memsize - sizeof(struct z3dd_handle) + sizeof(struct z3bd_handle))
      != &zh->z3bd) {
    return NULL;
  }
  return zh;
}

struct z3dd_handle *z3d_decode(struct z3dd_handle *zh, int *error,
                        __u32 *pending_bits, int *pending_nb,
                        __u8 *code, __u32 codesize, __u32 *taken,
                        __u8 **data, __u32 *given)
{
  if ((*given = z3bd_get(&zh->z3bd, data)) != 0) {
    *taken = 0;
    return zh;
  }
  if (*data != NULL) {
    *taken = z3bd_put(&zh->z3bd, code, codesize);
    *given = z3bd_get(&zh->z3bd, data);
    return zh;
  }
  *error = z3bd_finish(&zh->z3bd, pending_bits, pending_nb);
D(fprintf(stderr, "z3d_decode finish: error=%d, pb=%08x/%d\n", *error, *pending_bits, *pending_nb);)
  if (*error == z3err_bd_notbfinal) {
    *error = z3err_none;
    *taken = 0;
    return zh;
  }
  return NULL;
}
#endif /* Z3LIB_ENCODE_ONLY */
