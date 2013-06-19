#ifndef _BYTESWAP_H
#define _BYTESWAP_H

static inline uint16 bswap_16(uint16 x) {
  return (x>>8) | (x<<8);
}

static inline uint32 bswap_32(uint32 x) {
  return (bswap_16(x&0xffff)<<16) | (bswap_16(x>>16));
}

static inline uint64 bswap_64(uint64 x) {
  return (((uint64)bswap_32(x&0xffffffffull))<<32) | (bswap_32(x>>32));
}

#endif
