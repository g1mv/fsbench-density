#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

// checksums
#include "cryptopp/adler32.h"
#include "cryptopp/crc.h"
#include "cryptopp/md5.h"
#include "cryptopp/sha.h"
//compressors
#include "cryptopp/zdeflate.h"
#include "cryptopp/zinflate.h"

namespace FsBenchCryptoPP
{

// checksums
void adler32(char* in, size_t isize, char* out)
{
    CryptoPP::Adler32 c;
    c.CalculateDigest((byte*)out, (const byte*)in, isize);
}
void crc(char* in, size_t isize, char* out)
{
    CryptoPP::CRC32 c;
    c.CalculateDigest((byte*)out, (const byte*)in, isize);
}
void md5(char* in, size_t isize, char* out)
{
    CryptoPP::Weak1::MD5 c;
    c.CalculateDigest((byte*)out, (const byte*)in, isize);
}
void sha224(char* in, size_t isize, char* out)
{
    CryptoPP::SHA224 c;
    c.CalculateDigest((byte*)out, (const byte*)in, isize);
}
void sha256(char* in, size_t isize, char* out)
{
    CryptoPP::SHA256 c;
    c.CalculateDigest((byte*)out, (const byte*)in, isize);
}
void sha384(char* in, size_t isize, char* out)
{
    CryptoPP::SHA384 c;
    c.CalculateDigest((byte*)out, (const byte*)in, isize);
}
void sha512(char* in, size_t isize, char* out)
{
    CryptoPP::SHA512 c;
    c.CalculateDigest((byte*)out, (const byte*)in, isize);
}

//compressors
#define BUFSIZE (1 * 1024 * 1024)

size_t deflate(char* in, size_t isize, char* out, size_t osize, void* mode)
{
    CryptoPP::Deflator x(NULL, *(intptr_t*)mode);

    size_t output_size = 0;
    size_t i = 0;
    for(; i+BUFSIZE < isize; i+=BUFSIZE)
    {
        x.Put2((byte*)&in[i], BUFSIZE, 0, true);
        output_size += x.Get((byte*)&out[output_size], osize-output_size);
    }
    x.Put2((byte*)&in[i], isize-i, 1, true);
    output_size += x.Get((byte*)&out[output_size], osize-output_size);

	return output_size;
}
size_t inflate(char* in, size_t isize, char* out, size_t osize, void* _)
{
    CryptoPP::Inflator x;
    

    size_t output_size = 0;
    size_t i = 0;
    for(; i+BUFSIZE < isize; i+=BUFSIZE)
    {
        x.Put2((byte*)&in[i], BUFSIZE, 0, true);
        output_size += x.Get((byte*)&out[output_size], osize-output_size);
    }
    x.Put2((byte*)&in[i], isize-i, 1, true);
    output_size += x.Get((byte*)&out[output_size], osize-output_size);

	return output_size;
}
} // FsBenchCryptoPP