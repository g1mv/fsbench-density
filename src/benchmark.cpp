/**
 * Written by m^2.
 * You can consider the code to be public domain.
 * If your country doesn't recognize author's right to relieve themselves of copyright,
 * you can use it under the terms of WTFPL version 2.0 or later.
 */

///////////////////////////
// INCLUDES
///////////////////////////
#include "misc.hpp"

#include "benchmark.hpp"
#include "threads.hpp"
#include "tools.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <climits>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace std;

///////////////////////////
// MACROS
///////////////////////////

#define KB 1024
#define MB (1024 * KB)

//I make all buffers aligned
#define ALIGNMENT ((uintptr_t)16)

#define ITERS(count) for(uint_least64_t i = 0; i < (uint_least64_t)(count); ++i)

///////////////////////////
// TYPES
///////////////////////////

typedef THREAD_RETURN_TYPE (*thread_function)(void * params);

///////////////////////////
// HELPERS
///////////////////////////

static void copy_input_buffer(const char * inbuf,
                              char * outbuf,
                              size_t size,
                              size_t bsize,
                              size_t padded_bsize)
{
    while (size)
    {
        size_t current_block_size = min(size, bsize);
        memcpy(outbuf, inbuf, current_block_size);
        size -= current_block_size;
        inbuf += current_block_size;
        outbuf += padded_bsize;
    }
}

static string speed_string(uint_least64_t bytes, uint_least64_t msec)
{
    if (msec)
    {
        const string units[] =
            { " B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB" };
        int units_no = sizeof(units) / sizeof(string);
        uint_least64_t divider = 1;
        for (int i = 0; i < units_no; ++i, divider *= KB)
        {

            double size_per_sec = (((double) bytes * 1000) / msec) / divider;
            if (size_per_sec < 10000)
            {
                std::stringstream ss;
                int precision;
                if(size_per_sec < 10)
                {
                    precision = 2;
                }
                else if(size_per_sec < 100)
                {
                    precision = 1;
                }
                else
                {
                    precision = 0;
                }
                ss << std::fixed << std::setprecision(precision) << size_per_sec;

                std::string ret;

                while (ss.good())
                {
                    char c = ss.get();
                    if (ss.good())
                        ret += c;
                }
                return ret + " " + units[i] + "/s";
            }
        }
        // out of units
    }
    return "fast :)";
}

static string efficiency_string(uint_least64_t saved_bytes, uint_least64_t msec)
{
    if (msec)
    {
        uint_least64_t divider = 1;
        for (int i = 0; divider < (UINT_LEAST64_MAX / KB); ++i, divider *= KB)
        {
            uintmax_t size_per_sec = (((uintmax_t) saved_bytes * 1000) / msec) / divider;
            if (size_per_sec <= 9999)
                return to_string(size_per_sec) + "e" + to_string(3 * i);
        }
    }
    return "high :)";
}

/**
 * Checks if the given codec has both encoder and decoder
 * @note It prints error to cerr. 
 * @return true if yes, false otherwise 
 */
static bool check_codec(const Codec & codec)
{
    if (codec.encoder == 0)
    {
        cerr << "ERROR: " << codec.name << " is just a decoder.\n";
        cerr << "Combine it with some encoder to test it.\n";
        return false;
    }
    else if (codec.decoder == 0)
    {
        cerr << "ERROR: " << codec.name << " is just an encoder.\n";
        cerr << "Combine it with some decoder to test it.\n";
        cerr << "If you don't want one, combine it with nop.\n";
        return false;
    }
    return true;
}

///////////////////////////
// OUTPUT PRINTING
///////////////////////////

static void printTime(uint_least64_t cmili,
                      uint_least64_t dmili,
                      uint_least64_t encoder_input_size,
                      uint_least64_t encoder_output_size,
                      uint_least64_t decoder_output_size,
                      unsigned citers,
                      unsigned diters,
                      bool csv)
{
    //because of timer inaccuracy some jobs may take 0 ms. And I divide by dmili later...
    dmili = max(dmili, (uint_least64_t) 1);

    if (csv)
    {
        cout << cmili << "," << encoder_input_size << "," << encoder_output_size << "," << citers
             << "," << dmili<< "," << decoder_output_size << "," << diters << endl;
    }
    else
    {
        ConsoleColour cc(RESULT_COLOUR);

        char compression_rate_buf[32];
        sprintf(compression_rate_buf, "%.3f", (float) encoder_input_size / encoder_output_size);
        int len = strlen(compression_rate_buf);
        string compression_rate;
        for (int i = 0; i < 6 - len; ++i)
            compression_rate += ' ';
        compression_rate += compression_rate_buf;

        streamsize old_width = cout.width(11);
        try
        {
            cout << std::right << encoder_output_size / citers;
            cout << " (x" << compression_rate << ")     ";
            string decoding_speed_str =
                    decoder_output_size ? speed_string(decoder_output_size, dmili) :
                                        string("-");
            cout.width(9);
            cout << speed_string(encoder_input_size, cmili) << " ";
            cout.width(9);
            cout << decoding_speed_str << "      ";
            cout.width(6);
            cout << efficiency_string(encoder_input_size - encoder_output_size, cmili) << " ";
            cout.width(6);
            cout << efficiency_string((encoder_input_size - encoder_output_size) / citers * diters, dmili);
        }
        catch(...)
        {
            cout.width(old_width);
            throw;
        }
        cout.width(old_width);
    }
}
static void printHeaders(bool csv)
{
    if(csv)
    {
        cout << "Codec,version,args,E.time (ms),E. input size,E. output size,E. iters,"
             << "D. time(ms),D. output size,D. iters" << endl;
    }
    else
    {
        {
            ConsoleColour cc(CODEC_COLOUR);
            cout << "Codec                                   version      args" << endl;
        }
        {
            ConsoleColour cc(RESULT_COLOUR);
            cout << "C.Size      (C.Ratio)        E.Speed   D.Speed      E.Eff. D.Eff." << endl;
        }
    }
}
static void printCodec(const Codec & codec, bool csv)
{
    if (csv)
        cout << codec.name << "," << codec.version << "," << codec.args << ",";
    else
    {
        ConsoleColour cc(CODEC_COLOUR);
        cout << codec.introduction() << endl;
    }
}

///////////////////////////
// CORRECTNESS VERIFICATION
///////////////////////////

// TODO: How about each chunk verifying itself?
static void check_decoding(uint32_t input_crc,
                           char * buf,
                           size_t size,
                           size_t bsize,
                           size_t padded_bsize)
{
    uint32_t output_crc = 0;
    while (size)
    {
        size_t current_block_size = min(size, bsize);
        output_crc = crc(buf, current_block_size, output_crc);
        memset(buf, 0, current_block_size); // clear the output buffer
        buf += padded_bsize;
        size -= current_block_size;
    }
    if (input_crc != output_crc)
    {
        cout << "CRC mismatch!\n";
    }
}

//////////////////////
// ENCODING / DECODING
//////////////////////

/**
 * Encodes input chunks
 *
 * An input array is a series of chunks
 * A chunk consists of 2 parts: data to be encoded and some extra space
 * Codecs can use the extra space as they want, in particular quite a few access data out of bound,
 * so this is some extra security.
 *
 * Produces 2 outputs:
 *  - encoded data. An array of encoded chunks laid out one after another.
 *    It can be returned in either input or output buffer
 *  - metadata. What is in what chunk, encoded size etc.
 *
 * @return compressed data size, that is - it ignores chunk_size pieces
 */
static THREAD_RETURN_TYPE encode(EncodeParams * params)
{
    uintmax_t total_in = 0;
    uintmax_t total_out = 0;

    Scheduler::WorkItem wi;
    while (params->scheduler->getChunk(wi) == 0)
    {
        char * op = wi.out;
        size_t processed_bytes = 0;
        unsigned i = 0;
        for (; processed_bytes < wi.isize; processed_bytes += params->workbsize, ++i)
        {
            size_t chunk_size = min(wi.isize - processed_bytes, params->bsize);
            size_t ret = CODING_ERROR;
            size_t block_size;
            if(chunk_size > params->ssize)
                block_size = round_up(chunk_size - params->ssize, params->ssize);
            else
                block_size = params->ssize;
            char * ip = wi.in + processed_bytes;
            for (unsigned long j=0; j<params->overhead_iters; ++j)
            {
                ret = params->encoder(ip,
                                      chunk_size,
                                      op,
                                      block_size,
                                      params->other);
            }
            bool failed_to_encode = ret == CODING_ERROR;

            size_t real_size = round_up(ret, params->ssize);

            if (!failed_to_encode && params->can_be_skipped)
            {
                failed_to_encode = real_size >= params->bsize || real_size >= chunk_size;
            }
            if (failed_to_encode)
            {
                assert(total_out < UINTMAX_MAX - round_up(chunk_size, params->ssize));
                assert(total_in  < UINTMAX_MAX - chunk_size);
                //FIXME: if not params->can_be_skipped this should return error
                //       This can wait as after SkippableCodec gets implemented there will be just error in this place
                wi.metadata[i].successfully_encoded = false;
                wi.metadata[i].encoded_size = params->bsize;
                wi.metadata[i].decoded_size = params->bsize;
                total_out += round_up(chunk_size, params->ssize);
                total_in  += chunk_size;
                if (params->verify)
                {
                    // optimisation: When unencoded data gets stored,
                    // there's no need for memcpy
                    // However, it's needed to verify if encoding succeeded

                    // Note: it's needed only for moving transforms, but doesn't hurt otherwise
                    // FIXME: What if a skippable transform failed and modified its input buffer?
                    memcpy(op, wi.in + processed_bytes, chunk_size);
                }
            }
            else
            {
                assert(total_out <= UINTMAX_MAX - real_size);
                assert(total_in  <= UINTMAX_MAX - chunk_size);

                wi.metadata[i].successfully_encoded = true;
                wi.metadata[i].encoded_size = ret;
                wi.metadata[i].decoded_size = chunk_size;
                total_out += real_size;
                total_in  += chunk_size;
                if (params->verify)
                {
                    // zero-out the part of the buffer that doesn't contain encoded data
                    // if a codec cheats by reporting size smaller than what it actually needs,
                    // this will detect it
                    if (params->transform_type == Codec::moving)
                        memset(op + ret, 0, params->workbsize - ret);
                    else
                        memset(wi.in + processed_bytes + ret, 0, params->workbsize - ret);
                }
            }
            op += params->workbsize;
        }
    }
    params->input_size  = total_in;
    params->output_size = total_out;

    return THREAD_RETURN;
}

static THREAD_RETURN_TYPE decode(DecodeParams * params)
{
    uintmax_t total_out = 0;

    Scheduler::WorkItem wi;
    while (params->scheduler->getChunk(wi) >= 0)
    {
        ptrdiff_t job_out = 0;
        char * ip = wi.in;
        char * op = wi.out;
        unsigned i = 0;
        size_t processed_bytes = 0;
        for (; processed_bytes < wi.isize; processed_bytes += params->workbsize, ++i)
        {
            size_t ichunk_size = wi.metadata[i].encoded_size;
            if (wi.metadata[i].successfully_encoded)
            {
                size_t ochunk_size = wi.metadata[i].decoded_size;
                size_t ret = CODING_ERROR;
                for (unsigned long j=0; j<params->overhead_iters; ++j)
                {
                    ret = params->decoder(ip,
                                          ichunk_size,
                                          op,
                                          ochunk_size,
                                          params->other);
                }

                if (ret == CODING_ERROR)
                    throw runtime_error("Decoding failed!");

                size_t real_out = (ret / params->ssize) * params->ssize;
                if (real_out != ret)
                    real_out += params->ssize;
                if (job_out + real_out >= wi.isize)
                    job_out += ret;
                else
                    job_out += real_out;
            }
            else
            {
                if (params->verify)
                {
                    memcpy(op, ip, ichunk_size);
                }
            }
            op += params->workbsize;
            ip += params->workbsize;
        }
        assert(total_out <= UINTMAX_MAX - job_out);
        total_out += job_out;
    }
    params->ret = total_out;

    return THREAD_RETURN;
}

//////////////////////////////////////
// THE MAIN TESTING FUNCTION'S HELPERS
//////////////////////////////////////

static inline void prepareEncoderData(Codec & codec,
                                      EncodeParams * params,
                                      size_t threads_no,
                                      size_t bsize,
                                      size_t ssize,
                                      size_t workbsize,
                                      bool verify,
                                      Scheduler * scheduler,
                                      unsigned long overhead_iters)
{
    void ** codec_eparams = codec.eparams();
    for (size_t i = 0; i < threads_no; ++i)
    {
        params[i].encoder = codec.encoder;
        params[i].other = codec_eparams ? (void*) &codec_eparams[i] : 0;
        params[i].bsize = bsize;
        params[i].workbsize = workbsize;
        params[i].ssize = ssize;
        params[i].verify = verify;
        params[i].scheduler = scheduler;
        params[i].can_be_skipped = codec.can_be_skipped;
        params[i].transform_type = codec.encode_transform_type;
        params[i].overhead_iters = overhead_iters;
    }
}

static inline void prepareDecoderData(Codec & codec,
                                      DecodeParams * params,
                                      size_t threads_no,
                                      size_t ssize,
                                      size_t workbsize,
                                      bool verify,
                                      Scheduler * scheduler,
                                      unsigned long overhead_iters)
{
    void ** codec_dparams = codec.dparams();
    for (size_t i = 0; i < threads_no; ++i)
    {
        params[i].decoder = codec.decoder;
        params[i].other = codec_dparams ? (void*) &codec_dparams[i] : 0;
        params[i].workbsize = workbsize;
        params[i].ssize = ssize;
        params[i].verify = verify;
        params[i].scheduler = scheduler;
        params[i].overhead_iters = overhead_iters;
    }
}
// CPU warmup
static void warmup(char ** workbufs,
                   char * inbuf,
                   size_t workbuf_size,
                   size_t inbuf_size,
                   size_t iters)
{
    ITERS(iters)
    {
        // I copy some more that I read, but I want just to touch the memory
        memcpy(workbufs[1], inbuf, min(inbuf_size, workbuf_size));
        memcpy(workbufs[0], workbufs[1], workbuf_size);
    }
}
// reads file size and resets the file position to the beginning of the file
static size_t file_size(ifstream & file)
{
    file.seekg(0, ios_base::end);
    size_t size = file.tellg();
    file.seekg(0, ios_base::beg);
    return size;
}

// prints a warning when something seems wrong with the way tests are performed
static void warn_if_needed(unsigned threads_no,
                           unsigned desired_threads_no,
                           size_t bsize,
                           size_t blocks)
{
    if (threads_no < desired_threads_no)
    {
        cerr << "\nWARNING: You have specified to use " << desired_threads_no
                << " threads, but have " << blocks << " block(s).\n";
        cerr << "         Only " << threads_no << " thread(s) will be used.\n";
        cerr << "         To resolve the issue, specify a lower block size.\n\n";
    }

    if (bsize > DEFAULT_BSIZE)
    {
        cerr << "\nWARNING: You're using a really large block size. Expect troubles.\n\n";
    }
}

static void allocate_working_data(THREAD_HANDLE *& threads,
                                  EncodeParams *& params,
                                  DecodeParams *& dparams,
                                  unsigned threads_no)
{
    // prepare space for thread handlers
    if (threads_no > 1)
    {
        threads = new THREAD_HANDLE[threads_no - 1]; // The main thread does 1 job, so I need 1 handle less
    }

    // prepare placeholders for (de)coding parameters
    params = new EncodeParams[threads_no];
    dparams = new DecodeParams[threads_no];
}

template<typename Params>
static void run_test(THREAD_HANDLE * threads,
                     Params * params,
                     unsigned threads_no,
                     thread_function test_function)
{
    for (unsigned i = 0; i < threads_no - 1; ++i)
    {
        int ret = pthread_create(threads + i,
                                 NULL,
                                 (THREAD_RETURN_TYPE(*)(void*))test_function, params + i);
                                         if (ret)
                                         {
                                             throw runtime_error("Can't start a thread: " +to_string(ret));
                                         }
    }

    // I don't spawn a thread to do the last job, use the current one
    test_function(&params[threads_no - 1]);

    // wait for other threads to do the job
    for (unsigned i = 0; i < threads_no - 1; ++i)
        pthread_join(threads[i], NULL);
}

/**
 * The main testing function
 *
 * @param codecs: a list of codecs to be tested
 * @param input_file: file to be used as a compression source
 * @param desired_bsize: data will be divided into blocks of bsize bytes and each block will be compressed independently.
 * @param desired_threads_no: number of threads to use. The code will not create more threads then blocks though.
 */
Tester::Tester(const list<CodecWithParams> & codecs,
               ifstream * input_file,
               size_t desired_bsize,
               unsigned desired_threads_no) :
               codecs(codecs)
{

    INIT_TIMER(this->ticksPerSecond);

    this->input_size = file_size(*input_file);
    if (this->input_size == 0)
        throw invalid_argument("The file is empty!");

    this->bsize = min(desired_bsize, this->input_size);

    // number of blocks
    this->blocks_no = round_up(this->input_size, this->bsize) / this->bsize;

    this->threads_no = min((size_t) desired_threads_no, this->blocks_no);

    // user warnings
    warn_if_needed(this->threads_no, desired_threads_no, this->bsize, this->blocks_no);

    this->workbuf_block_size = this->bsize; // I need some space for tests
    for (list<CodecWithParams>::const_iterator it = this->codecs.begin(); it != this->codecs.end(); ++it)
        this->workbuf_block_size = max(this->workbuf_block_size, it->codec.max_encoded_size(this->bsize));

    this->workbuf_block_size = round_up(this->workbuf_block_size + OVERRUN_PAD, (size_t)ALIGNMENT);

    this->inbuf_size = this->input_size;
    this->workbuf_size = this->workbuf_block_size * this->blocks_no;

    // allocate possibly misaligned buffers
    this->_inbuf = new (nothrow) char[this->inbuf_size + ALIGNMENT - 1]; // some extra space, so I can align it
    this->_workbuf = new (nothrow) char[2 * this->workbuf_size + ALIGNMENT - 1]; // some extra space, so I can align it
    this->metadatabuf = new (nothrow) BlockInfo[this->blocks_no];

    if (!this->_inbuf || !this->_workbuf || !this->metadatabuf)
    {
        string message = "Not enough memory!\n";
        message += "Tried to allocate " + to_string(this->inbuf_size + ALIGNMENT - 1) + "+"
                + to_string(2 * this->workbuf_size + ALIGNMENT - 1);
        message += "+" + to_string(this->blocks_no * sizeof(BlockInfo)) + " bytes.";
        throw runtime_error(message);
    }
    // aligned buffers
    unsigned misalignment = (uintptr_t)this->_inbuf & (ALIGNMENT - 1);
    this->inbuf = this->_inbuf + ((ALIGNMENT - misalignment) % ALIGNMENT);
    misalignment = (uintptr_t)this->_workbuf & (ALIGNMENT - 1);
    this->workbufs[0] = this->_workbuf + ((ALIGNMENT - misalignment) % ALIGNMENT);
    this->workbufs[1] = this->workbufs[0] + this->workbuf_size; // workbuf_size is a multiply of ALIGNMENT, so this one is aligned too

    input_file->read(this->inbuf, this->input_size);

    if (input_file->fail())
    {
        std::string info = "Error reading file: ";
        info += strerror(errno);
        throw runtime_error(info);
    }

    this->input_crc = crc(this->inbuf, this->input_size, 0);
}

Tester::~Tester()
{
    delete[] this->_inbuf;
    delete[] this->_workbuf;
    delete[] this->metadatabuf;
}

/**
 * The main testing function
 *
 * @param iters: number of measurements to be taken. The function reports the best of them.
 * @param overhead_iters: number of times each block is encoded in a loop
 * @param iter_time: desired time of a single iteration, in ms
 * @param ssize: Sector size - the smallest simulated I/O size. Output size of each block will be rounded up to ssize.
 * @param verify: If true, check if decoding produced correct output.
 * @param warmup_iters: number of memcpy runs before the actual compression, to warm the CPU up.
 * @param csv: output data as csv
 * @param job_size: scheduler produces data in chunks. job_size is a suggested size of such chunk (in bytes).
 */
void Tester::test(unsigned long iters,
                  unsigned long overhead_iters,
                  unsigned iter_time,
                  size_t ssize,
                  bool verify,
                  unsigned warmup_iters,
                  bool csv,
                  size_t job_size)
{
    LARGE_INTEGER cstart_ticks, dstart_ticks, cend_ticks, dend_ticks;

    // warm the CPU up
    warmup(this->workbufs, this->inbuf, this->workbuf_size, this->inbuf_size, warmup_iters);

    printHeaders(csv);
    // some space needed by the main loop
    THREAD_HANDLE * threads = 0;
    EncodeParams * params;
    DecodeParams * dparams;
    allocate_working_data(threads, params, dparams, this->threads_no);

    // the main loop
    for (list<CodecWithParams>::const_iterator it = this->codecs.begin(); it != this->codecs.end(); ++it)
    {
        Codec & codec = it->codec;
        const string & codec_init_params = it->params;

        codec.init(codec_init_params, this->threads_no, min(this->input_size, this->bsize));
        // note: checking can't be done earlier
        //       because codecs are allowed to set (en|de)coders in init()
        if (!check_codec(codec))
            continue;
        printCodec(codec, csv);

        double   best_cspeed = -1;
        double   best_dspeed = -1;
        uint32_t best_ctime = -1;
        uint32_t best_dtime = -1;

        intmax_t best_encoder_input_size  = -1;
        intmax_t best_encoder_output_size = -1;
        intmax_t best_decoder_output_size = -1; // amount of data decompressed successfully. On incompressible files expected to be 0
        unsigned best_citers = 0;
        unsigned best_diters = 0;

        size_t input_buffer = -1; // index of workbufs array
        size_t output_buffer = -1; // index of workbufs array

        ITERS(iters)
        {

            input_buffer = 0;
            output_buffer = 1; // if a transform is in place, it could be 0 as well 

            copy_input_buffer(this->inbuf, this->workbufs[input_buffer], this->input_size, this->bsize, this->workbuf_block_size);

            bool last_block_is_full = round_up(this->input_size, this->bsize) == this->input_size;
            size_t size_in_blocks = this->input_size
                    + (this->workbuf_block_size - this->bsize) * (last_block_is_full ? this->blocks_no : (this->blocks_no - 1));

            Scheduler compression_scheduler(this->workbufs[input_buffer],
                                            this->workbufs[output_buffer],
                                            this->metadatabuf,
                                            size_in_blocks,
                                            this->workbuf_block_size,
                                            job_size,
                                            cstart_ticks,
                                            this->ticksPerSecond,
                                            iter_time);
            // initialize thread data
            prepareEncoderData(codec,
                               params,
                               this->threads_no,
                               this->bsize,
                               ssize,
                               this->workbuf_block_size,
                               verify,
                               &compression_scheduler,
                               overhead_iters);

            // ready...set...go!
            GET_TIME(cstart_ticks);
            run_test(threads, params, this->threads_no, (thread_function) encode);
            GET_TIME(cend_ticks);

            if (verify)
            {
                if (codec.encode_transform_type == Codec::moving)
                    // clear the previous buffer
                    memset(this->workbufs[input_buffer], 0, this->workbuf_size);
                else
                    // clear the working buffer
                    memset(this->workbufs[output_buffer], 0, this->workbuf_size);
            }

            input_buffer =
                    codec.encode_transform_type == Codec::moving ? output_buffer : input_buffer;
            output_buffer = 1 - input_buffer;

            Scheduler decompression_scheduler(this->workbufs[input_buffer],
                                              this->workbufs[output_buffer],
                                              this->metadatabuf,
                                              this->workbuf_size,
                                              this->workbuf_block_size,
                                              job_size,
                                              dstart_ticks,
                                              this->ticksPerSecond,
                                              iter_time);

            // prepare decompression settings
            prepareDecoderData(codec,
                               dparams,
                               this->threads_no,
                               ssize,
                               this->workbuf_block_size,
                               verify,
                               &decompression_scheduler,
                               overhead_iters);

            // ready...set...go!
            GET_TIME(dstart_ticks);
            run_test(threads, dparams, this->threads_no, (thread_function) decode);
            GET_TIME(dend_ticks);

            uint32_t ctime = ticks_to_msec(cstart_ticks, cend_ticks, this->ticksPerSecond);
            uint32_t dtime = ticks_to_msec(dstart_ticks, dend_ticks, this->ticksPerSecond);

            intmax_t encoder_input_size  = 0;
            intmax_t encoder_output_size = 0;
            intmax_t decoder_output_size = 0;
            for (unsigned j = 0; j < this->threads_no; ++j)
            {
                encoder_input_size  += params[j].input_size;
                encoder_output_size += params[j].output_size;
                decoder_output_size += dparams[j].ret;
            }
            encoder_input_size  *= overhead_iters;
            encoder_output_size *= overhead_iters;
            decoder_output_size *= overhead_iters;

            double cspeed = (double)encoder_input_size  / ctime;
            double dspeed = (double)decoder_output_size / dtime;
            if (cspeed > best_cspeed)
            {
                best_cspeed = cspeed;
                best_ctime = ctime;
                best_encoder_input_size  = encoder_input_size;
                best_encoder_output_size = encoder_output_size;
                best_citers = encoder_input_size / this->input_size;
            }
            if (dspeed > best_dspeed)
            {
                best_dspeed = dspeed;
                best_dtime = dtime;
                best_decoder_output_size = decoder_output_size;
                best_diters = decoder_output_size / this->input_size;
            }

            if (!csv) // update the best result after every iteration
            {
                printTime(best_ctime,
                          best_dtime,
                          best_encoder_input_size,
                          best_encoder_output_size,
                          best_decoder_output_size,
                          best_citers,
                          best_diters,
                          csv);
                cout << '\r';
            }
        } // end iters loop
        if (csv) // with csv we haven't written anything yet
            printTime(best_ctime,
                      best_dtime,
                      best_encoder_input_size,
                      best_encoder_output_size,
                      best_decoder_output_size,
                      best_citers,
                      best_diters,
                      csv);
        else
            cout << endl;
        if (verify)
        {
            unsigned data_buffer =
                    codec.decode_transform_type == Codec::moving ? output_buffer : input_buffer;
            check_decoding(this->input_crc, this->workbufs[data_buffer], this->input_size, this->bsize, this->workbuf_block_size);
        }

        codec.cleanup();
    } // end loop over codecs
    printHeaders(csv);

    // TODO: exception safety
    delete[] params;
    delete[] dparams;
    if (threads)
        delete[] threads;
}
