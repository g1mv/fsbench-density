/**
 * Implementations of Codecs that are fairly simple to handle
 *
 * Written by m^2.
 * You can consider the code to be public domain.
 * If your country doesn't recognize author's right to relieve themselves of copyright,
 * you can use it under the terms of WTFPL version 2.0 or later.
 */
#include "misc.hpp"
#include "simple_codecs.hpp"
#include "tools.hpp"

#include <algorithm>
#include <climits>
#include <cstring>
#include <iostream>
#include <memory.h> // TODO: Is it needed? If yes, is it C or C++?
#include <stdexcept>

using namespace std;

//////////////////////////////////////////////////////
// Codecs that use some of the abstract classes
// Here they have functions used by their constructors
//////////////////////////////////////////////////////

#ifdef FSBENCH_USE_AR
namespace FsBenchAr
{
    extern "C"
    {
#include "ar/lzh.h"
    }
    struct meminfo
    {
        const char * in_data;
        int in_bytes_left;
        char * out_data;
        int out_bytes_left;
    };
    int read(void * data, int n, void * p)
    {
        meminfo * mem = (meminfo*)p;
        int to_copy = std::min(n, mem->in_bytes_left);
        memcpy(data, mem->in_data, to_copy);
        mem->in_data += to_copy;
        mem->in_bytes_left -= to_copy;
        return to_copy;
    }
    int write(void * data, int n, void * p)
    {
        meminfo * mem = (meminfo*)p;
        int to_copy = std::min(n, mem->out_bytes_left);
        memcpy(mem->out_data, data, to_copy);
        mem->out_data += to_copy;
        mem->out_bytes_left -= to_copy;
        return to_copy;
    }
    // type is different from stdlib's one... unsigned vs. size_t
    void * _malloc(unsigned n)
    {
        return malloc(n);
    }

    size_t ar_c(char * in, size_t isize, char * out, size_t osize, void *)
    {
        meminfo mem;
        mem.in_data        = in;
        mem.in_bytes_left  = isize;
        mem.out_data       = out;
        mem.out_bytes_left = osize;
        int ret = lzh_freeze(read,
                             write,
                             _malloc, 
                             free,
                             &mem);
        return ret == 0 ? osize - mem.out_bytes_left : CODING_ERROR;
    }
    size_t ar_d(char * in, size_t isize, char * out, size_t osize, void *)
    {
        meminfo mem;
        mem.in_data        = in;
        mem.in_bytes_left  = isize;
        mem.out_data       = out;
        mem.out_bytes_left = osize;
        int ret = lzh_melt(read,
                           write,
                           _malloc, 
                           free,
                           osize,
                           &mem);
        return ret == 0 ? osize - mem.out_bytes_left : CODING_ERROR;
    }
}

#endif//FSBENCH_USE_AR
#ifdef FSBENCH_USE_BLAKE2
extern "C"
{
#include "blake2/blake2.h"
}
#define BLAKE2_FUNC_DEFINITION(name, size)                           \
        void fsbench_ ## name (char * in, size_t isize, char * out)    \
        {                                                            \
            name((uint8_t*)out, (const void*)in, 0, size, isize, 0); \
        }
BLAKE2_FUNC_DEFINITION(blake2s, 32)
BLAKE2_FUNC_DEFINITION(blake2b, 64)
BLAKE2_FUNC_DEFINITION(blake2sp, 32)
BLAKE2_FUNC_DEFINITION(blake2bp, 64)
#endif//FSBENCH_USE_BLAKE2
#ifdef FSBENCH_USE_BLOSC
extern "C"
{
#include "blosc/blosclz.h"
}

size_t blosc_c(char * in, size_t isize, char * out, size_t osize, void * mode)
{
    int ret = blosclz_compress(*(intptr_t*)mode, in, isize, out, osize);
    return ret > 0 ? ret : CODING_ERROR;
}
size_t blosc_d(char * in, size_t isize, char * out, size_t osize, void *)
{
    return blosclz_decompress(in, isize, out, osize);
}
#endif//FSBENCH_USE_BLOSC
#ifdef FSBENCH_USE_BROTLI

#include "brotli/enc/encode.h"
#include "brotli/dec/decode.h"

size_t brotli_c(char * in, size_t isize, char * out, size_t osize, void *)
{
    brotli::BrotliParams p;
    size_t actual_osize = osize;
    return brotli::BrotliCompressBuffer(p, isize, (const uint8_t*)in, &actual_osize, (uint8_t*)out) == 0 ? CODING_ERROR : actual_osize;
}
size_t brotli_d(char * in, size_t isize, char * out, size_t osize, void *)
{
    size_t actual_osize = osize;
    return BrotliDecompressBuffer(isize, (const uint8_t*)in, &actual_osize, (uint8_t*)out) == 0 ? CODING_ERROR : actual_osize;
}
#endif//FSBENCH_USE_BROTLI
#ifdef FSBENCH_USE_BZ2
extern "C"
{
#include "bzlib.h"
}

size_t bz2_c(char * in, size_t isize, char * out, size_t osize, void * mode)
{
    unsigned int _osize = osize;
    int ret = BZ2_bzBuffToBuffCompress(out, &_osize, in, isize, *(intptr_t*)mode, 0, 0);
    return ret == BZ_OK ? _osize : CODING_ERROR;
}
size_t bz2_d(char * in, size_t isize, char * out, size_t osize, void *)
{
    unsigned int _osize = osize;
    int ret = BZ2_bzBuffToBuffDecompress(out, &_osize, in, isize, 0, 0);
    return ret == BZ_OK ? _osize : CODING_ERROR;
}
#endif//FSBENCH_USE_BZ2
#ifdef FSBENCH_USE_CITYHASH
#include "CityHash/city.h"

void CityHash32(char * in, size_t isize, char * out)
{
    *(uint64_t*) out = CityHash32((const char*)in, isize);
}
void CityHash64(char * in, size_t isize, char * out)
{
    *(uint64_t*) out = CityHash64((const char*)in, isize);
}
void CityHash128(char * in, size_t isize, char * out)
{
    uint128 ret = CityHash128((const char*)in, isize);
    ((uint64_t*)out)[0] = Uint128Low64(ret);
    ((uint64_t*)out)[1] = Uint128High64(ret);
}
#endif//FSBENCH_USE_CITYHASH
#ifdef FSBENCH_USE_CRAPWOW
extern "C"
{
#include "CrapWow.h"
}
void CrapWow(char * in, size_t isize, char * out)
{
    *(uint32_t*) out = CrapWow((const uint8_t*)in, (uint32_t)isize, (uint32_t)0);
}
#endif//FSBENCH_USE_CRAPWOW
#ifdef FSBENCH_USE_CRUSH
#include "crush.hpp"
size_t crush_c(char * in, size_t isize, char * out, size_t, void * level)
{
    return crush::compress(*(intptr_t*)level, (uint8_t*)in, isize, (uint8_t*)out);
}
size_t crush_d(char * in, size_t, char * out, size_t osize, void *)
{
    return crush::decompress((uint8_t*)in, (uint8_t*)out, osize);
}
#endif//FSBENCH_USE_CRUSH
#ifdef FSBENCH_USE_DOBOZ
#include "Doboz/Compressor.h"
#include "Doboz/Decompressor.h"
size_t Doboz_c(char * in, size_t isize, char * out, size_t, void *)
{
    doboz::Compressor c;
    size_t ret;
    return c.compress(in,isize,out,c.getMaxCompressedSize(isize),ret) == doboz::RESULT_OK ? ret : CODING_ERROR;
}
size_t Doboz_d(char * in, size_t isize, char * out, size_t osize, void *)
{
    doboz::Decompressor d;
    return d.decompress(in,isize,out,osize) == doboz::RESULT_OK ? osize : CODING_ERROR;
}
#endif//FSBENCH_USE_DOBOZ
#ifdef FSBENCH_USE_FARMHASH
#define NAMESPACE_FOR_HASH_FUNCTIONS farmhash
#include "farmhash/farmhash.h"
#undef NAMESPACE_FOR_HASH_FUNCTIONS

void FarmHash32(char * in, size_t isize, char * out)
{
    *(uint32_t*) out = farmhash::Hash32((const char*)in, isize);
}
void FarmHash64(char * in, size_t isize, char * out)
{
    *(uint64_t*) out = farmhash::Hash64((const char*)in, isize);
}
void FarmHash128(char * in, size_t isize, char * out)
{
    farmhash::uint128_t ret = farmhash::Hash128((const char*)in, isize);
    ((uint64_t*)out)[0] = farmhash::Uint128Low64(ret);
    ((uint64_t*)out)[1] = farmhash::Uint128High64(ret);
}
#endif//FSBENCH_USE_FARMHASH
#ifdef FSBENCH_USE_FASTCRYPTO
extern "C"
{
#include "umac.h"
#include "vmac.h"
}
namespace FsBenchFastCrypto
{
    // TODO: uhash doesn't support blocks > 16 MB
    void uhash(char * in, size_t isize, char * out)
    {
        // uhash may overwrite data after the input
        // That's where fsbench stores checksum
        // If this function is called during 'decoding',
        // there's a hash stored already that will be destroyed,
        // later it will be compared with what we return
        // which will cause errors. That is - unless we protect the buffer.
        // Let's make a backup that we'll restore later.
        char backup[32];
        memcpy(backup, in + isize, sizeof(backup));
        char key[16] = {0};
        uhash_ctx_t ctx = uhash_alloc(key);
        // FIXME: what if malloc fails?
        ::uhash(ctx, in, isize, out);
        uhash_free(ctx);
        memcpy(in + isize, backup, sizeof(backup));
    }

    // TODO: uhash doesn't support blocks > 16 MB
    void umac(char * in, size_t isize, char * out)
    {
        // uhash may overwrite data after the input
        // That's where fsbench stores checksum
        // If this function is called during 'decoding',
        // there's a hash stored already that will be destroyed,
        // later it will be compared with what we return
        // which will cause errors. That is - unless we protect the buffer.
        // Let's make a backup that we'll restore later.
        char backup[32];
        memcpy(backup, in + isize, sizeof(backup));
        char key[16] = {0};
        char nonce[8] = {0};
        umac_ctx_t ctx = umac_new(key);
        // FIXME: what if malloc fails?
        ::umac(ctx, in, isize, out, nonce);
        umac_delete(ctx);
        memcpy(in + isize, backup, sizeof(backup));
    }

    void vhash(char * in, size_t isize, char * out)
    {
        vmac_ctx_t ctx;
        unsigned char user_key[VMAC_KEY_LEN/CHAR_BIT] = {0};
        ::vmac_set_key(user_key, &ctx);
        memset(&ctx, 0, sizeof(ctx));
        uint64_t out1 = 0; // Depending on compilation options, vhash may set it or not.
                           // If it doesn't, having a constant simplifies things
        ((uint64_t*)out)[0] = ::vhash((unsigned char*)in, (unsigned int)isize, &out1, &ctx);
        ((uint64_t*)out)[1] = out1;
    }

    void vmac(char * in, size_t isize, char * out)
    {
        vmac_ctx_t ctx;
        unsigned char user_key[VMAC_KEY_LEN/CHAR_BIT] = {0};
        unsigned char n[16] = {0};
        ::vmac_set_key(user_key, &ctx);
        memset(&ctx, 0, sizeof(ctx));
        uint64_t out1 = 0; // Depending on compilation options, vhash may set it or not.
                           // If it doesn't, having a constant simplifies things
        ((uint64_t*)out)[0] = ::vmac((unsigned char*)in, (unsigned int)isize, n, &out1, &ctx);
        ((uint64_t*)out)[1] = out1;
    }
} //namespace FsBenchFastCrypto

#endif //FSBENCH_USE_FASTCRYPTO
#ifdef FSBENCH_USE_FASTLZ
extern "C"
{
#include "fastlz.h"
}
size_t fastlz1_c(char * in, size_t isize, char * out, size_t, void *)
{
    return fastlz_compress_level(1, in, isize, out);
}
size_t fastlz2_c(char * in, size_t isize, char * out, size_t, void *)
{
    return fastlz_compress_level(2, in, isize, out);
}
size_t fastlz_d (char * in, size_t isize, char * out, size_t osize, void *)
{
    return fastlz_decompress(in, isize, out, osize);
}
size_t fastlz_m (size_t input_size)
{
    return max((size_t)66, input_size + input_size/20 + 1);
}

#endif//FSBENCH_USE_FASTLZ
#ifdef FSBENCH_USE_FSE
extern "C"
{
#include "fse.h"
}
#include <cstdio>
size_t fse_c(char * in, size_t isize, char * out, size_t, void *)
{
    int res = FSE_compress((unsigned char*)out, (const unsigned char*)in, isize);
    return res == -1 ? CODING_ERROR : res;
}
size_t fse_d (char * in, size_t, char * out, size_t osize, void *)
{
    int res = FSE_decompress((unsigned char*)out, osize, (const unsigned char*)in);
    return res == -1 ? CODING_ERROR : res;
}
size_t fse_m (size_t input_size)
{
    return FSE_compressBound(input_size);
}
#endif//FSBENCH_USE_FSE
#ifdef FSBENCH_USE_HALIBUT

#define HALIBUT_BUF_SIZE (1 * 1024 * 1024)
extern "C"
{
#include "halibut-deflate.h"
}
size_t halibut_c(char * in, size_t isize, char * out, size_t osize, void *)
{
    size_t total_out = 0;
    deflate_compress_ctx * ctx = deflate_compress_new(DEFLATE_TYPE_BARE);
    do
    {
        void * tmp_buf = 0;
        int received_size = 0;
        int chunk_size = min((size_t)HALIBUT_BUF_SIZE, isize);
        int flush = isize > HALIBUT_BUF_SIZE ? DEFLATE_NO_FLUSH : DEFLATE_END_OF_DATA;
        deflate_compress_data(ctx, in, chunk_size, flush, &tmp_buf, &received_size);
        if(received_size)
        {
            if((size_t)received_size > osize)
            {
                total_out = CODING_ERROR;
                break;
            }
            memcpy(out, tmp_buf, received_size);

            total_out += received_size;
            out += received_size;
            osize -= received_size;

            free(tmp_buf);
        }
        in += chunk_size;
        isize -= chunk_size;

    }while(isize > 0);
    deflate_compress_free(ctx);
    return total_out;
}
size_t halibut_d(char * in, size_t isize, char * out, size_t osize, void *)
{
    size_t total_out = 0;
    deflate_decompress_ctx * ctx = deflate_decompress_new(DEFLATE_TYPE_BARE);
    do
    {
        void * tmp_buf = 0;
        int received_size = 0;

        int chunk_size = min((size_t)HALIBUT_BUF_SIZE, isize);
        int ret = deflate_decompress_data(ctx, in, chunk_size, &tmp_buf, &received_size);
        if(ret != DEFLATE_NO_ERR)
        {
            total_out = CODING_ERROR;
            break;
        }
        if(received_size)
        {
            if((size_t)received_size > osize)
            {
                total_out = CODING_ERROR;
                break;
            }
            memcpy(out, tmp_buf, received_size);

            total_out += received_size;
            out += received_size;
            osize -= received_size;

            free(tmp_buf);
        }
        in += chunk_size;
        isize -= chunk_size;

    }while(isize > 0);
    deflate_decompress_free(ctx);
    return total_out;
}

#endif//FSBENCH_USE_HALIBUT
#ifdef FSBENCH_USE_LODEPNG
#include "lodepng.h"
size_t lodepng_c(char * in, size_t isize, char * out, size_t, void * mode)
{
    LodePNGCompressSettings settings;
    lodepng_compress_settings_init(&settings);
    settings.windowsize = *(intptr_t*)mode;

    unsigned char * tmp = 0;
    size_t _osize = 0;
    unsigned ret = lodepng_deflate(&tmp, &_osize, (unsigned char*)in, isize, &settings);
    if(ret != 0)
    return CODING_ERROR;
    memcpy(out, tmp, _osize);
    free(tmp);
    return _osize;
}
size_t lodepng_d(char * in, size_t isize, char * out, size_t, void *)
{
    LodePNGDecompressSettings settings;
    lodepng_decompress_settings_init(&settings);
    settings.ignore_adler32 = 1;

    unsigned char * tmp = 0;
    size_t _osize = 0;
    unsigned ret = lodepng_inflate(&tmp, &_osize, (unsigned char*)in, isize, &settings);
    if(ret != 0)
    return CODING_ERROR;
    memcpy(out, tmp, _osize);
    free(tmp);
    return _osize;
}
#endif//FSBENCH_USE_LODEPNG
#ifdef FSBENCH_USE_LZ4
extern "C"
{
#include "lz4.h"
#include "lz4hc.h"
}
size_t LZ4_c(char * in, size_t isize, char * out, size_t osize, void *)
{
    return LZ4_compress_limitedOutput(in, out, isize, osize);
}
size_t LZ4hc_c(char * in, size_t isize, char * out, size_t, void *)
{
    return LZ4_compressHC(in, out, isize);
}
size_t LZ4_d_fast (char * in, size_t, char * out, size_t osize, void *)
{
    return LZ4_decompress_fast(in, out, osize) > 0 ? osize : CODING_ERROR;
}
size_t LZ4_d_safe (char * in, size_t isize, char * out, size_t osize, void *)
{
    return LZ4_decompress_safe(in, out, isize, osize) > 0 ? osize : CODING_ERROR;
}

#endif//FSBENCH_USE_LZ4
#ifdef FSBENCH_USE_LZF

extern "C"
{
#include "lzf.h"
}
size_t LZF_ultra_c(char * in, size_t isize, char * out, size_t osize, void *)
{
    return lzf_compress_ultra(in, isize, out, osize);
}
size_t LZF_very_c(char * in, size_t isize, char * out, size_t osize, void *)
{
    return lzf_compress_very(in, isize, out, osize);
}
size_t LZF_c(char * in, size_t isize, char * out, size_t osize, void *)
{
    return lzf_compress(in, isize, out, osize);
}
size_t LZF_d(char * in, size_t isize, char * out, size_t osize, void *)
{
    return lzf_decompress(in, isize, out, osize);
}

#endif//FSBENCH_USE_LZF
#ifdef FSBENCH_USE_LZFX

extern "C"
{
#include "lzfx.h"
}
size_t LZFX_c(char * in, size_t isize, char * out, size_t osize, void *)
{
    unsigned int ret = osize;
    return lzfx_compress(in, isize, out, &ret) >=0 ? ret : CODING_ERROR;
}
size_t LZFX_d(char * in, size_t isize, char * out, size_t osize, void *)
{
    unsigned int ret = osize;
    return lzfx_decompress(in, isize, out, &ret) >=0 ? ret : CODING_ERROR;
}

#endif//FSBENCH_USE_LZFX
#ifdef FSBENCH_USE_LZG

extern "C"
{
#include "liblzg/include/lzg.h"
}
size_t LZG_c(char * in, size_t isize, char * out, size_t, void * mode)
{
    lzg_encoder_config_t cfg;
    cfg.level = *(intptr_t*)mode;
    cfg.fast = LZG_TRUE;
    cfg.progressfun = NULL;
    cfg.userdata = NULL;
    return LZG_Encode((const unsigned char*)in, isize, (unsigned char*)out, LZG_MaxEncodedSize(isize), &cfg);
}
size_t LZG_d(char * in, size_t isize, char * out, size_t osize, void *)
{
    return LZG_Decode((const unsigned char*)in, isize, (unsigned char*)out, osize);
}
size_t LZG_m(size_t input_size)
{
    return LZG_MaxEncodedSize(input_size);
}

#endif//FSBENCH_USE_LZG
#ifdef FSBENCH_USE_LZMAT
extern "C"
{
#include "lzmat.h"
}
size_t lzmat_c(char * in, size_t isize, char * out, size_t osize, void *)
{
    MP_U32 retsize = osize;
    int ret = lzmat_encode((MP_U8*)out, &retsize, (MP_U8*)in, isize);
    return ret == LZMAT_STATUS_OK ? retsize : CODING_ERROR;
}
size_t lzmat_d(char * in, size_t isize, char * out, size_t osize, void *)
{
    MP_U32 ret = osize;
    lzmat_decode((MP_U8*)out, &ret, (MP_U8*)in, isize);
    return ret;
}
#endif//FSBENCH_USE_LZMAT
#ifdef FSBENCH_USE_LZP_DS
int LZPEncode(unsigned char* In, unsigned int Size, unsigned char* Out, int MinLen);
int LZPDecode(unsigned char* In, unsigned int Size, unsigned char* Out, int MinLen);
size_t lzp_ds_c(char * in, size_t isize, char * out, size_t, void * min_len)
{
    return LZPEncode((unsigned char*)in, isize, (unsigned char*)out, *(uintptr_t*)min_len);
}
size_t lzp_ds_d(char * in, size_t isize, char * out, size_t, void * min_len)
{
    return LZPDecode((unsigned char*)in, isize, (unsigned char*)out, *(uintptr_t*)min_len);
}
#endif//FSBENCH_USE_LZP_DS
#ifdef FSBENCH_USE_LZWC
extern "C"
{
#include "LZWC.h"
}
size_t LZWC_c(char * in, size_t isize, char * out, size_t osize, void *)
{
    size_t ret = lzwc_compress((const unsigned char*)in, isize, (unsigned char*)out, osize);
    return ret > 0 ? ret : CODING_ERROR;
}
size_t LZWC_d(char * in, size_t isize, char * out, size_t osize, void *)
{
    size_t ret = lzwc_decompress((const unsigned char*)in, isize, (unsigned char*)out, osize);
    return ret > 0 ? ret : CODING_ERROR;
}
#endif//FSBENCH_USE_LZWC
#ifdef FSBENCH_USE_LZX_COMPRESS
extern "C"
{
#include "lzx_compress/lzx_compress.h"
}
int LZX_get_bytes(void * arg, int n, void * buf)
{
    pair<char*,int> * data = (pair<char*,int>*)arg;
    int bytes_to_copy = min(n, data->second);
    memcpy(buf, data->first, bytes_to_copy);
    data->first += bytes_to_copy;
    data->second -= bytes_to_copy;
    return bytes_to_copy;
}
int LZX_at_eof(void * arg)
{
    pair<char*,int> * data = (pair<char*,int>*)arg;
    return data->second;
}
int LZX_put_bytes(void * arg, int n, void * buf)
{
    pair<char*,size_t> * data = (pair<char*,size_t>*)arg;
    size_t bytes_to_copy = min((size_t)n, data->second);
    memcpy(data->first, buf, bytes_to_copy);
    data->first += bytes_to_copy;
    data->second -= bytes_to_copy;
    return bytes_to_copy;
}

size_t LZX_c(char * in, size_t isize, char * out, size_t osize, void * window_size)
{
    lzx_data * lzxd;
    lzx_results lzxr;

    pair<char*,size_t> get_data;
    pair<char*,size_t> put_data;

    get_data.first = in;
    get_data.second = isize;
    put_data.first = out;
    put_data.second = osize;

    lzx_init(&lzxd, *(intptr_t*)window_size, LZX_get_bytes, &get_data, LZX_at_eof,
            LZX_put_bytes, &put_data, NULL, NULL);

    size_t block_size = 1 << *(intptr_t*)window_size;

    while(get_data.second > 0)
    {
        lzx_reset(lzxd);

        size_t used_block_size = min(block_size, get_data.second);

        lzx_compress_block(lzxd, used_block_size, 1);
    }
    lzx_finish(lzxd, &lzxr);

    return lzxr.len_compressed_output;
}
#endif// FSBENCH_USE_LZX_COMPRES
#ifdef FSBENCH_USE_MMINI
extern "C"
{
#include "mmini.h"
}

// TODO: Make an abstract codec for codecs with maximum block size
//       There's also Shrinker with such properties, maybe others 
/*
size_t mmini_c(char * in, size_t isize, char * out, size_t osize, void * buffer)
{
    size_t ret = mmini_compress((const unsigned char*)in, isize, (unsigned char*)out, osize, buffer);
    return ret ? ret : CODING_ERROR;
}
size_t mmini_d(char * in, size_t isize, char * out, size_t osize, void * buffer)
{
    size_t ret = mmini_decompress((const unsigned char*)in, isize, (unsigned char*)out, osize, buffer);
    return ret ? ret : CODING_ERROR;
}
*/
size_t mmini_lzl_c(char * in, size_t isize, char * out, size_t osize, void *)
{
    size_t ret = mmini_lzl_compress((const unsigned char*)in, isize, (unsigned char*)out, osize);
    return ret ? ret : CODING_ERROR;
}
size_t mmini_lzl_d(char * in, size_t isize, char * out, size_t osize, void *)
{
    size_t ret = mmini_lzl_decompress((const unsigned char*)in, isize, (unsigned char*)out, osize);
    return ret ? ret : CODING_ERROR;
}
size_t mmini_huffman_c(char * in, size_t isize, char * out, size_t osize, void * buffer)
{
    size_t ret = mmini_huffman_compress((const unsigned char*)in, isize, (unsigned char*)out, osize, *(void**)buffer);
    return ret ? ret : CODING_ERROR;
}
size_t mmini_huffman_d(char * in, size_t isize, char * out, size_t osize, void * buffer)
{
    size_t ret = mmini_huffman_decompress((const unsigned char*)in, isize, (unsigned char*)out, osize, *(void**)buffer);
    return ret ? ret : CODING_ERROR;
}
#endif//FSBENCH_USE_MMINI
#ifdef FSBENCH_USE_MURMUR
#include "MurmurHash3.h"
void murmur_x86_32(char * in, size_t isize, char * out)
{
    MurmurHash3_x86_32(in, isize, 0, out);
}
void murmur_x86_128(char * in, size_t isize, char * out)
{
    MurmurHash3_x86_128(in, isize, 0, out);
}
void murmur_x64_128(char * in, size_t isize, char * out)
{
    MurmurHash3_x64_128(in, isize, 0, out);
}
#endif//FSBENCH_USE_MURMUR
#ifdef FSBENCH_USE_NAKAMICHI

#define NAKAMICHI_VARIANT(name)                                                          \
    extern "C"                                                                           \
    {                                                                                    \
    unsigned int name ## Compress(char* ret, char* src, unsigned int srcSize);           \
    unsigned int name ## Decompress(char* ret, char* src, unsigned int srcSize);         \
    }                                                                                    \
    size_t nakamichi_ ## name ## _c(char * in, size_t isize, char * out, size_t, void *) \
    {                                                                                    \
        return name ## Compress(out, in, isize);                                         \
    }                                                                                    \
    size_t nakamichi_ ## name ## _d(char * in, size_t isize, char * out, size_t, void *) \
    {                                                                                    \
        return name ## Decompress(out, in, isize);                                       \
    }
NAKAMICHI_VARIANT(Aratama)
NAKAMICHI_VARIANT(Butsuhira)
NAKAMICHI_VARIANT(ButsuhiraBranchless)
NAKAMICHI_VARIANT(Daikuni)
NAKAMICHI_VARIANT(Hanabi)
NAKAMICHI_VARIANT(Hanazakari)
NAKAMICHI_VARIANT(Hitomi)
NAKAMICHI_VARIANT(Inazuma)
NAKAMICHI_VARIANT(Jiten)
NAKAMICHI_VARIANT(Kaibutsu)
NAKAMICHI_VARIANT(Kaidanji)
NAKAMICHI_VARIANT(Kaiko)
NAKAMICHI_VARIANT(Kinezumi)
NAKAMICHI_VARIANT(Kinroba)
NAKAMICHI_VARIANT(Kinutora)
NAKAMICHI_VARIANT(Kitsune)
NAKAMICHI_VARIANT(Kumataka)
NAKAMICHI_VARIANT(Nekomata)
NAKAMICHI_VARIANT(Nin)
NAKAMICHI_VARIANT(Nirenpatsu)
NAKAMICHI_VARIANT(Sanagi)
NAKAMICHI_VARIANT(Sanbashi)
NAKAMICHI_VARIANT(Sanrenpatsu)
NAKAMICHI_VARIANT(Sanshi)
NAKAMICHI_VARIANT(Suiken)
NAKAMICHI_VARIANT(Tengu)
NAKAMICHI_VARIANT(Washi)
NAKAMICHI_VARIANT(Yoko)
NAKAMICHI_VARIANT(Zangetsu)

extern "C"
{
#include "nakamichi.h"
}
size_t nakamichi_m_c(char * in, size_t isize, char * out, size_t osize, void *)
{
    nssize_t ret = CompressM(in, isize, out, osize);
    return N_ERROR(ret) ? CODING_ERROR : ret;
}
size_t nakamichi_m_d(char * in, size_t isize, char * out, size_t osize, void *)
{
    nssize_t ret = DecompressM(in, isize, out, osize);
    return N_ERROR(ret) ? CODING_ERROR : ret;
}
#endif//FSBENCH_USE_NAKAMICHI
#ifdef FSBENCH_USE_NOBUO_ITO_LZSS
extern "C"
{
    unsigned int compressLZSS(char* ret, char* src, unsigned int srcSize);
    unsigned int uncompressLZSS(char* ret, char* src, unsigned int srcSize);
}
size_t nobuo_ito_lzss_c(char * in, size_t isize, char * out, size_t, void *)
{
    return compressLZSS(out, in, isize);
}
size_t nobuo_ito_lzss_d(char * in, size_t isize, char * out, size_t, void *)
{
    return uncompressLZSS(out, in, isize);
}
#endif//FSBENCH_USE_NOBUO_ITO_LZSS
#ifdef FSBENCH_USE_PG_LZ

extern "C"
{
#include <pg_lzcompress.h>
}
size_t pg_lz_c(char * in, size_t isize, char * out, size_t, void * strategy)
{
    PGLZ_Header * pgout = reinterpret_cast<PGLZ_Header *>(out);
    const PGLZ_Strategy * pglz_strategy = *(intptr_t*) strategy ? 
            PGLZ_strategy_default : PGLZ_strategy_always;
    return pglz_compress(in,
                         isize,
                         pgout,
                         pglz_strategy) ? VARSIZE(pgout) : CODING_ERROR;
}
size_t pg_lz_d(char * in, size_t, char * out, size_t osize, void *)
{
    PGLZ_Header * pgin = reinterpret_cast<PGLZ_Header *>(in);
    return pglz_decompress(pgin, out) ? osize : CODING_ERROR;
}
#endif//FSBENCH_USE_PG_LZ
#ifdef FSBENCH_USE_QUICKLZZIP

extern "C"
{
#include "quicklzzip.h"
}
size_t qlzzip_c(char * in, size_t isize, char * out, size_t, void *)
{
    unsigned out_bits = qlz_deflate((unsigned char*)in, (unsigned char*)out, 0, (unsigned)isize, 1);
    // round up
    if(out_bits & 7)
    out_bits += 8;
    return out_bits / 8;
}

#endif//FSBENCH_USE_QUICKLZZIP
#ifdef FSBENCH_USE_LRRLE


#define LRRLE(version)                                                                                       \
    extern "C"                                                                                               \
    {                                                                                                        \
        uint_fast64_t lrrle ## version ## _compress  (const void * in_, void * out_, uint_fast64_t in_size); \
        uint_fast64_t lrrle ## version ## _decompress(const void * in_, void * out_, uint_fast64_t in_size); \
    }                                                                                                        \
    size_t lrrle ## version ## _c(char * in, size_t isize, char * out, size_t, void *)                       \
    {                                                                                                        \
        return lrrle ## version ## _compress(in, out, isize);                                                \
    }                                                                                                        \
    size_t lrrle ## version ## _d(char * in, size_t, char * out, size_t osize, void *)                       \
    {                                                                                                        \
        return lrrle ## version ## _decompress(in, out, osize);                                              \
    }
LRRLE(64)
LRRLE(128)
LRRLE(192)
LRRLE(256)

#endif// FSBENCH_USE_LRRLE
#ifdef FSBENCH_USE_RLE64

#include "RLE64.hpp"

#define RLE_64_FUNCTION(FSBENCH_NAME, ORIGINAL_NAME)                    \
size_t FSBENCH_NAME(char * in, size_t isize, char * out, size_t, void *)\
{                                                                       \
    return ORIGINAL_NAME(in, out, isize);                               \
}
RLE_64_FUNCTION(RLE64_c, RLE64_Compress64)
RLE_64_FUNCTION(RLE64_d, RLE64_Uncompress64)
RLE_64_FUNCTION(RLE32_c, RLE64_Compress32)
RLE_64_FUNCTION(RLE32_d, RLE64_Uncompress32)
RLE_64_FUNCTION(RLE16_c, RLE64_Compress16)
RLE_64_FUNCTION(RLE16_d, RLE64_Uncompress16)
RLE_64_FUNCTION(RLE8_c, RLE64_Compress8)
RLE_64_FUNCTION(RLE8_d, RLE64_Uncompress8)

#endif// FSBENCH_USE_RLE64
#ifdef FSBENCH_USE_SANMAYCE_FNV
extern "C"
{
#include "sanmayce.h"
}
void fnv1_jesteress(char * in, size_t isize, char * out)
{
    *(uint32_t*) out = FNV1A_Hash_Jesteress(in, isize);
}
void fnv1_mantis(char * in, size_t isize, char * out)
{
    *(uint32_t*) out = FNV1A_Hash_Mantis(in, isize);
}
void fnv1_meiyan(char * in, size_t isize, char * out)
{
    *(uint32_t*) out = FNV1A_Hash_Meiyan(in, isize);
}
void fnv1_tesla(char * in, size_t isize, char * out)
{
    *(uint64_t*) out = FNV1A_Hash_Tesla(in, isize);
}
void fnv1_tesla3(char * in, size_t isize, char * out)
{
    *(uint64_t*) out = FNV1A_Hash_Tesla3(in, isize);
}
void fnv1_yorikke(char * in, size_t isize, char * out)
{
    *(uint32_t*) out = FNV1A_Hash_Yorikke(in, isize);
}
void fnv1_yoshimitsu_triad(char * in, size_t isize, char * out)
{
    *(uint32_t*) out = FNV1A_Hash_YoshimitsuTRIAD(in, isize);
}
#ifdef FSBENCH_SSE2
void fnv1_yoshimitsu_triad_iixmm(char * in, size_t isize, char * out)
{
    *(uint32_t*) out = FNV1A_Hash_YoshimitsuTRIADiiXMM(in, isize);
}
void fnv1_penumbra(char * in, size_t isize, char * out)
{
    *(uint32_t*) out = FNV1A_Hash_penumbra(in, isize);
}
#endif
void fnv1_yoshimura(char * in, size_t isize, char * out)
{
    *(uint64_t*) out = FNV1A_Hash_Yoshimura(in, isize);
}
#endif//FSBENCH_USE_SANMAYCE_FNV
#ifdef FSBENCH_USE_SIPHASH
#include "siphash.hpp"
void siphash(char * in, size_t isize, char * out)
{
    const unsigned char k[16] =
    {   0};
    FsBenchSipHash::crypto_auth((unsigned char*)out, (const unsigned char*) in, isize, k);
}
#endif//FSBENCH_USE_SIPHASH
#ifdef FSBENCH_USE_SHRINKER
extern "C"
{
#include "Shrinker.h"
}

#define SHRINKER_MIN_SIZE (32)
#define SHRINKER_MAX_SIZE ((1 << 27) - 1)
inline int32_t get_next_shrinker_chunk_size(size_t size_left)
{
    if(size_left >= SHRINKER_MAX_SIZE + SHRINKER_MIN_SIZE)
    return SHRINKER_MAX_SIZE;
    if(size_left > SHRINKER_MAX_SIZE)
    return size_left / 2; // when there's just slightly over SHRINKER_MAX_SIZE left, the last 2 chunks are roughly equal
    return size_left;
}

size_t Shrinker_c(char * in, size_t isize, char * out, size_t, void *)
{
    size_t total_out = 0;

    while(isize > 0)
    {
        size_t chunk_size = get_next_shrinker_chunk_size(isize);
        int ret = shrinker_compress(in, out+sizeof(int32_t), chunk_size);
        if(ret == -1)
        return CODING_ERROR;
        *(int32_t*) out = ret;
        out += sizeof(uint32_t);

        total_out += ret + sizeof(int32_t); // I'm not sure if it's right, but in the output size I include my metadata size
                                            // Either way, it's required by the Codec interface
        in += chunk_size;
        isize -= chunk_size;
        out += ret;
    }
    return total_out;
}
size_t Shrinker_d(char * in, size_t, char * out, size_t osize, void *)
{
    size_t osize_left = osize;

    while(osize_left > 0)
    {
        size_t chunk_size = get_next_shrinker_chunk_size(osize_left);
        int32_t compressed_size = *(int32_t*)in;
        in += sizeof(int32_t);

        int ret = shrinker_decompress(in, out, chunk_size);
        if(ret == -1)
        return CODING_ERROR;
        in += compressed_size;
        out += chunk_size;
        osize_left -= chunk_size;
    }
    return osize;
}
size_t Shrinker_m(size_t input_size)
{
    return input_size + (input_size / SHRINKER_MAX_SIZE + 1) * sizeof(int); //overhead of 1 int per SHRINKER_MAX_SIZE block in my code
}
#endif// FSBENCH_USE_SHRINKER
#ifdef FSBENCH_USE_SNAPPY

#include "snappy/snappy.h"

size_t snappy_c(char * in, size_t isize, char * out, size_t, void *)
{
    size_t ret;
    snappy::RawCompress(in, isize, out, &ret);
    return ret;
}
size_t snappy_d(char * in, size_t isize, char * out, size_t osize, void *)
{
    if(!snappy::RawUncompress(in, isize, out))
    {
        return CODING_ERROR;
    }
    return osize;
}
size_t snappy_m(size_t input_size)
{   return snappy::MaxCompressedLength(input_size);}

#endif// FSBENCH_USE_SNAPPY
#ifdef FSBENCH_USE_SPOOKY
#include "SpookyV2.h"
void spooky(char * in, size_t isize, char * out)
{
    uint64_t d1=0, d2=0;
    SpookyHash::Hash128(in, isize, &d1, &d2);
    memcpy(out, &d1, sizeof(d1));
    memcpy(out + sizeof(d1), &d2, sizeof(d2));
}
#endif//FSBENCH_USE_SPOOKY
#ifdef FSBENCH_USE_WFLZ
extern "C"
{
#include "wfLZ.h"
}
size_t wfLZ_c(char * in, size_t isize, char * out, size_t osize, void *buf)
{
    UNUSED(osize);
    return wfLZ_Compress((const uint8_t*)in, isize, (uint8_t*)out, (uint8_t*)buf, 0);
}
size_t wfLZ_fast_c(char * in, size_t isize, char * out, size_t osize, void *buf)
{
    UNUSED(osize);
    return wfLZ_CompressFast((const uint8_t*)in, isize, (uint8_t*)out, (uint8_t*)buf, 0);
}
size_t wfLZ_d (char * in, size_t isize, char * out, size_t osize, void *)
{
    UNUSED(isize);
    wfLZ_Decompress((const uint8_t*)in, (uint8_t*)out);
    return osize;
}
size_t wfLZ_m(size_t input_size)
{
    return wfLZ_GetMaxCompressedSize(input_size);
}
size_t wfLZ_mem()
{
    return wfLZ_GetWorkMemSize();
}

#endif//FSBENCH_USE_WFLZ
#ifdef FSBENCH_USE_XXHASH
extern "C"
{
#include "xxhash.h"
}
void xxhash(char * in, size_t isize, char * out)
{
    *(unsigned int*) out = XXH32(in, isize, 0);
}
void xxhash64(char * in, size_t isize, char * out)
{
    *(unsigned long long*) out = XXH64(in, isize, 0);
}
#endif//FSBENCH_USE_XXHASH
#ifdef FSBENCH_USE_ZFS

extern "C"
{
#include "zfs/zfs.h"
}
size_t LZJB_c(char * in, size_t isize, char * out, size_t osize, void *)
{
    size_t ret = lzjb_compress2010((uint8_t*)in, (uint8_t*)out, isize, osize, 0);
    return ret == isize ? CODING_ERROR : ret;
}
size_t LZJB_d(char * in, size_t isize, char * out, size_t osize, void *)
{
    return lzjb_decompress2010((uint8_t*)in, (uint8_t*)out, isize, osize, 0);
}
void fletcher2(char * in, size_t isize, char * out)
{
    fletcher_2_native(in, isize, (uint64_t*)out);
}
void fletcher4(char * in, size_t isize, char * out)
{
    fletcher_4_native(in, isize, (uint64_t*)out);
}
#endif//FSBENCH_USE_ZFS
#if defined(FSBENCH_USE_ZLIB) || defined(FSBENCH_USE_ZLIB_INTEL)
extern "C"
{
#include "zlib/zlib.h"
}
size_t zlib_c(char * in, size_t isize, char * out, size_t osize, void * mode)
{
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = NULL;

    int ret = deflateInit2(&stream, *(intptr_t*)mode, Z_DEFLATED, -15, 9, Z_DEFAULT_STRATEGY);
    if(ret != Z_OK)
    {
        return CODING_ERROR;
    }
    stream.next_in = (Bytef*)in;
    stream.avail_in = isize;
    stream.next_out = (Bytef*)out;
    stream.avail_out = osize;
    ret = deflate(&stream, Z_FINISH);
    int ret2 = deflateEnd(&stream);
    if(ret != Z_STREAM_END || ret2 != Z_OK)
    {
        return CODING_ERROR;
    }
    return stream.total_out;
}
size_t zlib_d(char * in, size_t isize, char * out, size_t osize, void *)
{
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = NULL;

    int ret = inflateInit2(&stream, -15);
    if(ret != Z_OK)
    {
        return CODING_ERROR;
    }
    stream.next_in = (Bytef*)in;
    stream.avail_in = isize;
    stream.next_out = (Bytef*)out;
    stream.avail_out = osize;
    ret = inflate(&stream, Z_FINISH);
    int ret2 = inflateEnd(&stream);
    if(ret != Z_STREAM_END || ret2 != Z_OK)
    {
        return CODING_ERROR;
    }
    return stream.total_out;
}
#endif //defined(FSBENCH_USE_ZLIB) || defined(FSBENCH_USE_ZLIB_INTEL)
#ifdef FSBENCH_USE_ZOPFLI
extern "C"
{
#include "zopfli/deflate.h"
}
size_t zopfli_c(char * in, size_t isize, char * out, size_t osize, void * mode)
{
    ZopfliOptions options;
    ZopfliInitOptions(&options);
    options.verbose = 0;
    options.numiterations = *(intptr_t*)mode;

    unsigned char bit_pointer = 0;
    unsigned char * outbuf = 0;
    size_t out_size = 0;
    ZopfliDeflate(&options,
            2, // best compression
            1,// final block
            (const unsigned char*)in,
            isize,
            &bit_pointer,
            (unsigned char**)&outbuf,
            &out_size);
    // NOTE: zopfli says that bp must be freed after use
    //       I don't know what do they mean, it's a char!
    if(out_size > osize)
    {
        free(outbuf);
        return CODING_ERROR;
    }
    memcpy(out, outbuf, out_size);
    free(outbuf);
    return out_size;
}
#endif //FSBENCH_USE_ZOPFLI
#ifdef FSBENCH_USE_ZSTD
extern "C"
{
#include "zstd.h"
}
size_t ZSTD_c(char * in, size_t isize, char * out, size_t osize, void *)
{
    size_t ret = ZSTD_compress(out, osize, in, isize);
    return ZSTD_isError(ret) ? CODING_ERROR : ret;
}
size_t ZSTD_d (char * in, size_t isize, char * out, size_t osize, void *)
{
    return ZSTD_isError( ZSTD_decompress(out, osize, in, isize) ) ? CODING_ERROR : osize ;
}

#endif//FSBENCH_USE_ZSTD
// a pseudocodec that does nothing.
size_t nop_c(char *, size_t, char *, size_t, void *)
{
    return 1;
}
size_t nop_d(char *, size_t, char *, size_t osize, void *)
{
    return osize;
}

#ifdef FSBENCH_USE_BLZ

extern "C"
{
#include "brieflz/brieflz.h"
}

size_t BriefLZ::compressor(char * in, size_t isize, char * out, size_t, void * work)
{
    return blz_pack(in, out, isize, *(void**)work);
}
size_t BriefLZ::decompressor(char * in, size_t, char * out, size_t osize, void *)
{
    return blz_depack(in, out, osize);
}

size_t BriefLZ::max_compressed_size(size_t input_size)
{
    return blz_max_packed_size(input_size);
}

void BriefLZ::init(const string &, unsigned threads_no, size_t isize, bool init_compressor, bool)
{
    if(init_compressor)
    {
        int piece_size = blz_workmem_size(isize);
        this->workmem = new char[threads_no * piece_size];
        this->params = new void *[threads_no];
        for(unsigned i=0; i<threads_no; ++i)
        this->params[i] = workmem + i * piece_size;
    }
}
void BriefLZ::cleanup()
{
    if(this->params)
    {
        delete[] this->workmem;
        delete[] this->params;
        this->workmem = 0;
        this->params = 0;
    }
}
string BriefLZ::help() const
{
    return this->name + " " + this->version + "\nTakes no parameters.\n";
}

void ** BriefLZ::eparams()
{
    return params;
}

#endif// FSBENCH_USE_BLZ
#ifdef FSBENCH_USE_LZHAM
const string & LZHAM::default_args(const string & args)
{
    return args == "" ? default_mode : args;
}
size_t LZHAM::compressor(char * in, size_t _isize, char * out, size_t _osize, void * _data)
{
    lzham_compress_state_ptr data = *(lzham_compress_state_ptr*)_data;
    lzham_compress_reinit(data);
    size_t total_in = 0;
    size_t total_out = 0;
    lzham_compress_status_t status;
    do
    {
        size_t isize = _isize - total_in;
        size_t osize = _osize - total_out;
        status = lzham_compress2(
                data,
                (const lzham_uint8*)(in + total_in), &isize,
                (lzham_uint8*) (out + total_out), &osize,
                LZHAM_FINISH);
        total_in += isize;
        total_out += osize;
    }while (status == LZHAM_COMP_STATUS_NOT_FINISHED && _osize > total_out && _isize > total_in);

    return status == LZHAM_COMP_STATUS_SUCCESS ? total_out : CODING_ERROR;
}
size_t LZHAM::decompressor(char * in, size_t _isize, char * out, size_t _osize, void * _data)
{;
    lzham_decompress_state_ptr state = (*(DecompressionState**)_data)->decompression_state;
    lzham_decompress_params* params = (*(DecompressionState**)_data)->decompression_params;

    lzham_decompress_reinit(state, params);

    size_t isize = _isize;
    size_t osize = _osize;

    lzham_decompress_status_t status = lzham_decompress(state, (const lzham_uint8*)in, &isize, (lzham_uint8*)out, &osize, true);
    if (status != LZHAM_DECOMP_STATUS_SUCCESS)
    {
        cerr<<"decompression error: "<<status<<endl;
        return CODING_ERROR;
    }

    return osize;
}
void LZHAM::init(const string & args, unsigned threads_no, size_t, bool init_compressor, bool init_decompressor)
{
    this->args = default_args(args);
    uintptr_t mode;
    try
    {
        from_string(this->args, mode);
    }
    catch(const bad_cast & bc)
    {
        throw InvalidParams(help());
    }

    if(init_compressor)
    {
        if(mode > (int)LZHAM_TOTAL_COMP_LEVELS)
        throw InvalidParams(help());

        lzham_compress_params comp_params;
        memset(&comp_params, 0, sizeof(comp_params));
        comp_params.m_struct_size = sizeof(comp_params);
        comp_params.m_dict_size_log2 = 17;
        comp_params.m_level = (lzham_compress_level)mode;
        comp_params.m_max_helper_threads = 0;
        comp_params.m_compress_flags = 0;

        this->compressor_params = new void *[threads_no];
        for(unsigned i=0; i<threads_no; ++i)
        {
            lzham_compress_state_ptr state = lzham_compress_init(&comp_params);
            this->compressor_params[i] = (void*)state;
        }
    }
    if(init_decompressor)
    {
        this->decompressor_params = new void *[threads_no];
        for(unsigned i=0; i<threads_no; ++i)
        {
            lzham_decompress_params * decomp_params = new lzham_decompress_params();
            memset(decomp_params, 0, sizeof(lzham_decompress_params));
            decomp_params->m_struct_size = sizeof(lzham_decompress_params);
            decomp_params->m_dict_size_log2 = 17;
            decomp_params->m_decompress_flags = LZHAM_DECOMP_FLAG_OUTPUT_UNBUFFERED;

            DecompressionState * state = new DecompressionState();

            state->decompression_state = lzham_decompress_init(decomp_params);
            state->decompression_params = decomp_params;
            this->decompressor_params[i] = (void*)state;
        }
    }
    this->threads_no = threads_no;
}
void LZHAM::cleanup()
{
    if(this->compressor_params)
    {
        for(unsigned i=0; i<this->threads_no; ++i)
        {
            lzham_compress_state_ptr state = (lzham_compress_state_ptr) this->compressor_params[i];
            lzham_compress_deinit(state);
        }
        delete[] this->compressor_params;
        this->compressor_params = 0;
    }
    if(this->decompressor_params)
    {
        for(unsigned i=0; i<this->threads_no; ++i)
        {
            DecompressionState * state = (DecompressionState*) this->decompressor_params[i];
            lzham_decompress_deinit(state->decompression_state);
            delete state->decompression_params;
            delete state;
        }
        delete[] this->decompressor_params;
        this->decompressor_params = 0;
    }
}

string LZHAM::help() const
{
    return "LZHAM " + this->version + "\nAvaivable parameters: \n Numbers from 0 to 4\n";
}

void ** LZHAM::eparams()
{
    return this->compressor_params;
}
void ** LZHAM::dparams()
{
    return this->decompressor_params;
}

const string LZHAM::default_mode("4");

#endif

#ifdef FSBENCH_USE_LZO
const map<const string, LZO::LzoType*> & LZO::lzo_types()
{
    if(LZO_TYPES.empty())
    {
        LZO_TYPES = map<const string, LzoType*>();
        LZO_TYPES["1"] = new LzoType(lzo1_compress, lzo1_decompress);
        LZO_TYPES["1_99"] = new LzoType(lzo1_99_compress, lzo1_decompress);
        LZO_TYPES["1b1"] = new LzoType(lzo1b_1_compress, lzo1b_decompress);
        LZO_TYPES["1b2"] = new LzoType(lzo1b_2_compress, lzo1b_decompress);
        LZO_TYPES["1b3"] = new LzoType(lzo1b_3_compress, lzo1b_decompress);
        LZO_TYPES["1b4"] = new LzoType(lzo1b_4_compress, lzo1b_decompress);
        LZO_TYPES["1b5"] = new LzoType(lzo1b_5_compress, lzo1b_decompress);
        LZO_TYPES["1b6"] = new LzoType(lzo1b_6_compress, lzo1b_decompress);
        LZO_TYPES["1b7"] = new LzoType(lzo1b_7_compress, lzo1b_decompress);
        LZO_TYPES["1b8"] = new LzoType(lzo1b_8_compress, lzo1b_decompress);
        LZO_TYPES["1b9"] = new LzoType(lzo1b_9_compress, lzo1b_decompress);
        LZO_TYPES["1b99"] = new LzoType(lzo1b_99_compress, lzo1b_decompress);
        LZO_TYPES["1b999"] = new LzoType(lzo1b_999_compress, lzo1b_decompress);
        LZO_TYPES["1c1"] = new LzoType(lzo1c_1_compress, lzo1c_decompress);
        LZO_TYPES["1c2"] = new LzoType(lzo1c_2_compress, lzo1c_decompress);
        LZO_TYPES["1c3"] = new LzoType(lzo1c_3_compress, lzo1c_decompress);
        LZO_TYPES["1c4"] = new LzoType(lzo1c_4_compress, lzo1c_decompress);
        LZO_TYPES["1c5"] = new LzoType(lzo1c_5_compress, lzo1c_decompress);
        LZO_TYPES["1c6"] = new LzoType(lzo1c_6_compress, lzo1c_decompress);
        LZO_TYPES["1c7"] = new LzoType(lzo1c_7_compress, lzo1c_decompress);
        LZO_TYPES["1c8"] = new LzoType(lzo1c_8_compress, lzo1c_decompress);
        LZO_TYPES["1c9"] = new LzoType(lzo1c_9_compress, lzo1c_decompress);
        LZO_TYPES["1c99"] = new LzoType(lzo1c_99_compress, lzo1c_decompress);
        LZO_TYPES["1c999"] = new LzoType(lzo1c_999_compress, lzo1c_decompress);
        LZO_TYPES["1f1"] = new LzoType(lzo1f_1_compress, lzo1f_decompress);
        LZO_TYPES["1f999"] = new LzoType(lzo1f_999_compress, lzo1f_decompress);
        LZO_TYPES["1x1"] = new LzoType(lzo1x_1_compress, lzo1x_decompress);
        LZO_TYPES["1x1_11"] = new LzoType(lzo1x_1_11_compress, lzo1x_decompress);
        LZO_TYPES["1x1_12"] = new LzoType(lzo1x_1_12_compress, lzo1x_decompress);
        LZO_TYPES["1x1_15"] = new LzoType(lzo1x_1_15_compress, lzo1x_decompress);
        LZO_TYPES["1x1_999"] = new LzoType(lzo1x_999_compress, lzo1x_decompress);
        LZO_TYPES["1y1"] = new LzoType(lzo1y_1_compress, lzo1y_decompress);
        LZO_TYPES["1y999"] = new LzoType(lzo1y_999_compress, lzo1y_decompress);
        LZO_TYPES["1z999"] = new LzoType(lzo1z_999_compress, lzo1z_decompress);
        LZO_TYPES["2a999"] = new LzoType(lzo2a_999_compress, lzo2a_decompress);
    }
    return LZO_TYPES;
}

const LZO::LzoType * LZO::get_type(const string & args)
{
    string args_lower = args;
    transform(args_lower.begin(), args_lower.end(), args_lower.begin(), ::tolower);
    return lzo_types().find(args_lower)->second;
}
size_t LZO::compressor(char * in, size_t isize, char * out, size_t, void * work)
{
    lzo_uint ret;
    static_lzo_compressor((uint8_t*)in, isize, (uint8_t*)out, &ret, *(char**)work);
    return ret;
}
size_t LZO::decompressor(char * in, size_t isize, char * out, size_t, void * work)
{
    lzo_uint ret;
    static_lzo_decompressor((uint8_t*)in, isize, (uint8_t*)out, &ret, *(char**)work);
    return ret;
}
string LZO::default_args(const string & args)
{
    return args == "" ? "1x1" : args;
}
void LZO::init(const string & args, unsigned threads_no, size_t, bool init_compressor, bool)
{
    int ret = lzo_init();
    if(ret != LZO_E_OK)
    {
        throw runtime_error("LZO refuses to initialise: " + to_string(ret));
    }
    if(init_compressor)
    {
        this->work = new char[LZO1B_999_MEM_COMPRESS * threads_no]();
        this->work_ptrs = new char *[threads_no];
        for(unsigned i = 0; i < threads_no; ++i)
        {
            this->work_ptrs[i] = this->work + LZO1B_999_MEM_COMPRESS * i;
        }
    }
    this->args = default_args(args);

    const LzoType * t = get_type(this->args);

    static_lzo_compressor = t->compressor;
    static_lzo_decompressor = t->decompressor;
}
void LZO::cleanup()
{
    if(this->work_ptrs)
    {
        delete[] this->work_ptrs;
        delete[] this->work;
        this->work_ptrs = 0;
        this->work = 0;
    }
}
string LZO::help() const
{
    const map<const string, LzoType*> & types = lzo_types();
    string ret = this->name + " " + this->version + "\nAvaivable parameters: \n";
    for(map<const string, LzoType*>::const_iterator it = types.begin(); it != types.end(); ++it)
    {
        ret += " ";
        ret += it->first;
        ret += "\n";
    }
    return ret;
}
void ** LZO::eparams()
{
    return (void**)this->work_ptrs;
}
void ** LZO::dparams()
{
    return eparams();
}
LZO::lzo_compress LZO::static_lzo_compressor = 0;
LZO::lzo_decompress LZO::static_lzo_decompressor = 0;
map<const string, LZO::LzoType*> LZO::LZO_TYPES;

#endif// FSBENCH_USE_LZO
#ifdef FSBENCH_USE_LZSS_IM
size_t LZSS_IM::compressor(char * in, size_t isize, char * out, size_t, void * p)
{
    Params * params = *(Params**)p;
    return params->coder.lzssim_encode((unsigned char*)in, (unsigned char*)out, isize, params->mode);
}
size_t LZSS_IM::decompressor(char * in, size_t, char * out, size_t osize, void *)
{
    LZSSIM::lzssim_decode((unsigned char*)in, (unsigned char*)out, osize);
    return osize;
}
void LZSS_IM::init(const string & args, unsigned threads_no, size_t, bool init_compressor, bool)
{
    this->threads_no = threads_no;
    this->args = args;
    if(init_compressor)
    {
        this->params = (void**)new Params *[threads_no];
        for(unsigned i=0; i<threads_no; ++i)
        {
            Params * p = new Params;
            p->mode = args == "x" || args == "X";
            this->params[i] = p;
        }
    }
}
void LZSS_IM::cleanup()
{
    if(this->params)
    {
        for(unsigned i=0; i<this->threads_no; ++i)
        {
            delete ((Params**)this->params)[i];
        }
        delete[] this->params;
        this->params = 0;
    }
}
void ** LZSS_IM::eparams()
{
    return params;
}
void ** LZSS_IM::dparams()
{
    return eparams();
}
#endif //FSBENCH_USE_LZSS_IM
#ifdef FSBENCH_USE_LZV1
extern "C"
{
#include "lzv1.h"
}

size_t LZV1_c(char * in, size_t isize, char * out, size_t osize, void * memory)
{
    return wLZV1((uch*)in, (uch*)out, *(ush**)memory, isize, osize);
}
size_t LZV1_d(char * in, size_t isize, char * out, size_t osize, void *)
{
    return rLZV1((uch*)in, (uch*)out, isize, osize);
}
#endif //FSBENCH_USE_LZV1
#ifdef FSBENCH_USE_NRV
#define NUMBER_OF_NRV_TYPES 3

const string & Nrv::default_args(const string & args)
{
    return args == "" ? default_mode : args;
}

size_t Nrv::compressor(char * in, size_t isize, char * out, size_t, void * mode)
{
    ucl_uint ret;
    current_type->compressor((const unsigned char*)in, isize, (unsigned char*)out, &ret, NULL, *(uintptr_t*)mode, NULL, NULL);
    return (int)ret;
}
size_t Nrv::decompressor(char * in, size_t isize, char * out, size_t, void *)
{
    ucl_uint ret;
    current_type->decompressor((const unsigned char*)in, isize, (unsigned char*)out, &ret, NULL);
    return (int)ret;
}
const string Nrv::full_name(const string & type)
{
    return string("nrv2")+type;
}
const Nrv::NrvType * Nrv::get_type(const string & type_name)
{
    for(int i=0; i<NUMBER_OF_NRV_TYPES; ++i)
    {
        if(case_insensitive_compare(nrv_types[i].name, type_name) == 0)
        return &nrv_types[i];
    }
    throw InvalidParams(help());
}
void Nrv::init(const string & args, unsigned threads_no, size_t, bool init_compressor, bool)
{
    if(init_compressor)
    {
        this->args = default_args(args); // I let the decoder ignore args, they are not used and they won't be printed in the output
        uintptr_t mode;
        try
        {
            from_string(this->args, mode);
        }
        catch(const bad_cast & bc)
        {
            throw InvalidParams(help());
        }

        if(mode < 1 || mode > 10)
        throw InvalidParams(help());
        this->params = new void *[threads_no];
        for(unsigned i=0; i<threads_no; ++i)
        {
            this->params[i] = (void*)mode;
        }
    }
    this->current_type = get_type(this->name);
}
void Nrv::cleanup()
{
    delete[] this->params;
}

string Nrv::help() const
{
    return this->name + " " + this->version + "\nAvaivable parameters: \n Numbers from 1 to 10\n";
}

void ** Nrv::eparams()
{
    return params;
}
const Nrv::NrvType Nrv::nrv_types[] =
{
    Nrv::NrvType("nrv2b", ucl_nrv2b_99_compress, ucl_nrv2b_decompress_8),
    Nrv::NrvType("nrv2d", ucl_nrv2d_99_compress, ucl_nrv2d_decompress_8),
    Nrv::NrvType("nrv2e", ucl_nrv2e_99_compress, ucl_nrv2e_decompress_8)
};
const Nrv::NrvType * Nrv::current_type = 0;
const string Nrv::default_mode("6");

#endif// FSBENCH_USE_NRV
#ifdef FSBENCH_USE_QUICKLZ

const map<const string, QuickLZ::QuickLZType*> & QuickLZ::qlz_types()
{
    if(QLZ_TYPES.empty())
    {
        QLZ_TYPES = map<const string, QuickLZType*>();
        QLZ_TYPES["1"] = new QuickLZType(qlz_compress, qlz_decompress);
        QLZ_TYPES["2"] = new QuickLZType(qlz_compress2, qlz_decompress2);
        QLZ_TYPES["3"] = new QuickLZType(qlz_compress3, (qlz_decompressor)qlz_decompress3);
    }
    return QLZ_TYPES;
}

const QuickLZ::QuickLZType * QuickLZ::get_type(const string & args)
{
    string args_lower = args;
    transform(args_lower.begin(), args_lower.end(), args_lower.begin(), ::tolower);

    return qlz_types().find(args_lower)->second;
}
size_t QuickLZ::compressor(char * in, size_t isize, char * out, size_t, void * work)
{
    return static_qlz_compressor(in, out, isize, *(qlz_state_compress**)work);
}
size_t QuickLZ::decompressor(char * in, size_t, char * out, size_t, void * work)
{
    return static_qlz_decompressor(in ,out, *(qlz_state_decompress**)work);
}
string QuickLZ::default_args(const string & args)
{
    return args == "" ? "1" : args;
}
void QuickLZ::init(const string & args, unsigned threads_no, size_t, bool init_compressor, bool init_decompressor)
{
    if(init_compressor)
    {
        this->qlz_compression_states = new qlz_state_compress *[threads_no];

        int state_size = max(qlz_get_setting3(1), max(qlz_get_setting2(1), qlz_get_setting(1)));

        for(unsigned i = 0; i < threads_no; ++i)
        {
            this->qlz_compression_states[i] = (qlz_state_compress*) new char[state_size] ();
        }
    }
    if(init_decompressor)
    {
        this->qlz_decompression_states = new qlz_state_decompress*[threads_no];

        int dstate_size = max(qlz_get_setting3(2), max(qlz_get_setting2(2), qlz_get_setting(2)));

        for(unsigned i = 0; i < threads_no; ++i)
        {
            this->qlz_decompression_states[i] = (qlz_state_decompress*) new char[dstate_size] ();
        }
    }
    this->threads_no = threads_no;

    this->args = default_args(args);

    const QuickLZType * t = get_type(this->args);

    static_qlz_compressor = t->compressor;
    static_qlz_decompressor = t->decompressor;
}
void QuickLZ::cleanup()
{
    if(this->qlz_compression_states)
    {
        for(unsigned i = 0; i < this->threads_no; ++i)
        {
            delete[] this->qlz_compression_states[i];
        }
        delete[] this->qlz_compression_states;
        this->qlz_compression_states = 0;
    }
    if(this->qlz_decompression_states)
    {
        for(unsigned i = 0; i < this->threads_no; ++i)
        {
            delete[] this->qlz_decompression_states[i];
        }
        delete[] this->qlz_decompression_states;
        this->qlz_decompression_states = 0;
    }
}
string QuickLZ::help() const
{
    const map<const string, QuickLZType*> & types = qlz_types();
    string ret = this->name + " " + this->version + "\nAvaivable parameters: \n";
    for(map<const string, QuickLZType*>::const_iterator it = types.begin(); it != types.end(); ++it)
    {
        ret += " ";
        ret += it->first;
        ret += "\n";
    }
    return ret;
}
void ** QuickLZ::eparams()
{
    return (void**)this->qlz_compression_states;
}
void ** QuickLZ::dparams()
{
    return (void**)this->qlz_decompression_states;
}
QuickLZ::qlz_compressor QuickLZ::static_qlz_compressor = 0;
QuickLZ::qlz_decompressor QuickLZ::static_qlz_decompressor = 0;
map<const string, QuickLZ::QuickLZType*> QuickLZ::QLZ_TYPES;

#endif //FSBENCH_USE_QUICKLZ
#ifdef FSBENCH_USE_TOR

#include "tornado/tor_test.h"

const string & Tor::default_args(const string & args)
{
    return args == "" ? default_mode : args;
}
size_t Tor::compressor(char * in, size_t isize, char * out, size_t, void * mode)
{
    return tor_compress(*(uintptr_t*)mode, (uint8_t*)in, (uint8_t*)out, isize);
}
size_t Tor::decompressor(char * in, size_t isize, char * out, size_t, void *)
{
    return tor_decompress((uint8_t*)in, (uint8_t*)out, isize);
}
void Tor::init(const string & args, unsigned threads_no, size_t, bool init_compressor, bool)
{
    if(init_compressor)
    {
        this->args = default_args(args);
        uintptr_t mode;
        try
        {
            from_string(this->args, mode);
        }
        catch(const bad_cast & bc)
        {
            throw InvalidParams(help());
        }

        if(mode < 1 || mode > 16)
        throw InvalidParams(help());
        this->params = new void *[threads_no];
        for(unsigned i=0; i<threads_no; ++i)
        {
            this->params[i] = (void*)mode;
        }
    }
}
void Tor::cleanup()
{
    if(this->params)
    delete[] this->params;
}
string Tor::help() const
{
    return this->name + " " + this->version + "\nAvaivable parameters: \n Numbers from 1 to 11\n";
}

void ** Tor::eparams()
{
    return params;
}
const string Tor::default_mode("6");

#endif// FSBENCH_USE_TOR
#ifdef FSBENCH_USE_YAPPY

#include "Yappy.hpp"

size_t Yappy::compressor(char * in, size_t isize, char * out, size_t, void * mode)
{
    return YappyCompress((ui8*)in, (ui8*)out, isize, *(uintptr_t*)mode) - (ui8*)out;
}
size_t Yappy::decompressor(char * in, size_t isize, char * out, size_t, void *)
{
    return YappyUnCompress((ui8*)in, (ui8*)(in+isize), (ui8*)out) - (ui8*)out;
}
Yappy::Yappy():
CodecWithIntModes("Yappy", _YAPPY_VERSION, compressor, decompressor, 1, 256, "10")
{
    YappyFillTables();
}
#endif// FSBENCH_USE_YAPPY

static inline uint16_t _bswap_16(uint16_t x) {
  return (x>>8) | (x<<8);
}

static inline uint32_t _bswap_32(uint32_t x) {
  return (_bswap_16(x&0xffff)<<16) | (_bswap_16(x>>16));
}

static inline uint64_t _bswap_64(uint64_t x) {
  return (((uint64_t)_bswap_32(x&0xffffffffull))<<32) | (_bswap_32(x>>32));
}

size_t c_bswap16(char * in, size_t isize, char *, size_t, void *)
{
    char * end = in + isize - sizeof(uint16_t);
    while (in < end)
    {
        *in = _bswap_16(*in);
        in += sizeof(uint16_t);
    }
    return isize;
}
size_t c_bswap32(char * in, size_t isize, char *, size_t, void *)
{
    char * end = in + isize - sizeof(uint32_t);
    while (in < end)
    {
        *in =  _bswap_32(*in);
        in += sizeof(uint32_t);
    }
    return isize;
}
size_t c_bswap64(char * in, size_t isize, char *, size_t, void *)
{
    char * end = in + isize - sizeof(uint64_t);
    while (in < end)
    {
        *in = _bswap_64(*in);
        in += sizeof(uint64_t);
    }
    return isize;
}
size_t c_memcpy(char * in, size_t isize, char *out, size_t, void *)
{
    memcpy(out, in, isize);
    return isize;
}
size_t c_memmove(char * in, size_t isize, char *out, size_t, void *)
{
    memmove(out, in, isize);
    return isize;
}
size_t c_memset(char * in, size_t isize, char *out, size_t, void *)
{
    memset(out, 0, isize);
    return isize;
}
