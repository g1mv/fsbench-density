#include "fsbench_zling.hpp"
#include "libzling.h"
#include <codecs.hpp>

using namespace baidu::zling;

namespace FsBenchZling
{
    
#include <cstdio>

size_t zling_c(char * in, size_t isize, char * out, size_t osize, void * mode)
{
    RAMInputter  inputter ((unsigned char*)in,  isize);
    RAMOutputter outputter((unsigned char*)out, osize);
    int ret = Encode(&inputter, &outputter, NULL, *(intptr_t*)mode);
    return ret == 0 ? outputter.GetOutputSize() : CODING_ERROR;
}
size_t zling_d(char * in, size_t isize, char * out, size_t osize, void * _)
{
    RAMInputter  inputter ((unsigned char*)in,  isize);
    RAMOutputter outputter((unsigned char*)out, osize);
    int ret = Decode(&inputter, &outputter);
    return ret == 0 ? outputter.GetOutputSize() : CODING_ERROR;
}

}
