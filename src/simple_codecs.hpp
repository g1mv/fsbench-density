/**
 * Declarations of Codecs that are fairly simple to handle
 *
 * Written by m^2.
 * You can consider the code to be public domain.
 * If your country doesn't recognize author's right to relieve themselves of copyright,
 * you can use it under the terms of WTFPL version 2.0 or later.
 */

#include "abstractCodecs.hpp"
#include <map>

using namespace std;

//////////////////////////////////////////////////////
// Codecs that use some of the abstract classes
// Here they have functions used by their constructors
//////////////////////////////////////////////////////

#ifdef FSBENCH_USE_AR
namespace FsBenchAr
{
    size_t ar_c(char * in, size_t isize, char * out, size_t osize, void * _);
    size_t ar_d(char * in, size_t isize, char * out, size_t osize, void * _);
}
#endif//FSBENCH_USE_AR
#ifdef FSBENCH_USE_BLAKE2
#define BLAKE2_FUNC(name) \
void fsbench_ ## name (char * in, size_t isize, char * out)
BLAKE2_FUNC(blake2s);
BLAKE2_FUNC(blake2b);
BLAKE2_FUNC(blake2sp);
BLAKE2_FUNC(blake2bp);
#endif//FSBENCH_USE_BLAKE2
#ifdef FSBENCH_USE_BLOSC
size_t blosc_c(char * in, size_t isize, char * out, size_t osize,void * mode);
size_t blosc_d(char * in, size_t isize, char * out, size_t osize,void * _);
#endif//FSBENCH_USE_BLOSC
#ifdef FSBENCH_USE_BZ2
size_t bz2_c(char * in, size_t isize, char * out, size_t osize, void * mode);
size_t bz2_d(char * in, size_t isize, char * out, size_t osize, void * _);
#endif//FSBENCH_USE_BZ2
#ifdef FSBENCH_USE_CITYHASH
void CityHash32(char * in, size_t isize, char * out);
void CityHash64(char * in, size_t isize, char * out);
void CityHash128(char * in, size_t isize, char * out);
#endif//FSBENCH_USE_CITYHASH
#ifdef FSBENCH_USE_CRAPWOW
void CrapWow(char * in, size_t isize, char * out);
#endif//FSBENCH_USE_CRAPWOW
#ifdef FSBENCH_USE_DOBOZ
size_t Doboz_c(char * in, size_t isize, char * out, size_t osize, void * _);
size_t Doboz_d(char * in, size_t isize, char * out, size_t osize, void * _);
#endif//FSBENCH_USE_DOBOZ
#ifdef FSBENCH_USE_FASTCRYPTO
namespace FsBenchFastCrypto
{
    void uhash(char * in, size_t isize, char * out);
    void vhash(char * in, size_t isize, char * out);
    void umac(char * in, size_t isize, char * out);
    void vmac(char * in, size_t isize, char * out);
} //namespace FsBenchFastCrypto
#endif//FSBENCH_USE_FASTCRYPTO
#ifdef FSBENCH_USE_FASTLZ
size_t fastlz1_c(char * in, size_t isize, char * out, size_t osize, void * _);
size_t fastlz2_c(char * in, size_t isize, char * out, size_t osize, void * _);
size_t fastlz_d (char * in, size_t isize, char * out, size_t osize, void * _);
size_t fastlz_m (size_t input_size);
#endif//FSBENCH_USE_FASTLZ
#ifdef FSBENCH_USE_HALIBUT
size_t halibut_c(char * in, size_t isize, char * out, size_t osize, void * _);
size_t halibut_d(char * in, size_t isize, char * out, size_t osize, void * _);
#endif//FSBENCH_USE_HALIBUT
#ifdef FSBENCH_USE_LODEPNG
size_t lodepng_c(char * in, size_t isize, char * out, size_t osize, void * mode);
size_t lodepng_d(char * in, size_t isize, char * out, size_t osize, void * _);
#endif//FSBENCH_USE_LODEPNG
#ifdef FSBENCH_USE_LZ4
size_t LZ4_c(char * in, size_t isize, char * out, size_t osize, void * _);
size_t LZ4hc_c(char * in, size_t isize, char * out, size_t osize, void * _);
size_t LZ4_d_fast(char * in, size_t isize, char * out, size_t osize, void * _);
size_t LZ4_d_safe(char * in, size_t isize, char * out, size_t osize, void * _);
#endif//FSBENCH_USE_LZ4
#ifdef FSBENCH_USE_LZF
size_t LZF_ultra_c(char * in,size_t isize, char * out, size_t osize, void * _);
size_t LZF_very_c(char * in,size_t isize, char * out, size_t osize, void * _);
size_t LZF_c(char * in,size_t isize, char * out, size_t osize, void * _);
size_t LZF_d(char * in,size_t isize, char * out, size_t osize, void * _);
#endif//FSBENCH_USE_LZF
#ifdef FSBENCH_USE_LZFX
size_t LZFX_c(char * in,size_t isize, char * out, size_t osize, void * _);
size_t LZFX_d(char * in,size_t isize, char * out, size_t osize, void * _);
#endif//FSBENCH_USE_LZFX
#ifdef FSBENCH_USE_LZG
size_t LZG_c(char * in, size_t isize, char * out, size_t osize, void * mode);
size_t LZG_d(char * in,size_t isize, char * out, size_t osize, void * _);
size_t LZG_m(size_t input_size);
#endif//FSBENCH_USE_LZG
#ifdef FSBENCH_USE_LZMAT
size_t lzmat_c(char * in,size_t isize, char * out, size_t osize, void * _);
size_t lzmat_d(char * in,size_t isize, char * out, size_t osize, void * _);
#endif//FSBENCH_USE_LZMAT
#ifdef FSBENCH_USE_LZWC
size_t LZWC_c(char * in,size_t isize, char * out, size_t osize, void * _);
size_t LZWC_d(char * in,size_t isize, char * out, size_t osize, void * _);
#endif//FSBENCH_USE_LZWC
#ifdef FSBENCH_USE_LZX_COMPRESS
size_t LZX_c(char * in, size_t isize, char * out, size_t osize, void* window_size);
#endif// FSBENCH_USE_LZX_COMPRES
#ifdef FSBENCH_USE_MMINI
extern "C"
{
#include "mmini.h" // for MMINI_HUFFHEAP_SIZE
}
size_t mmini_c(char * in, size_t isize, char * out, size_t osize, void * buffer);
size_t mmini_d(char * in, size_t isize, char * out, size_t osize, void * buffer);
size_t mmini_huffman_c(char * in, size_t isize, char * out, size_t osize, void * buffer);
size_t mmini_huffman_d(char * in, size_t isize, char * out, size_t osize, void * buffer);
size_t mmini_lzl_c(char * in, size_t isize, char * out, size_t osize, void * _);
size_t mmini_lzl_d(char * in, size_t isize, char * out, size_t osize, void * _);
#endif// FSBENCH_USE_MMINI
#ifdef FSBENCH_USE_MURMUR
void murmur_x86_32(char * in, size_t isize, char * out);
void murmur_x86_128(char * in, size_t isize, char * out);
void murmur_x64_128(char * in, size_t isize, char * out);
#endif//FSBENCH_USE_MURMUR
#ifdef FSBENCH_USE_QUICKLZZIP
size_t qlzzip_c(char * in, size_t isize, char * out, size_t osize, void * _);
#endif//FSBENCH_USE_QUICKLZZIP
#ifdef FSBENCH_USE_RLE64
size_t RLE64_c(char * in, size_t isize, char * out, size_t osize, void * _);
size_t RLE64_d(char * in, size_t isize, char * out, size_t osize, void * _);
size_t RLE32_c(char * in, size_t isize, char * out, size_t osize, void * _);
size_t RLE32_d(char * in, size_t isize, char * out, size_t osize, void * _);
size_t RLE16_c(char * in, size_t isize, char * out, size_t osize, void * _);
size_t RLE16_d(char * in, size_t isize, char * out, size_t osize, void * _);
size_t RLE8_c(char * in, size_t isize, char * out, size_t osize, void * _);
size_t RLE8_d(char * in, size_t isize, char * out, size_t osize, void * _);
#endif// FSBENCH_USE_RLE64
#ifdef FSBENCH_USE_SANMAYCE_FNV
void fnv1_jesteress(char * in, size_t isize, char * out);
void fnv1_mantis(char * in, size_t isize, char * out);
void fnv1_meiyan(char * in, size_t isize, char * out);
void fnv1_tesla(char * in, size_t isize, char * out);
void fnv1_tesla3(char * in, size_t isize, char * out);
void fnv1_yorikke(char * in, size_t isize, char * out);
void fnv1_yoshimitsu_triad(char * in, size_t isize, char * out);
void fnv1_yoshimura(char * in, size_t isize, char * out);
void fnv1_yoshimitsu_triad_iixmm(char * in, size_t isize, char * out);
void fnv1_penumbra(char * in, size_t isize, char * out);
#endif//FSBENCH_USE_SANMAYCE_FNV
#ifdef FSBENCH_USE_SHRINKER
size_t Shrinker_c(char * in, size_t isize, char * out, size_t osize, void * _);
size_t Shrinker_d(char * in, size_t isize, char * out, size_t osize, void * _);
size_t Shrinker_m(size_t input_size);
#endif// FSBENCH_USE_SHRINKER
#ifdef FSBENCH_USE_SIPHASH
void siphash(char * in, size_t isize, char * out);
#endif// FSBENCH_USE_SIPHASH
#ifdef FSBENCH_USE_SNAPPY
size_t snappy_c(char * in,size_t isize, char * out, size_t osize, void * _);
size_t snappy_d(char * in,size_t isize, char * out, size_t osize, void * _);
size_t snappy_m(size_t input_size);
#endif// FSBENCH_USE_SNAPPY
#ifdef FSBENCH_USE_SPOOKY
void spooky(char * in, size_t isize, char * out);
#endif//FSBENCH_USE_SPOOKY
#ifdef FSBENCH_USE_XXHASH
void xxhash(char * in, size_t isize, char * out);
void xxhash_256(char * in, size_t isize, char * out);
#endif//FSBENCH_USE_XXHASH
#ifdef FSBENCH_USE_ZFS
size_t LZJB_c(char * in,size_t isize, char * out, size_t osize, void * _);
size_t LZJB_d(char * in,size_t isize, char * out, size_t osize, void * _);
void fletcher2(char * in, size_t isize, char * out);
void fletcher4(char * in, size_t isize, char * out);
#endif//FSBENCH_USE_ZFS
#ifdef FSBENCH_USE_ZLIB
size_t zlib_c(char * in, size_t isize, char * out, size_t osize, void * mode);
size_t zlib_d(char * in, size_t isize, char * out, size_t osize, void * _);
#endif //FSBENCH_USE_ZLIB
#ifdef FSBENCH_USE_ZOPFLI
size_t zopfli_c(char * in, size_t isize, char * out, size_t osize, void * mode);
#endif //FSBENCH_USE_ZOPFLI

// a pseudocodec that does nothing.
size_t nop_c(char * in, size_t isize, char * out, size_t osize, void * _);
size_t nop_d(char * in, size_t isize, char * out, size_t osize, void * _);

#ifdef FSBENCH_USE_BLZ

struct BriefLZ : Codec
{
private:

    static size_t compressor(char * in, size_t isize, char * out, size_t osize, void * work);
    static size_t decompressor(char * in, size_t isize, char * out, size_t osize, void * work);
    char * workmem;

    static size_t max_compressed_size(size_t input_size);

public:

    BriefLZ() :
    Codec("BriefLZ", _BLZ_VERSION, compressor, decompressor, max_compressed_size),
    workmem(0),
    params(0)
    {}

    virtual void init(const string & args, unsigned threads_no, size_t isize, bool init_compressor, bool init_decompressor);
    virtual void cleanup();
    string help() const;
    void ** eparams();

protected:
    void ** params;
};

#endif// FSBENCH_USE_BLZ
#ifdef FSBENCH_USE_LZHAM

#define LZHAM_NO_ZLIB_COMPATIBLE_NAMES
#include "lzham/include/lzham.h"

struct LZHAM : Codec
{
private:

    struct DecompressionState
    {
        lzham_decompress_state_ptr decompression_state;
        lzham_decompress_params * decompression_params;
    };

    static const string default_mode;
    unsigned threads_no;

    const string & default_args(const string & args);
    static size_t compressor(char * in, size_t _isize, char * out, size_t _osize, void * _data);
    static size_t decompressor(char * in, size_t _isize, char * out, size_t _osize, void * _data);
public:

    LZHAM() :
    Codec("LZHAM", _LZHAM_VERSION, compressor, decompressor, no_blowup),
    compressor_params(0),
    decompressor_params(0)
    {}
    virtual void init(const string & args, unsigned threads_no, size_t isize, bool init_compressor, bool init_decompressor);
    virtual void cleanup();

    virtual string help() const;

    void ** eparams();
    void ** dparams();

protected:
    void ** compressor_params;
    void ** decompressor_params;
};

#endif

#ifdef FSBENCH_USE_LZO
extern "C"
{
#include "lzo/lzo1.h"
#include "lzo/lzo1a.h"
#include "lzo/lzo1b.h"
#include "lzo/lzo1c.h"
#include "lzo/lzo1f.h"
#include "lzo/lzo1x.h"
#include "lzo/lzo1y.h"
#include "lzo/lzo1z.h"
#include "lzo/lzo2a.h"
}

struct LZO : Codec
{
private:

    typedef int
    (*lzo_compress)( const lzo_bytep src, lzo_uint src_len,
            lzo_bytep dst, lzo_uintp dst_len,
            lzo_voidp wrkmem );

    typedef int
    (*lzo_decompress) ( const lzo_bytep src, lzo_uint src_len,
            lzo_bytep dst, lzo_uintp dst_len,
            lzo_voidp wrkmem /* NOT USED */);
    struct LzoType
    {
        const lzo_compress compressor;
        const lzo_decompress decompressor;
        LzoType(lzo_compress compressor, lzo_decompress decompressor) :
        compressor(compressor),
        decompressor(decompressor)
        {}
    };
    static map<const string, LzoType*> LZO_TYPES;
    static const map<const string, LzoType*> & lzo_types();

    static const LzoType * get_type(const string & args);
    static size_t compressor(char * in, size_t isize, char * out, size_t osize, void * work);
    static size_t decompressor(char * in, size_t isize, char * out, size_t osize, void * work);
    static string default_args(const string & args);

    char ** work_ptrs;
    char  * work;

    static lzo_compress static_lzo_compressor; // set by init() //FIXME: send it as a parameter instead of using statics
    static lzo_decompress static_lzo_decompressor;// set by init()

public:
    LZO() :
    Codec("LZO", _LZO_VERSION, compressor, decompressor, no_blowup),
    work_ptrs(0),
    work(0)
    {}
    virtual void init(const string & args, unsigned threads_no, size_t isize, bool init_compressor, bool init_decompressor);
    virtual void cleanup();
    string help() const;
    void ** eparams();
    void ** dparams();

};
#endif// FSBENCH_USE_LZO
#ifdef FSBENCH_USE_LZSS_IM

#include "lzss.hpp"

struct LZSS_IM : Codec
{
    struct Params
    {
        LZSSIM coder;
        int mode;
    };
    static size_t compressor(char * in, size_t isize, char * out, size_t osize, void* p);
    static size_t decompressor(char * in, size_t isize, char * out, size_t osize, void * _);
    unsigned threads_no;
public:

    LZSS_IM() :
    Codec("LZSS-IM", _LZSSIM_VERSION, compressor, decompressor),
    params(0)
    {}
    virtual void init(const string & args, unsigned threads_no, size_t isize, bool init_compressor, bool init_decompressor);
    virtual void cleanup();
    void ** eparams();
    void ** dparams();

protected:
    void ** params;
};
#endif //FSBENCH_USE_LZSS_IM
#ifdef FSBENCH_USE_LZV1
extern "C"
{
#include "lzv1.h" // for ush type
}
size_t LZV1_c(char * in, size_t isize, char * out, size_t osize, void * memory);
size_t LZV1_d(char * in, size_t isize, char * out, size_t osize, void * memory);
#endif //FSBENCH_USE_LZV1
#ifdef FSBENCH_USE_NRV
extern "C"
{
#include "ucl/ucl.h"
}
struct Nrv : Codec
{
private:
#define NUMBER_OF_NRV_TYPES 3

    typedef int (*nrv_compress) ( const ucl_bytep src, ucl_uint src_len,
            ucl_bytep dst, ucl_uintp dst_len,
            ucl_progress_callback_p cb,
            int level,
            const struct ucl_compress_config_p conf,
            ucl_uintp result );
    typedef int (*nrv_decompress) ( const ucl_bytep src, ucl_uint src_len,
            ucl_bytep dst, ucl_uintp dst_len,
            ucl_voidp wrkmem );
    struct NrvType
    {
        const string name;
        nrv_compress compressor;
        nrv_decompress decompressor;
        NrvType(const string & name, nrv_compress compressor, nrv_decompress decompressor) :
        name(name),
        compressor(compressor),
        decompressor(decompressor)
        {}
    };
    static const NrvType nrv_types[];
    static const NrvType * current_type; //set by init()  //FIXME: send it as a parameter instead of using statics

    static const string default_mode;
    const string & default_args(const string & args);

    static size_t compressor(char * in, size_t isize, char * out, size_t osize, void * mode);
    static size_t decompressor(char * in, size_t isize, char * out, size_t osize, void * _);
    static const string full_name(const string & type);
    const NrvType * get_type(const string & type_name);

public:

    Nrv(const string & type) :
    Codec(full_name(type), _NRV_VERSION, compressor, decompressor, no_blowup),
    params(0)
    {}
    virtual void init(const string & args, unsigned threads_no, size_t isize, bool init_compressor, bool init_decompressor);
    virtual void cleanup();

    virtual string help() const;

    void ** eparams();

protected:
    void ** params;
};
#endif// FSBENCH_USE_NRV
#ifdef FSBENCH_USE_QUICKLZ

extern "C"
{
#include "quicklz/quicklz.h"
}

struct QuickLZ : Codec
{
private:

    typedef size_t (*qlz_compressor) (const void * source, char * destination, size_t size, qlz_state_compress * state);
    typedef size_t (*qlz_decompressor)(const char * source, void * destination, qlz_state_decompress * state);

    struct QuickLZType
    {
        const qlz_compressor compressor;
        const qlz_decompressor decompressor;
        QuickLZType(qlz_compressor compressor, qlz_decompressor decompressor) :
        compressor(compressor),
        decompressor(decompressor)
        {}
    };
    static map<const string, QuickLZType*> QLZ_TYPES;
    static const map<const string, QuickLZType*> & qlz_types();

    static const QuickLZType * get_type(const string & args);
    static size_t compressor(char * in, size_t isize, char * out, size_t osize, void * work);
    static size_t decompressor(char * in, size_t isize, char * out, size_t osize, void * work);
    static string default_args(const string & args);

    static qlz_compressor static_qlz_compressor; // set by init()  //FIXME: send it as a parameter instead of using statics
    static qlz_decompressor static_qlz_decompressor;// set by init()

public:
    qlz_state_compress   ** qlz_compression_states;
    qlz_state_decompress ** qlz_decompression_states;
    unsigned threads_no;

    QuickLZ() :
    Codec("QuickLZ", _QLZ_VERSION, compressor, decompressor),
    qlz_compression_states(0),
    qlz_decompression_states(0)
    {}

    virtual void init(const string & args, unsigned threads_no, size_t isize, bool init_compressor, bool init_decompressor);
    virtual void cleanup();
    string help() const;
    void ** eparams();
    void ** dparams();
};
#endif //FSBENCH_USE_QUICKLZ
#ifdef FSBENCH_USE_TOR

#include "tornado/tor_test.h"

struct Tor : Codec
{
private:

    static const string default_mode;
    const string & default_args(const string & args);
    static size_t compressor(char * in, size_t isize, char * out, size_t osize, void * mode);
    static size_t decompressor(char * in, size_t isize, char * out, size_t osize, void * _);

public:

    Tor() :
    Codec("tornado", _TOR_VERSION, compressor, decompressor),
    params(0)
    {}
    virtual void init(const string & args, unsigned threads_no, size_t isize, bool init_compressor, bool init_decompressor);
    virtual void cleanup();

    virtual string help() const;

    void ** eparams();

protected:
    void ** params;
};

#endif// FSBENCH_USE_TOR
#ifdef FSBENCH_USE_YAPPY
struct Yappy : CodecWithIntModes
{
private:
    static size_t compressor(char * in, size_t isize, char * out, size_t osize, void * mode);
    static size_t decompressor(char * in, size_t isize, char * out, size_t osize, void * _);
public:
    Yappy();
};
#endif// FSBENCH_USE_YAPPY

size_t c_bswap16(char * in, size_t isize, char * out, size_t osize, void *);
size_t c_bswap32(char * in, size_t isize, char * out, size_t osize, void *);
size_t c_bswap64(char * in, size_t isize, char * out, size_t osize, void *);
size_t c_memcpy (char * in, size_t isize, char *out, size_t, void *);
size_t c_memmove(char * in, size_t isize, char *out, size_t, void *);
