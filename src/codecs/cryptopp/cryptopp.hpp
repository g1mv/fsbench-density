#ifndef _CRYPTOPP_HPP_Jp07BDFISjuay96H
#define _CRYPTOPP_HPP_Jp07BDFISjuay96H

#include "codecs.hpp"

namespace FsBenchCryptoPP
{

size_t deflate(char* in, size_t isize, char* out, size_t osize, void* mode);
size_t inflate(char* in, size_t isize, char* out, size_t osize, void* _);
void adler32(char* in, size_t isize, char* out);
void crc(char* in, size_t isize, char* out);
void md5(char* in, size_t isize, char* out);
void sha224(char* in, size_t isize, char* out);
void sha256(char* in, size_t isize, char* out);
void sha384(char* in, size_t isize, char* out);
void sha512(char* in, size_t isize, char* out);

} //FsBenchCryptoPP


#endif // _CRYPTOPP_HPP_Jp07BDFISjuay96H