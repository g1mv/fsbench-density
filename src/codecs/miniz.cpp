#include <iostream>

#include "miniz.hpp"
#include "codecs.hpp"

using namespace std;

extern "C"
{
    #define MINIZ_NO_ARCHIVE_APIS
    #define MINIZ_NO_STDIO
    #define MINIZ_NO_TIME

    #include "miniz.c"
}

size_t miniz_c(char* in, size_t isize, char* out, size_t osize, void* mode)
{
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree  = Z_NULL;
    stream.opaque = NULL;
    
    int ret = deflateInit2(&stream, *(intptr_t*)mode, Z_DEFLATED, -15, 9, Z_DEFAULT_STRATEGY);
    if(ret != Z_OK)
        return CODING_ERROR;
    stream.next_in   = (Bytef*)in;
    stream.avail_in  = isize;
    stream.next_out  = (Bytef*)out;
    stream.avail_out = osize;
    ret = deflate(&stream, Z_FINISH);
    int ret2 = deflateEnd(&stream);
    if(ret != Z_STREAM_END || ret2 != Z_OK)
        return CODING_ERROR;
    return stream.total_out;
}
size_t miniz_d(char* in, size_t isize, char* out, size_t osize, void* _)
{
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree  = Z_NULL;
    stream.opaque = NULL;
    
    int ret = inflateInit2(&stream, -15);
    if(ret != Z_OK)
        return CODING_ERROR;
    stream.next_in   = (Bytef*)in;
    stream.avail_in  = isize;
    stream.next_out  = (Bytef*)out;
    stream.avail_out = osize;
    ret = inflate(&stream, Z_FINISH);
    int ret2 = inflateEnd(&stream);
    if(ret != Z_STREAM_END || ret2 != Z_OK)
        return CODING_ERROR;
    return stream.total_out;
}