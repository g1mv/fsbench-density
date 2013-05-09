#ifndef _7Z_HPP_JS0pDFISjuay96H
#define _7Z_HPP_JS0pDFISjuay96H

#include "codecs.hpp"

namespace FsBench7z
{

    size_t deflate(char*in, size_t isize, char*out, size_t osize, void*_);
    size_t inflate(char*in, size_t isize, char*out, size_t osize, void*_);
    size_t deflate64(char*in, size_t isize, char*out, size_t osize, void*_);
    size_t inflate64(char*in, size_t isize, char*out, size_t osize, void*_);

#if 0
size_t unlzx(char*in, size_t isize, char*out, size_t osize, void*_);
#endif
} //FsBench7z

#endif // _7Z_HPP_JS0pDFISjuay96H
