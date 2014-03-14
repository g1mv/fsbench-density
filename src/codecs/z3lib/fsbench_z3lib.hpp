#ifndef FSBENCH_Z3LIB_HPP_kfaj0KI6g
#define FSBENCH_Z3LIB_HPP_kfaj0KI6g

extern "C"
{
#include "z3lib.h"
#include "z3blib.h"
#include "z3dlib.h"
}

#include <algorithm>

namespace FsBenchZ3Lib
{

const size_t mem_size = std::max(Z3DD_MEMSIZE, Z3DE_MEMSIZE_MIN);

size_t z3lib_c(char * in, size_t isize, char * out, size_t osize, void * memory);
size_t z3lib_d(char * in, size_t isize, char * out, size_t osize, void * memory);

}


#endif // FSBENCH_Z3LIB_HPP_kfaj0KI6g