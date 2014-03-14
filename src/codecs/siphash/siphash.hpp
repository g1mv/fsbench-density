/*
 * siphash24.hpp
 *
 *  Created on: 16 Dec 2012
 *      Author: m
 */

#ifndef SIPHASH24_H_o9H7gs
#define SIPHASH24_H_o9H7gs

namespace FsBenchSipHash
{
    extern "C"
    {
        int crypto_auth( unsigned char *out, const unsigned char *in, unsigned long long inlen, const unsigned char *k );
    }
}

#endif /* SIPHASH24_H_o9H7gs */
