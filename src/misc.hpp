#ifndef __FSBENCH_COMMON_HPP_AJNDF87k

#define __FSBENCH_COMMON_HPP_AJNDF87k
#define UNUSED(p) (void)(p)
#define ARRAY_ELEMS(arr) (sizeof(arr) / sizeof((arr)[0]))

#define STRIGIFY(x) #x


#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#if __cplusplus >= 201103L // C++ 2011
#include <cinttypes>
#include <cstdint>
#else
extern "C"
{
#include <inttypes.h>
#include <stdint.h>
}
#endif // C++ 2011

#endif
