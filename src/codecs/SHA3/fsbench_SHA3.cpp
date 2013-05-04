
// broken definitions in SHA3 API...
#include "stdint.h"
typedef uint32_t u_int32_t;
typedef uint64_t u_int64_t;
#include <climits>
#include <cstdlib>
#include <cstring> 

#include "fsbench_SHA3.hpp"
#define SHA3_FUNCTION(digest_size)                                                             \
void fsbench_hash_##digest_size(char* in, size_t isize, char* out)                             \
{                                                                                              \
    /* FIXME: Ignores return code */                                                           \
    Hash(digest_size, (const BitSequence*)in, (DataLength)isize*CHAR_BIT, (BitSequence*)out);\
}


namespace FsBenchSHA3
{
#ifdef FSBENCH_USE_SHA3_RND3
namespace Blake
{
#   include "Blake/blake_opt64.c"
    SHA3_FUNCTION(224)
    SHA3_FUNCTION(256)
    SHA3_FUNCTION(384)
    SHA3_FUNCTION(512)
}
#endif
#ifdef FSBENCH_USE_SHA3_RND2
namespace BlueMidnightWish
{
#   include "BlueMidnightWish/BlueMidnightWish_opt.c"
    SHA3_FUNCTION(384)
    SHA3_FUNCTION(512)
}
namespace BlueMidnightWish2
{
#   include "BlueMidnightWish/BlueMidnightWish_opt_002.c"
    SHA3_FUNCTION(224)
    SHA3_FUNCTION(256)
}
#endif
#ifdef FSBENCH_USE_SHA3_RND2
namespace CubeHash
{
#   include "CubeHash/cubehash.c"
    SHA3_FUNCTION(224)
    SHA3_FUNCTION(256)
    SHA3_FUNCTION(384)
    SHA3_FUNCTION(512)
}
#endif
#ifdef FSBENCH_USE_SHA3_RND1
namespace Edon_R
{
#   include "Edon-R/Edon-R_opt.c"
    SHA3_FUNCTION(224)
    SHA3_FUNCTION(256)
    SHA3_FUNCTION(384)
    SHA3_FUNCTION(512)
}
#endif
#ifdef FSBENCH_USE_SHA3_RND3_GROESTL
namespace Groestl
{
#   include "Groestl/Groestl-opt.c"
    SHA3_FUNCTION(224)
    SHA3_FUNCTION(256)
    SHA3_FUNCTION(384)
    SHA3_FUNCTION(512)
}
#endif
#ifdef FSBENCH_USE_SHA3_RND3
namespace JH
{
#   include "JH/jh_ansi_opt64.h"
    SHA3_FUNCTION(224)
    SHA3_FUNCTION(256)
    SHA3_FUNCTION(384)
    SHA3_FUNCTION(512)
}
#endif
#ifdef FSBENCH_USE_SHA3_RND3
namespace Keccak
{
#   include "Keccak/KeccakDuplex.c"
#   include "Keccak/KeccakF-1600-opt64.c"
#   include "Keccak/KeccakNISTInterface.c"
#   include "Keccak/KeccakSponge.c"
    SHA3_FUNCTION(224)
    SHA3_FUNCTION(256)
    SHA3_FUNCTION(384)
    SHA3_FUNCTION(512)
}
#endif
#ifdef FSBENCH_USE_SHA3_RND3
namespace Skein
{
#   include "Skein/skein.c"
#   include "Skein/skein_block.c"
#   include "Skein/SHA3api_ref.c"
    SHA3_FUNCTION(224)
    SHA3_FUNCTION(256)
    SHA3_FUNCTION(384)
    SHA3_FUNCTION(512)
    SHA3_FUNCTION(1024)
}
#endif
#ifdef FSBENCH_USE_SHA3_RND1
namespace SWIFFTX
{
#   include "SWIFFTX/SWIFFTX.c"
#   include "SWIFFTX/SHA3.c"
    SHA3_FUNCTION(224)
    SHA3_FUNCTION(256)
    SHA3_FUNCTION(384)
    SHA3_FUNCTION(512)
}
#endif
} // FsBenchSHA3
