#include <cstring>

#include "gipfeli.h"
#include "codecs.hpp"

namespace FsBenchGipfeli
{
using namespace util::compression;

size_t compress(char * in, size_t isize, char * out, size_t, void *)
{
    Compressor *compressor = NewGipfeliCompressor();
    UncheckedByteArraySink sink(out);
    ByteArraySource source(in, isize);
    return compressor->CompressStream(&source, &sink);
}
size_t decompress(char * in, size_t isize, char * out, size_t osize, void *)
{
    Compressor *compressor = NewGipfeliCompressor();
    UncheckedByteArraySink sink(out);
    ByteArraySource source(in, isize);
    if (!compressor->UncompressStream(&source, &sink))
        return CODING_ERROR;
    return osize;
}
size_t max_size(size_t input_size)
{
    return NewGipfeliCompressor()->MaxCompressedLength(input_size);
}

} // FsBenchGipfeli
