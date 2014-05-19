
#include <stdint.h>

typedef int64_t nssize_t;
#define MAX_NSIZE   2147483647L

////////////
// ERRROS //
////////////

#define N_STREAM_ERROR	   ((nssize_t)(-1ULL))
#define N_BUFFER_TOO_SMALL ((nssize_t)(-2ULL))
#define N_OUT_OF_MEMORY    ((nssize_t)(-3ULL))
#define N_INVALID_ARGUMENT ((nssize_t)(-4ULL))
#define N_GENERIC_ERROR	   ((nssize_t)(-5ULL))

#define N_ERROR(code)	   ((code) < 0)

nssize_t DecompressM(const void * _in, nssize_t in_size, void * _out, nssize_t out_size);
nssize_t CompressM  (const void * _in, nssize_t in_size, void * _out, nssize_t out_size);