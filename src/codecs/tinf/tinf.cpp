
#include "tinf.hpp"

extern "C"
{
#include "tinf.h"
}

namespace FsBenchTinf
{

size_t inflate(char*in, size_t isize, char*out, size_t osize, void*_)
{
    tinf_init();//FIXME: Do init() only once
    unsigned int _osize = (unsigned int)osize;
    int ret = tinf_uncompress(out, &_osize, in, isize);
    if(ret != TINF_OK)
        return CODING_ERROR;
    return _osize;
}

} // FsBenchTinf