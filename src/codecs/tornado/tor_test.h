#ifndef TOR_TEST_H_p9U8Nb
#define TOR_TEST_H_p9U8Nb

#if __cplusplus >= 201103L // C++ 2011
    #include <cstdint>
#else
extern "C"
{
    #include <stdint.h>
}
#endif // C++ 2011

uint32_t tor_compress(uint8_t method, uint8_t* inbuf, uint8_t* outbuf, uint32_t size);
uint32_t tor_decompress(uint8_t* inbuf, uint8_t* outbuf, uint32_t size);

#endif // TOR_TEST_H_p9U8Nb