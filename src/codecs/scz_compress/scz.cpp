#include "scz.hpp"
#include "misc.hpp"

extern "C"
{
#include "scz.h"
}

#include <algorithm>
#include <cstdlib>
#include <memory.h>

size_t Scz::compress(char * in, size_t isize, char * out, size_t, void * mode)
{

    // scz theoretically supports buffers larger than SCZ_MAX_BUF, but as far as I can see, it's broken
    // to overcome this, I just split the data into small pieces that I feed the codec with

    size_t total_out = 0;

    intptr_t bufsize = *(intptr_t*)mode;

    while(isize > 0)
    {
        char * tmp_buf;
        int compressed_size;
        size_t chunk_size = std::min(bufsize, (intptr_t)isize);

        int ret = Scz_Compress_Buffer2Buffer(in, chunk_size, &tmp_buf, &compressed_size, 1);
        if(!ret)
            return CODING_ERROR;

        memcpy(&out[sizeof(int32_t)], tmp_buf, compressed_size);
        free(tmp_buf);

        *(int32_t*) out = compressed_size;
        out += sizeof(uint32_t);

        total_out += compressed_size + sizeof(int32_t); // I'm not sure if it's right, but in the output size I include my metadata size
        in        += chunk_size;
        isize     -= chunk_size;
        out       += compressed_size;
    }
    return total_out;
}
size_t Scz::decompress(char * in, size_t, char * out, size_t osize, void * mode)
{
    size_t osize_left = osize;

    intptr_t bufsize = *(intptr_t*)mode;

    while(osize_left > 0)
    {
        char * tmp_buf;
        int decompressed_size;

        size_t chunk_size = std::min(bufsize, (intptr_t)osize_left);
        int32_t compressed_size = *(int32_t*)in;
        in += sizeof(int32_t);

        int ret = Scz_Decompress_Buffer2Buffer(in, compressed_size, &tmp_buf, &decompressed_size);
        if(!ret)
            return CODING_ERROR;
        memcpy(out, tmp_buf, decompressed_size);
        free(tmp_buf);
        in         += compressed_size;
        out        += chunk_size;
        osize_left -= chunk_size;
    }
    return osize;
}
void Scz::cleanup()
{
    scz_cleanup();
    this->CodecWithIntModes::cleanup();
}
