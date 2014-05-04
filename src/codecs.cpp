/**
 *Implementations of the Codec interface
 *
 * Written by m^2.
 * You can consider the code to be public domain.
 * If your country doesn't recognize author's right to relieve themselves of copyright,
 * you can use it under the terms of WTFPL version 2.0 or later.
 */

#include "codecs.hpp"
#include "simple_codecs.hpp"
#include "tools.hpp"

#include <algorithm>
#include <climits>

using namespace std;

//////////////////////////////////////////////////////
// Codecs that use some of the abstract classes
// Here they have functions used by their constructors
//////////////////////////////////////////////////////

#ifdef FSBENCH_USE_7Z
#include "7z/7z.hpp"
#endif//FSBENCH_USE_7Z
#ifdef FSBENCH_USE_BCL
#include "bcl.hpp"
#endif//FSBENCH_USE_BCL
#ifdef FSBENCH_USE_ECRYPT
#include "ecrypt/fsbench_ecrypt.hpp"
#endif//FSBENCH_USE_ECRYPT
#ifdef FSBENCH_USE_CRYPTOPP
#include "cryptopp/cryptopp.hpp"
#endif//FSBENCH_USE_CRYPTOPP
#ifdef FSBENCH_USE_GIPFELI
#include "fsbench_gipfeli.hpp"
#endif//FSBENCH_USE_GIPFELI
#ifdef FSBENCH_USE_MINIZ
#include "miniz.hpp"
#endif//FSBENCH_USE_MINIZ
#ifdef FSBENCH_USE_SCZ
#include "scz.hpp"
#endif// FSBENCH_USE_SCZ
#if defined(FSBENCH_USE_SHA3_RND1) || defined(FSBENCH_USE_SHA3_RND2) || defined(FSBENCH_USE_SHA3_RND3)
#include "fsbench_SHA3.hpp"
#endif// defined(FSBENCH_USE_SHA3_RND1) || defined(FSBENCH_USE_SHA3_RND2) || defined(FSBENCH_USE_SHA3_RND3)
#ifdef FSBENCH_USE_TINF
#include "tinf/tinf.hpp"
#endif//FSBENCH_USE_TINF
#ifdef FSBENCH_USE_Z3LIB
#include "z3lib/fsbench_z3lib.hpp"
#endif//FSBENCH_USE_Z3LIB
#ifdef FSBENCH_USE_ZLING
#include "zling/fsbench_zling.hpp"
#endif//FSBENCH_USE_ZLING
/////////////////////////////////
// Global variables 
// and helpers to manipulate them
/////////////////////////////////

// Just looks up a codec in CODECS
static Codec * raw_find_codec(const string & name)
{
    for (list<Codec*>::iterator it = CODECS.begin(); it != CODECS.end(); ++it)
    {
        if (!case_insensitive_compare((*it)->name, name))
        {
            return *it;
        }
    }
    return 0;
}


// Looks up a codec in CODECS.
// When it can't be found, but is a combination of other codecs,
// it adds it to CODECS
// The addition thing is here because it is needed to create a new codec
// But who is supposed to dispose it?
// Not the caller because it can't tell whether the pointer shows something created now or before
// The could be worked around, but I think that only in some ugly ways
// Not we, because the codec wouldn't live long enough
// This has some funny side effects like codec1/codec2/codec3 that works iff you did codec2/codec3 before
Codec * find_codec(const string & name)
{
    Codec * ret = raw_find_codec(name);
    if (!ret)
    {
        // maybe it's a pipeline of existing codecs?
        string::size_type pos = name.find('+');
        if (pos != string::npos)
        {
            string first_name = name.substr(0, pos);
            string second_name = name.substr(pos + 1);
            Codec * first = find_codec(first_name);
            Codec * second = find_codec(second_name);
            if (first && second)
            {
                // good, it's a pipeline of existing codecs
                PipelineCodec * codec = new PipelineCodec(*first, *second);
                CODECS.push_back(codec);
                ret = codec;
            }
        }
        else
        {
            // maybe it's a combination of some existing encoder and decoder?
            string::size_type pos = name.find('/');
            if (pos != string::npos)
            {
                string encoder_name = name.substr(0, pos);
                string decoder_name = name.substr(pos + 1);
                Codec * encoder = raw_find_codec(encoder_name);
                Codec * decoder = raw_find_codec(decoder_name);
                if (encoder && decoder)
                {
                    // good, it's a combination of existing codecs
                    CombinationCodec * codec = new CombinationCodec(*encoder, *decoder);
                    CODECS.push_back(codec);
                    ret = codec;
                }
            }
        }
    }
    return ret;
}

#ifdef FSBENCH_USE_FASTLZ
static const CodecArgs fastlz_args[] =
{
    CodecArgs("1", fastlz1_c, fastlz_d),
    CodecArgs("2", fastlz2_c, fastlz_d)};
#endif
#ifdef FSBENCH_USE_LZF
static const CodecArgs LZF_args[] =
{
    CodecArgs("", LZF_c, LZF_d),
    CodecArgs("very", LZF_very_c, LZF_d),
    CodecArgs("ultra", LZF_ultra_c, LZF_d)
};
#endif
#ifdef FSBENCH_USE_RLE64
static const CodecArgs RLE64_args[] =
{
    CodecArgs("8", RLE8_c, RLE8_d),
    CodecArgs("16", RLE16_c, RLE16_d),
    CodecArgs("32", RLE32_c, RLE32_d),
    CodecArgs("64", RLE64_c, RLE64_d)
};
#endif

Codec * codecs[] =
            {
#ifdef FSBENCH_USE_FASTLZ
              new MultifunctionCodec(
                      "fastlz", _FASTLZ_VERSION,
                      fastlz_args,
                      2,
                      "1",
                      fastlz_m),
#endif
#ifdef FSBENCH_USE_LZF
              new MultifunctionCodec(
                      "LZF", _LZF_VERSION,
                      LZF_args,
                      3,
                      "",
                      no_blowup),
#endif
#ifdef FSBENCH_USE_RLE64
              new MultifunctionCodec(
                      "RLE64", _RLE64_VERSION,
                      RLE64_args,
                      4,
                      "64"),
#endif
#ifdef FSBENCH_USE_7Z
              new CodecWithIntModes("7z-deflate", _7z_VERSION, FsBench7z::deflate, FsBench7z::inflate, 1, 9, "5", no_blowup),
              new CodecWithIntModes("7z-deflate64", _7z_VERSION, FsBench7z::deflate64, FsBench7z::inflate64, 1, 9, "5", no_blowup),
#endif
#ifdef FSBENCH_USE_BLOSC
              new CodecWithIntModes("blosc", _BLOSC_VERSION, blosc_c, blosc_d, 1, 9, "5", no_blowup),
#endif
#ifdef FSBENCH_USE_BZ2
              new CodecWithIntModes("bzip2", _BZ2_VERSION, bz2_c, bz2_d, 1, 9, "9", no_blowup),
#endif
#ifdef FSBENCH_USE_CRUSH
              new CodecWithIntModes("crush", _CRUSH_VERSION, crush_c, crush_d, 0, 2, "0"),
#endif
#ifdef FSBENCH_USE_CRYPTOPP
              new CodecWithIntModes("cryptopp-deflate", _CRYPTOPP_VERSION, FsBenchCryptoPP::deflate, FsBenchCryptoPP::inflate, 1, 9, "6", no_blowup),
#endif
#ifdef FSBENCH_USE_LODEPNG
              new CodecWithIntModes("lodepng", _LODEPNG_VERSION, lodepng_c, lodepng_d, 64, 32768, "32768", no_blowup),
#endif
#ifdef FSBENCH_USE_LZG 
              new CodecWithIntModes("lzg", _LZG_VERSION, LZG_c, LZG_d, 1, 9, "5", LZG_m),
#endif
#ifdef FSBENCH_USE_LZP_DS
              new CodecWithIntModes("lzp_ds", _LZP_DS_VERSION, lzp_ds_c, lzp_ds_d, 1, 512, "32"),
#endif
#ifdef FSBENCH_USE_LZX_COMPRESS
              new CodecWithIntModes("lzx_compress", _LZX_VERSION, LZX_c, 0, 15, 21, "21"),
#endif
#ifdef FSBENCH_USE_MINIZ
              new CodecWithIntModes("miniz", _MINIZ_VERSION, miniz_c, miniz_d, 1, 10, "6", no_blowup),
#endif
#ifdef FSBENCH_USE_PG_LZ
              new CodecWithIntModes("pg_lz", _PG_LZ_VERSION, pg_lz_c, pg_lz_d, 0, 1, "0", no_blowup),
#endif
#ifdef FSBENCH_USE_ZLIB
              new CodecWithIntModes("zlib", _ZLIB_VERSION, zlib_c, zlib_d, 1, 9, "6", no_blowup),
#endif
#ifdef FSBENCH_USE_ZLING
              new CodecWithIntModes("zling", _ZLING_VERSION, FsBenchZling::zling_c, FsBenchZling::zling_d, 0, 4, "0", no_blowup),
#endif
#ifdef FSBENCH_USE_ZOPFLI
              // I say no_blowup because while zopfli doesn't control its overhead,
              // it uses auxiliary arrays, so won't write outside of the regular ones
              new CodecWithIntModes("zopfli", _ZOPFLI_VERSION, zopfli_c, 0, 1, 1000000, "15", no_blowup),
#endif
#if 0
#ifdef FSBENCH_USE_7Z
              new Codec("7z-lzx", _7z_VERSION, 0, FsBench7z::unlzx, no_blowup),
#endif
#endif
#ifdef FSBENCH_USE_AR
              new Codec("ar", _AR_VERSION, FsBenchAr::ar_c, FsBenchAr::ar_d, no_blowup),
#endif
#ifdef FSBENCH_USE_BCL
              new Codec("bcl-huffman", _BCL_VERSION, FsBenchBCL::huffman, FsBenchBCL::unhuffman, FsBenchBCL::huffman_maxsize),
              new Codec("bcl-lz", _BCL_VERSION, FsBenchBCL::lz, FsBenchBCL::unlz, FsBenchBCL::lz_maxsize),
              new Codec("bcl-rle", _BCL_VERSION, FsBenchBCL::rle, FsBenchBCL::unrle, FsBenchBCL::rle_maxsize),
              new FsBenchBCL::LZFast(),
#endif
#ifdef FSBENCH_USE_DOBOZ
              new Codec("Doboz", _DOBOZ_VERSION, Doboz_c, Doboz_d, no_blowup),
#endif
#ifdef FSBENCH_USE_FSE
              new Codec("fse",   _FSE_VERSION,   fse_c,   fse_d,   fse_m),
#endif
#ifdef FSBENCH_USE_GIPFELI
              new Codec("gipfeli", _GIPFELI_VERSION, FsBenchGipfeli::compress, FsBenchGipfeli::decompress, FsBenchGipfeli::max_size),
#endif
#ifdef FSBENCH_USE_HALIBUT
              new Codec("Halibut-deflate", _HALIBUT_VERSION, halibut_c, halibut_d, no_blowup),
#endif
#ifdef FSBENCH_USE_LZ4
              new Codec("LZ4", _LZ4_VERSION, LZ4_c, LZ4_d_fast, no_blowup),
              new Codec("LZ4hc", _LZ4_VERSION, LZ4hc_c, LZ4_d_fast, no_blowup),
              new Codec("LZ4safe", _LZ4_VERSION, 0, LZ4_d_safe, no_blowup),
#endif
#ifdef FSBENCH_USE_LZFX
              new Codec("LZFX", _LZFX_VERSION, LZFX_c, LZFX_d),
#endif
#ifdef FSBENCH_USE_ZFS
              new Codec("lzjb", _ZFS_VERSION, LZJB_c, LZJB_d, no_blowup),
#endif
#ifdef FSBENCH_USE_LZMAT
              new Codec("lzmat", _LZMAT_VERSION, lzmat_c, lzmat_d, no_blowup),
#endif
#ifdef FSBENCH_USE_LZV1
              new BufferedCodec("LZV1", _LZV1_VERSION, LZV1_c, LZV1_d, no_blowup, sizeof(ush) * 16384, sizeof(ush)),
#endif
#ifdef FSBENCH_USE_LZWC
              new Codec("LZWC", _LZWC_VERSION, LZWC_c, LZWC_d, no_blowup),
#endif
#ifdef FSBENCH_USE_MMINI
              new BufferedCodec("mmini_huffman", _MMINI_VERSION, mmini_huffman_c, mmini_huffman_d, no_blowup, MMINI_HUFFHEAP_SIZE),
              new Codec("mmini_lzl", _MMINI_VERSION, mmini_lzl_c, mmini_lzl_d, no_blowup),
#endif
#ifdef FSBENCH_USE_NAKAMICHI
              new Codec("Nakamichi",      _NAKAMICHI_VERSION,      nakamichi_c,      nakamichi_d),
#endif
#ifdef FSBENCH_USE_NOBUO_ITO_LZSS
              new Codec("Nobuo-Ito-LZSS", _NOBUO_ITO_LZSS_VERSION, nobuo_ito_lzss_c, nobuo_ito_lzss_d),
#endif
#ifdef FSBENCH_USE_QUICKLZZIP
              new Codec("QuickLZ-zip", _QLZZIP_VERSION, qlzzip_c, 0),
#endif
#ifdef FSBENCH_USE_SHRINKER
              new Codec("Shrinker", _SHRINKER_VERSION, Shrinker_c, Shrinker_d, Shrinker_m),
#endif
#ifdef FSBENCH_USE_SNAPPY
              new Codec("Snappy", _SNAPPY_VERSION, snappy_c, snappy_d, snappy_m),
#endif
#ifdef FSBENCH_USE_TINF
              new Codec("tinf", _TINF_VERSION, 0, FsBenchTinf::inflate, no_blowup),
#endif
#ifdef FSBENCH_USE_Z3LIB
              new BufferedCodec("z3lib", _Z3LIB_VERSION, FsBenchZ3Lib::z3lib_c, FsBenchZ3Lib::z3lib_d, no_blowup, FsBenchZ3Lib::mem_size),
#endif
#ifdef FSBENCH_USE_NRV
              new Nrv("b"),
              new Nrv("d"),
              new Nrv("e"),
#endif
#ifdef FSBENCH_USE_BLZ
              new BriefLZ(),
#endif
#ifdef FSBENCH_USE_LZHAM
              new LZHAM(),
#endif
#ifdef FSBENCH_USE_LZO
              new LZO(),
#endif
#ifdef FSBENCH_USE_LZSS_IM
              new LZSS_IM(),
#endif
#ifdef FSBENCH_USE_QUICKLZ
              new QuickLZ(),
#endif
#ifdef FSBENCH_USE_SCZ
              new Scz(),
#endif
#ifdef FSBENCH_USE_TOR
              new Tor(),
#endif
#ifdef FSBENCH_USE_YAPPY
              new Yappy,
#endif
#ifdef FSBENCH_USE_BLAKE2
              new Checksum<64>("Blake2b",  _BLAKE2_VERSION, fsbench_blake2b),
              new Checksum<64>("Blake2bp", _BLAKE2_VERSION, fsbench_blake2bp),
              new Checksum<32>("Blake2s",  _BLAKE2_VERSION, fsbench_blake2s),
              new Checksum<32>("Blake2sp", _BLAKE2_VERSION, fsbench_blake2sp),
#endif
#ifdef FSBENCH_USE_CITYHASH
              new Checksum<    sizeof(uint32_t)>("CityHash32",  _CITYHASH_VERSION, CityHash32),
              new Checksum<    sizeof(uint64_t)>("CityHash64",  _CITYHASH_VERSION, CityHash64),
              new Checksum<2 * sizeof(uint64_t)>("CityHash128", _CITYHASH_VERSION, CityHash128),
#endif
#ifdef FSBENCH_USE_CRAPWOW
              new Checksum<sizeof(unsigned int)>("CrapWow", _CRAPWOW_VERSION, CrapWow),
#endif
#ifdef FSBENCH_USE_CRYPTOPP
              new Checksum<sizeof(uint32_t)>("cryptopp-adler32", _CRYPTOPP_VERSION, FsBenchCryptoPP::adler32),
              new Checksum<sizeof(uint32_t)>("cryptopp-crc32",   _CRYPTOPP_VERSION, FsBenchCryptoPP::crc),
              new Checksum<128/CHAR_BIT> ("cryptopp-md5",        _CRYPTOPP_VERSION, FsBenchCryptoPP::md5),
              new Checksum<224/CHAR_BIT> ("cryptopp-sha224",     _CRYPTOPP_VERSION, FsBenchCryptoPP::sha224),
              new Checksum<256/CHAR_BIT> ("cryptopp-sha256",     _CRYPTOPP_VERSION, FsBenchCryptoPP::sha256),
              new Checksum<384/CHAR_BIT> ("cryptopp-sha384",     _CRYPTOPP_VERSION, FsBenchCryptoPP::sha384),
              new Checksum<512/CHAR_BIT> ("cryptopp-sha512",     _CRYPTOPP_VERSION, FsBenchCryptoPP::sha512),
#endif
#ifdef FSBENCH_USE_FASTCRYPTO
              new Checksum<64/CHAR_BIT> ("uhash", _FASTCRYPTO_VERSION, FsBenchFastCrypto::uhash),
              new Checksum<128/CHAR_BIT>("vhash", _FASTCRYPTO_VERSION, FsBenchFastCrypto::vhash),
              new Checksum<64/CHAR_BIT> ("umac",  _FASTCRYPTO_VERSION, FsBenchFastCrypto::umac),
              new Checksum<128/CHAR_BIT>("vmac",  _FASTCRYPTO_VERSION, FsBenchFastCrypto::vmac),
#endif
#ifdef FSBENCH_USE_MURMUR
              new Checksum<sizeof(uint32_t)>("murmur3_x86_32", _MURMUR_VERSION, murmur_x86_32),
              new Checksum<128/CHAR_BIT>("murmur3_x86_128", _MURMUR_VERSION, murmur_x86_128),
              new Checksum<128/CHAR_BIT>("murmur3_x64_128", _MURMUR_VERSION, murmur_x64_128),
#endif
#ifdef FSBENCH_USE_SIPHASH
              new Checksum<8>("SipHash24", _SIPHASH_VERSION, siphash),
#endif
#ifdef FSBENCH_USE_SPOOKY
              new Checksum<2 * sizeof(uint64_t)>("SpookyHash", _SPOOKY_VERSION, spooky),
#endif
#ifdef FSBENCH_USE_SANMAYCE_FNV
              new Checksum<sizeof(uint32_t)>("FNV1a-Jesteress", _SANMAYCE_VERSION, fnv1_jesteress),
              new Checksum<sizeof(uint32_t)>("FNV1a-Mantis", _SANMAYCE_VERSION, fnv1_mantis),
              new Checksum<sizeof(uint32_t)>("FNV1a-Meiyan", _SANMAYCE_VERSION, fnv1_meiyan),
              new Checksum<sizeof(uint64_t)>("FNV1a-Tesla", _SANMAYCE_VERSION, fnv1_tesla),
              new Checksum<sizeof(uint64_t)>("FNV1a-Tesla3", _SANMAYCE_VERSION, fnv1_tesla3),
              new Checksum<sizeof(uint32_t)>("FNV1a-Yorikke", _SANMAYCE_VERSION, fnv1_yorikke),
              new Checksum<sizeof(uint32_t)>("FNV1a-YoshimitsuTRIAD", _SANMAYCE_VERSION, fnv1_yoshimitsu_triad),
              new Checksum<sizeof(uint64_t)>("FNV1a-Yoshimura", _SANMAYCE_VERSION, fnv1_yoshimura),
#   ifdef FSBENCH_SSE2
              new Checksum<sizeof(uint32_t)>("FNV1a-YoshimitsuTRIADiiXMM", _SANMAYCE_VERSION, fnv1_yoshimitsu_triad_iixmm),
              new Checksum<sizeof(uint32_t)>("FNV1a-penumbra", _SANMAYCE_VERSION, fnv1_penumbra),
#   endif
#endif
#if defined(FSBENCH_USE_SHA3_RND1) || defined(FSBENCH_USE_SHA3_RND2) || defined(FSBENCH_USE_SHA3_RND3) || defined(FSBENCH_USE_SHA3_RND3_GROESTL)
#   define QUOTE(x) #x
#   define SHA3_CHECKSUM(name, digest_size, version)\
    new Checksum<digest_size/CHAR_BIT>(QUOTE(name##digest_size),              version,            FsBenchSHA3::name::fsbench_hash_##digest_size),
#   ifdef FSBENCH_USE_SHA3_RND1
#   define EDON_R(digest_size)\
    new Checksum<digest_size/CHAR_BIT>("Edon-R" QUOTE(digest_size),           _EDON_R_VERSION,    FsBenchSHA3::Edon_R::fsbench_hash_##digest_size),
              EDON_R(224)
              EDON_R(256)
              EDON_R(384)
              EDON_R(512)
              SHA3_CHECKSUM(SWIFFTX, 224, _SHA3_RND1_VERSION)
              SHA3_CHECKSUM(SWIFFTX, 256, _SHA3_RND1_VERSION)
              SHA3_CHECKSUM(SWIFFTX, 384, _SHA3_RND1_VERSION)
              SHA3_CHECKSUM(SWIFFTX, 512, _SHA3_RND1_VERSION)
#   endif
#   ifdef FSBENCH_USE_SHA3_RND2
#   define BMW2(digest_size)\
    new Checksum<digest_size/CHAR_BIT>("BlueMidnightWish" QUOTE(digest_size), _SHA3_RND2_VERSION, FsBenchSHA3::BlueMidnightWish2::fsbench_hash_##digest_size),
              BMW2(224)
              BMW2(256)
              SHA3_CHECKSUM(BlueMidnightWish, 384, _SHA3_RND1_VERSION)
              SHA3_CHECKSUM(BlueMidnightWish, 512, _SHA3_RND1_VERSION)

#       if defined(FSBENCH_SSE2)
              SHA3_CHECKSUM(CubeHash, 224, _SHA3_RND1_VERSION)
              SHA3_CHECKSUM(CubeHash, 256, _SHA3_RND1_VERSION)
              SHA3_CHECKSUM(CubeHash, 384, _SHA3_RND1_VERSION)
              SHA3_CHECKSUM(CubeHash, 512, _SHA3_RND1_VERSION)
#       endif
#   endif
#   ifdef FSBENCH_USE_SHA3_RND3
              SHA3_CHECKSUM(Blake, 224, _SHA3_VERSION)
              SHA3_CHECKSUM(Blake, 256, _SHA3_VERSION)
              SHA3_CHECKSUM(Blake, 384, _SHA3_VERSION)
              SHA3_CHECKSUM(Blake, 512, _SHA3_VERSION)
              SHA3_CHECKSUM(Keccak, 224, _KECCAK_VERSION)
              SHA3_CHECKSUM(Keccak, 256, _KECCAK_VERSION)
              SHA3_CHECKSUM(Keccak, 384, _KECCAK_VERSION)
              SHA3_CHECKSUM(Keccak, 512, _KECCAK_VERSION)
              SHA3_CHECKSUM(JH, 224, _SHA3_VERSION)
              SHA3_CHECKSUM(JH, 256, _SHA3_VERSION)
              SHA3_CHECKSUM(JH, 384, _SHA3_VERSION)
              SHA3_CHECKSUM(JH, 512, _SHA3_VERSION)
              SHA3_CHECKSUM(Skein, 224, _SHA3_VERSION)
              SHA3_CHECKSUM(Skein, 256, _SHA3_VERSION)
              SHA3_CHECKSUM(Skein, 384, _SHA3_VERSION)
              SHA3_CHECKSUM(Skein, 512, _SHA3_VERSION)
              SHA3_CHECKSUM(Skein, 1024, _SHA3_VERSION)
#   endif
#   ifdef FSBENCH_USE_SHA3_RND3_GROESTL
              SHA3_CHECKSUM(Groestl, 224, _SHA3_VERSION)
              SHA3_CHECKSUM(Groestl, 256, _SHA3_VERSION)
              SHA3_CHECKSUM(Groestl, 384, _SHA3_VERSION)
              SHA3_CHECKSUM(Groestl, 512, _SHA3_VERSION)
#   endif
#endif // defined(FSBENCH_USE_SHA3_RND1) || defined(FSBENCH_USE_SHA3_RND2) || defined(FSBENCH_USE_SHA3_RND3)
#ifdef FSBENCH_USE_XXHASH
              new Checksum<sizeof(unsigned int)>("xxhash", _XXHASH_VERSION, xxhash),
#endif
#ifdef FSBENCH_USE_ZFS
              new Checksum<4 * sizeof(uint64_t)>("fletcher2", _ZFS_VERSION, fletcher2),
              new Checksum<4 * sizeof(uint64_t)>("fletcher4", _ZFS_VERSION, fletcher4),
#endif
#ifdef FSBENCH_USE_ECRYPT
              new FsBenchECrypt::AES128Bernstein(),
              new FsBenchECrypt::AES256Hongjun(),
              new FsBenchECrypt::ChaCha(),
              new FsBenchECrypt::HC128(),
              new FsBenchECrypt::HC256(),
              new FsBenchECrypt::Lex(),
              new FsBenchECrypt::Rabbit(),
              new FsBenchECrypt::RC4(),
              new FsBenchECrypt::Salsa20_8(),
              new FsBenchECrypt::Salsa20_12(),
              new FsBenchECrypt::Salsa20(),
              new FsBenchECrypt::Snow2(),
              new FsBenchECrypt::Sosemanuk(),
              new FsBenchECrypt::Trivium(),
#endif
              new Codec("nop",     "0", nop_c,     nop_d,     no_blowup, Codec::in_place, Codec::in_place),
              new Codec("bswap16", "0", c_bswap16, c_bswap16, no_blowup, Codec::in_place, Codec::in_place),
              new Codec("bswap32", "0", c_bswap32, c_bswap32, no_blowup, Codec::in_place, Codec::in_place),
              new Codec("bswap64", "0", c_bswap64, c_bswap64, no_blowup, Codec::in_place, Codec::in_place),
              new Codec("memcpy",  "0", c_memcpy,  c_memcpy,  no_blowup, Codec::moving,   Codec::moving),
              new Codec("memmove", "0", c_memmove, c_memmove, no_blowup, Codec::moving,   Codec::moving) };

list<Codec*> CODECS = list<Codec*>(codecs, codecs + sizeof(codecs) / sizeof(Codec*));

static list<CodecWithParams> createParamsList(list<pair<Codec*, const string> > lst)
{
    list<CodecWithParams> ret;
    for (list<pair<Codec*, const string> >::iterator it = lst.begin(); it != lst.end(); ++it)
    {
        if (it->first)
            ret.push_back(CodecWithParams(*(it->first), it->second));
    }
    return ret;
}
#define MKLIST(name, array) list<CodecWithParams> (name) = createParamsList(list< pair<Codec*,const string> >((array), (array) + sizeof((array)) / sizeof(pair<Codec*,const string>)))

static const pair<Codec*, const string> default_codecs[] =
    { make_pair(raw_find_codec("LZ4"), ""),
      make_pair(raw_find_codec("LZO"), ""),
      make_pair(raw_find_codec("QuickLZ"), ""),
      make_pair(raw_find_codec("Snappy"), "") };
MKLIST(DEFAULT_CODECS, default_codecs);

static const pair<Codec*, const string> fast_compressors[] =
    { make_pair(raw_find_codec("bcl-rle"), ""),
      make_pair(raw_find_codec("blosc"), ""),
      make_pair(raw_find_codec("fastlz"), ""),
      make_pair(raw_find_codec("LZ4"), ""),
      make_pair(raw_find_codec("LZF"), "very"),
      make_pair(raw_find_codec("LZJB"), ""),
      make_pair(raw_find_codec("LZO"), ""),
      make_pair(raw_find_codec("QuickLZ"), ""),
      make_pair(raw_find_codec("RLE64"), ""),
      make_pair(raw_find_codec("Shrinker"), ""),
      make_pair(raw_find_codec("Snappy"), "") };
MKLIST(FAST_COMPRESSORS, fast_compressors);

static const pair<Codec*, const string> all_compressors[] =
    { make_pair(raw_find_codec("7z-deflate"), ""),
      make_pair(raw_find_codec("7z-deflate64"), ""),
      make_pair(raw_find_codec("ar"), ""),
      make_pair(raw_find_codec("bcl-huffman"), ""),
      make_pair(raw_find_codec("bcl-lzfast"), ""),
      make_pair(raw_find_codec("bcl-rle"), ""),
      make_pair(raw_find_codec("blosc"), ""),
      make_pair(raw_find_codec("BriefLZ"), ""),
      make_pair(raw_find_codec("bzip2"), ""),
      make_pair(raw_find_codec("crush"), ""),
      make_pair(raw_find_codec("cryptopp-deflate"), ""),
      make_pair(raw_find_codec("Doboz"), ""),
      make_pair(raw_find_codec("fastlz"), ""),
      make_pair(raw_find_codec("fse"), ""),
      make_pair(raw_find_codec("gipfeli"), ""),
      make_pair(raw_find_codec("halibut-deflate"), ""),
      make_pair(raw_find_codec("lodepng"), ""),
      make_pair(raw_find_codec("LZ4"), ""),
      make_pair(raw_find_codec("LZ4hc"), ""),
      make_pair(raw_find_codec("LZF"), ""),
      make_pair(raw_find_codec("LZFX"), ""),
      make_pair(raw_find_codec("lzg"), ""),
      make_pair(raw_find_codec("LZHAM"), ""),
      make_pair(raw_find_codec("LZJB"), ""),
      make_pair(raw_find_codec("lzmat"), ""),
      make_pair(raw_find_codec("LZO"), ""),
      make_pair(raw_find_codec("LZSS-IM"), ""),
      make_pair(raw_find_codec("lzv1"), ""),
      make_pair(raw_find_codec("lzwc"), ""),
      make_pair(find_codec("lzx_compress/nop"), ""),
      make_pair(raw_find_codec("miniz"), ""),
      make_pair(raw_find_codec("mmini_huffman"), ""),
      make_pair(raw_find_codec("mmini_lzl"), ""),
      make_pair(raw_find_codec("Nakamichi"), ""),
      make_pair(raw_find_codec("Nobuo-Ito-LZSS"), ""),
      make_pair(raw_find_codec("nrv2b"), ""),
      make_pair(raw_find_codec("nrv2d"), ""),
      make_pair(raw_find_codec("nrv2e"), ""),
      make_pair(raw_find_codec("pg_lz"), ""),
      make_pair(raw_find_codec("QuickLZ"), ""),
      make_pair(find_codec("QuickLZ-zip/zlib"), ""),
      make_pair(raw_find_codec("RLE64"), ""),
      make_pair(raw_find_codec("scz"), ""),
      make_pair(raw_find_codec("Shrinker"), ""),
      make_pair(raw_find_codec("Snappy"), ""),
      make_pair(find_codec("zlib/tinf"), ""),
      make_pair(raw_find_codec("Tornado"), ""),
      make_pair(raw_find_codec("Yappy"), ""),
      make_pair(raw_find_codec("z3lib"), ""),
      make_pair(raw_find_codec("zlib"), ""),
      make_pair(raw_find_codec("zling"), ""),
      make_pair(find_codec("zopfli/zlib"), "")
        };
MKLIST(ALL_COMPRESSORS, all_compressors);

static const pair<Codec*, const string> all_ciphers[] =
    { make_pair(raw_find_codec("AES128Bernstein"), ""),
      make_pair(raw_find_codec("AES256Hongjun"), ""),
      make_pair(raw_find_codec("ChaCha"), ""),
      make_pair(raw_find_codec("HC128"), ""),
      make_pair(raw_find_codec("HC256"), ""),
      make_pair(raw_find_codec("Lex"), ""),
      make_pair(raw_find_codec("Rabbit"), ""),
      make_pair(raw_find_codec("RC4"), ""),
      make_pair(raw_find_codec("Salsa20_8"), ""),
      make_pair(raw_find_codec("Salsa20_12"), ""),
      make_pair(raw_find_codec("Salsa20"), ""),
      make_pair(raw_find_codec("Snow2"), ""),
      make_pair(raw_find_codec("Sosemanuk"), ""),
      make_pair(raw_find_codec("Trivium"), "")
        };
MKLIST(ALL_CIPHERS, all_ciphers);

static const pair<Codec*, const string> all_checksums[] =
        { make_pair(raw_find_codec("Blake2b"), ""),
          make_pair(raw_find_codec("Blake2bp"), ""),
          make_pair(raw_find_codec("Blake2s"), ""),
          make_pair(raw_find_codec("Blake2sp"), ""),
          make_pair(raw_find_codec("CityHash32"), ""),
          make_pair(raw_find_codec("CityHash64"), ""),
          make_pair(raw_find_codec("CityHash128"), ""),
          make_pair(raw_find_codec("CrapWow"), ""),
          make_pair(raw_find_codec("cryptopp-adler32"), ""),
          make_pair(raw_find_codec("cryptopp-crc32"), ""),
          make_pair(raw_find_codec("cryptopp-md5"), ""),
          make_pair(raw_find_codec("cryptopp-sha224"), ""),
          make_pair(raw_find_codec("cryptopp-sha256"), ""),
          make_pair(raw_find_codec("cryptopp-sha384"), ""),
          make_pair(raw_find_codec("cryptopp-sha512"), ""),
          make_pair(raw_find_codec("uhash"), ""),
          make_pair(raw_find_codec("vhash"), ""),
          make_pair(raw_find_codec("umac"), ""),
          make_pair(raw_find_codec("vmac"), ""),
          make_pair(raw_find_codec("SipHash24"), ""),
          make_pair(raw_find_codec("murmur3_x86_32"), ""),
          make_pair(raw_find_codec("murmur3_x86_128"), ""),
          make_pair(raw_find_codec("murmur3_x64_128"), ""),
          make_pair(raw_find_codec("SpookyHash"), ""),
          make_pair(raw_find_codec("FNV1a-Jesteress"), ""),
          make_pair(raw_find_codec("FNV1a-Mantis"), ""),
          make_pair(raw_find_codec("FNV1a-Meiyan"), ""),
          make_pair(raw_find_codec("FNV1a-Penumbra"), ""),
          make_pair(raw_find_codec("FNV1a-Tesla"), ""),
          make_pair(raw_find_codec("FNV1a-Tesla3"), ""),
          make_pair(raw_find_codec("FNV1a-Yorikke"), ""),
          make_pair(raw_find_codec("FNV1a-YoshimitsuTRIAD"), ""),
          make_pair(raw_find_codec("FNV1a-YoshimitsuTRIADiiXMM"), ""),
          make_pair(raw_find_codec("FNV1a-Yoshimura"), ""),
          make_pair(raw_find_codec("Edon-R224"), ""),
          make_pair(raw_find_codec("Edon-R256"), ""),
          make_pair(raw_find_codec("Edon-R384"), ""),
          make_pair(raw_find_codec("Edon-R512"), ""),
          make_pair(raw_find_codec("SWIFFTX224"), ""),
          make_pair(raw_find_codec("SWIFFTX256"), ""),
          make_pair(raw_find_codec("SWIFFTX384"), ""),
          make_pair(raw_find_codec("SWIFFTX512"), ""),
          make_pair(raw_find_codec("BlueMidnightWish224"), ""),
          make_pair(raw_find_codec("BlueMidnightWish256"), ""),
          make_pair(raw_find_codec("BlueMidnightWish384"), ""),
          make_pair(raw_find_codec("BlueMidnightWish512"), ""),
          make_pair(raw_find_codec("CubeHash224"), ""),
          make_pair(raw_find_codec("CubeHash256"), ""),
          make_pair(raw_find_codec("CubeHash384"), ""),
          make_pair(raw_find_codec("CubeHash512"), ""),
          make_pair(raw_find_codec("Blake224"), ""),
          make_pair(raw_find_codec("Blake256"), ""),
          make_pair(raw_find_codec("Blake384"), ""),
          make_pair(raw_find_codec("Blake512"), ""),
          make_pair(raw_find_codec("Keccak224"), ""),
          make_pair(raw_find_codec("Keccak256"), ""),
          make_pair(raw_find_codec("Keccak384"), ""),
          make_pair(raw_find_codec("Keccak512"), ""),
          make_pair(raw_find_codec("JH224"), ""),
          make_pair(raw_find_codec("JH256"), ""),
          make_pair(raw_find_codec("JH384"), ""),
          make_pair(raw_find_codec("JHX512"), ""),
          make_pair(raw_find_codec("Skein224"), ""),
          make_pair(raw_find_codec("Skein256"), ""),
          make_pair(raw_find_codec("Skein384"), ""),
          make_pair(raw_find_codec("Skein512"), ""),
          make_pair(raw_find_codec("Skein1024"), ""),
          make_pair(raw_find_codec("Groestl224"), ""),
          make_pair(raw_find_codec("Groestl256"), ""),
          make_pair(raw_find_codec("Groestl384"), ""),
          make_pair(raw_find_codec("Groestl512"), ""),
          make_pair(raw_find_codec("xxhash"), ""),
          make_pair(raw_find_codec("fletcher2"), ""),
          make_pair(raw_find_codec("fletcher4"), "")
        };
MKLIST(ALL_CHECKSUMS, all_checksums);


static const pair<Codec*, const string> all_others[] =
    { make_pair(raw_find_codec("bswap16"), ""),
      make_pair(raw_find_codec("bswap32"), ""),
      make_pair(raw_find_codec("bswap64"), ""),
      make_pair(raw_find_codec("memcpy"),  ""),
      make_pair(raw_find_codec("memmove"), ""),
      make_pair(raw_find_codec("nop"), "")
        };
MKLIST(ALL_OTHERS, all_others);
