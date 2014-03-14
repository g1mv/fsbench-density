/* ugly type names */

typedef	unsigned char	uch;
typedef	unsigned short	ush;
typedef	unsigned int	uit;

#undef ONLY_64K /* 64k-max encoder is faster */
                /* but only veeeery slightly */


int
wLZV1 (uch * in, uch * out, ush * heap, int len, int out_len);

int
rLZV1 (uch const *const in, uch * const out, int ilen, int len);