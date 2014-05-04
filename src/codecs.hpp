/**
 * Written by m^2.
 * You can consider the code to be public domain.
 * If your country doesn't recognize author's right to relieve themselves of copyright,
 * you can use it under the terms of WTFPL version 2.0 or later.
 *
 *
 * To add a codec:
 * 1. Implement a Codec class for it or - if applicable - functions required by one of abstract ones
 * 2. Add sources to CMakeFiles.txt
 * 3. Add codec version to codecs.hpp
 * 4. Add codec to all_compressors / all_checksums / all_ciphers in codecs.cpp
 * 5. Update readme
 * 6. Update changelog
 */
#ifndef CODECS_HPP_BhjgkfG8
#define CODECS_HPP_BhjgkfG8

#include <list>
#include <exception>
#include <string>
#include <vector>

// some codecs have rough bounds checking
// to overcome the problem I allocate some extra data
#define OVERRUN_PAD 32

// in    - input buffer
// isize - input data size
// out   - output buffer
// osize - desired output size. Codec can write up to max_encoded_size(in), 
//         but if it fails to compress to osize or less, 
//         it can quit trying before the end of data
//         it's warranted that the actual buffer size is not smaller than max_encoded_size(isize)
//         and that actual size of the input buffer is just as big (which matters i.e. for in_place codecs)
typedef size_t (*encoder_t)(char * in, size_t isize, char * out, size_t osize, void * args);
// in    - input buffer
// isize - input data size
// out   - output buffer
// osize - output data size = decoded size
typedef size_t (*decoder_t)(char * in, size_t isize, char * out, size_t osize, void * args);
typedef size_t (*max_encoded_size_t)(size_t input_size);

// When some codec can warrant no size blowup (and i.e. error instead), this function can be used
// Still, I reserve a couple of bytes for buffer overruns, some codecs need it and it's not worth introducing another function for them
inline size_t no_blowup(size_t input_size)
{
    return input_size + OVERRUN_PAD;
}

// should be returned by encoder_t / decoder_t when something breaks (i.e. there's a bug or meeting osize requirements is impossible)
#define CODING_ERROR 0

/**
 * @brief A base class for all encoders / decoders
 */
struct Codec
{
    struct InvalidParams: std::exception
    {
        virtual const char * what() const throw ();
        InvalidParams(const std::string & message);
        ~InvalidParams() throw ()
        {
        }
    private:
        const std::string message;
    };
    enum transform_type
    {
        in_place, // takes an input buffer and modifies it in place
        moving,  // takes 2 buffers, input and output
        buffered,  // takes 2 buffers, input/output and a buffer
        selective // takes 2 buffers and puts data in whichever it prefers, returning information about its destination
    };

    const std::string name;
    const std::string version;
    std::string args;

    encoder_t encoder;
    decoder_t decoder;
    virtual size_t max_encoded_size(size_t input_size);

    transform_type encode_transform_type;
    transform_type decode_transform_type;

    bool can_be_skipped; //when a compressor fails to reduce size, the data can be left uncompressed and decompression can be skipped
                         //this doesn't hold for checksums, they have to be calculated even though they don't save space ;)

    virtual void ** eparams(); // parmeters to be sent to compressors
    virtual void ** dparams(); // parmeters to be sent to decompressors 

    /**
     * Creates a codec
     * @throw InvalidParams Dude, something's wrong with your command line. 
     */
    Codec(const std::string & name,
          const std::string & version,
          encoder_t encoder,
          decoder_t decoder,
          max_encoded_size_t max_size = _max_compressed_size,
          transform_type encode_transform_type = moving,
          transform_type decode_transform_type = moving,
          bool can_be_skipped = true);

    // returns a help message
    virtual std::string help() const;
    // returns codec name, version, args and whatever other information the codec would like to say about itself
    virtual std::string introduction() const;

    // (de)initialisation functions
    // set the following fields:
    // - args
    // - encoder
    // - decoder
    // - max_compressed_size
    // - (subclasses) whatever else do they need.
    // - encode_transform_type
    // - decode_transform_type
    // the following are set by constructor:
    // - name
    // - version
    // - can_be_skipped
    virtual void init(const std::string & args,
                      unsigned threads_no,
                      size_t isize,
                      bool init_compressor = true,
                      bool init_decompressor = true);
    virtual void cleanup();
    virtual ~Codec()
    {
    }

protected:
    // the default, fairly-safe function to be used with codecs that don't have their own
    static size_t _max_compressed_size(size_t input_size);
    max_encoded_size_t real_max_compressed_size;

};

/**
 * @brief Stores a codec and parameters that are supposed to be used by it
 */
struct CodecWithParams
{
    Codec & codec;
    const std::string params;
    CodecWithParams(Codec & codec, const std::string & params) :
            codec(codec), params(params)
    {
    }
    CodecWithParams operator=(const CodecWithParams & src)
    {
        return CodecWithParams(src.codec, src.params);
    }
};

extern std::list<Codec*> CODECS;
extern std::list<CodecWithParams> DEFAULT_CODECS;
extern std::list<CodecWithParams> FAST_COMPRESSORS;
extern std::list<CodecWithParams> ALL_COMPRESSORS;
extern std::list<CodecWithParams> ALL_CHECKSUMS;
extern std::list<CodecWithParams> ALL_CIPHERS;
extern std::list<CodecWithParams> ALL_OTHERS;

Codec * find_codec(const std::string & name);

// TODO: each codec should have a version number by itself,
// so there's one place less to change when adding / updating 
#define _7z_VERSION         "9.20"
#define _AR_VERSION         "2013-06-03"
#define _BCL_VERSION        "1.2.0"
#define _BLAKE2_VERSION     "20121223"
#define _BLOSC_VERSION      "1.2.3"
#define _BLZ_VERSION        "1.0.5"
#define _BZ2_VERSION        "1.0.6"
#define _CITYHASH_VERSION   "1.1.0"
#define _CRAPWOW_VERSION    "2012-06-07"
#define _CRUSH_VERSION      "0.0.1"
#define _CRYPTOPP_VERSION   "5.6.1"
#define _DOBOZ_VERSION      "2011-03-19"
#define _ECRYPT_VERSION     "2012-06-10"
#define _EDON_R_VERSION     "v20"
#define _FASTCRYPTO_VERSION "2007-04-17"
#define _FASTLZ_VERSION     "0.1.0"
#define _FSE_VERSION        "2014-04-07"
#define _GIPFELI_VERSION    "2011-10-19"
#define _HALIBUT_VERSION    "SVN r9550"
#define _KECCAK_VERSION     "3.2"
#define _LODEPNG_VERSION    "20120729"
#define _LZ4_VERSION        "r114"
#define _LZF_VERSION        "3.6"
#define _LZFX_VERSION       "r16"
#define _LZG_VERSION        "1.0.6"
#define _LZHAM_VERSION      "SVN r96"
#define _LZMAT_VERSION      "1.1"
#define _LZO_VERSION        "2.06"
#define _LZSSIM_VERSION     "2008-07-31"
#define _LZV1_VERSION       "0.5"
#define _LZWC_VERSION       "0.4"
#define _LZX_VERSION        "2005-07-06"
#define _MINIZ_VERSION      "1.11"
#define _MMINI_VERSION      "2012-12-23"
#define _MURMUR_VERSION     "2012-02-29"
#define _NAKAMICHI_VERSION  "Kaidanji"
#define _NRV_VERSION        "1.03"
#define _PG_LZ_VERSION      "9.3.4"
#define _QLZ_VERSION        "1.5.1b6"
#define _QLZZIP_VERSION     "0.4"
#define _RLE64_VERSION      "R3.00"
#define _SANMAYCE_VERSION   "2013-06-16"
#define _SIPHASH_VERSION    "reference"
#define _SCZ_VERSION        "2008-11-25"
#define _SHA3_VERSION       "SHA3 Final 64bit opt"
#define _SHA3_RND1_VERSION  "SHA3 rnd 1 64bit opt"
#define _SHA3_RND2_VERSION  "SHA3 rnd 2 64bit opt"
#define _SHRINKER_VERSION   "r6"
#define _SNAPPY_VERSION     "1.1.0"
#define _SPOOKY_VERSION     "V2 2012-08-05"
#define _TINF_VERSION       "1.00"
#define _TOR_VERSION        "0.6a"
#define _XXHASH_VERSION     "r29"
#define _XXHASH256_VERSION  "1"
#define _YAPPY_VERSION      "v2"
#define _ZFS_VERSION        "FreeBSD r263244"
#define _Z3LIB_VERSION      "1.3"
#define _ZLIB_VERSION       "1.2.8"
#define _ZLING_VERSION      "20140324"
#define _ZOPFLI_VERSION     "1.0.0"
#endif // CODECS_HPP_BhjgkfG8
