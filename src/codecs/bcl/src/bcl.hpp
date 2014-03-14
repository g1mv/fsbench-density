#ifndef _BCL_HPP_Jp07BDFISjuay96H
#define _BCL_HPP_Jp07BDFISjuay96H

#include "codecs.hpp"

namespace FsBenchBCL
{

size_t rle  (char* in, size_t isize, char* out, size_t osize, void* _);
size_t unrle(char* in, size_t isize, char* out, size_t osize, void* _);
size_t rle_maxsize(size_t input_size);

size_t huffman  (char* in, size_t isize, char* out, size_t osize, void* _);
size_t unhuffman(char* in, size_t isize, char* out, size_t osize, void* _);
size_t huffman_maxsize(size_t input_size);

size_t lz  (char* in, size_t isize, char* out, size_t osize, void* _);
size_t unlz(char* in, size_t isize, char* out, size_t osize, void* _);
size_t lz_maxsize(size_t input_size);

struct LZFast : Codec
{
    LZFast():
        Codec("bcl-lzfast", _BCL_VERSION, lzfast, unlz, lz_maxsize)
    {}
    static size_t lzfast(char* in, size_t isize, char* out, size_t osize, void* _);
    virtual void init(const std::string& args, unsigned threads_no, size_t isize, bool init_compressor, bool init_decompressor);
    virtual void cleanup();
    virtual void** eparams();

private:
    unsigned int* workmem;
    void** params;

};

} //FsBenchBCL


#endif // _BCL_HPP_Jp07BDFISjuay96H