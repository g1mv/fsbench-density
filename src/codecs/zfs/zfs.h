#ifndef ZFS_H_dyUG8
#define ZFS_H_dyUG8

#include <stddef.h>
#include <stdint.h>

#define uchar_t unsigned char


size_t lzjb_compress2010(uchar_t *s_start, uchar_t *d_start, size_t s_len, size_t d_len, int n);
size_t lzjb_decompress2010(uchar_t *s_start, uchar_t *d_start, size_t s_len, size_t d_len, int n);

void fletcher_2_native  (const void *buf, uint64_t size, uint64_t *zcp);
void fletcher_2_byteswap(const void *buf, uint64_t size, uint64_t *zcp);
void fletcher_4_native  (const void *buf, uint64_t size, uint64_t *zcp);
void fletcher_4_byteswap(const void *buf, uint64_t size, uint64_t *zcp);

#endif //LZJB2010_H_dyUG8
