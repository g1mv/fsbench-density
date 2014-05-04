###################
## CONFIGURATION ##
###################

# Codecs
# You can enable or disable compilation of individual codecs here
set(USE_7Z                1)
set(USE_AR                0) # inefficient. And I've seen an infinite loop
set(USE_BCL               0) # inefficient
set(USE_BLAKE2            0) # doesn't compile with MSVC10. Requires SSE2, so you need to enable it below too.
set(USE_BLOSC             0) # inefficient
set(USE_BLZ               0) # inefficient
set(USE_BZ2               0) # there are barely any competitors for it
set(USE_CITYHASH          1)
set(USE_CRAPWOW           0) # weak and while fast, there are much faster ones
set(USE_CRUSH             0) # Nothing great
set(USE_CRYPTOPP          0) # doesn't link on my FreeBSD
set(USE_DOBOZ             0) # inefficient
set(USE_ECRYPT            1)
set(USE_FASTCRYPTO        1)
set(USE_FASTLZ            0) # inefficient
set(USE_FSE               0) # crashes on scc/clang 3.1; decoding fails on anything with gcc 4.2 on my FreeBSD box
set(USE_GIPFELI           0) # rarely works...
set(USE_HALIBUT           0) # inefficient
set(USE_LODEPNG           0) # inefficient
set(USE_LZ4               1)
set(USE_LZF               1)
set(USE_LZFX              0) # inefficient
set(USE_LZG               0) # inefficient
set(USE_LZHAM             0) # known to cause problems on some platforms
set(USE_LZMAT             1)
set(USE_LZO               1)
set(USE_LZSS_IM           0) # inefficient
set(USE_LZV1              0) # inefficient
set(USE_LZWC              0) # inefficient
set(USE_LZX_COMPRESS      0) # inefficient, no decompressor
set(USE_MINIZ             1)
set(USE_MMINI             0) # doesn't compile with MSVC10
set(USE_MURMUR            1)
set(USE_NAKAMICHI         1)
set(USE_NRV               0) # inefficient
set(USE_PG_LZ             0) # inefficient
set(USE_QUICKLZ           1)
set(USE_QUICKLZZIP        0) # quite good, but single-threaded. I may fix it one day.
set(USE_RLE64             1)
set(USE_SANMAYCE_FNV      1)
set(USE_SCZ               0) # single threaded and far too inefficient to be worth fixing...
set(USE_SHA3_RND1         0) # didn't make it to the final...
set(USE_SHA3_RND2         0) # didn't make it to the final...
set(USE_SHA3_RND3         1)
set(USE_SHA3_RND3_GROESTL 0) # doesn't compile with MSVC10
set(USE_SHRINKER          0) # doesn't work on some platforms
set(USE_SIPHASH           1)
set(USE_SNAPPY            1)
set(USE_SPOOKY            1)
set(USE_TINF              0) # inefficient
set(USE_TOR               1)
set(USE_XXHASH            1)
set(USE_YAPPY             0) # Nothing great
set(USE_Z3LIB             0) # Nothing great
set(USE_ZFS               0) # Nothing great
set(USE_ZLIB              1)
set(USE_ZLING             1)
set(USE_ZOPFLI            1)

# This script can feed certain compilers with additional optimisation flags aside from the usual -O2 or so. Other compilers use the defaults.
# If you don't want the extra flags, set the variable below to 0
set(OPTIMIZE_COMPILER_FLAGS 0)

# Set it to 1 to enable SSE2 in codecs that support it.
# Currently it's used only by Yappy and on my machine it hurts speed slightly
# note: if you set it to 1, you may have to set CPU architecture in compiler flags
set(USE_SSE2 0)

# if 1, codecs can use inline asm
# please note that very few codecs support it and sometimes when a codec does, its implementation in fsbench doesn't
set(USE_ASM 0)

# Type of a build
set(CMAKE_BUILD_TYPE Release)

#############################################
## CODE, NORMALLY YOU SHOULDN'T EDIT BELOW ##
#############################################

cmake_minimum_required(VERSION 2.6)
project (FSBENCH) 


include_directories (${FSBENCH_SOURCE_DIR})
include_directories (${FSBENCH_SOURCE_DIR}/codecs)


#-------------------#
# platform-specific #
#-------------------#

# autodetect architecture
# fsbench was tested only on x86 and AMD64
# expect troubles elsewhere

IF(${CMAKE_C_SIZEOF_DATA_PTR} STREQUAL 4)
    set(ARCH "x86")
ELSEIF(${CMAKE_C_SIZEOF_DATA_PTR} STREQUAL 8)
    set(ARCH "AMD64")
ELSE(${CMAKE_C_SIZEOF_DATA_PTR} STREQUAL 4)
    message(FATAL_ERROR "CMAKE_C_SIZEOF_DATA_PTR is neither 4 nor 8. This is not supported")
ENDIF(${CMAKE_C_SIZEOF_DATA_PTR} STREQUAL 4)

IF(${ARCH} STREQUAL "x86")
    IF(USE_SSE2)
        add_definitions( -DFSBENCH_SSE2) 
    ENDIF(USE_SSE2)
ENDIF(${ARCH} STREQUAL "x86")

IF(${ARCH} STREQUAL "AMD64")
    IF(USE_SSE2)
        add_definitions( -DFSBENCH_SSE2) 
    ENDIF(USE_SSE2)
    add_definitions(-DFREEARC_INTEL_BYTE_ORDER)
    add_definitions(-D__x86_64__)
ENDIF(${ARCH} STREQUAL "AMD64")
IF(${ARCH} STREQUAL "x86")
    add_definitions(-DFREEARC_INTEL_BYTE_ORDER)
ENDIF(${ARCH} STREQUAL "x86")

IF(MSVC) # Cmake doesn't use the strongest optimisation flags here, so I fix it myself
    string(REPLACE "/O2" "/Ox" CMAKE_CXX_FLAGS_RELEASE
    "${CMAKE_CXX_FLAGS_RELEASE}")
ENDIF(MSVC)
set(GCC_FLAGS "-fno-tree-vectorize -static") # the code crashes when I compile it with mingw 4.5.2 and w/out -fno-tree-vectorize
                                             # with mingw64 4.6.2 it's fine. Don't know what's up. FIXME.
set(GXX_FLAGS ${GCC_FLAGS})
set(MSVC_FLAGS "/W4")

#--------------#
# optimization #
#--------------#

IF(USE_ASM)
    add_definitions(-DFSBENCH_USE_ASM)
ENDIF(USE_ASM)

IF(OPTIMIZE_COMPILER_FLAGS)
    #gcc optimization
    set(GCC_FLAGS "${GCC_FLAGS} -fomit-frame-pointer -ffast-math -march=native -mtune=generic")
    set(GCC_LINKFLAGS "${GCC_LINKFLAGS} -s")

    set(GXX_FLAGS "${GXX_FLAGS} ${GCC_FLAGS}")

    #msvc optimization
    set(MSVC_FLAGS "${MSVC_FLAGS} /GF /GL /Gs /Gy /fp:fast /Ot")
    set(MSVC_LINK_FLAGS "${MSVC_LINK_FLAGS} /LTCG")
    IF(${ARCH} STREQUAL "x86")
        set(MSVC_FLAGS "${MSVC_FLAGS} /arch:SSE2")
    ENDIF(${ARCH} STREQUAL "x86")
ENDIF(OPTIMIZE_COMPILER_FLAGS)

#---------#
# sources #
#---------#

set(MAIN_SRC abstractCodecs.cpp benchmark.cpp codecs.cpp main.cpp scheduler.cpp simple_codecs.cpp tools.cpp)

IF(USE_7Z)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/7z/CPP) 
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/7z/CPP/7zip) 
    set(7z_SRC 
        codecs/7z/7z.cpp

        codecs/7z/C/Alloc.c
        codecs/7z/C/HuffEnc.c
        codecs/7z/C/LzFind.c
        codecs/7z/C/Sort.c
        codecs/7z/CPP/7zip/Common/InBuffer.cpp
        codecs/7z/CPP/7zip/Common/OutBuffer.cpp
        codecs/7z/CPP/7zip/Compress/BitlDecoder.cpp
        codecs/7z/CPP/7zip/Compress/DeflateDecoder.cpp
        codecs/7z/CPP/7zip/Compress/DeflateEncoder.cpp
        codecs/7z/CPP/7zip/Compress/LzOutWindow.cpp

        # I didn't make the 2 below work
        #codecs/7z/CPP/7zip/Compress/Lzx86Converter.cpp
        #codecs/7z/CPP/7zip/Compress/LzxDecoder.cpp
        )

    add_definitions(-DFSBENCH_USE_7Z)
ENDIF(USE_7Z)

IF(USE_BCL)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/bcl/src)
    set(BCL_SRC 
        codecs/bcl/src/bcl.cpp

        codecs/bcl/src/huffman.c
        codecs/bcl/src/lz.c
        codecs/bcl/src/rle.c
        )

    add_definitions(-DFSBENCH_USE_BCL)
ENDIF(USE_BCL)

IF(USE_BZ2)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/bzip2-1.0.6)
    set(BZ2_SRC 
        codecs/bzip2-1.0.6/blocksort.c
        codecs/bzip2-1.0.6/bzlib.c
        codecs/bzip2-1.0.6/compress.c
        codecs/bzip2-1.0.6/crctable.c
        codecs/bzip2-1.0.6/decompress.c
        codecs/bzip2-1.0.6/huffman.c
        codecs/bzip2-1.0.6/randtable.c
        )

    add_definitions(-DFSBENCH_USE_BZ2)
ENDIF(USE_BZ2)

IF(USE_CRYPTOPP)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/cryptopp)
    set(CRYPTOPP_SRC 
        codecs/cryptopp/cryptopp.cpp

        codecs/cryptopp/adler32.cpp
        codecs/cryptopp/algparam.cpp
        codecs/cryptopp/cpu.cpp
        codecs/cryptopp/crc.cpp
        codecs/cryptopp/cryptlib.cpp
        codecs/cryptopp/filters.cpp
        codecs/cryptopp/fips140.cpp
        codecs/cryptopp/hrtimer.cpp
        codecs/cryptopp/iterhash.cpp
        codecs/cryptopp/md5.cpp
        codecs/cryptopp/misc.cpp
        codecs/cryptopp/mqueue.cpp
        codecs/cryptopp/queue.cpp
        codecs/cryptopp/sha.cpp
        codecs/cryptopp/vmac.cpp
        codecs/cryptopp/wait.cpp
        codecs/cryptopp/zdeflate.cpp
        codecs/cryptopp/zinflate.cpp)

    add_definitions(-DFSBENCH_USE_CRYPTOPP)
ENDIF(USE_CRYPTOPP)

IF(USE_ECRYPT)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/ecrypt/)
    set(ECRYPT_SRC
        codecs/ecrypt/fsbench_ecrypt.cpp

        codecs/ecrypt/aes-ctr-bernstein.c
        codecs/ecrypt/aes-ctr-hongjun.c
        codecs/ecrypt/chacha.c
        codecs/ecrypt/hc-128.c
        codecs/ecrypt/hc-256.c
        codecs/ecrypt/lex.c
        codecs/ecrypt/rabbit.c
        codecs/ecrypt/rc4.c
        codecs/ecrypt/salsa20-8.c
        codecs/ecrypt/salsa20-12.c
        codecs/ecrypt/salsa20.c
        codecs/ecrypt/snow-2.0.c
        codecs/ecrypt/sosemanuk.c
        codecs/ecrypt/trivium.c)
    add_definitions(-DFSBENCH_USE_ECRYPT)
ENDIF(USE_ECRYPT)

IF(USE_LZHAM)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/lzham/include) 
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/lzham/lzhamcomp)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/lzham/lzhamdecomp) 
    set(LZHAM_SRC 
        codecs/lzham/lzhamcomp/lzham_lzbase.cpp codecs/lzham/lzhamcomp/lzham_lzcomp.cpp codecs/lzham/lzhamcomp/lzham_lzcomp_internal.cpp 
        codecs/lzham/lzhamcomp/lzham_lzcomp_state.cpp codecs/lzham/lzhamcomp/lzham_match_accel.cpp codecs/lzham/lzhamcomp/lzham_pthreads_threading.cpp 
        codecs/lzham/lzhamdecomp/lzham_assert.cpp codecs/lzham/lzhamdecomp/lzham_checksum.cpp codecs/lzham/lzhamdecomp/lzham_huffman_codes.cpp 
        codecs/lzham/lzhamdecomp/lzham_lzdecomp.cpp codecs/lzham/lzhamdecomp/lzham_lzdecompbase.cpp codecs/lzham/lzhamdecomp/lzham_mem.cpp 
        codecs/lzham/lzhamdecomp/lzham_platform.cpp codecs/lzham/lzhamdecomp/lzham_polar_codes.cpp codecs/lzham/lzhamdecomp/lzham_prefix_coding.cpp 
        codecs/lzham/lzhamdecomp/lzham_symbol_codec.cpp codecs/lzham/lzhamdecomp/lzham_timer.cpp codecs/lzham/lzhamdecomp/lzham_vector.cpp 
        codecs/lzham/lzhamlib/lzham_lib.cpp)

    add_definitions(-DFSBENCH_USE_LZHAM)
ENDIF(USE_LZHAM)

IF(USE_LZO)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/lzo) 
    set(LZO_SRC 
        codecs/lzo/lzo1.c
        codecs/lzo/lzo1a.c codecs/lzo/lzo1a_99.c
        codecs/lzo/lzo1b_1.c codecs/lzo/lzo1b_2.c codecs/lzo/lzo1b_3.c codecs/lzo/lzo1b_4.c codecs/lzo/lzo1b_5.c codecs/lzo/lzo1b_6.c codecs/lzo/lzo1b_7.c
        codecs/lzo/lzo1b_8.c codecs/lzo/lzo1b_9.c codecs/lzo/lzo1b_99.c codecs/lzo/lzo1b_9x.c codecs/lzo/lzo1b_cc.c codecs/lzo/lzo1b_d1.c codecs/lzo/lzo1b_rr.c
        codecs/lzo/lzo1c_1.c codecs/lzo/lzo1c_2.c codecs/lzo/lzo1c_3.c codecs/lzo/lzo1c_4.c codecs/lzo/lzo1c_5.c codecs/lzo/lzo1c_6.c codecs/lzo/lzo1c_7.c
        codecs/lzo/lzo1c_8.c codecs/lzo/lzo1c_9.c codecs/lzo/lzo1c_99.c codecs/lzo/lzo1c_9x.c codecs/lzo/lzo1c_cc.c codecs/lzo/lzo1c_d1.c codecs/lzo/lzo1c_rr.c
        codecs/lzo/lzo1f_1.c codecs/lzo/lzo1f_9x.c codecs/lzo/lzo1f_d1.c
        codecs/lzo/lzo1x_1.c codecs/lzo/lzo1x_1k.c codecs/lzo/lzo1x_1l.c codecs/lzo/lzo1x_1o.c
        codecs/lzo/lzo1x_9x.c codecs/lzo/lzo1x_d1.c codecs/lzo/lzo1x_o.c 
        codecs/lzo/lzo1y_1.c codecs/lzo/lzo1y_9x.c codecs/lzo/lzo1y_d1.c codecs/lzo/lzo1y_o.c codecs/lzo/lzo1z_9x.c codecs/lzo/lzo1z_d1.c
        codecs/lzo/lzo1_99.c codecs/lzo/lzo2a_9x.c codecs/lzo/lzo2a_d1.c codecs/lzo/lzo_init.c)

    add_definitions(-DFSBENCH_USE_LZO)
ENDIF(USE_LZO)

IF(USE_NRV)
    set(NRV_SRC 
        codecs/ucl/alloc.c
        codecs/ucl/n2b_99.c codecs/ucl/n2b_d.c
        codecs/ucl/n2d_99.c codecs/ucl/n2d_d.c
        codecs/ucl/n2e_99.c codecs/ucl/n2e_d.c)

    add_definitions(-DFSBENCH_USE_NRV)
ENDIF(USE_NRV)

IF(USE_ZOPFLI)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/zopfli) 
    set(ZOPFLI_SRC
        codecs/zopfli/blocksplitter.c
        codecs/zopfli/cache.c
        codecs/zopfli/deflate.c
        codecs/zopfli/hash.c
        codecs/zopfli/katajainen.c
        codecs/zopfli/lz77.c
        codecs/zopfli/squeeze.c
        codecs/zopfli/tree.c
        codecs/zopfli/util.c)
    add_definitions(-DFSBENCH_USE_ZOPFLI)
ENDIF(USE_ZOPFLI)

IF(USE_ZLIB)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/zlib) 
    set(ZLIB_SRC 
        codecs/zlib/adler32.c
        codecs/zlib/crc32.c
        codecs/zlib/deflate.c
        codecs/zlib/inffast.c
        codecs/zlib/inflate.c
        codecs/zlib/inftrees.c
        codecs/zlib/trees.c
        codecs/zlib/zutil.c)

    add_definitions(-DFSBENCH_USE_ZLIB)

    add_definitions(-DNO_VIZ) # Lack of this definition causes troubles sometimes
ENDIF(USE_ZLIB)

IF(USE_ZLING)
    set(ZLING_SRC 
        codecs/zling/fsbench_zling.cpp

        codecs/zling/libzling.cpp
        codecs/zling/libzling_huffman.cpp
        codecs/zling/libzling_lz.cpp
        codecs/zling/libzling_utils.cpp)

    add_definitions(-DFSBENCH_USE_ZLING)
ENDIF(USE_ZLING)

IF(USE_AR)
    set(OTHER_SRC ${OTHER_SRC} codecs/ar/lzhclib.c codecs/ar/lzhxlib.c)
    add_definitions(-DFSBENCH_USE_AR)
ENDIF(USE_AR)

IF(USE_BLAKE2 AND USE_SSE2)
    # sadly, it's needed
    # Never tested on other compilers
    set(GCC_FLAGS "${GCC_FLAGS} -std=c99")

    include_directories (${FSBENCH_SOURCE_DIR}/codecs/blake2)
    set(OTHER_SRC ${OTHER_SRC} codecs/blake2/blake2b.c codecs/blake2/blake2bp.c codecs/blake2/blake2s.c codecs/blake2/blake2sp.c)
    add_definitions(-DFSBENCH_USE_BLAKE2)
ENDIF(USE_BLAKE2 AND USE_SSE2)

IF(USE_BLOSC)
    set(OTHER_SRC ${OTHER_SRC} codecs/blosc/blosclz.c)
    add_definitions(-DFSBENCH_USE_BLOSC)
ENDIF(USE_BLOSC)
IF(USE_BLZ)
    set(OTHER_SRC ${OTHER_SRC} codecs/brieflz/brieflz.c codecs/brieflz/blz_depack.c codecs/brieflz/blz_crc32.c)
    add_definitions(-DFSBENCH_USE_BLZ)
ENDIF(USE_BLZ)
IF(USE_CITYHASH)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/CityHash)
    set(OTHER_SRC ${OTHER_SRC} codecs/CityHash/city.cc)
    add_definitions(-DFSBENCH_USE_CITYHASH)
ENDIF(USE_CITYHASH)
IF(USE_CRAPWOW)
    set(OTHER_SRC ${OTHER_SRC} codecs/CrapWow.c)
    add_definitions(-DFSBENCH_USE_CRAPWOW)
ENDIF(USE_CRAPWOW)
IF(USE_CRUSH)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/crush) 
    set(OTHER_SRC ${OTHER_SRC} codecs/crush/crush.cpp)
    add_definitions(-DFSBENCH_USE_CRUSH)
ENDIF(USE_CRUSH)
IF(USE_DOBOZ)
    set(OTHER_SRC ${OTHER_SRC} codecs/Doboz/Compressor.cpp codecs/Doboz/Decompressor.cpp codecs/Doboz/Dictionary.cpp)
    add_definitions(-DFSBENCH_USE_DOBOZ)
ENDIF(USE_DOBOZ)
IF(USE_FASTCRYPTO)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/fastcrypto)
    set(OTHER_SRC ${OTHER_SRC} codecs/fastcrypto/rijndael-alg-fst.c codecs/fastcrypto/umac.c codecs/fastcrypto/vmac.c)
    add_definitions(-DFSBENCH_USE_FASTCRYPTO)
ENDIF(USE_FASTCRYPTO)
IF(USE_FASTLZ)
    set(OTHER_SRC ${OTHER_SRC} codecs/fastlz.c)
    add_definitions(-DFSBENCH_USE_FASTLZ)
ENDIF(USE_FASTLZ)
IF(USE_FSE)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/fse)
    set(OTHER_SRC ${OTHER_SRC} codecs/fse/fse.c)
    add_definitions(-DFSBENCH_USE_FSE)
ENDIF(USE_FSE)
IF(USE_GIPFELI)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/gipfeli)
    set(OTHER_SRC ${OTHER_SRC} codecs/gipfeli/fsbench_gipfeli.cpp codecs/gipfeli/decompress.cc codecs/gipfeli/entropy.cc codecs/gipfeli/entropy_code_builder.cc codecs/gipfeli/gipfeli.cc codecs/gipfeli/lz77.cc)
    add_definitions(-DFSBENCH_USE_GIPFELI)
ENDIF(USE_GIPFELI)
IF(USE_HALIBUT)
    set(OTHER_SRC ${OTHER_SRC} codecs/halibut-deflate.c)
    add_definitions(-DFSBENCH_USE_HALIBUT)
ENDIF(USE_HALIBUT)
IF(USE_LODEPNG)
    set(OTHER_SRC ${OTHER_SRC} codecs/lodepng.cpp)
    add_definitions(-DFSBENCH_USE_LODEPNG)
ENDIF(USE_LODEPNG)
IF(USE_LZ4)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/lz4)
    set(OTHER_SRC ${OTHER_SRC} codecs/lz4/lz4.c codecs/lz4/lz4hc.c)
    add_definitions(-DFSBENCH_USE_LZ4)
ENDIF(USE_LZ4)
IF(USE_LZF)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/lzf) 
    set(OTHER_SRC ${OTHER_SRC} codecs/lzf/lzf_c.c codecs/lzf/lzf_c_ultra.c codecs/lzf/lzf_c_very.c codecs/lzf/lzf_d.c)
    add_definitions(-DFSBENCH_USE_LZF)
ENDIF(USE_LZF)
IF(USE_LZFX)
    set(OTHER_SRC ${OTHER_SRC} codecs/lzfx.c)
    add_definitions(-DFSBENCH_USE_LZFX)
ENDIF(USE_LZFX)
IF(USE_LZG)
    set(OTHER_SRC ${OTHER_SRC} codecs/liblzg/lib/checksum.c codecs/liblzg/lib/decode.c codecs/liblzg/lib/encode.c)
    add_definitions(-DFSBENCH_USE_LZG)
ENDIF(USE_LZG)
IF(USE_LZMAT)
    set(OTHER_SRC ${OTHER_SRC} codecs/lzmat_dec.c codecs/lzmat_enc.c)
    add_definitions(-DFSBENCH_USE_LZMAT)
ENDIF(USE_LZMAT)
IF(USE_LZSS_IM)
    set(OTHER_SRC ${OTHER_SRC} codecs/lzss.cpp)
    add_definitions(-DFSBENCH_USE_LZSS_IM)
ENDIF(USE_LZSS_IM)
IF(USE_LZV1)
    set(OTHER_SRC ${OTHER_SRC} codecs/lzv1.c)
    add_definitions(-DFSBENCH_USE_LZV1)
ENDIF(USE_LZV1)
IF(USE_LZWC)
    set(OTHER_SRC ${OTHER_SRC} codecs/LZWC.c)
    add_definitions(-DFSBENCH_USE_LZWC)
ENDIF(USE_LZWC)
IF(USE_LZX_COMPRESS)
    set(OTHER_SRC ${OTHER_SRC} codecs/lzx_compress/lz_nonslide.c codecs/lzx_compress/lzx_layer.c)
    add_definitions(-DFSBENCH_USE_LZX_COMPRESS)
ENDIF(USE_LZX_COMPRESS)
IF(USE_MINIZ)
    set(OTHER_SRC ${OTHER_SRC} codecs/miniz.cpp)
    add_definitions(-DFSBENCH_USE_MINIZ)
ENDIF(USE_MINIZ)
IF(USE_MMINI)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/mmini)
    set(OTHER_SRC ${OTHER_SRC} codecs/mmini/huffman.c codecs/mmini/lzl.c)
    add_definitions(-DFSBENCH_USE_MMINI)
ENDIF(USE_MMINI)
IF(USE_MURMUR)
    set(OTHER_SRC ${OTHER_SRC} codecs/MurmurHash3.cpp)
    add_definitions(-DFSBENCH_USE_MURMUR)
ENDIF(USE_MURMUR)
IF(USE_NAKAMICHI)
    set(OTHER_SRC ${OTHER_SRC} codecs/Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX.c codecs/Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD.c)
    add_definitions(-DFSBENCH_USE_NAKAMICHI)
ENDIF(USE_NAKAMICHI)
IF(USE_PG_LZ)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/pg_lz)
    set(OTHER_SRC ${OTHER_SRC} codecs/pg_lz/pg_lzcompress.c)
    add_definitions(-DFSBENCH_USE_PG_LZ)
ENDIF(USE_PG_LZ)
IF(USE_QUICKLZ)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/quicklz) 
    set(OTHER_SRC ${OTHER_SRC} codecs/quicklz/quicklz1.c codecs/quicklz/quicklz2.c codecs/quicklz/quicklz3.c )
    add_definitions(-DFSBENCH_USE_QUICKLZ)
ENDIF(USE_QUICKLZ)
IF(USE_QUICKLZZIP)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/quicklz) 
    set(OTHER_SRC ${OTHER_SRC} codecs/quicklzzip.c)
    add_definitions(-DFSBENCH_USE_QUICKLZZIP)
ENDIF(USE_QUICKLZZIP)
IF(USE_RLE64)
    set(OTHER_SRC ${OTHER_SRC} codecs/RLE64.cpp)
    add_definitions(-DFSBENCH_USE_RLE64)
ENDIF(USE_RLE64)
IF(USE_SANMAYCE_FNV)
    set(OTHER_SRC ${OTHER_SRC} codecs/sanmayce.c)
    add_definitions(-DFSBENCH_USE_SANMAYCE_FNV)
ENDIF(USE_SANMAYCE_FNV)
IF(USE_SCZ)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/scz_compress)
    set(OTHER_SRC ${OTHER_SRC} codecs/scz_compress/scz_compress_lib.c codecs/scz_compress/scz_decompress_lib.c codecs/scz_compress/scz.cpp)
    add_definitions(-DFSBENCH_USE_SCZ)
ENDIF(USE_SCZ)
IF(USE_SHA3_RND1 OR USE_SHA3_RND2 OR USE_SHA3_RND3 OR USE_SHA3_RND3_GROESTL)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/SHA3) 
    set(OTHER_SRC ${OTHER_SRC} codecs/SHA3/fsbench_SHA3.cpp)
    IF(USE_SHA3_RND1)
        add_definitions(-DFSBENCH_USE_SHA3_RND1)
    ENDIF(USE_SHA3_RND1)
    IF(USE_SHA3_RND2)
        add_definitions(-DFSBENCH_USE_SHA3_RND2)
    ENDIF(USE_SHA3_RND2)
    IF(USE_SHA3_RND3)
        add_definitions(-DFSBENCH_USE_SHA3_RND3)
    ENDIF(USE_SHA3_RND3)
    IF(USE_SHA3_RND3_GROESTL)
        add_definitions(-DFSBENCH_USE_SHA3_RND3_GROESTL)
    ENDIF(USE_SHA3_RND3_GROESTL)
ENDIF(USE_SHA3_RND1 OR USE_SHA3_RND2 OR USE_SHA3_RND3 OR USE_SHA3_RND3_GROESTL)
IF(USE_SHRINKER)
    set(OTHER_SRC ${OTHER_SRC} codecs/Shrinker.c)
    add_definitions(-DFSBENCH_USE_SHRINKER)
ENDIF(USE_SHRINKER)
IF(USE_SIPHASH)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/siphash) 
    set(OTHER_SRC ${OTHER_SRC} codecs/siphash/siphash24.c)
    add_definitions(-DFSBENCH_USE_SIPHASH)
ENDIF(USE_SIPHASH)
IF(USE_SNAPPY)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/snappy) 
    set(OTHER_SRC ${OTHER_SRC} codecs/snappy/snappy-sinksource.cc codecs/snappy/snappy-stubs-internal.cc codecs/snappy/snappy.cc)
    add_definitions(-DFSBENCH_USE_SNAPPY)
ENDIF(USE_SNAPPY)
IF(USE_SPOOKY)
    set(OTHER_SRC ${OTHER_SRC} codecs/SpookyV2.cpp)
    add_definitions(-DFSBENCH_USE_SPOOKY)
ENDIF(USE_SPOOKY)
IF(USE_TINF)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/tinf/src)
    set(OTHER_SRC ${OTHER_SRC} codecs/tinf/tinf.cpp codecs/tinf/src/tinflate.c)
    add_definitions(-DFSBENCH_USE_TINF)
ENDIF(USE_TINF)
IF(USE_TOR)
    IF(WIN32)
        add_definitions( -DFREEARC_WIN)
    ENDIF(WIN32)
    IF(UNIX)
        add_definitions(-DFREEARC_UNIX)
    ENDIF(UNIX)
    set(OTHER_SRC ${OTHER_SRC} codecs/tornado/tor_test.cpp)
    add_definitions(-DFSBENCH_USE_TOR)
    add_definitions(-D_UNICODE -DUNICODE)
ENDIF(USE_TOR)
IF(USE_XXHASH)
    set(OTHER_SRC ${OTHER_SRC} codecs/xxhash.c)
    add_definitions(-DFSBENCH_USE_XXHASH)
ENDIF(USE_XXHASH)
IF(USE_YAPPY)
    set(OTHER_SRC ${OTHER_SRC} codecs/Yappy.cpp)
    add_definitions(-DFSBENCH_USE_YAPPY)
ENDIF(USE_YAPPY)
IF(USE_ZFS)
    set(OTHER_SRC ${OTHER_SRC} codecs/zfs/zfs_fletcher.c codecs/zfs/zfs_lzjb2010.c)
    add_definitions(-DFSBENCH_USE_ZFS)
ENDIF(USE_ZFS)
IF(USE_Z3LIB)
    include_directories (${FSBENCH_SOURCE_DIR}/codecs/z3lib)
    set(OTHER_SRC ${OTHER_SRC} codecs/z3lib/fsbench_z3lib.cpp codecs/z3lib/z3blib.c codecs/z3lib/z3dlib.c)
    add_definitions(-DFSBENCH_USE_Z3LIB)
ENDIF(USE_Z3LIB)


set(SRC ${MAIN_SRC}
        ${7z_SRC}
        ${BCL_SRC}
        ${BZ2_SRC}
        ${CRYPTOPP_SRC}
        ${ECRYPT_SRC}
        ${LZHAM_SRC}
        ${LZO_SRC}
        ${NRV_SRC}
        ${ZLIB_SRC}
        ${ZLING_SRC}
        ${ZOPFLI_SRC}
        ${OTHER_SRC})



#----------------#
# compiler flags #
#----------------#


IF(CMAKE_COMPILER_IS_GNUCXX OR ${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
    set_source_files_properties(${MAIN_SRC} PROPERTIES COMPILE_FLAGS "-Wall -Wextra")
    set(CXX_FLAGS ${GXX_FLAGS})
    set(EXE_LINKER_FLAGS ${EXE_LINKER_FLAGS} ${GCC_LINKFLAGS})
ENDIF(CMAKE_COMPILER_IS_GNUCXX OR ${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)

IF(CMAKE_COMPILER_IS_GNUCC OR ${CMAKE_C_COMPILER_ID} STREQUAL Clang)
    set(C_FLAGS   ${GCC_FLAGS})
    set(EXE_LINKER_FLAGS ${EXE_LINKER_FLAGS} ${GCC_LINKFLAGS})
ENDIF(CMAKE_COMPILER_IS_GNUCC OR ${CMAKE_C_COMPILER_ID} STREQUAL Clang)

IF(MSVC)
    set(CXX_FLAGS ${MSVC_FLAGS})
    set(C_FLAGS   ${MSVC_FLAGS})
    set(EXE_LINKER_FLAGS ${EXE_LINKER_FLAGS} ${MSVC_LINK_FLAGS})
ENDIF(MSVC)

#-------------#
# compilation #
#-------------#

set(CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS}        ${CXX_FLAGS}")
set(CMAKE_C_FLAGS          "${CMAKE_C_FLAGS}          ${C_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${EXE_LINKER_FLAGS}")

add_executable (fsbench ${SRC})
IF (UNIX)
    target_link_libraries (fsbench -lrt -lpthread)
ENDIF(UNIX)


#---------------#
# documentation #
#---------------#
find_package(Doxygen)

if(DOXYGEN_FOUND)
configure_file(${PROJECT_SOURCE_DIR}/../doc/Doxyfile.in ${PROJECT_SOURCE_DIR}/../doc/Doxyfile @ONLY)
add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${PROJECT_SOURCE_DIR}/../doc/Doxyfile COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif(DOXYGEN_FOUND)