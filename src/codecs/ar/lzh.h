#ifndef LZH_H
#define LZH_H
#include "stdint.h"

/* 
 * * Modified prototypes to allow passing extra param
 *   via read and write prototypes
 *
 * * Added global structure to keep track of required
 *   variables making the code thread-safe and able
 *
 * - SanguineRose 2/29/2012
 */

/* This is what pstdint.h is for... */
typedef uint8_t   uchar;   /*  8 bits or more */
typedef uint32_t  uint;    /* 16 bits or more */
typedef uint16_t  ushort;  /* 16 bits or more */
typedef uint32_t  ulong;   /* 32 bits or more */
typedef short node;

#ifndef CHAR_BIT
    #define CHAR_BIT            8
#endif

#ifndef UCHAR_MAX
    #define UCHAR_MAX           255
#endif

#define BITBUFTYPE ushort 
#define BITBUFSIZ (CHAR_BIT * sizeof (BITBUFTYPE))
#define DICBIT    13                              /* 12(-lh4-) or 13(-lh5-) */
#define DICSIZ (1U << DICBIT)
#define MAXMATCH 256                              /* formerly F (not more than UCHAR_MAX + 1) */
#define THRESHOLD  3                              /* choose optimal value */
#define NC (UCHAR_MAX + MAXMATCH + 2 - THRESHOLD) /* alphabet = {0, 1, 2, ..., NC - 1} */
#define CBIT 9                                    /* $\lfloor \log_2 NC \rfloor + 1$ */
#define CODE_BIT  16                              /* codeword length */

typedef void * void_pointer;
typedef int (*type_fnc_read) (void  *data, int n, void *p);
typedef int (*type_fnc_write) (void  *data, int n, void *p);
typedef void_pointer (*type_fnc_malloc) (unsigned n);
typedef void (*type_fnc_free) (void  *p);

#define NP (DICBIT + 1)
#define NT (CODE_BIT + 3)
#define PBIT 4      /* smallest integer such that (1U << PBIT) > NP */
#define TBIT 5      /* smallest integer such that (1U << TBIT) > NT */
#if NT > NP
    #define NPT NT
#else
    #define NPT NP
#endif

int lzh_freeze (type_fnc_read   pfnc_read,
                type_fnc_write  pfnc_write,
		type_fnc_malloc pfnc_malloc,
		type_fnc_free   pfnc_free,
		void *p);

int lzh_melt (type_fnc_read   pfnc_read,
              type_fnc_write  pfnc_write,
	      type_fnc_malloc pfnc_malloc,
	      type_fnc_free   pfnc_free,
	      ulong origsize,
	      void *p);

#endif /* ifndef LZH_H */
