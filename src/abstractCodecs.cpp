/**
 * This file contains some abstract implementations of the Codec interface that make it simple to plug-in existing codecs
 *
 * Written by m^2.
 * You can consider the code to be public domain.
 * If your country doesn't recognize author's right to relieve themselves of copyright,
 * you can use it under the terms of WTFPL version 2.0 or later.
 */

#include "abstractCodecs.hpp"
#include "misc.hpp"
#include "tools.hpp"

#include <cassert>
#include <cstring>
#include <iomanip>
#include <sstream>

using namespace std;

///////////////////////////
// Codec class
///////////////////////////

Codec::Codec(const std::string & name,
             const std::string & version,
             encoder_t encoder,
             decoder_t decoder,
             max_encoded_size_t max_size,
             transform_type encode_transform_type,
             transform_type decode_transform_type,
             bool can_be_skipped):
        name(name),
        version(version),
        encoder(encoder),
        decoder(decoder),
        encode_transform_type(encode_transform_type),
        decode_transform_type(decode_transform_type),
        can_be_skipped(can_be_skipped),
        real_max_compressed_size(max_size)
{
}
void Codec::init(const string & args,
                 unsigned,
                 size_t,
                 bool,
                 bool)
{
    this->args = args;
}
void Codec::cleanup()
{
}

string Codec::help() const
{
    return this->name + " " + this->version + "\nTakes no parameters";
}

size_t Codec::max_encoded_size(size_t input_size)
{
    return this->real_max_compressed_size(input_size);
}

string Codec::introduction() const
{
    ostringstream oss(ostringstream::out);
    oss << setw(40) << left << this->name << setw(12) << this->version << " " << this->args;
    return oss.str();
}

Codec::InvalidParams::InvalidParams(const string & message) :
        message(message)
{
}

const char * Codec::InvalidParams::what() const throw ()
{
    return this->message.c_str();
}

size_t Codec::_max_compressed_size(size_t input_size)
{
    // I don't know the overhead of any individual codec that doesn't inform about it
    // So as a workaround I use these fairly-safe defaults
    return input_size + input_size / 8 + 32;
}

void ** Codec::eparams()
{
    return 0;
}
void ** Codec::dparams()
{
    return 0;
}
///////////////////////////
// MultifunctionCodec
///////////////////////////

const CodecArgs & MultifunctionCodec::get_type(const string & args)
{
    for (size_t i = 0; i < len; ++i)
    {
        if (case_insensitive_compare(allowed_args[i].args, args) == 0)
            return allowed_args[i];
    }
    throw InvalidParams(help());
}
const string & MultifunctionCodec::default_args(const string & args)
{
    return args == "" ? default_arg : args;
}

MultifunctionCodec::MultifunctionCodec(const string & name,
                                       const string & version,
                                       const CodecArgs * allowed_args,
                                       size_t len,
                                       const string & default_arg,
                                       max_encoded_size_t max_size) :
        Codec(name, version, 0, 0, max_size), // encoder and decoder are filled by init()
        allowed_args(allowed_args), len(len), default_arg(default_arg)
{
}
void MultifunctionCodec::init(const string & args,
                              unsigned,
                              size_t,
                              bool,
                              bool)
{
    this->args = default_args(args);
    const CodecArgs & codec_args = get_type(this->args);

    this->encoder = codec_args.encoder;
    this->decoder = codec_args.decoder;
}

string MultifunctionCodec::help() const
{
    string ret = this->name + " " + this->version + "\nAvaivable parameters: \n";
    for (size_t i = 0; i < len; ++i)
    {
        if (!allowed_args[i].args.empty())
        {
            ret += " ";
            ret += allowed_args[i].args;
            ret += "\n";
        }
    }
    return ret;
}

///////////////////////////
// CodecWithIntModes
///////////////////////////

const string & CodecWithIntModes::default_args(const string & args) const
{
    return args == "" ? default_mode : args;
}

CodecWithIntModes::CodecWithIntModes(const string & name,
                                     const string & version,
                                     encoder_t encoder,
                                     decoder_t decoder,
                                     intptr_t min_mode,
                                     intptr_t max_mode,
                                     const string & default_mode,
                                     max_encoded_size_t max_size,
                                     transform_type encode_transform_type,
                                     transform_type decode_transform_type,
                                     bool can_be_skipped) :
        Codec(name,
              version,
              encoder,
              decoder,
              max_size,
              encode_transform_type,
              decode_transform_type,
              can_be_skipped),
        min_mode(min_mode),
        max_mode(max_mode),
        default_mode(default_mode),
        params(0)
{
}
void CodecWithIntModes::init(const string & args,
                             unsigned threads_no,
                             size_t,
                             bool,
                             bool)
{
    this->args = default_args(args);
    intptr_t mode;
    try
    {
        from_string(this->args, mode);
    }
    catch (const bad_cast &)
    {
        throw InvalidParams(help());
    }
    if (mode < min_mode || mode > max_mode)
        throw InvalidParams(help());
    this->params = new void *[threads_no];
    for (unsigned i = 0; i < threads_no; ++i)
    {
        this->params[i] = (void*) mode;
    }
}

void CodecWithIntModes::cleanup()
{
    if (this->params)
        delete[] this->params;
    this->params = 0;
}

string CodecWithIntModes::help() const
{
    string ret;

    ret = this->name + " " + this->version + "\n";
    ret += "Available parameters: \n";
    ret += " Numbers from " + to_string(min_mode) + " to " + to_string(max_mode) + "\n";

    return ret;
}

void ** CodecWithIntModes::eparams()
{
    return this->params;
}
void ** CodecWithIntModes::dparams()
{
    return eparams();
}

///////////////////////////
// CombinationCodec
///////////////////////////

CombinationCodec::CombinationCodec(Codec & encoder, Codec & decoder) :
        Codec(encoder.name + "/" + decoder.name,
              encoder.version + "/" + decoder.version,
              encoder.encoder,
              decoder.decoder,
              0),
        encoderObject(encoder),
        decoderObject(decoder)
{
}

void CombinationCodec::init(const string & args,
                            unsigned threads_no,
                            size_t isize,
                            bool init_encoder,
                            bool init_decoder)
{
    string encoder_args;
    string decoder_args;

    string::size_type pos = args.find('/');
    if (pos == string::npos)
    {
        // either no parameters or parameters for encoder only
        // either way...
        encoder_args = args;
    }
    else
    {
        encoder_args = args.substr(0, pos);
        decoder_args = args.substr(pos + 1);
    }
    if (init_encoder)
        encoderObject.init(encoder_args, threads_no, isize, true, false);
    // A tricky part. 
    // To support "Codec/Codec,Params/Params2" properly,
    // That is: a single codec with separate params for compression and decompression,
    // I have to read encoder args before they are overwritten by decoder initiation
    // then initiate decoder and in turn read its params
    this->args = init_encoder ? encoderObject.args : encoder_args;

    if (init_decoder)
        decoderObject.init(decoder_args, threads_no, isize, false, true);

    this->encoder = encoderObject.encoder;
    this->decoder = decoderObject.decoder;

    this->args += "/";
    this->args += init_decoder ? decoderObject.args : decoder_args;

    this->encode_transform_type = init_encoder ? encoderObject.encode_transform_type : in_place;
    this->decode_transform_type = init_decoder ? decoderObject.decode_transform_type : in_place;
}

void CombinationCodec::cleanup()
{
    encoderObject.cleanup();
    decoderObject.cleanup();
}

void ** CombinationCodec::eparams()
{
    return encoderObject.eparams();
}
void ** CombinationCodec::dparams()
{
    return decoderObject.dparams();
}
size_t CombinationCodec::max_encoded_size(size_t input_size)
{
    return encoderObject.max_encoded_size(input_size);
}

///////////////////////////
// PipelineCodec
///////////////////////////

PipelineCodec::PipelineCodec(Codec & first_codec, Codec & second_codec) :
        Codec(first_codec.name + "+" + second_codec.name,
              first_codec.version + "+" + second_codec.version,
              encode,
              decode,
              0),    //TODO
        first_codec(first_codec),
        second_codec(second_codec),
        encoder_params(0),
        decoder_params(0),
        encoder_params_ptrs(0),
        decoder_params_ptrs(0)
{
}

void PipelineCodec::init(const string & args,
                         unsigned threads_no,
                         size_t isize,
                         bool init_encoder,
                         bool init_decoder)
{
    string first_codec_args;
    string second_codec_args;

    string::size_type pos = args.find('+');
    if (pos == string::npos)
    {
        // either no parameters or parameters for the 1st codec only
        // either way...
        first_codec_args = args;
        second_codec_args = "";
    }
    else
    {
        first_codec_args = args.substr(0, pos);
        second_codec_args = args.substr(pos + 1);
    }
    first_codec.init(first_codec_args, threads_no, isize, init_encoder, init_decoder);
    second_codec.init(second_codec_args, threads_no, isize, init_encoder, init_decoder);

    if (init_encoder)
        this->encode_transform_type =
                _get_combined_transform_type(first_codec.encode_transform_type,
                                             second_codec.encode_transform_type);
    if (init_decoder)
        this->decode_transform_type =
                _get_combined_transform_type(first_codec.decode_transform_type,
                                             second_codec.decode_transform_type);

    if (init_encoder)
    {
        this->encoder_params = new EncodeParams[threads_no];
        this->encoder_params_ptrs = new void *[threads_no];
    }
    if (init_encoder)
    {
        this->decoder_params = new DecodeParams[threads_no];
        this->decoder_params_ptrs = new void *[threads_no];
    }

    for (size_t i = 0; i < threads_no; ++i)
    {
        if (init_encoder)
        {
            void ** eparams = first_codec.eparams();
            this->encoder_params[i].first_codec_params = eparams ? eparams[i] : 0;
            eparams = second_codec.eparams();
            this->encoder_params[i].second_codec_params = eparams ? eparams[i] : 0;
            this->encoder_params[i].first_encoder = &first_codec;
            this->encoder_params[i].second_encoder = &second_codec;
            this->encoder_params_ptrs[i] = &this->encoder_params[i];
        }
        if (init_decoder)
        {
            void ** dparams = first_codec.dparams();
            this->decoder_params[i].first_codec_params = dparams ? dparams[i] : 0;
            dparams = second_codec.dparams();
            this->decoder_params[i].second_codec_params = dparams ? dparams[i] : 0;
            this->decoder_params[i].first_decoder = &first_codec;
            this->decoder_params[i].second_decoder = &second_codec;
            this->decoder_params_ptrs[i] = &this->encoder_params[i];
        }
    }

    this->encoder = encode;
    this->decoder = decode;

    this->args += "+";
    this->args += init_decoder ? second_codec.args : second_codec_args;
}
size_t PipelineCodec::max_encoded_size(size_t input_size)
{
    return second_codec.max_encoded_size(first_codec.max_encoded_size(input_size));
}
size_t PipelineCodec::encode(char * in, size_t isize, char * out, size_t osize, void * args)
{
    EncodeParams * params = *(EncodeParams**) args;
    size_t first_size = params->first_encoder->encoder(in,
                                                       isize,
                                                       out,
                                                       osize,
                                                       &params->first_codec_params);
    if (first_size == CODING_ERROR)
        return CODING_ERROR;

    char * new_input = 0;
    char * new_output = 0;

    switch (params->first_encoder->encode_transform_type)
    {
    case in_place:
    case buffered:
        new_input = in;
        new_output = out;
    break;
    case moving:
        new_input = out;
        new_output = in;
    break;
    default:
        assert(0);
    break;
    }
    size_t second_size = params->second_encoder->encoder(new_input,
                                                         first_size,
                                                         new_output,
                                                         osize,
                                                         &params->second_codec_params);
    if (second_size == CODING_ERROR)
        return CODING_ERROR;

    memcpy(new_output + second_size, &first_size, sizeof(first_size)); // second_decoder takes size of its output as a parameter,
                                                 // so I have to store it.
    return second_size + sizeof(first_size);
}
size_t PipelineCodec::decode(char * in, size_t isize, char * out, size_t osize, void * args)
{
    DecodeParams * params = *(DecodeParams**) args;
    size_t first_osize; /// output size for the second_decoder
    memcpy(&first_osize, in + isize - sizeof(first_osize), sizeof(first_osize));
    size_t ret = params->second_decoder->decoder(in,
                                                 isize,
                                                 out,
                                                 first_osize,
                                                 &params->second_codec_params);
    if (ret == CODING_ERROR)
        return CODING_ERROR;

    switch (params->second_decoder->decode_transform_type)
    {
    case in_place:
    case buffered:
        ret = params->first_decoder->encoder(in, ret, out, osize, &params->first_decoder);
    break;
    case moving:
        ret = params->first_decoder->encoder(out, ret, in, isize, &params->first_decoder);
    break;
    default:
        assert(0);
    break;
    }
    return ret;
}
void PipelineCodec::cleanup()
{
    first_codec.cleanup();
    second_codec.cleanup();
    if (encoder_params)
    {
        delete[] encoder_params;
        encoder_params = 0;
    }
    if (decoder_params)
    {
        delete[] decoder_params;
        decoder_params = 0;
    }
    if (encoder_params_ptrs)
    {
        delete[] encoder_params_ptrs;
        encoder_params_ptrs = 0;
    }
    if (decoder_params_ptrs)
    {
        delete[] decoder_params_ptrs;
        decoder_params_ptrs = 0;
    }
}

void ** PipelineCodec::eparams()
{
    return this->encoder_params_ptrs;
}
void ** PipelineCodec::dparams()
{
    return this->decoder_params_ptrs;
}

Codec::transform_type PipelineCodec::_get_combined_transform_type(Codec::transform_type first,
                                                                  Codec::transform_type second)
{
    if (first == in_place && second == in_place)
        return in_place;
    if (first == moving)
        return second == moving ? buffered : moving;
    else
        return second == moving ? moving : buffered;
}
void BufferedCodec::init(const std::string &,
                         unsigned threads_no,
                         size_t,
                         bool init_encoder,
                         bool init_decoder)
{
    if (init_encoder || init_decoder)
    {
        this->threads_no = threads_no;
        this->buffers = new void *[threads_no];
        this->params = new void *[threads_no];
        for (unsigned i = 0; i < threads_no; ++i)
        {
            this->buffers[i] = new char[bufsize + bufalignment - 1];
            this->params[i] = (void*) ((uintptr_t) this->buffers[i] & ~(bufalignment - 1));
        }
    }
}
void BufferedCodec::cleanup()
{
    if (this->buffers)
    {
        for (unsigned i = 0; i < this->threads_no; ++i)
        {
            delete[] (char*) this->buffers[i];
        }
        delete[] this->buffers;
        delete[] this->params;
        this->buffers = 0;
        this->params = 0;
    }
}
void ** BufferedCodec::dparams()
{
    return params;
}
void ** BufferedCodec::eparams()
{
    return params;
}
