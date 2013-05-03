#include "bcl.hpp"

extern "C"
{
#include "huffman.h"
#include "lz.h"
#include "rle.h"
}

namespace FsBenchBCL
{

size_t huffman(char* in, size_t isize, char* out, size_t osize, void* _)
{
    return Huffman_Compress((unsigned char*)in, (unsigned char*)out, isize);
}

size_t unhuffman(char* in, size_t isize, char* out, size_t osize, void* _)
{
    Huffman_Uncompress((unsigned char*)in, (unsigned char*)out, isize, osize);
    return osize;
}

size_t huffman_maxsize(size_t input_size)
{
    return input_size + input_size/100 + 320;
}

size_t lz(char* in, size_t isize, char* out, size_t osize, void* _)
{
    return LZ_Compress((unsigned char*)in, (unsigned char*)out, isize);
}

size_t unlz(char* in, size_t isize, char* out, size_t osize, void* _)
{
    LZ_Uncompress((unsigned char*)in, (unsigned char*)out, isize);
    return osize;
}

size_t lz_maxsize(size_t input_size)
{
    return input_size + input_size/256 + 1;
}

size_t rle(char* in, size_t isize, char* out, size_t osize, void* _)
{
    return RLE_Compress((unsigned char*)in, (unsigned char*)out, isize);
}

size_t unrle(char* in, size_t isize, char* out, size_t osize, void* _)
{
    RLE_Uncompress((unsigned char*)in, (unsigned char*)out, isize);
    return osize;
}

size_t rle_maxsize(size_t input_size)
{
    return input_size + input_size/256 + 1;
}

size_t LZFast::lzfast(char* in, size_t isize, char* out, size_t osize, void* work)
{
    return LZ_CompressFast((unsigned char*)in, (unsigned char*)out, isize, *(unsigned int**)work);
}
void LZFast::init(const std::string& args, unsigned threads_no, size_t isize, bool init_compressor, bool init_decompressor)
{
    if(init_compressor)
    {
        int piece_size = (isize + 65536);
        this->workmem = new unsigned int[threads_no * piece_size];
        this->params = new void*[threads_no];
        for(unsigned i=0; i<threads_no; ++i)
            this->params[i] = workmem + i*piece_size;
    }
}
void LZFast::cleanup()
{
    if(this->params)
    {
        delete[] this->workmem;
        delete[] this->params;
        this->workmem = 0;
        this->params  = 0;
    }
}
void** LZFast::eparams()
{
    return params;
}


} // FsBenchBCL