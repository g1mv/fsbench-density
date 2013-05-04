/**
 * Written by m^2.
 * You can consider the code to be public domain.
 * If your country doesn't recognize author's right to relieve themselves of copyright,
 * you can use it under the terms of WTFPL version 2.0 or later.
 */

///////////////////////////
// INCLUDES
///////////////////////////
#include "benchmark.hpp"
#include <limits.h>
#include "threads.hpp"
#include "tools.hpp"

#include <cstdio>
#include <cstring>
#include <climits>
#include <fstream>
#include <iostream>
#include <stdexcept>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64)
#include <windows.h>
#define INIT_TIMER(x) if (!QueryPerformanceFrequency(&x)) { cout<<"QueryPerformance not present"; }
#define GET_TIME(x) QueryPerformanceCounter(&x);
#else
#include <time.h>
typedef struct timespec LARGE_INTEGER;
#define INIT_TIMER(x)
#define GET_TIME(x) if(clock_gettime( CLOCK_REALTIME, &x) == -1 ){ cout<<"clock_gettime error"; }
#endif

using namespace std;

///////////////////////////
// MACROS
///////////////////////////

#define KB 1024
#define MB (1024*KB)

//I make all buffers aligned
#define ALIGNMENT 16

#define ITERS(count) for(uint64_t i = 0; i < (uint64_t)(count); ++i)

///////////////////////////
// TYPES
///////////////////////////

typedef THREAD_RETURN_TYPE (*thread_function)(void* params);

///////////////////////////
// HELPERS
///////////////////////////

void copy_input_buffer(const char* inbuf,
                       char* outbuf,
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

uint64_t ticks_to_msec(const LARGE_INTEGER& start_ticks,
                       const LARGE_INTEGER& end_ticks,
                       const LARGE_INTEGER& ticksPerSecond)
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64)
    return (end_ticks.QuadPart - start_ticks.QuadPart) / (ticksPerSecond.QuadPart / 1000);
#else
    return 1000 * (end_ticks.tv_sec - start_ticks.tv_sec)
            + (end_ticks.tv_nsec - start_ticks.tv_nsec) / 1000000;
#endif

}

inline size_t round_up(size_t A, size_t B)
{
    // rounds A up to the closest multiply of B
    // note: A has to be in range (-B, INT_MAX-B].
    size_t ret = (A / B) * B; // this rounds towards 0
    if (A > ret) // so we need to round up manually
        ret += B;
    return ret;
}

unsigned getSmallItersCount(const char* inbuf,
                            char* outbuf,
                            size_t size,
                            unsigned threads_no)
{
    // determine the number of small iters, that is how many times do we need to compress a file to get a single timing large enough to be meaningful
    // I assume that 'meaningful' is 'larger than 0.5 s.'
    // I do it by increasing number of small_iters until it reaches 0.25 s. and then calculate the number needed to get 0.5
    // I estimate that the fastest codecs are 3x slower than memcpy
    // It's quite naive, for example doesn't take threading into account
    // Also, it's tweaked for the fastest codecs and too cautious when testing only slow ones; you'll get good results but you'll have to wait for them longer than necessary
    unsigned small_iters;
    LARGE_INTEGER ticksPerSecond;

    INIT_TIMER(ticksPerSecond);

    for (small_iters = 1; small_iters < 8192; small_iters *= 2) // 8192 is a safe default. small_iters can be increased inside the loop 2+epsilon times
    {
        LARGE_INTEGER start_ticks, end_ticks;
        GET_TIME(start_ticks);
        ITERS(small_iters)
            memcpy(outbuf, inbuf, size);

        GET_TIME(end_ticks);

        uint64_t msec = ticks_to_msec(start_ticks, end_ticks, ticksPerSecond);
        if (msec > 250)
        {
            uint64_t estimation = small_iters;
            estimation *= threads_no;
            estimation *= 1000;
            estimation /= msec; // enough iterations for 1 second
            estimation /= 2; // but I want 1/2 second
            estimation /= 3; // estimation that codecs are 3x slower than memcpy
            small_iters = max(estimation, (uint64_t) 1);
            break;
        }
    }
    if (small_iters <= 0)
        throw runtime_error("Something's wrong, can't determine a right number of small iters.");
    return small_iters;
}

string speed_string(uint64_t bytes, uint64_t msec)
{
    if (msec)
    {
        const string units[] =
            { " B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB" }; //OK, uint64_t is not enough to reach YB ;)
        int units_no = sizeof(units) / sizeof(string);
        uint64_t divider = 1;
        for (int i = 0; i < units_no; ++i, divider *= KB)
        {
            uintmax_t size_per_sec = (((uintmax_t) bytes * 1000) / msec) / divider;
            if (size_per_sec <= 9999)
                return to_string(size_per_sec) + " " + units[i] + "/s";
        }
        // out of units
    }
    return "fast :)";
}

string efficiency_string(uint64_t saved_bytes, uint64_t msec)
{
    if (msec)
    {
        uint64_t divider = 1;
        for (int i = 0; divider < (UINT64_MAX / KB); ++i, divider *= KB)
        {
            uintmax_t size_per_sec = (((uintmax_t) saved_bytes * 1000) / msec) / divider;
            if (size_per_sec <= 9999)
                return to_string(size_per_sec) + "e" + to_string(3 * i);
        }
    }
    return "high :)";
}

///////////////////////////
// OUTPUT PRINTING
///////////////////////////

void printMemcpy(uint64_t milisecs, uint64_t input_size, uint32_t iters, bool csv)
{
    //because of timer inaccuracy some jobs may take 0 ms. And I divide by dmili later...
    milisecs = max(milisecs, (uint64_t) 1);
    if (csv)
    {
        cout << milisecs << " ms," << speed_string(input_size * iters, milisecs) << ","
                << input_size << " B\n";
    }
    else
    {
        streamsize old_width = cout.width(0);
        try
        {
            if (milisecs)
            {
                uint64_t speed = input_size * iters / milisecs * 1000 / MB;
                cout << "memcpy: " << milisecs << " ms, " << input_size << " bytes = "
                        << speed << " MB/s\n";
            }
            else
                cout << "memcpy: done in 0 ms";
        }
        catch(...)
        {
            cout.width(old_width);
            throw;
        }
        cout.width(old_width);
    }
}

void printTime(uint64_t cmili,
               uint64_t dmili,
               uint64_t input_size,
               uint64_t compressible_bytes,
               uint64_t compressed_size,
               uint32_t iters,
               bool csv)
{
    //because of timer inaccuracy some jobs may take 0 ms. And I divide by dmili later...
    dmili = max(dmili, (uint64_t) 1);

    if (csv)
    {
        cout << cmili << "," << dmili << "," << compressed_size << "," << compressible_bytes
                << endl;
    }
    else
    {
        ConsoleColour cc(RESULT_COLOUR);
        // I suck at iostream
        char buf[8];
        sprintf(buf, "%.3f", (float) input_size / compressed_size);
        int len = strlen(buf);
        string compression_rate;
        for (int i = 0; i < 6 - len; ++i)
            compression_rate += ' ';
        compression_rate += buf;

        streamsize old_width = cout.width(11);
        try
        {
            cout << std::right << compressed_size;
            cout << " (x" << compression_rate << ")      ";
            string decoding_speed_str =
                    compressible_bytes ? speed_string(input_size * iters, dmili) :
                                         string("-");
            cout.width(9);
            cout << speed_string(input_size * iters, cmili) << " ";
            cout.width(9);
            cout << decoding_speed_str << "      ";
            cout.width(6);
            cout << efficiency_string(input_size - compressed_size, cmili) << " ";
            cout.width(6);
            cout << efficiency_string(input_size - compressed_size, dmili);
        }
        catch(...)
        {
            cout.width(old_width);
            throw;
        }
        cout.width(old_width);
    }
}
void printHeaders()
{
    {
        ConsoleColour cc(CODEC_COLOUR);
        cout << "Codec                                   version      args" << endl;
    }
    {
        ConsoleColour cc(RESULT_COLOUR);
        cout << "C.Size      (C.Ratio)        C.Speed   D.Speed      C.Eff. D.Eff." << endl;
    }
}
void printCodec(const Codec& codec, bool csv)
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
void check_decoding(uint32_t input_crc,
                    char* buf,
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
THREAD_RETURN_TYPE encode(EncodeParams* params)
{
    uintmax_t total = 0; // summing up across small iterations can yield a large number
    uintmax_t successfully_encoded_bytes = 0;

    total = 0;

    Scheduler::WorkItem wi;
    while (params->scheduler->getChunk(wi) == 0)
    {
        char* p = wi.out;
        size_t processed_bytes = 0;
        unsigned i = 0;
        for (; processed_bytes < wi.isize; processed_bytes += params->workbsize, ++i)
        {
            size_t chunk_size = min(wi.isize - processed_bytes, params->bsize);
            size_t ret = params->encoder(wi.in + processed_bytes,
                                         chunk_size,
                                         p,
                                         round_up(chunk_size - params->ssize,
                                                  params->ssize),
                                         params->other);

            bool failed_to_encode = ret == CODING_ERROR;

            size_t real_size = round_up(ret, params->ssize);

            if (!failed_to_encode && params->can_be_skipped)
            {
                failed_to_encode = real_size >= params->bsize || real_size >= chunk_size;
            }
            if (failed_to_encode)
            {
                //FIXME: if not params->can_be_skipped this should return error
                //       This can wait as after SkippableCodec gets implemented there will be just error in this place
                wi.metadata[i].successfully_encoded = false;
                wi.metadata[i].encoded_size = params->bsize;
                wi.metadata[i].decoded_size = params->bsize;
                total += round_up(chunk_size, params->ssize);
                if (params->verify)
                {
                    // optimisation: When unencoded data gets stored,
                    // there's no need for memcpy
                    // However, it's needed to verify if encoding succeeded

                    // Note: it's needed only for moving transforms, but doesn't hurt otherwise
                    // FIXME: What if a skippable transform failed and modified its input buffer?
                    memcpy(p, wi.in + processed_bytes, chunk_size);
                }
            }
            else
            {
                wi.metadata[i].successfully_encoded = true;
                wi.metadata[i].encoded_size = ret;
                wi.metadata[i].decoded_size = chunk_size;
                total += real_size;
                successfully_encoded_bytes += chunk_size;
                if (params->verify)
                {
                    // zero-out the part of the buffer that doesn't contain encoded data
                    // if a codec cheats by reporting size smaller than what it actually needs,
                    // this will detect it
                    if (params->transform_type == Codec::moving)
                        memset(p + ret, 0, params->workbsize - ret);
                    else
                        memset(wi.in + processed_bytes + ret, 0, params->workbsize - ret);
                }
            }
            p += params->workbsize;
        }
    }
    params->encoded_bytes = total;
    params->successfully_encoded_bytes = successfully_encoded_bytes;
    return THREAD_RETURN;
}

THREAD_RETURN_TYPE decode(DecodeParams* params)
{
    uintmax_t total_out = 0; // summing up across small iterations can yield a large number

    Scheduler::WorkItem wi;
    while (params->scheduler->getChunk(wi) >= 0)
    {
        ptrdiff_t job_out = 0;
        char* ip = wi.in;
        char* op = wi.out;
        unsigned i = 0;
        size_t processed_bytes = 0;
        for (; processed_bytes < wi.isize; processed_bytes += params->workbsize, ++i)
        {
            size_t ichunk_size = wi.metadata[i].encoded_size;
            if (wi.metadata[i].successfully_encoded)
            {
                size_t ochunk_size = wi.metadata[i].decoded_size;
                size_t ret = params->decoder(ip,
                                             ichunk_size,
                                             op,
                                             ochunk_size,
                                             params->other);

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
                job_out += ichunk_size;
            }
            op += params->workbsize;
            ip += params->workbsize;
        }
        total_out += job_out;
    }
    params->ret = total_out;

    return THREAD_RETURN;
}

//////////////////////////////////////
// THE MAIN TESTING FUNCTION'S HELPERS
//////////////////////////////////////

inline void prepareEncoderData(Codec& codec,
                               EncodeParams* params,
                               size_t threads_no,
                               size_t bsize,
                               size_t ssize,
                               size_t workbsize,
                               bool verify,
                               Scheduler* scheduler)
{
    void** codec_eparams = codec.eparams();
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
    }
}

inline void prepareDecoderData(Codec& codec,
                               DecodeParams* params,
                               size_t threads_no,
                               size_t ssize,
                               size_t workbsize,
                               bool verify,
                               Scheduler* scheduler)
{
    void** codec_dparams = codec.dparams();
    for (size_t i = 0; i < threads_no; ++i)
    {
        params[i].decoder = codec.decoder;
        params[i].other = codec_dparams ? (void*) &codec_dparams[i] : 0;
        params[i].workbsize = workbsize;
        params[i].ssize = ssize;
        params[i].verify = verify;
        params[i].scheduler = scheduler;
    }
}
// CPU warmup
void warmup(char** workbufs,
            char* inbuf,
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
size_t file_size(ifstream& file)
{
    file.seekg(0, ios_base::end);
    size_t size = file.tellg();
    file.seekg(0, ios_base::beg);
    return size;
}

// prints a warning when something seems wrong with the way tests are performed
void warn_if_needed(unsigned threads_no,
                    unsigned desired_threads_no,
                    size_t input_size,
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

    if (input_size < 16 * MB * threads_no)
        cerr << "\nWARNING: This file is too small, use at least 16 MB per thread.\n\n";

    if (bsize > DEFAULT_BSIZE)
    {
        cerr << "\nWARNING: You're using a really large block size. Expect troubles.\n\n";
    }
}

void allocate_working_data(THREAD_HANDLE*& threads,
                           EncodeParams*& params,
                           DecodeParams*& dparams,
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
void run_test(THREAD_HANDLE* threads,
              Params* params,
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
 * @param iters: number of measurements to be taken. The function reports the best of them.
 * @param small_iters: desired number of iterations for each measurement. 0 to determine it automatically.
 * @param bsize: data will be divided into blocks of bsize bytes and each block will be compressed independently.
 * @param ssize: Sector size - the smallest simulated I/O size. Output size of each block will be rounded up to ssize.
 * @param verify: If true, check if decoding produced correct output.
 * @param warmup_iters: number of memcpy runs before the actual compression, to warm the CPU up.
 * @param desired_threads_no: number of threads to use. The code will not create more threads then blocks though.
 * @param csv: output data as csv
 * @param job_size: scheduler produces data in chunks. job_size is a suggested size of such chunk (in bytes).
 * 
 * @return number of small_iters actually used. The same as small_iters unless small_iters == 0.
 */
unsigned test(list<CodecWithParams>& codecs,
              ifstream& input_file,
              unsigned iters,
              unsigned small_iters,
              size_t bsize,
              size_t ssize,
              bool verify,
              unsigned warmup_iters,
              unsigned desired_threads_no,
              bool csv,
              size_t job_size)
{
    LARGE_INTEGER ticksPerSecond, cstart_ticks, dstart_ticks, cend_ticks, dend_ticks;

    INIT_TIMER(ticksPerSecond);

    size_t size = file_size(input_file);
    if (size == 0)
        throw invalid_argument("The file is empty!");
    bsize = min(bsize, size);

    // number of blocks
    size_t blocks = round_up(size, bsize) / bsize;

    unsigned threads_no = min((size_t) desired_threads_no, blocks);

    // user warnings
    warn_if_needed(threads_no, desired_threads_no, size, bsize, blocks);

    size_t workbuf_block_size = bsize; // I need some space for tests
    for (list<CodecWithParams>::iterator it = codecs.begin(); it != codecs.end(); ++it)
        workbuf_block_size = max(workbuf_block_size, it->codec.max_encoded_size(bsize));

    workbuf_block_size = round_up(workbuf_block_size + OVERRUN_PAD, ALIGNMENT);

    size_t inbuf_size = size + OVERRUN_PAD;
    size_t workbuf_size = workbuf_block_size * blocks;

    // allocate possibly misaligned buffers
    char* _inbuf = new (nothrow) char[inbuf_size + ALIGNMENT - 1]; // some extra space, so I can align it
    char* _workbuf = new (nothrow) char[2 * workbuf_size + ALIGNMENT - 1]; // some extra space, so I can align it
    BlockInfo* metadatabuf = new (nothrow) BlockInfo[blocks];

    if (!_inbuf || !_workbuf || !metadatabuf)
    {
        string message = "Not enough memory!\n";
        message += "Tried to allocate " + to_string(inbuf_size + ALIGNMENT - 1) + "+"
                + to_string(2 * workbuf_size + ALIGNMENT - 1);
        message += "+" + to_string(blocks * sizeof(BlockInfo)) + " bytes.";
        throw runtime_error(message);
    }

    // aligned buffers
    char* workbufs[2];
    char* inbuf = (char*) (
            ((uintptr_t) _inbuf & ~(ALIGNMENT - 1)) ? ((uintptr_t) _inbuf & ~(ALIGNMENT - 1))
                                                              + ALIGNMENT :
                                                      (uintptr_t) _inbuf & ~(ALIGNMENT - 1));
    workbufs[0] = (char*) (
            ((uintptr_t) _workbuf & ~(ALIGNMENT - 1)) ? ((uintptr_t) _workbuf & ~(ALIGNMENT - 1))
                                                                + ALIGNMENT :
                                                        (uintptr_t) _workbuf & ~(ALIGNMENT - 1));
    workbufs[1] = workbufs[0] + workbuf_size; // workbuf_size is a multiply of ALIGNMENT, so this one is aligned too

    input_file.read(inbuf, size);

    if (input_file.fail())
        throw runtime_error("Error reading file!");

    uint32_t input_crc = crc(inbuf, size, 0);

    // warm the CPU up
    warmup(workbufs, inbuf, workbuf_size, inbuf_size, warmup_iters);

    if (!small_iters)
    {
        small_iters = getSmallItersCount(inbuf, workbufs[1], size, threads_no);
    }

    // check memcpy performance
    GET_TIME(cstart_ticks);
    ITERS(small_iters)
        memcpy(workbufs[1], inbuf, size);
    GET_TIME(cend_ticks);
    printMemcpy(ticks_to_msec(cstart_ticks, cend_ticks, ticksPerSecond), size, small_iters, csv);

    printHeaders();
    // some space needed by the main loop
    THREAD_HANDLE* threads = 0;
    EncodeParams* params;
    DecodeParams* dparams;
    allocate_working_data(threads, params, dparams, threads_no);

    // the main loop
    for (list<CodecWithParams>::iterator it = codecs.begin(); it != codecs.end(); ++it)
    {
        Codec& codec = it->codec;
        const string& codec_init_params = it->params;

        codec.init(codec_init_params, threads_no, min(size, bsize));
        printCodec(codec, csv);

        uint32_t fastest_ctime = -1;
        uint32_t fastest_dtime = -1;

        intmax_t complen = -1;
        intmax_t compressible_bytes = -1; // amount of data compressed successfully. On incompressible files expected to be 0

        size_t input_buffer = -1; // index of workbufs array
        size_t output_buffer = -1; // index of workbufs array

        ITERS(iters)
        {
            complen = 0;
            compressible_bytes = 0;

            input_buffer = 0;
            output_buffer = 1; // if a transform is in place, it could be 0 as well 

            copy_input_buffer(inbuf, workbufs[input_buffer], size, bsize, workbuf_block_size);

            bool last_block_is_full = round_up(size, bsize) == size;
            size_t size_in_blocks = size
                    + (workbuf_block_size - bsize) * (last_block_is_full ? blocks : (blocks - 1));

            Scheduler scheduler = Scheduler(workbufs[input_buffer],
                                            workbufs[output_buffer],
                                            metadatabuf,
                                            size_in_blocks,
                                            workbuf_block_size,
                                            small_iters,
                                            job_size);
            // initialize thread data
            prepareEncoderData(codec,
                               params,
                               threads_no,
                               bsize,
                               ssize,
                               workbuf_block_size,
                               verify,
                               &scheduler);

            // ready...set...go!
            GET_TIME(cstart_ticks);
            run_test(threads, params, threads_no, (thread_function) encode);
            GET_TIME(cend_ticks);

            for (unsigned j = 0; j < threads_no; ++j)
            {
                complen += params[j].encoded_bytes;
                compressible_bytes += params[j].successfully_encoded_bytes;
            }

            complen /= small_iters;
            compressible_bytes /= small_iters;

            if (verify)
            {
                if (codec.encode_transform_type == Codec::moving)
                    // clear the previous buffer
                    memset(workbufs[input_buffer], 0, workbuf_size);
                else
                    // clear the working buffer
                    memset(workbufs[output_buffer], 0, workbuf_size);
            }

            input_buffer =
                    codec.encode_transform_type == Codec::moving ? output_buffer : input_buffer;
            output_buffer = 1 - input_buffer;

            scheduler = Scheduler(workbufs[input_buffer],
                                  workbufs[output_buffer],
                                  metadatabuf,
                                  workbuf_size,
                                  workbuf_block_size,
                                  small_iters,
                                  job_size);

            // prepare decompression settings
            prepareDecoderData(codec,
                               dparams,
                               threads_no,
                               ssize,
                               workbuf_block_size,
                               verify,
                               &scheduler);

            // ready...set...go!
            GET_TIME(dstart_ticks);
            run_test(threads, dparams, threads_no, (thread_function) decode);
            GET_TIME(dend_ticks);

            uint32_t ctime = ticks_to_msec(cstart_ticks, cend_ticks, ticksPerSecond);
            uint32_t dtime = ticks_to_msec(dstart_ticks, dend_ticks, ticksPerSecond);

            fastest_ctime = min(fastest_ctime, ctime);
            fastest_dtime = min(fastest_dtime, dtime);

            if (!csv) // update the best result after every iteration
            {
                printTime(fastest_ctime,
                          fastest_dtime,
                          size,
                          compressible_bytes,
                          complen,
                          small_iters,
                          csv);
                cout << '\r';
            }
        } // end iters loop
        if (csv) // with csv we haven't written anything yet
            printTime(fastest_ctime,
                      fastest_dtime,
                      size,
                      compressible_bytes,
                      complen,
                      small_iters,
                      csv);
        else
            cout << endl;
        if (verify)
        {
            unsigned data_buffer =
                    codec.decode_transform_type == Codec::moving ? output_buffer : input_buffer;
            check_decoding(input_crc, workbufs[data_buffer], size, bsize, workbuf_block_size);
        }

        codec.cleanup();
    } // end loop over codecs
    printHeaders();

    delete[] params;
    delete[] dparams;
    if (threads)
        delete[] threads;

    delete[] _inbuf;
    delete[] _workbuf;
    delete[] metadatabuf;
    return small_iters;
}
