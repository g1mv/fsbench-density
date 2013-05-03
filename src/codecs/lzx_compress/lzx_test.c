/*
    File lzx_test.c, adjunct to lzxcomp library
    Copyright (C) 2002 Matthew T. Russotto

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; version 2.1 only

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/*
 *  This test program reads uncompressed data from standard in, 
 *  which must be a file, and writes compressed data (preceded by the
 *  length of the uncompressed data) on standard out 
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "lzx_config.h"
#include "lzx_compress.h"
static int test_get_bytes(void *arg, int n, void *buf)
{
  return fread(buf, 1, n, (FILE *)arg);
}

static int test_put_bytes(void *arg, int n, void *buf)
{
  return fwrite(buf, 1, n, (FILE *)arg);
}

static void test_mark_frame(void *arg, uint32_t uncomp, uint32_t comp)
{
#ifdef DEBUG_MARK_FRAME
  fprintf((FILE *)arg, "Aligned data at %d (in compressed stream, %d)\n", uncomp, comp);
#endif
}

static int at_eof(void *arg)
{
  return feof((FILE *)arg);
}

int main(int argc, char *argv[])
{
  int wsize_code = atoi(argv[1]);
  lzx_data *lzxd;
  int subd_ok = 1;
  int do_reset = 1;
  uint32_t total_length;
  int block_size;
  lzx_results lzxr;

  if ((wsize_code < 15) || (wsize_code > 21)) {
    fprintf(stderr, "Window size must be between 15 and 21 inclusive\n");
    exit(-1);
  }
  if (argc > 2) {
    subd_ok = atoi(argv[2])&1;
    do_reset = atoi(argv[2])&2;
  }
  lzx_init(&lzxd, wsize_code, test_get_bytes, stdin, at_eof, 
	   test_put_bytes, stdout, test_mark_frame, stderr);

  /* undocumented fact, according to Caie -- block size cannot exceed window size.  (why not?) */
  block_size = 1<< (wsize_code) ;

  fseek(stdin, 0, SEEK_END);
  total_length = ftell(stdin);
  fseek(stdin, 0, SEEK_SET);
#ifndef LZX_BIG_ENDIAN
  total_length = ((total_length & 0xFF000000) >> 24) |
    ((total_length & 0x000000FF) << 24) |
    ((total_length & 0x00FF0000) >> 8) |
    ((total_length & 0x0000FF00) << 8);

#endif
  fwrite(&total_length, 1, sizeof(uint32_t), stdout);

  while (!at_eof(stdin)) {
    if (do_reset)
      lzx_reset(lzxd);
    lzx_compress_block(lzxd, block_size, subd_ok);
  }
  lzx_finish(lzxd, &lzxr);
  fprintf(stderr, "compressed: %ld, uncompressed %ld, ratio %f\n", lzxr.len_compressed_output, lzxr.len_uncompressed_input, 100.0 - 100 * (double)lzxr.len_compressed_output/(double)lzxr.len_uncompressed_input);
  return 0;
}
