#include <cstring>

#include "gipfeli.h"
#include "codecs.hpp"

namespace FsBenchGipfeli
{
using namespace gipfeli;

size_t compress(char * in, size_t isize, char * out, size_t, void *)
{
    Compressor compressor;
    Status status = compressor.Init();
    if(status != kOk)
        return CODING_ERROR;
    std::string output_str;
    status = compressor.Compress(in, isize, &output_str);
    if(status != kOk)
        return CODING_ERROR;
    size_t compressed_size = output_str.size();
    memcpy(out, output_str.data(), compressed_size);
    return compressed_size;
}
size_t decompress(char * in, size_t isize, char * out, size_t, void *)
{
    Uncompressor decompressor;
    Status status = decompressor.Init();
    if(status != kOk)
        return CODING_ERROR;
    std::string output_str;
    status = decompressor.Uncompress(in, isize, &output_str);
    if(status != kOk)
        return CODING_ERROR;
    size_t compressed_size = output_str.size();
    memcpy(out, output_str.data(), compressed_size);
    return compressed_size;
}
size_t max_size(size_t input_size)
{
    return Compressor::MaxCompressedSize(input_size);
}

} // FsBenchGipfeli
