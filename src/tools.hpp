/**
 * Generic helper functions
 *
 * Written by m^2.
 * You can consider the code to be public domain.
 * If your country doesn't recognize author's right to relieve themselves of copyright,
 * you can use it under the terms of WTFPL version 2.0 or later.
 */
#ifndef TOOLS_HPP_BhjgkfG8
#define TOOLS_HPP_BhjgkfG8

#include <cassert>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <typeinfo>

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
#endif

#include "common.hpp"


/////////////////
// Timing
/////////////////

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64)
#include <windows.h>
#define INIT_TIMER(x) if(!QueryPerformanceFrequency(&x)) { cerr<<"QueryPerformance not present"; }
#define GET_TIME(x) QueryPerformanceCounter(&x);
#else
#include <time.h>
typedef struct timespec LARGE_INTEGER;
#define INIT_TIMER(x)
#define GET_TIME(x) if(clock_gettime(CLOCK_REALTIME, &x) == -1){ cerr<<"clock_gettime error"; }
#endif

static inline uint_least64_t ticks_to_msec(const LARGE_INTEGER & start_ticks,
                                           const LARGE_INTEGER & end_ticks,
                                           const LARGE_INTEGER & ticksPerSecond)
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64)
    return (end_ticks.QuadPart - start_ticks.QuadPart) / (ticksPerSecond.QuadPart / 1000);
#else
    UNUSED(ticksPerSecond);
    return 1000 * (end_ticks.tv_sec - start_ticks.tv_sec)
            + (end_ticks.tv_nsec - start_ticks.tv_nsec) / 1000000;
#endif

}

int case_insensitive_compare(const std::string & s1, const std::string & s2);

template<typename T>
std::string to_string(const T & item)
{
    std::stringstream ss;

    ss << item;

    std::string ret;

    while (ss.good())
    {
        char c = ss.get();
        if (ss.good())
            ret += c;
    }
    return ret;
}

template<typename T, typename S>
void from_string(const S & str, T & item)
{
    std::stringstream ss;

    ss << str;
    ss >> item;

    if (ss.good())
        throw std::bad_cast();
}

void setHighestPriority();

#define CODEC_COLOUR 2
#define RESULT_COLOUR 7

struct ConsoleColour
{
    ConsoleColour(int colour);
    ~ConsoleColour();
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64)
private:
    unsigned old_colour;
    HANDLE hcon;
#endif
};

uint32_t crc(char * data, size_t size, uint32_t crc = 0);

/**
 * Rounds A up to the closest multiply of B
 * @note   A has to be in range (-B, numeric_limits<T>::max() - B].
 * @return the rounding result
 */
template<typename T>
static inline size_t round_up(T A, T B)
{
    // rounds A up to the closest multiply of B
    // note: A has to be in range (-B, INT_MAX-B].
    assert(A <= std::numeric_limits<T>::max() - B);
    assert(!std::numeric_limits<T>::is_signed || A > -B);
    T ret = (A / B) * B; // this rounds towards 0
    if (A > ret) // so we need to round up manually
        ret += B;
    return ret;
}

#endif // TOOLS_HPP_BhjgkfG8
