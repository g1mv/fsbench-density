/*
(C) 2011, Dell Inc. Written by Przemyslaw Skibinski (inikep@gmail.com)

    LICENSE

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 3 of
    the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details at
    Visit <http://www.gnu.org/copyleft/gpl.html>.

*/

#define FREEARC_STANDALONE_TORNADO

#include "Tornado.cpp"
#include "Common.cpp"
#include <stdint.h>


int compress_all_at_once = 0;

struct Results
{
	uint32_t inlen; uint32_t outlen;
	uint32_t inpos;
	uint8_t *inbuf; uint8_t *outbuf;
};

// Callback function called by compression routine to read/write data.
// Also it's called by the driver to init/shutdown its processing
int ReadWriteCallback (const char *what, void *buf, int size, void *r_)
{
  Results &r = *(Results*)r_;        // Accumulator for compression statistics

//  printf("what=%s size=%d\n", what, size);

  if (strequ(what,"init")) {

	  r.inpos = r.outlen = 0;
	  return FREEARC_OK;

  } else if (strequ(what,"read")) {
    if (r.inpos + size > r.inlen)
		size = r.inlen - r.inpos;

	memcpy(buf, r.inbuf+r.inpos, size);
	r.inpos += size;
    return size;

  } else if (strequ(what,"write") || strequ(what,"quasiwrite")) {
    if (strequ(what,"write")) {
		memcpy(r.outbuf+r.outlen, buf, size);
		r.outlen += size;
		return size;
	}
    return 0; // ignored by caller

  } else if (strequ(what,"done")) {
    // Print final compression statistics
    return FREEARC_OK;

  } else {
    return FREEARC_ERRCODE_NOT_IMPLEMENTED;
  }
}

uint32_t tor_compress(uint8_t method, uint8_t* inbuf, uint8_t* outbuf, uint32_t size)
{
	Results r;
	r.inbuf = inbuf;
	r.outbuf = outbuf;
	r.inlen = size;

	ReadWriteCallback ("init", NULL, 0, &r);
    PackMethod m = std_Tornado_method[method];
	if (r.inlen >= 0)
		m.buffer = mymin (m.buffer, r.inlen+LOOKAHEAD*2);
	int result = tor_compress (m, ReadWriteCallback, &r);
	return r.outlen;
}

uint32_t tor_decompress(uint8_t* inbuf, uint8_t* outbuf, uint32_t size)
{
	Results r;
	r.inbuf = inbuf;
	r.outbuf = outbuf;
	r.inlen = size;

	ReadWriteCallback ("init", NULL, 0, &r);
	int result = tor_decompress(ReadWriteCallback, &r);
	ReadWriteCallback ("done", NULL, 0, &r);
	return r.outlen;
}
