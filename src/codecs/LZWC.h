/*
 * LZWC.h
 *
 *  Created on: 15 Jan 2013
 *      Author: m
 */

#ifndef LZWC_H_
#define LZWC_H_


size_t lzwc_compress(const unsigned char* input, size_t input_size, unsigned char* output, size_t output_size);
size_t lzwc_decompress(const unsigned char* input, size_t input_size, unsigned char* output, size_t output_size);

#endif /* LZWC_H_ */
