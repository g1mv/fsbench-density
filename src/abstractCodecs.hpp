/**
 * This file contains some abstract definitions of the Codec interface that make it simple to plug-in existing codecs
 *
 * Written by m^2.
 * You can consider the code to be public domain.
 * If your country doesn't recognize author's right to relieve themselves of copyright,
 * you can use it under the terms of WTFPL version 2.0 or later.
 */

#ifndef ABSTRACTCODECS_HPP_B9fgkfG8
#define ABSTRACTCODECS_HPP_B9fgkfG8

#include "codecs.hpp"
#include "misc.hpp"

#include <cstring>
#include <string>

/**
 * MultifunctionCodec
 *
 * Codecs that have several compression / decompression functions
 * Each function pair is referred to by an argument
 */
struct CodecArgs
{
    std::string args;
    encoder_t encoder;
    decoder_t decoder;
    CodecArgs(const std::string & args, encoder_t encoder, decoder_t decoder) :
            args(args), encoder(encoder), decoder(decoder)
    {
    }
};

struct MultifunctionCodec: Codec
{
private:

    const CodecArgs * allowed_args;
    size_t len;
    const std::string default_arg;

    const CodecArgs & get_type(const std::string & args);
    const std::string & default_args(const std::string & args);

public:

    MultifunctionCodec(const std::string & name,
                       const std::string & version,
                       const CodecArgs * allowed_args,
                       size_t len,
                       const std::string & default_arg,
                       max_encoded_size_t max_size = _max_compressed_size);

    virtual std::string help() const;
    virtual void init(const std::string & args,
                      unsigned threads_no,
                      size_t isize,
                      bool init_encoder = true,
                      bool init_decoder = true);

};

/**
 * CodecWithIntModes
 *
 * Such codecs have just 1 compression and 1 decompression function
 * that take ints as parameters to specify compression mode
 */
struct CodecWithIntModes: Codec
{
private:

    const int min_mode;
    const int max_mode;
    const std::string default_mode;

    const std::string & default_args(const std::string & args) const;

public:

    CodecWithIntModes(const std::string & name,
                      const std::string & version,
                      encoder_t encoder,
                      decoder_t decoder,
                      intptr_t min_mode,
                      intptr_t max_mode,
                      const std::string & default_mode,
                      max_encoded_size_t max_size = _max_compressed_size,
                      transform_type encode_transform_type = moving,
                      transform_type decode_transform_type = moving,
                      bool can_be_skipped = true);

    virtual std::string help() const;
    virtual void init(const std::string & args,
                      unsigned threads_no,
                      size_t isize,
                      bool init_encoder = true,
                      bool init_decoder = true);
    virtual void cleanup();

    virtual void ** eparams(); // parmeters to be sent to compressors
    virtual void ** dparams(); // parmeters to be sent to decompressors 

protected:
    void ** params;

};

/**
 * CombinationCodec
 *
 * Takes encoder from 1 codec and decoder from the other
 */
struct CombinationCodec: Codec
{
private:

    Codec & encoderObject;
    Codec & decoderObject;

public:

    CombinationCodec(Codec& encoder, Codec& decoder);

    // Initialises both codecs.
    // args takes form: [encoder_args[/decoder_args]]
    virtual void init(const std::string& args,
                      unsigned threads_no,
                      size_t isize,
                      bool init_encoder = true,
                      bool init_decoder = true);
    virtual void cleanup();

    virtual void ** eparams();    // parmeters to be sent to compressors
    virtual void ** dparams();    // parmeters to be sent to decompressors 

    size_t max_encoded_size(size_t input_size);
};

/**
 * PipelineCodec
 *
 * Takes 2 codecs and runs them one after another
 *
 * @TODO: major rework. There's no sensible handling of skippable / unskippable codecs
 *        Right now, skipping a codec is done by benchmark.cpp::encode().
 *        There should be a codec subclass, SkippableCodec that handles it.
 *        Generally, each codec would have to return an information 
 *        in which buffer it returned its data. This information would have to be stored somewhere.
 *        Also, SkippableCodec would have to include the information that the data was skipped somewhere.
 *        Now, a skipped codec's decoder is not being called, with such implementation,
 *        SkippableCodec would be called with a buffer and its size, just that.
 *        So it's a 1-byte flag. It should be at the end of the file because:
 *        * At the beginning it would break alignment
 *        * When encoder skipps a buffer, it has to put a flag in the original buffer.
 *          At the end is the simplest way.
 */
struct PipelineCodec: Codec
{
private:

    Codec & first_codec;
    Codec & second_codec;

    struct EncodeParams
    {
        void * first_codec_params;
        void * second_codec_params;
        Codec * first_encoder;
        Codec * second_encoder;
    };
    struct DecodeParams
    {
        void * first_codec_params;
        void * second_codec_params;
        Codec * first_decoder;
        Codec * second_decoder;
    };
    EncodeParams * encoder_params;
    DecodeParams * decoder_params;
    void ** encoder_params_ptrs;
    void ** decoder_params_ptrs;

    static size_t encode(char * in, size_t isize, char * out, size_t osize, void * args);
    static size_t decode(char * in, size_t isize, char * out, size_t osize, void * args);
    size_t max_encoded_size(size_t input_size);

    static transform_type _get_combined_transform_type(transform_type first, transform_type second);

public:

    PipelineCodec(Codec & first_codec, Codec & second_codec);

    // Initialises both codecs.
    // args takes form: [encoder_args[/decoder_args]]
    virtual void init(const std::string & args,
                      unsigned threads_no,
                      size_t isize,
                      bool init_encoder = true,
                      bool init_decoder = true);
    virtual void cleanup();

    virtual void ** eparams();    // parmeters to be sent to compressors
    virtual void ** dparams();    // parmeters to be sent to decompressors 
};
/**
 * Checksum
 *
 * Checksum is an in-place codec 
 * that appends its digest right after the end of the data
 */
template<size_t digest_size>
class Checksum: public Codec
{
    typedef void (*checksum_t)(char * in, size_t isize, char * out);
private:
    checksum_t checksummer;
    void ** params;

public:
    Checksum(const std::string & name, const std::string & version, checksum_t checksummer) :
            Codec(name,
                  version,
                  encode,
                  decode,
                  max_output_size,
                  Codec::in_place,
                  Codec::in_place,
                  false),
            checksummer(checksummer),
            params(0)
    {
    }

    static size_t max_output_size(size_t size)
    {
        return size + digest_size;
    }
    static size_t encode(char * in, size_t isize, char * out, size_t osize, void * args)
    {
        UNUSED(out);
        UNUSED(osize);
        checksum_t checksummer = *(checksum_t*) args;
        // Temporary digest. It's needed, so checksums that write past their buffers have it easier
        // Also, helps them with alignment management
        // I use large ints instead of chars to get it aligned
        const size_t tmp_digest_size1 = digest_size / sizeof(uintmax_t);
        const size_t tmp_digest_size =
                tmp_digest_size1 * sizeof(uintmax_t) == digest_size ? tmp_digest_size1 :
                                                                      tmp_digest_size1 + 1;
        uintmax_t tmp_digest[tmp_digest_size];
        checksummer(in, isize, (char*)tmp_digest);
        memcpy(in + isize, tmp_digest, digest_size);
        return isize + digest_size;
    }
    static size_t decode(char * in, size_t isize, char * out, size_t osize, void * args)
    {
        UNUSED(out);
        UNUSED(osize);
        checksum_t checksummer = *(checksum_t*) args;
        // there checksum of the original data is supposed to be stored right after the data
        size_t data_size = isize - digest_size;
        const size_t tmp_digest_size1 = digest_size / sizeof(uintmax_t);
        const size_t tmp_digest_size =
                tmp_digest_size1 * sizeof(uintmax_t) == digest_size ? tmp_digest_size1 :
                                                                      tmp_digest_size1 + 1;
        uintmax_t digest[tmp_digest_size];
        checksummer(in, data_size, (char*)digest);
        if (memcmp(in + data_size, digest, digest_size) != 0)
        {
            return CODING_ERROR;
        }
        return data_size;
    }
    /**
     * Initialises a checksum codec
     *
     * @param args
     * @param threads_no
     * @param isize
     * @param init_encoder
     * @param init_decoder
     *
     * @todo Do other codecs handle allocation correctly when called first with init_encoder and then init_decoder
     *       instead of just once with both set to true?
     * @todo docs
     */
    virtual void init(const std::string & args,
                      unsigned threads_no,
                      size_t isize,
                      bool init_encoder = true,
                      bool init_decoder = true)
    {
        UNUSED(args);
        UNUSED(isize);
        UNUSED(init_encoder);
        UNUSED(init_decoder);
        if (!params)
        {
            params = new void *[threads_no];
            for (unsigned i = 0; i < threads_no; ++i)
                params[i] = (void*) checksummer;
        }
    }
    virtual void cleanup()
    {
        if (params)
        {
            delete[] params;
            params = 0;
        }
    }
    virtual void ** eparams()
    {
        return params;
    }
    virtual void ** dparams()
    {
        return eparams();
    }
};

/**
 * Codecs that need an extra buffer of working memory
 */
class BufferedCodec: public Codec
{
public:
    /**
     * @param bufsize:      Size of a working-memory buffer for each thread, in bytes.
     *                      It's passed to both encoder and decoder
     * @param bufalignment: Alignment of the working-memory buffer. Must be a power of 2.
     */
    BufferedCodec(const std::string & name,
                  const std::string & version,
                  encoder_t compressor,
                  decoder_t decompressor,
                  max_encoded_size_t max_blowup,
                  size_t bufsize,
                  size_t bufalignment = 1) :
            Codec(name, version, compressor, decompressor, max_blowup),
            params(0),
            bufsize(bufsize),
            bufalignment(bufalignment),
            threads_no(0)
    {
    }
    virtual void init(const std::string & args,
                      unsigned threads_no,
                      size_t isize,
                      bool init_compressor,
                      bool init_decompressor);
    virtual void cleanup();
    void ** dparams();
    void ** eparams();

protected:
    void ** params;  /// Aligned buffers passed to encoders / decoder
    void ** buffers; /// Raw, possibly misaligned buffers
private:
    size_t bufsize;
    size_t bufalignment;
    unsigned threads_no;
};

#endif // ABSTRACTCODECS_HPP_B9fgkfG8
