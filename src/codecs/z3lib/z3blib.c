/*
 * z3lib (c)1993,2000,2005,2006/GPL,BSD Oskar Schirmer, schirmer@scara.com
 * combined huffman/lz compression library
 * now rfc1951 conformant
 */

#include "z3lib.h"
#include "z3liblib.h"
#include "z3crc32.h"
#include "z3blib.h"

// #define D(x) x
#define DE(x)

#ifndef D
#define D(x)
#endif

#define z3b_moddictbits(x) ((x) & ((1UL << Z3B_DICTBITS) - 1))
const static __u8 z3b_hclen_sort[Z3B_NBCODELENGTHSCODES] =
  {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

#ifndef Z3LIB_DECODE_ONLY
#define z3be_data(zh,i) z3relref(__u8, (zh)->data)[((__s32)(i))]
#define z3be_histogram3(zh,bits,code) \
        z3relref(__u32, (zh)->histogram3)[((bits)<<8)+(code)]
#define z3be_distances3(zh,dist) z3be_histogram3(zh,Z3BE_REF3TOOLARGE_MAX,dist)

struct z3be_handle *z3be_start(void *memory, unsigned int memsize,
                        int preferlonger, int limitlength3)
{
  struct z3be_handle *zh;
  unsigned int size;
  if ((memory == NULL)
   || (memsize < (limitlength3 ?
        (Z3BE_MEMSIZE_MIN + Z3BE_MEMSIZE_EXTRA3) : Z3BE_MEMSIZE_MIN))) {
    return NULL;
  }
  zh = (struct z3be_handle *)memory;
  size = memsize - sizeof(struct z3be_handle) + sizeof(zh->mem);
  if (size > (1UL << 23)) {
    size = (1UL << 23);
  }
  if (limitlength3) {
    size -= Z3BE_MEMSIZE_EXTRA3;
    z3relrefset(zh->histogram3, zh->mem[size]);
    memset(&zh->mem[size], 0, Z3BE_MEMSIZE_EXTRA3);
  } else {
    zh->histogram3 = NULL;
  }
  zh->limitwp = zh->dsize = size/2;
  z3relrefset(zh->data, zh->mem[size/2]);
  zh->dpos = 0;
  zh->dshift = 0;
  zh->ipos = 0;
  zh->mlen = Z3B_LZMINLENGTH-1;
  zh->pbits = 0;
  zh->pbits_nb = 0;
  zh->state = z3be_state_end;
  zh->bfinal = 0;
  zh->longermlen = preferlonger ? Z3B_LZMINLENGTH-1 : 0;
  z3relrefset(zh->ll.code, zh->huff_ll_code[0]);
  z3relrefset(zh->ll.size, zh->huff_ll_size[0]);
  z3relrefset(zh->dc.code, zh->huff_dc_code[0]);
  z3relrefset(zh->dc.size, zh->huff_dc_size[0]);
  memset(&zh->hashtab[0], -1, sizeof(zh->hashtab));
        /* -1 is NIL, >=0 is position+dshift mod 32k */
  memset(&zh->histogram, 0, sizeof(struct z3be_histogram));
  zh->histogram.literal[Z3B_CODEENDOFBLOCK] = 1;
D(fprintf(stderr, "START mem=%p, data=%p, size=%d, dsize=%d\n", zh->mem, &z3be_data(zh, 0), size, zh->dsize);)
  return zh;
}

static __s16 z3be_hash(const __u8 *data)
{
  __s16 r;
  if ((data[0] == data[1])
   && (data[0] == data[2])) {
    r = data[0] + (1 << Z3BE_HASHBITS);
  } else {
    r = (((((__u8)(9 * data[0])) << 2) ^ data[1]) << 2) ^ data[2];
DE(if ((r & ((1 << Z3BE_HASHBITS) - 1)) != r) fprintf(stderr, "hash calc ERROR %d %02x %02x %02x\n", r, data[0], data[1], data[2]);)
  }
  return r;
}

static void z3be_put_hashtable(struct z3be_handle *zh, __u32 tabhash, __u32 pos)
{
  __s16 e, r, d;
  r = z3b_moddictbits(pos + zh->dshift);
DE(if ((tabhash != z3be_hash(&z3be_data(zh, pos))) && !zh->bfinal) fprintf(stderr, "hashtab ERROR: %d, t=%lu\n", z3be_hash(&z3be_data(zh, pos)), tabhash);)
  e = zh->hashtab[tabhash];
  if (e >= 0) { /* link into chain for new hash value */
    d = z3b_moddictbits(r - e);
    if (z3be_hash(&z3be_data(zh, pos-d)) == tabhash) {
D(fprintf(stderr, "HASH1 th=%lu, pos=%lu, r=%d, e=%d, d=%d\n", tabhash, pos, r, e, d);)
      if ((d == 1)
       && (tabhash & (1 << Z3BE_HASHBITS))) {
        d = zh->hashforw[z3b_moddictbits(r - 1)];
        if (d >= 0) {
          d = -1;
        } else if ((d - 1) < 0) {
          d -= 1;
        }
D(fprintf(stderr, "HASH2 th=%lu, pos=%lu, r=%d, e=%d, d=%d\n", tabhash, pos, r, e, d);)
      }
    } else { /* the reference from hashtab was old */
      d = 0;
D(fprintf(stderr, "HASH3 th=%lu, pos=%lu, r=%d, e=%ld, d=%ld\n", tabhash, pos, r, e, d);)
    }
  } else { /* this point is the only one for the new hash value */
    d = 0;
D(fprintf(stderr, "HASH0 th=%lu, pos=%lu, r=%d, e=%d\n", tabhash, pos, r, e);)
  }
  zh->hashforw[r] = d;
  zh->hashtab[tabhash] = r;
D(fprintf(stderr, "%d<-tab[%lu], %d<-forw[%d]=%d\n", r, tabhash, e, r, d);)
}

static void z3be_histogram_literal(struct z3be_histogram *h, __u8 d)
{
  h->literal[d] += 1;
}

static void z3be_put_literal(struct z3be_handle *zh, __u32 tabhash)
{
  __u32 i;
  i = zh->ipos;
D(fprintf(stderr, "  LITERAL: th=%lu, data=%02x\n", tabhash, z3be_data(zh, i));)
  z3be_histogram_literal(&zh->histogram, z3be_data(zh, i));
  zh->mem[i] = (__u8)-1; /* mark as literal */
  z3be_put_hashtable(zh, tabhash, i);
  zh->ipos = i + 1;
}

const static __u16 z3be_lengths_base_number[((Z3B_NBLENGTHCODES+3)>>2)-1] =
  {10, 18, 34, 66, 130, 257, 0xFFFF};
const static __u16 z3be_lengths_class[((Z3B_NBLENGTHCODES+3)>>2)-1] = {
  257-3, (265<<1)-11, (269<<2)-19, (273<<3)-35, (277<<4)-67, (281<<5)-131,
  (285<<6)-258
};

static __u32 z3be_class_length(__u32 length)
{
  __u32 i;
  i = 0;
  while (z3be_lengths_base_number[i] < (__u16)length) {
    i += 1;
  }
  return (length + z3be_lengths_class[i]) >> i;
}

static __u32 z3be_class_distance(__u32 distance)
{
  __u32 i, d;
  i = 0;
  d = distance;
  while (d >= 4) {
    i += 2;
    d >>= 1;
  }
  return d + i;
}

static void z3be_histogram_ref3(struct z3be_handle *zh, __u32 distclass,
                                        __u8 *data)
{
  __u32 c;
  if (distclass >= (Z3B_NBDISTANCECODES - 2 * Z3BE_REF3TOOLARGE_MAX)) {
    c = distclass - (Z3B_NBDISTANCECODES - 2 * Z3BE_REF3TOOLARGE_MAX);
    z3be_distances3(zh, c) += 1;
    c /= 2;
    z3be_histogram3(zh, c, data[0]) += 1;
    z3be_histogram3(zh, c, data[1]) += 1;
    z3be_histogram3(zh, c, data[2]) += 1;
  }
}

static void z3be_histogram_reference(struct z3be_handle *zh, __u32 length,
                                        __u32 distance, __u8 *data)
{
  __u32 c;
  zh->histogram.literal[z3be_class_length(length)] += 1;
  c = z3be_class_distance(distance);
  zh->histogram.distance[c] += 1;
  if ((zh->histogram3 != NULL)
   && (length == 3)) {
    z3be_histogram_ref3(zh, c, data);
  }
}

static void z3be_put_reference(struct z3be_handle *zh, __u32 len, __u32 tabhash)
{
  __u32 l, i;
  i = zh->ipos;
  l = i - zh->match - 1;
DE(if (l >= (1UL << Z3B_DICTBITS)) fprintf(stderr, "reference ERROR 1\n");)
DE(if ((len < Z3B_LZMINLENGTH) || (len > Z3B_LZMAXLENGTH)) fprintf(stderr, "reference ERROR 2\n");)
DE({int x=0;do{if(z3be_data(zh,i+x)!=z3be_data(zh,zh->match+x))fprintf(stderr,"reference ERROR 3: data[%d+%d]=%02x,data[%d+%d]=%02x\n",i,x,z3be_data(zh,i+x),zh->match,x,z3be_data(zh,zh->match+x));}while(++x<len);})
D(fprintf(stderr, "  REF: th=%lu, i=%lu, distance=%lu, length=%lu, data=%02x %02x %02x\n", tabhash, i, l, len, z3be_data(zh, i), z3be_data(zh, i+1), z3be_data(zh, i+2));)
  z3be_histogram_reference(zh, len, l, &z3be_data(zh, i));
  zh->mem[i] = (__u8)(l >> 8); /* >=0, mark as reference */
  zh->mem[i+1] = (__u8)l;
  zh->mem[i+2] = (__u8)(len - Z3B_LZMINLENGTH);
  z3be_put_hashtable(zh, tabhash, i);
  l = zh->dpos - zh->ipos - (Z3B_LZMINLENGTH - 1);
  if (l > len) {
    l = len;
  }
  while (--l > 0) {
    i += 1;
    z3be_put_hashtable(zh, z3be_hash(&z3be_data(zh, i)), i);
  }
  zh->ipos += len;
}

static int z3be_put_few(struct z3be_handle *zh)
{
  __u32 t, l, n, j, m, lj, t2, lr;
  __s32 h, i, s;
  t = z3be_hash(&z3be_data(zh, zh->ipos));
  l = zh->dpos - zh->ipos;
  if (l > Z3B_LZMAXLENGTH) {
    l = Z3B_LZMAXLENGTH;
  }
  if (zh->mlen < Z3B_LZMINLENGTH) {
    h = zh->hashtab[t];
D(fprintf(stderr, "FEW0 mlen=%d, t=%04x, h=%d\n", zh->mlen, t, h);)
    if (h >= 0) {
      i = z3b_moddictbits(h - zh->dshift - zh->ipos)
        + zh->ipos - (1UL << Z3B_DICTBITS);
      if (z3be_hash(&z3be_data(zh, i)) != t) {
D(fprintf(stderr, "FEW0A\n");)
        zh->hashtab[t] = h = -1;
      } else if (t & (1 << Z3BE_HASHBITS)) {
        n = z3lib_countsingle((__u8)t, &z3be_data(zh, zh->ipos+1), l-1);
        zh->mrunl = n;
D(fprintf(stderr, "FEW0B n=%d, data:%02x\n", n, z3be_data(zh, zh->ipos+1));)
        if (n > Z3B_LZMINLENGTH) {
          zh->mlen = n - 1;
        }
      } else {
        zh->mrunl = 0;
      }
    }
  } else {
    h = 0;
    i = zh->match;
  }
  lr = zh->mrunl;
D(fprintf(stderr, "FEW1 ipos/dpos: %lu/%lu, h=%ld, l=%lu, mrunl=%lu, t=%04x, i=%ld\n", zh->ipos, zh->dpos, h, l, lr, t, i);)
  if (h >= 0) {
    do {
      n = z3lib_countequal(&z3be_data(zh, zh->ipos), &z3be_data(zh, i), l);
      if (n > zh->mlen) {
D(fprintf(stderr, "FEW3 match=%ld, mlen=%lu, h=%ld\n", i, n, h);)
        zh->mlen = n;
        zh->match = i;
      } else {
D(fprintf(stderr, "FEW2     i=%ld,    n=%lu, h=%ld, data:%02x %02x...\n", i, n, h, z3be_data(zh,i), z3be_data(zh,i+1));)
      }
      h = zh->hashforw[z3b_moddictbits(i + zh->dshift)];
      if (h < 0) {
        j = Z3B_LZMINLENGTH - h;
        if (j >= lr) {
          if (zh->mlen < lr) {
            s = i + Z3B_LZMINLENGTH - (zh->ipos - (1L << Z3B_DICTBITS));
            if (s > lr) {
              s = lr;
            }
D(else fprintf(stderr, "FEW3C s=%ld, lr=%lu\n", s, lr);)
            if (zh->mlen < s) {
D(fprintf(stderr, "FEW3B match=%ld, mlen=%lu, h=%ld, i=%ld, data", i-(s-Z3B_LZMINLENGTH), s, h, i);{int x=0;while(x<s){fprintf(stderr," %02x",z3be_data(zh,x+i-(lr-Z3B_LZMINLENGTH)));x+=1;}}fprintf(stderr,"\n");)
              zh->mlen = s;
              zh->match = i + Z3B_LZMINLENGTH - s;
            }
          }
          if (j > lr) {
            i += Z3B_LZMINLENGTH - lr - 1;
            if ((i + (1L << Z3B_DICTBITS)) >= (__s32)zh->ipos) {
              n = z3lib_countequal(&z3be_data(zh, zh->ipos),
                        &z3be_data(zh, i), l);
              if (n > zh->mlen) {
D(fprintf(stderr, "FEW3A match=%ld, mlen=%lu, h=%ld\n", i, n, h);)
                zh->mlen = n;
                zh->match = i;
              }
              h = zh->hashforw[z3b_moddictbits(i + zh->dshift)];
              if (h < 0) {
                i += h;
                if ((i + (1L << Z3B_DICTBITS)) >= (__s32)zh->ipos) {
                  h = zh->hashforw[z3b_moddictbits(i + zh->dshift)];
                }
              }
            }
          } else {
            i += h;
            if ((i + (1L << Z3B_DICTBITS)) >= (__s32)zh->ipos) {
              h = zh->hashforw[z3b_moddictbits(i + zh->dshift)];
            }
          }
        } else {
          i += h;
          if ((i + (1L << Z3B_DICTBITS)) >= (__s32)zh->ipos) {
            h = zh->hashforw[z3b_moddictbits(i + zh->dshift)];
          }
        }
      }
      i -= h;
    } while ((h > 0)
          && (n < l)
          && ((i + (1L << Z3B_DICTBITS)) >= (__s32)zh->ipos));
  }
  if (zh->mlen < l) {
    if (zh->mlen >= Z3B_LZMINLENGTH) {
      if (zh->mlen >= lr) {
D(fprintf(stderr, "FEW4/REF: mlen=%lu, match=%ld\n", zh->mlen, zh->match);)
        if (zh->longermlen) {
          zh->longermlen = zh->mlen;
          j = zh->ipos;
          do {
            m = zh->longermlen;
            j += 1;
            l = zh->dpos - j;
            if (l > Z3B_LZMAXLENGTH) {
              l = Z3B_LZMAXLENGTH;
            }
            n = z3lib_countsingle(z3be_data(zh, j-1), &z3be_data(zh, j), l);
            if (n > zh->longermlen) {
D(fprintf(stderr, "FEW92 match=%ld, lmlen=%lu, j=%lu\n", j-1, n, j);)
              zh->longermlen = n;
              zh->longermatch = j-1;
              lj = j;
            }
            t2 = z3be_hash(&z3be_data(zh, j));
            h = zh->hashtab[t2];
            i = z3b_moddictbits(h - zh->dshift - j) + j - (1UL << Z3B_DICTBITS);
            if ((h >= 0) && (z3be_hash(&z3be_data(zh, i)) != t2)) {
              h = -1;
            }
            if ((h >= 0)
             && (n < l)
             && ((i + (1L << Z3B_DICTBITS)) >= (__s32)j)) {
              if (lr >= Z3B_LZMINLENGTH) {
                lr -= 1;
              } else if (t2 & (1 << Z3BE_HASHBITS)) {
                lr = z3lib_countsingle((__u8)t2, &z3be_data(zh, j+1), l-1);
              } else {
                lr = 0;
              }
              do {
                n = z3lib_countequal(&z3be_data(zh, j), &z3be_data(zh, i), l);
                if (n > zh->longermlen) {
D(fprintf(stderr, "FEW93 match=%ld, lmlen=%lu, j=%lu, h=%ld\n", i, n, j, h);)
                  zh->longermlen = n;
                  zh->longermatch = i;
                  lj = j;
                }
                h = zh->hashforw[z3b_moddictbits(i + zh->dshift)];
                if (h < 0) {
                  if ((Z3B_LZMINLENGTH - h) > lr) {
D(fprintf(stderr, "FEW94 h=%ld, j:%lu, ipos:%lu, lr:%lu, i=%ld, t2=%04x\n", h, j, zh->ipos, lr, i, t2);)
                    i += Z3B_LZMINLENGTH - lr - 1;
                    if ((i + (1L << Z3B_DICTBITS)) >= (__s32)j) {
                      n = z3lib_countequal(&z3be_data(zh, j),
                                &z3be_data(zh, i), l);
                      if (n > zh->longermlen) {
D(fprintf(stderr, "FEW93A match=%ld, lmlen=%lu, j=%lu, h=%ld\n", i, n, j, h);)
                        zh->longermlen = n;
                        zh->longermatch = i;
                        lj = j;
                      }
                      h = zh->hashforw[z3b_moddictbits(i + zh->dshift)];
D(fprintf(stderr, "FEW95 h=%ld, i=%ld\n", h, i);)
                      if (h < 0) {
                        i += h;
                        h = zh->hashforw[z3b_moddictbits(i + zh->dshift)];
D(fprintf(stderr, "FEW96 h=%ld, i=%ld\n", h, i);)
                      }
                    }
                  } else {
                    i += h;
                    if (((i + (1L << Z3B_DICTBITS)) >= (__s32)j)
                     && ((Z3B_LZMINLENGTH - h) > zh->longermlen)) {
D(fprintf(stderr, "FEW93B match=%ld, lmlen=%lu, j=%lu, h=%ld\n", i, Z3B_LZMINLENGTH-h, j, h);)
                      zh->longermlen = Z3B_LZMINLENGTH - h;
                      zh->longermatch = i;
                      lj = j;
                    }
                    h = zh->hashforw[z3b_moddictbits(i + zh->dshift)];
                  }
                }
                i -= h;
              } while ((h > 0)
                    && (n < l)
                    && ((i + (1L << Z3B_DICTBITS)) >= (__s32)j));
            }
          } while ((zh->longermlen > m)
                && (zh->longermlen < l)
                && (j < (zh->ipos + zh->mlen)));
D(fprintf(stderr, "FEW8 m=%lu, l=%lu, lmlen=%lu, lj=%lu\n", m, l, zh->longermlen, lj);)
          if (zh->longermlen > zh->mlen) {
            j = lj - zh->ipos;
            if ((zh->longermlen < (zh->dpos - lj))
             || (zh->longermlen == Z3B_LZMAXLENGTH)
             || (j >= zh->mlen)) {
              if (j < Z3B_LZMINLENGTH) {
D(fprintf(stderr, "FEW80\n");)
                z3be_put_literal(zh, t);
                while (--j > 0) {
                  t = z3be_hash(&z3be_data(zh, zh->ipos));
                  z3be_put_literal(zh, t);
                }
              } else {
D(fprintf(stderr, "FEW81\n");)
                z3be_put_reference(zh, j, t);
              }
              zh->longermlen = zh->mlen = Z3B_LZMINLENGTH-1;
              return 1;
            } else {
D(fprintf(stderr, "FEW82\n");)
              return 0;
            }
          } else {
D(fprintf(stderr, "FEW83\n");)
            z3be_put_reference(zh, zh->mlen, t);
            zh->longermlen = zh->mlen = Z3B_LZMINLENGTH-1;
            return ((zh->dpos - zh->ipos) >= Z3B_LZMINLENGTH);
          }
        } else {
D(fprintf(stderr, "FEW84\n");)
          z3be_put_reference(zh, zh->mlen, t);
          zh->mlen = Z3B_LZMINLENGTH-1;
          return ((zh->dpos - zh->ipos) >= Z3B_LZMINLENGTH);
        }
      } else {
D(fprintf(stderr, "FEW5A/LIT\n");)
        zh->mlen = Z3B_LZMINLENGTH-1;
        z3be_put_literal(zh, t);
        return (l > Z3B_LZMINLENGTH);
      }
    } else {
D(fprintf(stderr, "FEW5/LIT\n");)
      z3be_put_literal(zh, t);
      return (l > Z3B_LZMINLENGTH);
    }
  } else if (l == Z3B_LZMAXLENGTH) {
D(fprintf(stderr, "FEW6/REF: mlen=%lu\n", zh->mlen);)
    z3be_put_reference(zh, zh->mlen, t);
    zh->mlen = Z3B_LZMINLENGTH-1;
    return ((zh->dpos - zh->ipos) >= Z3B_LZMINLENGTH);
  } else {
D(fprintf(stderr, "FEW7\n");)
    return 0;
  }
}

__u32 z3be_put(struct z3be_handle *zh, __u8 *data, __u32 datasize)
{
  __u32 i, d;
D(fprintf(stderr, "PUT(%p, %p, %d), shift=%lu, %02x %02x ...\n", zh, data, datasize, zh->dshift, data[0], data[1]);)
  i = zh->limitwp - zh->dpos;
  if (i > datasize) {
    i = datasize;
  }
  if (i > 0) {
    memcpy(&z3be_data(zh, zh->dpos), data, i);
    d = zh->dpos;
    while (((d - zh->dpos) < i)
        && ((d - zh->ipos) < (Z3B_LZMINLENGTH-1))) {
D(fprintf(stderr, "PUT2: i=%lu, d=%lu, dpos=%lu, ipos=%lu, dshift=%lu\n", i, d, zh->dpos, zh->ipos, zh->dshift);)
      if ((d + zh->dshift) >= (Z3B_LZMINLENGTH-1)) {
        z3be_put_hashtable(zh,
                z3be_hash(&z3be_data(zh, d - (Z3B_LZMINLENGTH-1))),
                d - (Z3B_LZMINLENGTH-1));
      }
      d += 1;
    }
    zh->dpos += i;
    if ((zh->dpos - zh->ipos) >= Z3B_LZMINLENGTH) {
      while (z3be_put_few(zh));
    }
  }
  return i;
}

void z3be_push(struct z3be_handle *zh)
{
  __u32 j;
  zh->bfinal = 1;
  if ((zh->dpos - zh->ipos) < Z3B_LZMINLENGTH) {
    while (zh->ipos < zh->dpos) {
      z3be_put_literal(zh, 0);
    }
  } else {
    if (zh->longermlen >= Z3B_LZMINLENGTH) {
      j = zh->dpos - zh->ipos - zh->longermlen;
      if (j < Z3B_LZMINLENGTH) {
D(fprintf(stderr, "PUSH80\n");)
DE(if (j == 0) fprintf(stderr, "PUSH ERROR\n");)
        do {
          z3be_put_literal(zh, 0);
        } while (--j > 0);
      } else {
D(fprintf(stderr, "PUSH81\n");)
        z3be_put_reference(zh, j, 0);
      }
      zh->match = zh->longermatch;
    }
    z3be_put_reference(zh, zh->dpos - zh->ipos, 0);
  }
}

static __u32 z3be_tell_fixed(struct z3be_histogram *h)
{
  __u32 i, n, l;
  i = 0;
  n = 0;
  do {
    n += h->literal[i];
  } while (++i < 144);
  l = 8 * n + 3;
  n = 0;
  do {
    n += h->literal[i];
  } while (++i < 256);
  l += 9 * n;
  n = 0;
  do {
    n += h->literal[i];
  } while (++i < 265);
  l += 7 * n;
  n = 0;
  do {
    n += h->literal[i];
    if (!(i & 3)) l -= n;
  } while (++i < 280);
  l += (7+4) * n - h->literal[i];
  n = 0;
  do {
    n += h->literal[i];
  } while (++i < 285);
  l += (8+5) * n + 8 * h->literal[i];
D(fprintf(stderr, "TELL fixed literal: %lu\n", l);)
  i = 0;
  n = 0;
  do {
    n += h->distance[i];
  } while (++i < 4);
  do {
    l -= n;
    n += h->distance[i++];
    n += h->distance[i++];
  } while (i < Z3B_NBDISTANCECODES);
  l += (5+13) * n;
D(fprintf(stderr, "TELL fixed total: %lu\n", l);)
  return l;
}

static __u16 *z3be_dynh_sortindex(__u32 *histo, __u16 *iold, __u16 *inew,
                        __u32 num)
{
  __u32 n;
  n = 0;
  do {
D(fprintf(stderr,"dynh sortindex histo[%3d]:%7d\n",n,histo[n]);)
    iold[n] = n;
  } while (++n < num);
  n = 1;
  while (n < num) {
    __u16 *t = inew;
    __u32 i = 0;
    do {
      __u32 a, b;
      a = b = 0;
      while ((b < n) && ((i+n+b) < num) && (a < n)) {
        if (histo[iold[i+a]] <= histo[iold[i+n+b]]) {
          *t++ = iold[i+a];
          a += 1;
        } else {
          *t++ = iold[i+n+b];
          b += 1;
        }
      }
      while ((b < n) && ((i+n+b) < num)) {
        *t++ = iold[i+n+b];
        b += 1;
      }
      while ((a < n) && ((i+a) < num)) {
        *t++ = iold[i+a];
        a += 1;
      }
      i += 2*n;
    } while (i < num);
    t = iold; iold = inew; inew = t;
    n <<= 1;
  }
  return iold;
}

static void z3be_dynh_cumulatetree(__u32 *histo, __u16 *index, __u32 *cumul,
                        __u16* refer, __u32 num)
{
  __u32 n, c, s;
  cumul[0] = histo[index[0]] + histo[index[1]];
  refer[1] = refer[0] = num;
D(fprintf(stderr, "dynh cumul %d(%lu) + %d(%lu) -> %d(%lu) [%d, %d]\n", 0, histo[index[0]], 1, histo[index[1]], num, cumul[0], index[0], index[1]);)
  n = 2;
  c = 0;
  s = 1;
  while (s < (num-1)) {
    if ((n >= num) || ((s > (c+1)) && (cumul[c+1] < histo[index[n]]))) {
      cumul[s] = cumul[c] + cumul[c+1];
      refer[num+c+1] = refer[num+c] = num+s;
D(fprintf(stderr, "dynh cumul %d(%lu) + %d(%lu) -> %d(%lu)\n", num+c, cumul[c], num+c+1, cumul[c+1], num+s, cumul[s]);)
      c += 2;
    } else if (((n+1) >= num) || (cumul[c] < histo[index[n+1]])) {
      cumul[s] = histo[index[n]] + cumul[c];
      refer[num+c] = refer[n] = num+s;
D(fprintf(stderr, "dynh cumul %d(%lu) + %d(%lu) -> %d(%lu) [%d]\n", n, histo[index[n]], num+c, cumul[c], num+s, cumul[s], index[n]);)
      c += 1;
      n += 1;
    } else {
      cumul[s] = histo[index[n]] + histo[index[n+1]];
      refer[n] = refer[n+1] = num+s;
D(fprintf(stderr, "dynh cumul %d(%lu) + %d(%lu) -> %d(%lu) [%d, %d]\n", n, histo[index[n]], n+1, histo[index[n+1]], num+s, cumul[s], index[n], index[n+1]);)
      n += 2;
    }
    s += 1;
  }
}

static void z3be_dynh_bitspercode(__u16 *refer, __u32 num, __u8 max,
                        __u32 *small, __u32 *large)
{
  __u32 s, le, lt;
  s = 2*num - 2;
  refer[s] = 0;
  do {
    __u32 i;
    s -= 1;
    refer[s] = i = refer[refer[s]] + 1;
D(fprintf(stderr, "dynh bitspercode (%d): %d\n", s, i);)
    if (i <= max) le = s;
    if (i < max) lt = s;
  } while (s > 0);
  *small = le;
  *large = lt;
}

static void z3be_dynh_cutdistantleaves(__u16 *refer,
                        __u32 small, __u32 large, __u32 num, __u8 max)
{
  __s32 t;
  t = 0;
  while (small > 0) {
    small -= 1;
    t += ((1UL << 31) >> refer[small]);
    t -= ((1UL << 31) >> max);
    refer[small] = max;
D(fprintf(stderr, "dynh cutdistant-(%3d): %3d [%08x]\n", small, max, t);)
    if (t < 0) {
      __u16 i;
      i = refer[large] + 1;
      refer[large] = i;
      if (i == max) large += 1;
      t += ((1UL << 31) >> i);
D(fprintf(stderr, "dynh cutdistant+(%3d): %3d [%08x]\n", large, i, t);)
    }
  }
  while (t > 0) {
    large -= 1;
    t -= ((1UL << 31) >> max);
    refer[large] = max-1;
D(fprintf(stderr, "dynh cutdistant/(%3d): %3d [%08x]\n", large, max-1, t);)
  }
DE(if (t != 0) fprintf(stderr, "dynh cutdistant ERROR %08x\n", t);)
}

static void z3be_dynamic_histogram(struct z3be_handle *zh,
                        __u32 *histo, __u8 *clen, __u32 num, __u8 max)
{
  __u32 n, small, large;
  __u16 *index, *non0, *refer;
D(fprintf(stderr, "dynamic_histogram(%p, %p, %lu, %d)\n", histo, clen, num, max);)
  memset(clen, 0, num);
  index = z3be_dynh_sortindex(histo,
                &zh->temp_dynh_i[0][0], &zh->temp_dynh_i[2][0], num);
  n = num;
  non0 = index;
  while ((n > 2) && (histo[*non0] == 0)) {
    n -= 1;
    non0 += 1;
  }
  if (n <= 2) {
    __u16 i;
    i = non0[2-n];
    while ((n > 0) && (histo[i] == 0)) {
      non0[2-n] = index[2-n];
      index[2-n] = i;
      n -= 1;
      i = non0[2-n];
    }
    n = 2;
  }
  refer = &zh->temp_dynh_i[1][-((index - &zh->temp_dynh_i[0][0]) / 2)];
  z3be_dynh_cumulatetree(histo, non0, &zh->temp_dynh_cumul[0], refer, n);
  z3be_dynh_bitspercode(refer, n, max, &small, &large);
  z3be_dynh_cutdistantleaves(refer, small, large, n, max);
  do {
    n -= 1;
    clen[non0[n]] = refer[n];
D(fprintf(stderr, "dynamic_histogram %3d:%3d\n", non0[n], refer[n]);)
  } while (n > 0);
}

static void z3be_dynclc_add(int cnt, __u8 code, __u32 *clcount)
{
  while (cnt > 0) {
    if (code == 0) {
      if (cnt < 3) {
        clcount[code] += cnt;
        cnt = 0;
      } else if (cnt >= 11) {
        clcount[18] += 1;
        cnt -= 138;
      } else {
        clcount[17] += 1;
        cnt = 0;
      }
    } else {
      if (cnt <= 3) {
        clcount[code] += cnt;
        cnt = 0;
      } else {
        clcount[16] += 1;
        if (cnt > 6) {
          cnt -= 6;
        } else {
          cnt = 1;
        }
      }
    }
  }
}

static __u32 z3be_dynclc_count(__u8 *clen, __u32 num, __u32 *clcount)
{
  __u32 n;
  int i, r;
  __u8 l;
  n = 0;
  i = 0;
  l = 0;
  do {
    if (l != clen[n]) {
      z3be_dynclc_add(i, l, clcount);
      l = clen[n];
      i = 0;
    }
    i += 1;
    n += 1;
  } while (n < num);
  r = i;
  if (l == 0) i -= Z3B_NBLENGTHCODES;
  if (i < 0) i = 0;
  z3be_dynclc_add(i, l, clcount);
D(fprintf(stderr, "TELL dynclc_count(%lu) -> %d\n", num, Z3B_NBLENGTHCODES - (r-i));)
  return Z3B_NBLENGTHCODES - (r-i);
}

struct z3be_extrabits {
  __u16 code;
  __s16 extra;
};
struct z3be_extrabits z3be_extrabits_clc[] = {
  {16, 2}, {17, 3}, {18, 7}, {19, -1}
};
struct z3be_extrabits z3be_extrabits_ll[] = {
  {265, 1}, {269, 2}, {273, 3}, {277, 4}, {281, 5}, {285, 0}, {286, -1}
};
struct z3be_extrabits z3be_extrabits_dc[] = {
  {4, 1}, {6, 2}, {8, 3}, {10, 4}, {12, 5}, {14, 6}, {16, 7}, {18, 8},
  {20, 9}, {22, 10}, {24, 11}, {26, 12}, {28, 13}, {30, -1}
};

static __u32 z3be_dynamic_amount(__u32 *histo, __u8 *clen,
                        struct z3be_extrabits *eb)
{
  __u32 n, s, e;
  n = 0;
  s = 0;
  e = 0;
  do {
    do {
      s += histo[n] * (clen[n] + e);
      n += 1;
    } while (n != eb->code);
    e = eb->extra;
D(fprintf(stderr, "dynamic_amount[%3d] e=%2d, s=%d\n", n, e, s);)
  } while ((*eb++).extra >= 0);
  return s;
}

static __u32 z3be_tell_dynamic(struct z3be_handle *zh,
                        struct z3be_weighing *weighing, __u32 ref3toolarge)
{
  __u32 n;
  weighing->histogram = zh->histogram;
  weighing->limit3 = ((1 << 7) << ref3toolarge);
  while (ref3toolarge < Z3BE_REF3TOOLARGE_MAX) {
    n = 0;
    do {
D(if(z3be_histogram3(zh,ref3toolarge,n)!=0)fprintf(stderr,"TELL DYN(%d) lit[%3d]:%6d + %d\n",ref3toolarge,n,weighing->histogram.literal[n],z3be_histogram3(zh,ref3toolarge,n));)
      weighing->histogram.literal[n] += z3be_histogram3(zh, ref3toolarge, n);
    } while (++n < (1 << 8));
    n = z3be_distances3(zh, 2 * ref3toolarge);
    weighing->histogram.distance[2 * ref3toolarge
                + (Z3B_NBDISTANCECODES - 2 * Z3BE_REF3TOOLARGE_MAX)] -= n;
    weighing->histogram.literal[Z3B_CODEENDOFBLOCK+1] -= n;
    n = z3be_distances3(zh, 2 * ref3toolarge + 1);
    weighing->histogram.literal[Z3B_CODEENDOFBLOCK+1] -= n;
    weighing->histogram.distance[2 * ref3toolarge + 1
                + (Z3B_NBDISTANCECODES - 2 * Z3BE_REF3TOOLARGE_MAX)] -= n;
    ref3toolarge += 1;
  }
  z3be_dynamic_histogram(zh, &weighing->histogram.literal[0],
                &weighing->codelen_ll[0], Z3B_NBCODES, 15);
  z3be_dynamic_histogram(zh, &weighing->histogram.distance[0],
                &weighing->codelen_dc[0], Z3B_NBDISTANCECODES, 15);
  memset(&zh->temp_tell_clcnt[0], 0, sizeof(zh->temp_tell_clcnt));
  weighing->hlit = z3be_dynclc_count(&weighing->codelen_ll[0], Z3B_NBCODES,
                &zh->temp_tell_clcnt[0]);
  weighing->hdist = z3be_dynclc_count(&weighing->codelen_dc[0],
                Z3B_NBDISTANCECODES, &zh->temp_tell_clcnt[0]);
  z3be_dynamic_histogram(zh, &zh->temp_tell_clcnt[0],
                &weighing->codelen_clc[0], Z3B_NBCODELENGTHSCODES, 7);
  n = Z3B_NBCODELENGTHSCODES;
  while ((z3b_hclen_sort[n-1] != 0)
      && (zh->temp_tell_clcnt[z3b_hclen_sort[n-1]] == 0)) {
    n -= 1;
  }
  weighing->hclen = n - 4;
D({int i=0;while(i<Z3B_NBCODELENGTHSCODES){fprintf(stderr,"TELL CLC %2d sort=%2d cnt=%d\n",i,z3b_hclen_sort[i],zh->temp_tell_clcnt[z3b_hclen_sort[i]]);i+=1;}fprintf(stderr, "TELL hclen=%d\n", weighing->hclen);})
  n = 3 + 5+5+4 + 3*n;
  n += z3be_dynamic_amount(&zh->temp_tell_clcnt[0],
                &weighing->codelen_clc[0], &z3be_extrabits_clc[0]);
  n += z3be_dynamic_amount(&weighing->histogram.literal[0],
                &weighing->codelen_ll[0], &z3be_extrabits_ll[0]);
  n += z3be_dynamic_amount(&weighing->histogram.distance[0],
                &weighing->codelen_dc[0], &z3be_extrabits_dc[0]);
  return n;
}

__u32 z3be_tell(struct z3be_handle *zh, struct z3be_weighing *weighing,
                        __u32 *inpipe)
{
  __u32 n, c, nb, cb;
  if (zh->state == z3be_state_end) {
    zh->state = z3be_state_init;
  }
  weighing->wpos = zh->ipos;
D(fprintf(stderr, "TELL wpos=%d, len(nc)=%d\n", weighing->wpos, 8*(weighing->wpos+5));)
  weighing->len[Z3B_BTYPE_NC] = 8 * (weighing->wpos + 5);
  weighing->len[Z3B_BTYPE_FIXED] = z3be_tell_fixed(&zh->histogram);
  if (zh->histogram3 != NULL) {
    c = cb = 0;
    nb = (__u32)-1;
    do {
      n = z3be_tell_dynamic(zh, weighing, c);
D(fprintf(stderr, "TELL_DYN(%lu) = %lu\n", c, n);)
      if (n < nb) {
        nb = n;
        cb = c;
      }
    } while (++c <= Z3BE_REF3TOOLARGE_MAX);
    if (cb < Z3BE_REF3TOOLARGE_MAX) {
      n = z3be_tell_dynamic(zh, weighing, cb);
      weighing->histogram = zh->histogram;
    }
  } else {
    n = z3be_tell_dynamic(zh, weighing, Z3BE_REF3TOOLARGE_MAX);
  }
  weighing->len[Z3B_BTYPE_DYNAMIC] = n;
D(fprintf(stderr, "TELL len(dyn)=%d\n", n);)
  weighing->btype = Z3B_BTYPE_DYNAMIC;
  if ((weighing->wpos < (1UL << 16)) && (weighing->len[Z3B_BTYPE_NC] < n)) {
    weighing->btype = Z3B_BTYPE_NC;
  }
  if (weighing->len[Z3B_BTYPE_FIXED] < n) {
    weighing->btype = Z3B_BTYPE_FIXED;
  }
  n = weighing->wpos;
  zh->limitwp = zh->dsize
        + ((n >= (1UL << Z3B_DICTBITS)) ? 0 : (n - (1UL << Z3B_DICTBITS)));
  n = zh->dsize - zh->dpos;
  *inpipe = zh->dpos - zh->ipos;
D(fprintf(stderr, "TELL type=%d, more space: %d\n", weighing->btype, n);)
  return n;
}

static void z3be_buildcodetable(struct z3be_handle_huff *hh,
                        __u8 *clen, __u32 nb)
{
  __u32 i, n, c, l, b;
  n = 0;
  i = nb;
  do {
    i -= 1;
    if ((z3relref(__u8, hh->size)[i] = clen[i]) > n) {
      n = clen[i];
    }
  } while (i > 0);
D(fprintf(stderr, "buildcodetable(%lu), n=%lu\n", nb, n);)
  l = 1;
  c = 0;
  while (l <= n) {
    i = 0;
    do {
      if (clen[i] == l) {
D(fprintf(stderr, "buildcodetable code[%3d] = %08x/%d\n", i, c, l);)
        z3relref(__u16, hh->code)[i] = c;
        b = l - 1;
        while ((c & (1 << b)) && (b != 0)) {
          b -= 1;
        }
        c += (3 << b) - (1 << l);
      }
      i += 1;
    } while (i < nb);
    l += 1;
  }
}

static void z3be_setfixedtable(struct z3be_handle *zh)
{
  __u8 clen[Z3B_NBCODESPLUSUNUSED];
  memset(&clen[0], 8, sizeof(clen));
  memset(&clen[144], 9, 256-144);
  memset(&clen[256], 7, 280-256);
  z3be_buildcodetable(&zh->ll, &clen[0], Z3B_NBCODESPLUSUNUSED);
  memset(&clen[0], 5, Z3B_NBDISTANCECODES);
  z3be_buildcodetable(&zh->dc, &clen[0], Z3B_NBDISTANCECODES);
}

static void z3be_get_last(struct z3be_handle *zh, __u32 wpos,
        struct z3be_histogram *histogram)
{
  __u32 i;
  __u8 l;
  if (wpos < zh->dpos) {
    z3lib_memmoveleft(&zh->mem[0], &zh->mem[wpos], zh->dpos - wpos);
  }
  z3lib_memmoveleft(&z3be_data(zh, -(1L << Z3B_DICTBITS)),
        &z3be_data(zh, wpos - (1L << Z3B_DICTBITS)),
        (1UL << Z3B_DICTBITS) + zh->dpos - wpos);
  i = 0;
  do {
DE(if(zh->histogram.literal[i]<histogram->literal[i])fprintf(stderr,"GETLAST ERROR MINUS/L[%3d] %d < %d\n",i,zh->histogram.literal[i],histogram->literal[i]);)
    zh->histogram.literal[i] -= histogram->literal[i];
  } while (++i < Z3B_NBCODES);
  i = 0;
  do {
DE(if(zh->histogram.distance[i]<histogram->distance[i])fprintf(stderr,"GETLAST ERROR MINUS/D[%3d] %d < %d\n",i,zh->histogram.distance[i],histogram->distance[i]);)
    zh->histogram.distance[i] -= histogram->distance[i];
  } while (++i < Z3B_NBDISTANCECODES);
/* should be 0 now, when wpos = zh->ipos */
  zh->histogram.literal[Z3B_CODEENDOFBLOCK] = 1;
  zh->dshift += wpos;
  zh->dpos -= wpos;
  zh->ipos -= wpos;
  zh->match -= wpos;
  zh->limitwp = zh->dsize;
  zh->state = z3be_state_end;
  if (zh->histogram3 != NULL) {
    memset(&z3relref(__u32, (zh)->histogram3)[0], 0, Z3BE_MEMSIZE_EXTRA3);
DE(i=0;do{if(z3be_histogram3(zh,0,i)!=0)fprintf(stderr,"GETLAST ERROR UNCLEAR1(%d)\n",i);i++;}while(i<8*256);i=0;do{if(z3be_distances3(zh,i)!=0)fprintf(stderr,"GETLAST ERROR UNCLEAR2(%d)\n",i);i++;}while(i<16);)
    i = 0;
    while (i < zh->ipos) {
      if (zh->mem[i] == (__u8)-1) {
        i += 1;
      } else {
        l = zh->mem[i+2];
        if (l == 0) {
          z3be_histogram_ref3(zh,
              z3be_class_distance((((__u32)zh->mem[i]) << 8) | zh->mem[i+1]),
              &z3be_data(zh, i));
        }
        i += l+3;
      }
    }
DE(if(i!=zh->ipos)fprintf(stderr,"GETLAST ERROR1: i=%lu, wpos=%lu, ipos=%lu\n", i, wpos, zh->ipos);)
  }
}

static void z3be_get_codebits(struct z3be_handle *zh, __u8 nb, __u16 bits)
{
D(fprintf(stderr, "  codebits: %08x/%d\n", bits, nb);)
  zh->pbits |= (bits << zh->pbits_nb);
  zh->pbits_nb += nb;
}

static void z3be_get_encode(struct z3be_handle *zh,
                struct z3be_handle_huff *hh, __u32 data)
{
D(fprintf(stderr, "  encode[%p]: %3d\n", hh, data);)
  z3be_get_codebits(zh,
        z3relref(__u8, hh->size)[data], z3relref(__u16, hh->code)[data]);
}

static void z3be_get_encode_clc(struct z3be_handle *zh,
                struct z3be_handle_huff *hh, int data)
{
  int h;
  h = zh->runlength;
  if ((data != zh->lcode) || (h >= ((zh->lcode == 0) ? 138 : 6))) {
    if (zh->lcode == 0) {
      if (h >= 11) {
        z3be_get_encode(zh, hh, 18);
        z3be_get_codebits(zh, 7, h - 11);
      } else if (h >= 3) {
        z3be_get_encode(zh, hh, 17);
        z3be_get_codebits(zh, 3, h - 3);
      } else {
        while (h > 0) {
          z3be_get_encode(zh, hh, zh->lcode);
          h -= 1;
        }
      }
    } else {
      if (h >= 3) {
        z3be_get_encode(zh, hh, 16);
        z3be_get_codebits(zh, 2, h - 3);
      } else {
        while (h > 0) {
          z3be_get_encode(zh, hh, zh->lcode);
          h -= 1;
        }
      }
    }
    if ((data != zh->lcode) && (data > 0)) {
      z3be_get_encode(zh, hh, data);
      zh->runlength = 0;
    } else {
      zh->runlength = 1;
    }
    zh->lcode = data;
  } else {
    zh->runlength += 1;
  }
}

static void z3be_get_encode_length(struct z3be_handle *zh,
                struct z3be_handle_huff *hh, __u32 length)
{
  __u32 i;
  i = 0;
  while (z3be_lengths_base_number[i] < (__u16)length) {
    i += 1;
  }
D(fprintf(stderr, "encode_length(%lu): i=%d\n", length, i);)
  z3be_get_encode(zh, hh, (length + z3be_lengths_class[i]) >> i);
  if ((length > 10) && (length < 258)) {
    z3be_get_codebits(zh, i,
        (length - z3be_lengths_base_number[i-1] - 1) & ((1 << i) - 1));
  }
}

static int z3be_get_encode_distance(struct z3be_handle *zh,
                struct z3be_handle_huff *hh, __u32 distance)
{
  __u32 i, l;
  i = 0;
  l = 4;
  while (distance > l) {
    i += 1;
    l <<= 1;
  }
D(fprintf(stderr, "encode_distance(%lu): i=%lu, l=%lu\n", distance, i, l);)
  z3be_get_encode(zh, hh, ((distance-1) >> i) + 2*i);
  if (i > 0) {
    zh->runlength = i;
    zh->lcode = (distance-1) & ((1 << i) - 1);
  }
  return i;
}

static void z3be_get_encode_distance2(struct z3be_handle *zh)
{
  z3be_get_codebits(zh, zh->runlength, zh->lcode);
}

static int z3be_get_one(struct z3be_handle *zh, struct z3be_weighing *weighing)
{
  __u32 n;
  switch (zh->state) {
  case z3be_state_init:
    if (zh->bfinal && (weighing->wpos == zh->ipos)) {
      zh->pbits |= (1 << zh->pbits_nb);
    }
D(fprintf(stderr,"init/bfinal: %d, wpos=%lu, dpos=%lu, ipos=%lu\n", zh->bfinal, weighing->wpos, zh->dpos, zh->ipos);)
    n = weighing->btype;
    zh->pbits |= (n << (zh->pbits_nb+1));
    zh->pbits_nb += 3;
    switch (n) {
    case Z3B_BTYPE_NC:
      zh->pbits_nb = (zh->pbits_nb + 7) & ~7;
      zh->state = z3be_state_nc;
      break;
    case Z3B_BTYPE_FIXED:
      z3be_setfixedtable(zh);
      zh->count = 0;
      zh->state = z3be_state_data;
      break;
    case Z3B_BTYPE_DYNAMIC:
      z3be_get_codebits(zh, 5+5+4,
          weighing->hlit | ((weighing->hdist | (weighing->hclen << 5)) << 5));
      zh->count = 0;
      zh->state = z3be_state_clc;
      break;
    }
    break;
  case z3be_state_nc:
    zh->pbits = weighing->wpos | ((~weighing->wpos) << 16);
    zh->pbits_nb = 32;
    zh->count = 0;
    zh->state = z3be_state_ncdata;
    break;
  case z3be_state_ncdata:
    if (zh->count >= weighing->wpos) {
D(fprintf(stderr,"GET LAST NC zh.count=%d, w.wpos=%d, zh.dpos=%d, ipos=%d\n",zh->count,weighing->wpos,zh->dpos,zh->ipos);)
      z3be_get_last(zh, zh->count, &weighing->histogram);
      return 0;
    }
    zh->pbits = z3be_data(zh, zh->count);
    zh->pbits_nb = 8;
    zh->count += 1;
    break;
  case z3be_state_clc:
    if (zh->count < (weighing->hclen + 4)) {
      z3be_get_codebits(zh, 3,
                weighing->codelen_clc[z3b_hclen_sort[zh->count]]);
      zh->count += 1;
    } else {
      z3be_buildcodetable(&zh->ll, &weighing->codelen_clc[0],
                Z3B_NBCODELENGTHSCODES);
      zh->runlength = 0;
      zh->lcode = 0;
      zh->count = 0;
      zh->state = z3be_state_ll;
    }
    break;
  case z3be_state_ll:
    if (zh->count < (weighing->hlit + 257)) {
      z3be_get_encode_clc(zh, &zh->ll, weighing->codelen_ll[zh->count]);
      zh->count += 1;
    } else {
      z3be_get_encode_clc(zh, &zh->ll, -1);
      zh->runlength = 0;
      zh->lcode = 0;
      zh->count = 0;
      zh->state = z3be_state_dc;
    }
    break;
  case z3be_state_dc:
    if (zh->count < (weighing->hdist + 1)) {
      z3be_get_encode_clc(zh, &zh->ll, weighing->codelen_dc[zh->count]);
      zh->count += 1;
    } else {
      z3be_get_encode_clc(zh, &zh->ll, -1);
      z3be_buildcodetable(&zh->ll, &weighing->codelen_ll[0], Z3B_NBCODES);
      z3be_buildcodetable(&zh->dc, &weighing->codelen_dc[0],
                Z3B_NBDISTANCECODES);
      zh->count = 0;
      zh->state = z3be_state_data;
    }
    break;
  case z3be_state_data:
    if (zh->count < weighing->wpos) {
      if ((zh->mem[zh->count] == (__u8)-1)
       || ((zh->histogram3 != NULL)
        && (zh->mem[zh->count+2] == 0)
        && (((((__u32)zh->mem[zh->count]) << 8) | zh->mem[zh->count+1])
                  >= weighing->limit3)
        && (zh->mem[zh->count+2] = zh->mem[zh->count+1] = (__u8)-1))) {
D({char c=z3be_data(zh,zh->count);fprintf(stderr, "encode LIT %08x '%c'\n", c, isprint(c) ? c : '.');})
        z3be_get_encode(zh, &zh->ll, z3be_data(zh, zh->count));
        zh->count += 1;
      } else {
D({char *c=&z3be_data(zh,zh->count); int x=-1; int d=1+((((__u32)zh->mem[zh->count]) << 8) | zh->mem[zh->count+1]); int i; int l=Z3B_LZMINLENGTH + (__u32)zh->mem[zh->count+2];fprintf(stderr,"encode REF(l=%d,d=%d,c=%d):",l,d,zh->count);i=0;while(i<l){fprintf(stderr," %02x", c[i]);i+=1;}fprintf(stderr,", '");i=0;while(i<l){fprintf(stderr,"%c",isprint(c[i]) ? c[i] : '.');if((x<0)&&(c[i]!=c[i-d]))x=i;i+=1;}fprintf(stderr,"'\n");if(x>=0){fprintf(stderr,"REF MISMATCH (%d) '",x);i=0;while(i<l){fprintf(stderr,"%c",isprint(c[i]) ? c[i-d] : '.');i+=1;}fprintf(stderr,"'\n");}})
        z3be_get_encode_length(zh, &zh->ll,
                Z3B_LZMINLENGTH + (__u32)zh->mem[zh->count+2]);
        zh->state = z3be_state_distance;
      }
    } else {
      z3be_get_encode(zh, &zh->ll, Z3B_CODEENDOFBLOCK);
D(fprintf(stderr,"GET LAST D zh.count=%d, w.wpos=%d, zh.dpos=%d, ipos=%d\n",zh->count,weighing->wpos,zh->dpos,zh->ipos);)
      z3be_get_last(zh, zh->count, &weighing->histogram);
    }
    break;
  case z3be_state_distance:
    if (z3be_get_encode_distance(zh, &zh->dc,
            1 + ((((__u32)zh->mem[zh->count]) << 8) | zh->mem[zh->count+1]))
                        != 0) {
      zh->state = z3be_state_distance2;
    } else {
      if (0) {
  case z3be_state_distance2:
        z3be_get_encode_distance2(zh);
      }
      zh->count += zh->mem[zh->count+2] + Z3B_LZMINLENGTH;
      zh->state = z3be_state_data;
    }
    break;
  case z3be_state_end:
    return 0;
  }
  return 1;
}

__u32 z3be_get(struct z3be_handle *zh, struct z3be_weighing *weighing,
                        __u8 *code, __u32 codesize)
{
  __u32 n;
  __u8 *c;
  int nb;
  c = code;
  n = codesize;
  do {
    nb = zh->pbits_nb;
    if (nb >= 8) {
      do {
D(fprintf(stderr, "get byte (%2d): %02x\n", nb, (__u8)zh->pbits);)
        nb -= 8;
        *c++ = (__u8)zh->pbits;
        zh->pbits >>= 8;
        n -= 1;
      } while ((n > 0) && (nb >= 8));
      zh->pbits_nb = nb;
    }
  } while ((n > 0) && z3be_get_one(zh, weighing));
  return codesize - n;
}

__u32 z3be_finish(struct z3be_handle *zh, __u8 *code)
{
  if (code != NULL) *code = (__u8)zh->pbits;
  return zh->pbits_nb;
}
#endif /* Z3LIB_DECODE_ONLY */

#ifndef Z3LIB_ENCODE_ONLY
struct z3bd_handle *z3bd_start(__u32 pending_bits, int pending_nb,
                        void *memory, unsigned int memsize)
{
  struct z3bd_handle *zh;
D(fprintf(stderr, "z3bd_start(%08x, %d)\n", pending_bits, pending_nb);)
  if ((memory == NULL) || (memsize < Z3BD_MEMSIZE)) {
    return NULL;
  }
  zh = (struct z3bd_handle *)memory;
  zh->pbits = pending_bits;
  zh->pbits_nb = pending_nb;
  zh->din = zh->dout = 0;
  zh->state = z3bd_state_init;
  zh->error = z3err_none;
  z3relrefset(zh->ll.bit1, zh->huff_ll_bit1[0]);
  z3relrefset(zh->ll.bit0, zh->huff_ll_bit0[0]);
  z3relrefset(zh->ll.length, zh->huff_ll_length[0]);
  z3relrefset(zh->dc.bit1, zh->huff_dc_bit1[0]);
  z3relrefset(zh->dc.bit0, zh->huff_dc_bit0[0]);
  z3relrefset(zh->dc.length, zh->huff_dc_length[0]);
  z3relrefset(zh->clc.bit1, zh->huff_clc_bit1[0]);
  z3relrefset(zh->clc.bit0, zh->huff_clc_bit0[0]);
  z3relrefset(zh->clc.length, zh->huff_clc_length[0]);
  return zh;
}

__u32 z3bd_get(struct z3bd_handle *zh, __u8 **data)
{
  __u32 n;
  if (data == NULL) {
    return 0;
  }
  if ((zh == NULL)
   || (zh->error != z3err_none)) {
    *data = NULL;
    return 0;
  }
  if (zh->din >= zh->dout) {
    n = zh->din - zh->dout;
  } else {
    n = (1UL << Z3B_DICTBITS) - zh->dout;
  }
  if ((n == 0) && (zh->state == z3bd_state_end)) {
    *data = NULL;
  } else {
    *data = &zh->data[zh->dout];
  }
  zh->dout = z3b_moddictbits(zh->dout + n);
D(fprintf(stderr, "z3bd_get done: %lu, (din=%lu, dout=%lu) %08x: %02x...\n", n, zh->din, zh->dout, *data, (*data == NULL) ? 0 : **data);)
  return n;
}

static int z3bd_buildcodetable(struct z3bd_handle_huff *hf, __u32 nb)
{
  __u32 c, l, n, i, b;
  __u16 start[32];
  __u16 chain[Z3B_NBCODESPLUSUNUSED];
  __u32 codes[Z3B_NBCODESPLUSUNUSED];
  __u16 bit[32+1];
D(fprintf(stderr, "z3bd_buildcodetable(%lu)\n", nb);)
  n = 0;
  do {
    start[n] = nb;
    n += 1;
  } while (n < 32);
  n = nb;
  do {
    n -= 1;
    l = z3relref(__u8, hf->length)[n];
    if (l > 31) return 0;
D(fprintf(stderr, "z3bd_buildcodetable, chain(%3d): l=%2d, ->%3d\n", n, l, start[l]);)
    chain[n] = start[l];
    start[l] = n;
  } while (n > 0);
D(fprintf(stderr, "z3bd_buildcodetable, chain done\n", nb);)
  c = 0;
  l = 0;
  do {
    memcpy(&hf->code[1 << l], &hf->code[0], (1 << l) * sizeof(hf->code[0]));
    memcpy(&hf->size[1 << l], &hf->size[0], (1 << l) * sizeof(hf->size[0]));
    l += 1;
    n = start[l];
    while (n < nb) {
      hf->code[c] = n;
      hf->size[c] = l;
      b = l - 1;
      while (c & (1 << b)) {
        if (b == 0) {
D(fprintf(stderr, "z3bd_buildcodetable, eight(%3d): code=%02x, size=%d\n", n, c, l);)
          if (chain[n] < nb) return 0;
          do {
            if (l < 8) {
              memcpy(&hf->code[1 << l], &hf->code[0],
                                        (1 << l) * sizeof(hf->code[0]));
              memcpy(&hf->size[1 << l], &hf->size[0],
                                        (1 << l) * sizeof(hf->size[0]));
            }
            l += 1;
            if (start[l] < nb) return 0;
          } while (l < 31);
D(for (i = 0; i < (1<<8); i++) fprintf(stderr, "z3bd_buildcodetable code[%02x]=%3d/%2d\n", i, hf->code[i], hf->size[i]);)
          return 1;
        }
        b -= 1;
      }
D(fprintf(stderr, "z3bd_buildcodetable, eight(%3d): code=%02x, size=%d, b=%2d\n", n, c, l, b);)
      c += (3 << b) - (1 << l);
      n = chain[n];
    }
  } while (l < 8);
D(fprintf(stderr, "z3bd_buildcodetable, eight\n", nb);)
  i = 0;
  do {
    l += 1;
    n = start[l];
    while (n < nb) {
      if (b < 8) {
        bit[7] = n;
        hf->code[c] = n;
        hf->size[c] = l;
D(fprintf(stderr, "z3bd_buildcodetable, long first\n", n);)
      } else {
        if ((b == 8)
         || (c & (1 << (b-1)))) {
          z3relref(__u16, hf->bit1)[bit[b-1]] = n;
D(fprintf(stderr, "z3bd_buildcodetable, long bit0[%3d]=%3d, bit1[%3d]=%3d\n", n, bit[b+1], bit[b-1], n);)
        }
D(else fprintf(stderr, "z3bd_buildcodetable, long bit0[%3d]=%3d\n", n, bit[b+1]);)
        z3relref(__u16, hf->bit0)[n] = bit[b+1];
        bit[b] = n;
      }
      codes[i++] = c;
      b = l - 1;
      while (c & (1 << b)) {
        if (b == 0) {
D(fprintf(stderr, "z3bd_buildcodetable, long(%3d): code=%08x, size=%2d\n", n, c, l);)
          if (chain[n] < nb) return 0;
          while (l < 31) {
            l += 1;
            if (start[l] < nb) return 0;
          }
D(for (i = 0; i < (1<<8); i++) fprintf(stderr, "z3bd_buildcodetable code[%02x]=%3d/%2d\n", i, hf->code[i], hf->size[i]);)
          return 1;
        }
        b -= 1;
      }
D(fprintf(stderr, "z3bd_buildcodetable, long(%3d): code=%08x, size=%2d, b=%2d\n", n, c, l, b);)
      c += (3 << b) - (1 << l);
      n = chain[n];
    }
  } while (l < 31);
D(for (i = 0; i < (1<<8); i++) fprintf(stderr, "z3bd_buildcodetable code[%02x]=%3d/%2d\n", i, hf->code[i], hf->size[i]);)
  return 1;
}

static void z3bd_setfixedtable(struct z3bd_handle *zh)
{
D(fprintf(stderr, "z3bd_setfixedtable\n");)
  memset(&zh->huff_ll_length[0], 8, sizeof(zh->huff_ll_length));
  memset(&zh->huff_ll_length[144], 9, 256-144);
  memset(&zh->huff_ll_length[256], 7, 280-256);
  z3bd_buildcodetable(&zh->ll, Z3B_NBCODESPLUSUNUSED);
  memset(&zh->huff_dc_length[0], 5, Z3B_NBDISTANCECODESPLUSUNUSED);
  z3bd_buildcodetable(&zh->dc, Z3B_NBDISTANCECODESPLUSUNUSED);
D(fprintf(stderr, "z3bd_setfixedtable done\n");)
}

static __u32 z3bd_huffman1(struct z3bd_handle *zh, struct z3bd_handle_huff *hf,
                        __u32 code, __u32 nbits)
{
  __u32 alternate;
  alternate = zh->alternate;
D(fprintf(stderr, "z3bd_huffman1(%2d), code=%04x, alternate=%04x, pbits=%08x\n", nbits, code, alternate, zh->pbits);)
  zh->pbits_nb -= nbits;
  do {
    if (zh->pbits & 1) {
      code = alternate;
      alternate = z3relref(__u16, hf->bit1)[alternate];
    } else {
      alternate = z3relref(__u16, hf->bit0)[alternate];
    }
    zh->pbits >>= 1;
    nbits -= 1;
  } while (nbits > 0);
D(fprintf(stderr, "z3bd_huffman1 done, code=%04x, alternate=%04x\n", code, alternate);)
  zh->alternate = alternate;
  return code;
}

static __u32 z3bd_huffman0(struct z3bd_handle *zh, struct z3bd_handle_huff *hf,
                        __u32 nbits)
{
  __u16 code;
  __u16 alternate;
  code = hf->code[(__u8)zh->pbits];
D(fprintf(stderr, "z3bd_huffman0(%2d), code=%04x\n", nbits, code);)
  zh->pbits_nb -= nbits;
  if (nbits <= 8) {
    zh->pbits >>= nbits;
D(fprintf(stderr, "z3bd_huffman0 done, code=%04x\n", code);)
  } else {
    nbits -= 8;
    zh->pbits >>= 8;
    alternate = z3relref(__u16, hf->bit1)[code];
    do {
      if (zh->pbits & 1) {
        code = alternate;
        alternate = z3relref(__u16, hf->bit1)[alternate];
      } else {
        alternate = z3relref(__u16, hf->bit0)[alternate];
      }
      zh->pbits >>= 1;
      nbits -= 1;
    } while (nbits > 0);
    zh->alternate = alternate;
D(fprintf(stderr, "z3bd_huffman0 done, code=%04x, alternate=%04x\n", code, alternate);)
  }
  return code;
}

static void z3bd_copystring(struct z3bd_handle *zh,
                        __u32 length, __u32 distance)
{
  __u32 i = zh->din;
  __u32 r = z3b_moddictbits(i - distance);
D(fprintf(stderr, "z3bd_copystring(%d, -%d: %d): %02x...\n", length, distance, r, zh->data[r]);)
  do {
    __u32 l;
    l = length;
    if (l > distance) l = distance;
    if (l > ((1UL << Z3B_DICTBITS) - i)) l = (1UL << Z3B_DICTBITS) - i;
    if (l > ((1UL << Z3B_DICTBITS) - r)) l = (1UL << Z3B_DICTBITS) - r;
    memcpy(&zh->data[i], &zh->data[r], l);
    i = z3b_moddictbits(i+l);
    r = z3b_moddictbits(r+l);
    length -= l;
  } while (length > 0);
  zh->din = i;
}

const static __u8 z3bd_lengths_extra_bits[(Z3B_NBLENGTHCODES+3)>>2] =
  {0, 0, 1, 2, 3, 4, 5, 0};
const static __u16 z3bd_lengths_base_length[(Z3B_NBLENGTHCODES+3)>>2] =
  {257-3, 261-7, (265<<1)-11, (269<<2)-19,
   (273<<3)-35, (277<<4)-67, (281<<5)-131, 285-258};
const static __u8 z3bd_codelengths_repeatbits[19-16] = {2, 3, 7};

static int z3bd_put_one(struct z3bd_handle *zh)
{
  __u32 n, m;
D(fprintf(stderr, "z3bd_put_one, state=%d\n", zh->state);)
  switch (zh->state) {
  case z3bd_state_init:
    if (zh->pbits_nb < 3) return 0;
    zh->bfinal = zh->pbits & 0x01;
D(fprintf(stderr, "z3bd_state_init bfinal=%d, btype=%d\n", zh->pbits & 0x01, (zh->pbits >> 1) & 0x03);)
    switch (zh->pbits & (0x03 << 1)) {
    case (Z3B_BTYPE_NC << 1):
      zh->state = z3bd_state_nc;
      break;
    case (Z3B_BTYPE_FIXED << 1):
      z3bd_setfixedtable(zh);
      zh->state = z3bd_state_data;
      break;
    case (Z3B_BTYPE_DYNAMIC << 1):
      zh->state = z3bd_state_dh;
      break;
    default:
      zh->error = z3err_bd_btype3;
      return 0;
    }
    zh->pbits >>= 3;
    zh->pbits_nb -= 3;
D(fprintf(stderr, "z3bd_put_one, init, done\n");)
    break;
  case z3bd_state_nc:
    if (zh->pbits_nb < 16) return 0;
    if (zh->pbits_nb & 7) {
      zh->pbits >>= (zh->pbits_nb & 7);
      zh->pbits_nb &= ~7;
    }
    zh->count = (__u16)zh->pbits;
    zh->state = z3bd_state_nclen;
    zh->pbits >>= 16;
    zh->pbits_nb -= 16;
    break;
  case z3bd_state_nclen:
    if (zh->pbits_nb < 16) return 0;
    if (zh->count != (((__u16)zh->pbits) ^ 0xFFFF)) {
      zh->error = z3err_bd_nlenmismatch;
      return 0;
    }
    zh->state = z3bd_state_ncdata;
    zh->pbits >>= 16;
    zh->pbits_nb -= 16;
    break;
  case z3bd_state_ncdata:
    if (zh->count == 0) {
      zh->state = z3bd_state_end;
      return 0;
    }
    if (zh->pbits_nb < 8) return 0;
    n = z3b_moddictbits(zh->din+1);
    if (n == zh->dout) return 0;
    zh->data[zh->din] = (__u8)zh->pbits;
    zh->din = n;
    zh->count -= 1;
    zh->pbits >>= 8;
    zh->pbits_nb -= 8;
    break;
  case z3bd_state_dh:
    if (zh->pbits_nb < (5+5+4)) return 0;
    zh->pbits_nb -= (5+5+4);
    zh->value = (zh->pbits & ((1 << 5) - 1)) + Z3B_CODEENDOFBLOCK+1;
D(fprintf(stderr, "z3bd_state_dh, HLIT=%d\n", zh->value);)
    if (zh->value > Z3B_NBCODES) {
      zh->error = z3err_bd_hlitexceed;
      return 0;
    }
    zh->pbits >>= 5;
    zh->distance = (zh->pbits & ((1 << 5) - 1)) + 1;
D(fprintf(stderr, "z3bd_state_dh, HDIST=%d\n", zh->distance);)
    if (zh->distance > Z3B_NBDISTANCECODES) {
      zh->error = z3err_bd_hdistexceed;
      return 0;
    }
    zh->pbits >>= 5;
    zh->clen = (zh->pbits & ((1 << 4) - 1)) + 4;
D(fprintf(stderr, "z3bd_state_dh, HCLEN=%d\n", zh->clen);)
    zh->pbits >>= 4;
    memset(&zh->huff_clc_length[0], 0, Z3B_NBCODELENGTHSCODES);
    zh->count = 0;
    zh->state = z3bd_state_codelengths;
    break;
  case z3bd_state_codelengths:
    if (zh->pbits_nb < 3) return 0;
    zh->pbits_nb -= 3;
    zh->huff_clc_length[z3b_hclen_sort[zh->count]] = zh->pbits & ((1 << 3) - 1);
D(fprintf(stderr, "z3bd_state_codelengths, clc.length[%d]=%d\n", z3b_hclen_sort[zh->count], zh->pbits & ((1 << 3) - 1));)
    zh->pbits >>= 3;
    zh->count += 1;
    if (zh->count >= zh->clen) {
      if (!z3bd_buildcodetable(&zh->clc, Z3B_NBCODELENGTHSCODES)) {
        zh->error = z3err_bd_codelengthtable;
        return 0;
      }
      memset(&zh->huff_ll_length[0], 0, Z3B_NBCODES);
      zh->count = 0;
      zh->state = z3bd_state_codeliterals;
    }
    break;
  case z3bd_state_codeliterals:
    m = zh->clc.size[(__u8)zh->pbits];
    if (m == 0) {
      zh->error = z3err_bd_codeliteralundefined;
      return 0;
    }
    if (zh->pbits_nb < m) return 0;
    n = z3bd_huffman0(zh, &zh->clc, m);
D(fprintf(stderr, "z3bd_state_codeliterals length(%d) %d %d\n", n, zh->huff_clc_length[n], m);)
    if (zh->huff_clc_length[n] != m) {
      zh->error = z3err_bd_codeliteralexceed;
      return 0;
    }
    if (n >= 16) {
      zh->clen = n;
  case z3bd_state_codeliterals2:
      n = z3bd_codelengths_repeatbits[zh->clen - 16];
      if (zh->pbits_nb < n) {
        zh->state = z3bd_state_codeliterals2;
        return 0;
      }
      zh->state = z3bd_state_codeliterals;
      zh->pbits_nb -= n;
      m = zh->pbits & ((1 << n) - 1);
      zh->pbits >>= n;
D(fprintf(stderr, "z3bd_state_codeliterals2 %02x/%d\n", m, n);)
      if (zh->clen == 16) {
        if (zh->count == 0) {
          zh->error = z3err_bd_codeliteralnoprevious;
          return 0;
        }
        n = zh->huff_ll_length[zh->count-1];
      } else {
        n = 0;
      }
      if (zh->clen == 18) {
        m += 11;
      } else {
        m += 3;
      }
      if ((zh->count + m) > zh->value) {
        zh->error = z3err_bd_codeliteraltoomany;
        return 0;
      }
    } else {
      m = 1;
    }
    do {
      zh->huff_ll_length[zh->count] = n;
      zh->count += 1;
      m -= 1;
    } while (m > 0);
    if (zh->count >= zh->value) {
      if (!z3bd_buildcodetable(&zh->ll, Z3B_NBCODES)) {
        zh->error = z3err_bd_codeliteraltable;
        return 0;
      }
      memset(&zh->huff_dc_length[0], 0, Z3B_NBDISTANCECODES);
      zh->count = 0;
      zh->state = z3bd_state_codedistances;
    }
    break;
  case z3bd_state_codedistances:
    m = zh->clc.size[(__u8)zh->pbits];
    if (m == 0) {
      zh->error = z3err_bd_codedistancesundefined;
      return 0;
    }
    if (zh->pbits_nb < m) return 0;
    n = z3bd_huffman0(zh, &zh->clc, m);
    if (zh->huff_clc_length[n] != m) {
D(fprintf(stderr, "z3bd_state_codedistances length(%d) %d %d\n", n, zh->huff_clc_length[n], m);)
      zh->error = z3err_bd_codedistancesexceed;
      return 0;
    }
    if (n >= 16) {
      zh->clen = n;
  case z3bd_state_codedistances2:
      n = z3bd_codelengths_repeatbits[zh->clen - 16];
      if (zh->pbits_nb < n) {
        zh->state = z3bd_state_codedistances2;
        return 0;
      }
      zh->state = z3bd_state_codedistances;
      zh->pbits_nb -= n;
      m = zh->pbits & ((1 << n) - 1);
      zh->pbits >>= n;
D(fprintf(stderr, "z3bd_state_codedistances2 %02x/%d\n", m, n);)
      if (zh->clen == 16) {
        if (zh->count == 0) {
          zh->error = z3err_bd_codedistancesnoprevious;
          return 0;
        }
        n = zh->huff_dc_length[zh->count-1];
      } else {
        n = 0;
      }
      if (zh->clen == 18) {
        m += 11;
      } else {
        m += 3;
      }
      if ((zh->count + m) > zh->distance) {
        zh->error = z3err_bd_codedistancestoomany;
        return 0;
      }
    } else {
      m = 1;
    }
    do {
      zh->huff_dc_length[zh->count] = n;
      zh->count += 1;
      m -= 1;
    } while (m > 0);
    if (zh->count >= zh->distance) {
      if (!z3bd_buildcodetable(&zh->dc, Z3B_NBDISTANCECODES)) {
        zh->error = z3err_bd_codedistancestable;
        return 0;
      }
      zh->state = z3bd_state_data;
    }
    break;
  case z3bd_state_data:
    n = zh->ll.size[(__u8)zh->pbits];
    if (n == 0) {
      zh->error = z3err_bd_dataundefined;
      return 0;
    }
    if (zh->pbits_nb < n) return 0;
    zh->value = z3bd_huffman0(zh, &zh->ll, n);
    if (0) {
  case z3bd_state_data1:
      zh->state = z3bd_state_data;
      n = zh->clen;
    }
    while ((m = zh->huff_ll_length[zh->value] - n) > 0) {
      if (zh->pbits_nb < m) {
        zh->clen = n;
        zh->state = z3bd_state_data1;
        return 0;
      }
      zh->value = z3bd_huffman1(zh, &zh->ll, zh->value, m);
      n += m;
    }
    if (zh->value == Z3B_CODEENDOFBLOCK) {
      zh->state = z3bd_state_end;
      return 0;
    }
    if (zh->value < Z3B_CODEENDOFBLOCK) {
      if (0) {
  case z3bd_state_literal:
        zh->state = z3bd_state_data;
      }
      n = z3b_moddictbits(zh->din+1);
      if (n == zh->dout) {
        zh->state = z3bd_state_literal;
        return 0;
      }
D(fprintf(stderr, "z3bd_state_literal: data[%d++]=%02x\n", zh->din, (__u8)zh->value);)
      zh->data[zh->din] = (__u8)zh->value;
      zh->din = n;
      break;
    }
    zh->state = z3bd_state_length;
    break;
  case z3bd_state_length:
    m = (zh->value-1-Z3B_CODEENDOFBLOCK) >> 2;
    n = z3bd_lengths_extra_bits[m];
    if (zh->pbits_nb < n) return 0;
    zh->value = ((zh->value << n) - z3bd_lengths_base_length[m])
      + ((n != 0) ? (zh->pbits & ((1 << n) - 1)) : 0);
D(fprintf(stderr, "z3bd_state_length: %08x/%d: %d\n", (zh->pbits & ((1 << n) - 1)), n, zh->value);)
    zh->pbits >>= n;
    zh->pbits_nb -= n;
  case z3bd_state_length2:
    if ((z3b_moddictbits(zh->dout - zh->din - 1)) < zh->value) {
      zh->state = z3bd_state_length2;
      return 0;
    }
    zh->state = z3bd_state_distance;
    break;
  case z3bd_state_distance:
    n = zh->dc.size[(__u8)zh->pbits];
    if (n == 0) {
      zh->error = z3err_bd_distanceundefined;
      return 0;
    }
    if (zh->pbits_nb < n) return 0;
    zh->distance = z3bd_huffman0(zh, &zh->dc, n);
    if (0) {
  case z3bd_state_distance1:
      n = zh->clen;
    }
    while ((m = zh->huff_dc_length[zh->distance] - n) > 0) {
      if (zh->pbits_nb < m) {
        zh->clen = n;
        zh->state = z3bd_state_distance1;
        return 0;
      }
      zh->distance = z3bd_huffman1(zh, &zh->dc, zh->distance, m);
      n += m;
    }
    zh->state = z3bd_state_distance2;
    break;
  case z3bd_state_distance2:
    n = (zh->distance >> 1);
    if (n > 0) n -= 1;
    if (zh->pbits_nb < n) return 0;
    m = ((zh->distance << n) - (n << (n+1)) + 1 + (zh->pbits & ((1 << n) - 1)));
D(fprintf(stderr, "z3bd_state_distance2: %08x/%d: %d\n", (zh->pbits & ((1 << n) - 1)), n, m);)
    z3bd_copystring(zh, zh->value, m);
    zh->pbits >>= n;
    zh->pbits_nb -= n;
    zh->state = z3bd_state_data;
    break;
  case z3bd_state_end:
D(fprintf(stderr, "z3bd_state_end\n");)
  default:
    return 0;
  }
D(fprintf(stderr, "z3bd_put_one, return 1\n");)
  return 1;
}

__u32 z3bd_put(struct z3bd_handle *zh, __u8 *code, __u32 codesize)
{
  __u32 n;
  __u8 *c;
  if (zh == NULL) {
    return 0;
  }
  n = codesize;
  c = code;
  do {
    int nb = zh->pbits_nb;
    if ((nb <= 24) && (n > 0)) {
      do {
        zh->pbits |= (*c++ << nb);
        n -= 1;
        nb += 8;
      } while ((nb <= 24) && (n > 0));
      zh->pbits_nb = nb;
    }
D(fprintf(stderr, "z3bd_put(%lu): n=%lu, din=%lu, dout=%lu, pb=%08x, nb=%d\n", codesize, codesize-n, zh->din, zh->dout, zh->pbits, nb);)
  } while (z3bd_put_one(zh));
D(fprintf(stderr, "z3bd_put done: n=%lu, din=%lu, dout=%lu\n", n, zh->din, zh->dout);)
  if (zh->error != z3err_none) return 0;
  return codesize - n;
}

int z3bd_finish(struct z3bd_handle *zh,
                        __u32 *pending_bits, int *pending_nb)
{
  int e;
  e = zh->error;
D(fprintf(stderr, "z3bd_finish: error=%d, state=%d, bfinal=%d\n", e, zh->state, zh->bfinal);)
  if ((e == z3err_none)
   && (zh->state == z3bd_state_end)
   && (!zh->bfinal)) {
    zh->bfinal = 1;
    zh->state = z3bd_state_init;
    return z3err_bd_notbfinal;
  }
  *pending_bits = zh->pbits;
  *pending_nb = zh->pbits_nb;
  return e;
}
#endif /* Z3LIB_ENCODE_ONLY */
