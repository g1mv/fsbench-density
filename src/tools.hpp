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
    assert(A >= std::numeric_limits<T>::max() - B);
    assert(!std::numeric_limits<T>::is_signed || A > -B);
    T ret = (A / B) * B; // this rounds towards 0
    if (A > ret) // so we need to round up manually
        ret += B;
    return ret;
}

#endif // TOOLS_HPP_BhjgkfG8
