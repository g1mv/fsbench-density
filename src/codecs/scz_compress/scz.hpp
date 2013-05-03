#ifndef _SCZ_HPP_Jp07BDFISjuay96H
#define _SCZ_HPP_Jp07BDFISjuay96H

#include "abstractCodecs.hpp"
#include "codecs.hpp"

struct Scz : CodecWithIntModes
{
    Scz():
        CodecWithIntModes("scz", _SCZ_VERSION, compress, decompress, 64, 16777214, "16777214")
    {}
    static size_t compress  (char* in, size_t isize, char* out, size_t osize, void* mode);
    static size_t decompress(char* in, size_t isize, char* out, size_t osize, void* mode);
    virtual void cleanup();
    
};

#endif // _SCZ_HPP_Jp07BDFISjuay96H