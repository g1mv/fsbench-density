/**
 * The main benchmarking interface
 *
 * Written by m^2.
 * You can consider the code to be public domain.
 * If your country doesn't recognize author's right to relieve themselves of copyright,
 * you can use it under the terms of WTFPL version 2.0 or later.
 */
#ifndef BENCHMARK_HPP_BhjgkfG8
#define BENCHMARK_HPP_BhjgkfG8

#define __STDC_LIMIT_MACROS

#include "codecs.hpp"
#include "scheduler.hpp"

#if __cplusplus >= 201103L // C++ 2011
#include <cstdint>
#else
extern "C"
{
#include <stdint.h>
}
#endif // C++ 2011

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

struct EncodeParams
{
    encoder_t encoder;
    Scheduler * scheduler;
    void * other;
    size_t workbsize; // size of each input block
    size_t bsize; // size of actual data in an input block; ibsize - padding
    size_t ssize;
    bool verify; // whether decompression should be verified. Affects compressed block format.
    uintmax_t successfully_encoded_bytes;// number of bytes processed successfully.
    // I.e. when compressing incompressible data expected to be 0.
    // Summed up across small iterations, can yield a large number.
    uintmax_t encoded_bytes;
    bool can_be_skipped; // when a compressor fails to shrink a block by at least X bytes, the data is left uncompressed,
    // which affects the subsequent parts of the encoding pipeline as well as decoding
    // but skipping of other transforms (checksumming, encryption) shouldn't be allowed
    Codec::transform_type transform_type;
};
struct DecodeParams
{
    decoder_t decoder;
    Scheduler * scheduler;
    void * other;
    size_t workbsize; // size of each input block
    size_t ssize;
    bool verify;
    uintmax_t ret;
};

class Tester
{
public:
    Tester(std::list<CodecWithParams> & codecs,
           std::ifstream & input_file,
           size_t bsize,
           unsigned desired_threads_no);

    unsigned test(unsigned iters,
                  unsigned small_iters,
                  size_t ssize,
                  bool verify,
                  unsigned warmup_iters,
                  bool csv,
                  size_t job_size);
    ~Tester();

private:
    std::list<CodecWithParams> & codecs;
    LARGE_INTEGER ticksPerSecond;
    size_t bsize;
    size_t input_size;
    size_t blocks_no;
    unsigned threads_no;
    char * workbufs[2];
    char * inbuf;
    char * _inbuf; ///< Possibly misaligned buffer holding inbuf
    char * _workbuf; ///< Possibly misaligned buffer holding both workbufs
    size_t workbuf_block_size;

    size_t inbuf_size;
    size_t workbuf_size;
    BlockInfo * metadatabuf;
    uint32_t input_crc;
};

///////////////////////////
// CONSTANTS
///////////////////////////

const int DEFAULT_ITERATIONS = 3;
// 1.6 GB
const unsigned long DEFAULT_BSIZE = 1717986918;
const int DEFAULT_SSIZE = 1;
const int DEFAULT_THREADS_NO = 1;
const int DEFAULT_WARMUP_ITERS = 10;

#endif // BENCHMARK_HPP_BhjgkfG8
