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
#include "tools.hpp"

struct EncodeParams
{
    encoder_t encoder;
    Scheduler * scheduler;
    void * other;
    size_t workbsize; // size of each input block
    size_t bsize; // size of actual data in an input block; ibsize - padding
    size_t ssize;
    bool verify; // whether decompression should be verified. Affects compressed block format.
    uintmax_t input_size;  // number of bytes processed
    uintmax_t output_size; // number of bytes produced
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
    Tester(const std::list<CodecWithParams> & codecs,
           std::ifstream * input_file,
           size_t bsize,
           unsigned desired_threads_no);

    void test(unsigned iters,
              unsigned iter_time,
              size_t ssize,
              bool verify,
              unsigned warmup_iters,
              bool csv,
              size_t job_size);
    ~Tester();

private:
    const std::list<CodecWithParams> & codecs;
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
const unsigned long DEFAULT_ITER_TIME = 500;
const int DEFAULT_SSIZE = 1;
const int DEFAULT_THREADS_NO = 1;
const int DEFAULT_WARMUP_ITERS = 10;

#endif // BENCHMARK_HPP_BhjgkfG8
