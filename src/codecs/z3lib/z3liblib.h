/*
 * z3lib (c)1993,2000,2005,2006/GPL,BSD Oskar Schirmer, schirmer@scara.com
 * combined huffman/lz compression library
 * now rfc1951 conformant
 */

#ifndef __Z3LIBLIB_H__
#define __Z3LIBLIB_H__

#ifdef Z3LIB_WITHOUT_LIBC
#define NULL (0)

static inline void z3lib_memmoveleft(char *trg, const char *src, int n)
{
  while (n > 0) {
    *trg++ = *src++;
    n -= 1;
  }
}

static inline void memcpy(void *trg, const void *src, int n)
{
  z3lib_memmoveleft(trg, src, n);
}

static inline void memset(void *trg, int data, int n)
{
  char *t = trg;
  while (n > 0) {
    *t++ = (char)data;
    n -= 1;
  }
}

static inline int memcmp(const void *s1, const void *s2, int n)
{
  const char *p1 = s1;
  const char *p2 = s2;
  int r = 0;
  while ((n > 0) && !(r = (*p2++ - *p1++))) n -= 1;
  return r;
}

static inline int strlen(const char *s)
{
  const char *p = s;
  while (*p != 0) p += 1;
  return p-s;
}

#else

static inline void z3lib_memmoveleft(char *trg, const char *src, int n)
{
  memmove(trg, src, n);
}

#endif

static inline int z3lib_countequal(const unsigned char *a,
                const unsigned char *b, int count)
{
  int n;
  if (count <= 0) {
    return 0;
  }
  n = count;
  while ((*a++ == *b++) && (--n > 0));
  return count - n;
}

static inline int z3lib_countsingle(const unsigned char a,
                const unsigned char *b, int count)
{
  int n;
  if (count <= 0) {
    return 0;
  }
  n = count;
  while ((a == *b++) && (--n > 0));
  return count - n;
}

#if defined(Z3LIB_ENCODE_ONLY) && defined(Z3LIB_DECODE_ONLY)
#error do not set both Z3LIB_ENCODE_ONLY and Z3LIB_DECODE_ONLY
#endif

#define z3relref(t,p) ((p)-(t*)0, ((t*)(((__u8*)&(p))+(unsigned long)(p))))
#define z3relrefset(p,d) (p=(void*)(((__u8*)&(d))-((__u8*)&(p))))

#ifndef EBADRQC
#define EBADRQC 1
#endif
#ifndef ENODATA
#define ENODATA 1
#endif
#ifndef EPROTO
#define EPROTO 1
#endif
#ifndef EBADMSG
#define EBADMSG 1
#endif
#ifndef EOVERFLOW
#define EOVERFLOW 1
#endif
#ifndef EILSEQ
#define EILSEQ 1
#endif
#ifndef EMSGSIZE
#define EMSGSIZE 1
#endif
#ifndef EPROTOTYPE
#define EPROTOTYPE 1
#endif
#ifndef EPROTONOSUPPORT
#define EPROTONOSUPPORT 1
#endif

#endif /* __Z3LIBLIB_H__ */
