
#include "fsbench_z3lib.hpp"

#include "codecs.hpp"

#include <algorithm>
#include <cassert>

namespace FsBenchZ3Lib
{
size_t z3lib_c(char * in, size_t isize, char * out, size_t osize, void * memory)
{
    char * out0 = out;
    z3de_handle *handle = z3d_encode_init(*(void**)memory, mem_size, 0, 0, 0, 0, 0, 0);
    while(handle != NULL)
    {
        __u32 taken = 0;
        __u32 given = 0;
        handle = z3d_encode(handle, (__u8*)in, isize, &taken, (__u8*)out, osize, &given);
        in += taken;
        isize -= taken;
        out += given;
        osize -= given;
    }
    return out - out0;
}
size_t z3lib_d(char * in, size_t isize, char * out, size_t osize, void * memory)
{
    char * out0 = out;
    z3dd_handle *handle = z3d_decode_init(0, 0, *(void**)memory, mem_size);
    __u32 pending_bits = 0;
    int error;
    while(handle != NULL && osize > 0)
    {
        __u32 taken = 0;
        __u32 given = 0;
        int pending_nb = 0;
        __u8 * tmp_out;
        handle = z3d_decode(handle, &error, &pending_bits, &pending_nb, (__u8*)in, isize, &taken, &tmp_out, &given);
        if(given > osize)
            return CODING_ERROR;
        memcpy(out, tmp_out, given);
        in += taken;
        isize -= taken;
        out += given;
        osize -= given;
    }
    assert(pending_bits == 0);
    return out - out0; // I should check that error==z3er_none. However doesn't appear to be set correctly.
}

}
