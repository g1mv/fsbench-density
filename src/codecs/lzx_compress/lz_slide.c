/*
    File lz_slide.c, part of lzxcomp library
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
 *  This file implements a Lempel-Ziv compression mechanism on top of a
 *  sliding-window string search algorithm.  Currently hash_slide.c, which
 *  uses a hash table method similar to that of gnuzip/zlib, is used.
 *
 *  Previously, a sliding-window suffix tree method was tried, but it
 *  resulted in poor compression at the LZX layer because it did not get the
 *  closest matching string.  SHORTSLIDE was an attempt to get around this
 *  by maintaining a close tree and a full tree.
 * 
 *  A binary search tree was also tried, but it was too slow, particularly
 *  when searching for the closest match
 *
 *  LZ_ONEBUFFER instructs the algorithm to use the same circular buffer for
 *  matches (lookahead) and the window itself.  This is required when
 *  hash_slide is used.
 * 
 *  LAZY is lazy evaluation, similar to the gnuzip/zlib method.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "hash_slide.h"
#include "lz_slide.h"

#define MAX_MATCH 253
#define MIN_MATCH 2

#ifdef SHORTSLIDE
#ifdef LZ_ONEBUFFER
#error Short Slide cannot be used with One Buffer
#endif
#endif
void lz_init(lz_info *lzi, int wsize, int max_match, int min_match,
	     int frame_size,
	     get_chars_t get_chars,
	     output_match_t output_match,
	     output_literal_t output_literal, void *user_data)
{
  assert(sizeof(SYMB) == sizeof(u_char));
  
  lzi->wsize = wsize;
  if (max_match > wsize)
    lzi->max_match = wsize;
  else
    lzi->max_match = max_match;

#ifdef LAZY
  lzi->match_buf_size = lzi->max_match + 1;
#else
  lzi->match_buf_size = lzi->max_match;
#endif

  lzi->min_match = min_match;


#ifdef LZ_ONEBUFFER
  lzi->slide_buf_size = wsize + lzi->match_buf_size;
  lzi->slide_buf = malloc(lzi->slide_buf_size);
  lzi->slide_bufe = lzi->slide_buf + lzi->slide_buf_size;

  lzi->match_buf = lzi->slide_buf;
  lzi->match_bufp = lzi->match_buf;
  lzi->match_bufe = lzi->slide_bufe;
#else
  lzi->slide_buf = malloc(wsize);
  lzi->slide_bufp = lzi->slide_buf;
  lzi->slide_bufe = lzi->slide_buf + wsize;

  lzi->match_buf = malloc(lzi->match_buf_size);
  lzi->match_bufp = lzi->match_buf;
  lzi->match_bufe = lzi->match_buf + lzi->match_buf_size;
#endif

#ifdef SHORTSLIDE
  /* it is possible to use the same slide buffer for both.  But that would use
     MORE memory, at least unless I can fix the slide code to allocate fewer nodes
  */
  lzi->short_slide = malloc(2048);
  lzi->short_slidep = lzi->short_slide;
  lzi->short_slidee = lzi->short_slide + 2048;
  lzi->chars_in_short_slide = 0;
  lzi->short_front_offset = 0;
  initslide(2048, lzi->short_slide, &lzi->short_tree);
#endif

  assert(lzi->slide_buf != NULL);
  assert(lzi->match_buf != NULL);
  
  lzi->frame_size = frame_size;
  lzi->chars_in_slide = lzi->chars_in_match = lzi->front_offset = 0;
  lzi->cur_loc = 0;
  lzi->loc_in_frame = 0;
  lzi->get_chars = get_chars;
  lzi->output_match = output_match;
  lzi->output_literal = output_literal;
  lzi->user_data = user_data;
  inithash(lzi->slide_buf_size, lzi->slide_buf, &lzi->main_tree);
}

void lz_release(lz_info *lzi)
{
  releasehash(lzi->main_tree);
#ifdef SHORTSLIDE
  releaseslide(lzi->short_tree);
  free(lzi->short_slide);
#endif
  free(lzi->slide_buf);
#ifndef LZ_ONEBUFFER
  free(lzi->match_buf);
#endif
}

void lz_reset(lz_info *lzi)
{
  lzi->slide_bufp = lzi->slide_buf;
  lzi->match_bufp = lzi->match_buf;
  lzi->chars_in_slide = lzi->chars_in_match = lzi->front_offset = 0;
  lzi->loc_in_frame = 0;
  resethash(lzi->main_tree);

#ifdef SHORTSLIDE
  lzi->short_slidep = lzi->short_slide;
  lzi->chars_in_short_slide = 0;
  lzi->short_front_offset = 0;
  resetslide(lzi->short_tree);
#endif
}

#ifdef LZSLIDE_MAIN
typedef struct lz_user_data
{
  FILE *infile;
  FILE *outfile;
  int R0, R1, R2;
} lz_user_data;

int tmp_get_chars(lz_info *lzi, int n, u_char *buf)
{
  lz_user_data *lzud = (lz_user_data *)lzi->user_data;
  return fread(buf, 1, n, lzud->infile);
}

int tmp_output_match(lz_info *lzi, int match_pos, int match_len)
{
  lz_user_data *lzud = (lz_user_data *)lzi->user_data;
  int mod_match_loc;
#if 0
  if (match_pos == lzud->R0) 
    match_pos = 0;
  else if (match_pos == lzud->R1) {
    lzud->R1 = lzud->R0;
    lzud->R0 = match_pos;
    match_pos = 1;
  }
  else if (match_pos == lzud->R2) {
    lzud->R2 = lzud->R0;
    lzud->R0 = match_pos;
    match_pos = 2;
  }
  else {
    lzud->R2 = lzud->R1;
    lzud->R1 = lzud->R0;
    lzud->R0 = match_pos;
  }
#endif
  mod_match_loc = match_pos + lzi->front_offset;
  if (mod_match_loc < 0)
    mod_match_loc += lzi->slide_buf_size;
  
  fprintf(lzud->outfile, "(%d, %d)(%d)\n", match_pos, match_len, mod_match_loc);
  return 0;
}

void tmp_output_literal(lz_info *lzi, u_char ch)
{
  lz_user_data *lzud = (lz_user_data *)lzi->user_data;
  fprintf(lzud->outfile, "'%c'", ch);
}

int main(int argc, char *argv[])
{
  int wsize = atoi(argv[1]);
  lz_info lzi;
  lz_user_data lzu = {stdin, stdout, 1, 1, 1};

  lz_init(&lzi, wsize, MAX_MATCH, MIN_MATCH, tmp_get_chars, tmp_output_match, tmp_output_literal,&lzu);
  lz_compress(&lzi);
}
#endif

static void advance_slide(lz_info *lzi, int nchars)
{
  int i;
  int excess = lzi->chars_in_slide + nchars - lzi->wsize;

  if (excess > 0) {
    advancetail(lzi->main_tree, excess);
    lzi->chars_in_slide -= excess;
  }

#ifdef SHORTSLIDE
  excess = lzi->chars_in_short_slide + nchars - 2048;
  if (excess > 0) {
    advancetail(lzi->short_tree, excess);
    lzi->chars_in_short_slide -= excess;
  }
#endif


#ifndef LZ_ONEBUFFER
  /* copy from match to slide */
  for (i = 0; i < nchars; i++) {
#ifdef SHORTSLIDE
    *(lzi->short_slidep++) = *(lzi->match_bufp);
    if (lzi->short_slidep == lzi->short_slidee)
      lzi->short_slidep = lzi->short_slide;
#endif
    *(lzi->slide_bufp++) = *(lzi->match_bufp++);

    if (lzi->slide_bufp == lzi->slide_bufe)
      lzi->slide_bufp = lzi->slide_buf;
    if (lzi->match_bufp == lzi->match_bufe) 
      lzi->match_bufp = lzi->match_buf;
  }
#else
  /* move the match buffer to keep up with the window */
  lzi->match_bufp += nchars;
  if (lzi->match_bufp >= lzi->slide_bufe) 
    lzi->match_bufp -= lzi->slide_buf_size;
#endif

#ifdef USE_PSBST
#ifndef LZ_ONEBUFFER
#error Need LZ_ONEBUFFER with PBST
#endif
  advancefront(lzi->main_tree, nchars, lzi->max_match, lzi->chars_in_match);
#else
  advancefront(lzi->main_tree, nchars);
#endif
  lzi->front_offset += nchars;
  if (lzi->front_offset >= lzi->slide_buf_size) lzi->front_offset -= lzi->slide_buf_size;
  lzi->chars_in_slide += nchars;
#ifdef SHORTSLIDE
  advancefront(lzi->short_tree, nchars);
  lzi->short_front_offset += nchars;
  if (lzi->short_front_offset >= 2048) lzi->short_front_offset -= 2048;
  lzi->chars_in_short_slide += nchars;
#endif
  lzi->chars_in_match -= nchars;
}

static void fill_match(lz_info *lzi, int maxchars)
{
  int room_in_match = lzi->match_buf_size - lzi->chars_in_match;
  u_char *read_start;
  int read_size;
  
  maxchars -= lzi->chars_in_match;
  read_start = lzi->match_bufp + lzi->chars_in_match;
#ifdef LZ_ONEBUFFER
  if (read_start >= lzi->match_bufe) read_start -= lzi->slide_buf_size;
  /* in one buffer mode, lzi->match_bufp + lzi_chars_in_match should never cross
     the match buffer boundary */
#else
  if (read_start >= lzi->match_bufe) read_start -= lzi->match_buf_size;
#endif

  read_size = lzi->match_bufe - read_start;
  if (read_size > room_in_match) read_size = room_in_match;
  if (read_size > maxchars) read_size = maxchars;
  read_size = lzi->get_chars(lzi, read_size, read_start);
  maxchars -= read_size;
  lzi->cur_loc += read_size;
  lzi->chars_in_match += read_size;
  room_in_match -= read_size;
  if (room_in_match && read_size && maxchars) {
    if (room_in_match < maxchars)
      read_size = room_in_match;
    else
      read_size = maxchars;
    
#ifdef LZ_ONEBUFFER
    read_size = lzi->get_chars(lzi, read_size, lzi->slide_buf);
#else
    read_size = lzi->get_chars(lzi, read_size, lzi->match_buf);
#endif
    lzi->chars_in_match += read_size;
    lzi->cur_loc += read_size;
  }
}

void lz_stop_compressing(lz_info *lzi) 
{
  lzi->stop = 1;
}

int lz_left_to_process(lz_info *lzi)
{
  return lzi->chars_in_match;
}

int lz_compress(lz_info *lzi, int nchars) 
{
  int match_len, match_loc, mod_match_loc;
  int advance_chars;
  int new_match_loc, new_match_len;
  int old_match_loc, old_match_len;
  u_char *new_bufp;
  int new_chars_in_match;
  int chars_in_match;
  int bank_literals = 0;
  u_char *old_slide_bufp;
  u_char *bank;
  short force_literals = 0;
  short lazy_match = 0;
  
  fill_match(lzi, nchars);
  lzi->stop = 0;
  while (lzi->chars_in_match && !lzi->stop && (nchars > 0)) {
    chars_in_match = lzi->chars_in_match;
    if (chars_in_match > lzi->max_match)
      chars_in_match = lzi->max_match;
    if (lzi->frame_size && (chars_in_match > (lzi->frame_size - lzi->loc_in_frame))) {
      chars_in_match = (lzi->frame_size - lzi->loc_in_frame);
    }
    if (chars_in_match > nchars) {
      chars_in_match = nchars;
    }
#ifdef LAZY
    if (!force_literals) {
      if (!lazy_match) {
	match_loc = longestmatch(lzi->main_tree, lzi->match_bufp,
				 chars_in_match, &match_len, 
				 lzi->match_bufe, lzi->match_buf);
      }
      else {
	match_loc = new_match_loc;
	match_len = new_match_len;
	lazy_match = 0;
      }
    }
    else {
      match_loc = -1;
      force_literals--;
    }
#else
    match_loc = longestmatch(lzi->main_tree, lzi->match_bufp,
			     chars_in_match, &match_len, 
			     lzi->match_bufe, lzi->match_buf);
#endif

    if ((match_loc == -1) || (match_len < lzi->min_match)) {/* literal */
      lzi->output_literal(lzi, *lzi->match_bufp);
      advance_chars = 1;
    }
    else { /* found a match */

#ifdef LAZY /* lazy evaluation.. based on the gzip algorithm */
      new_bufp = lzi->match_bufp;
      /* make sure the first character isn't matched */
      if (lzi->chars_in_slide == lzi->wsize) {
	advancetail(lzi->main_tree, 1);
	lzi->chars_in_slide--;
      }

      if (++new_bufp == lzi->match_bufe) new_bufp = lzi->match_buf;
      new_chars_in_match = chars_in_match - 1;
      if (--new_chars_in_match > (match_len + 1)) {
	new_match_loc = longestmatch(lzi->main_tree, new_bufp,
				     new_chars_in_match, &new_match_len, 
				     lzi->match_bufe, lzi->match_buf);
	if ((new_match_loc != -1 ) && (new_match_len > (match_len+1))) {
	  /*	  force_literals = 1;  used before the bank method */
	  /* assumes window is big enough that the bank will never be pushed off the end */
	  if (!bank_literals) {
	    bank = lzi->match_bufp; 
	    old_match_loc = match_loc - lzi->front_offset;
	    if (old_match_loc >= 0)
	      old_match_loc -= lzi->slide_buf_size;
	    old_match_len = match_len;
	    old_slide_bufp = lzi->slide_bufp;
	  }
	  bank_literals++;
	  lazy_match = 1;
	  advance_chars = 1;
	}
      }
      if (!lazy_match) {
	if ((bank_literals > 2) && ((bank_literals - old_match_loc) < lzi->wsize)) {
	  /* this bank stuff is pretty marginal.  The idea is a succession of lazy matches may leave multiple
	     literals which can be re-combined into a match.  In practice, it rarely happens and when it
	     does the match is usually rejected for being too short and too far away.
	     
	     The second condition above makes sure we haven't lost the original match
	  */
	  new_bufp = lzi->slide_bufp;
	  lzi->slide_bufp = old_slide_bufp;
	  if (old_match_len > bank_literals)
	    old_match_len = bank_literals;
	  if (lzi->output_match(lzi, old_match_loc, old_match_len) == 0) {
	    /*	    fprintf(stderr, "leftover match %d %d %d %d %d\n", bank_literals, old_match_len, match_len, old_match_loc, lzi->cur_loc); */
	    bank_literals -= old_match_len;
	    bank += old_match_len;
	    if (bank >= lzi->match_bufe) bank = bank - (lzi->match_bufe - lzi->match_buf);
	  }
	  lzi->slide_bufp = new_bufp;
	}
	while (bank_literals) {
	  lzi->output_literal(lzi, *bank);
	  if (++bank == lzi->match_bufe) bank = lzi->match_buf;
	  bank_literals--;
	}
#endif
	mod_match_loc = match_loc - lzi->front_offset;
	if (mod_match_loc >= 0)
	  mod_match_loc -= lzi->slide_buf_size;
	  
#ifdef SHORTSLIDE
	new_match_loc = longestmatch(lzi->short_tree, lzi->match_bufp,
				     match_len, &new_match_len, 
				     lzi->match_bufe, lzi->match_buf);
	new_match_loc = new_match_loc - lzi->short_front_offset;
	if (new_match_loc >= 0)
	  new_match_loc -= 2048;
	if ((new_match_loc > mod_match_loc) && (new_match_len == match_len)) {
	  /*	fprintf(stderr, "Found closer match %d  %d  %d %d\n", lzi->cur_loc, match_len, -mod_match_loc, -new_match_loc); */
	mod_match_loc = new_match_loc;
	}
#endif
	if (lzi->output_match(lzi, mod_match_loc, match_len) == 0)
	  advance_chars = match_len;
	else /* match rejected! */ {
	  lzi->output_literal(lzi, *lzi->match_bufp);
	  advance_chars = 1;
	}
#ifdef LAZY
      }
#endif
    }
    advance_slide(lzi, advance_chars);
    lzi->loc_in_frame += advance_chars;
    if (lzi->frame_size && (lzi->loc_in_frame >= lzi->frame_size))
      lzi->loc_in_frame -= lzi->frame_size;
    nchars -= advance_chars;
    fill_match(lzi, nchars);
  }
#ifdef DEBUG_LZ
  assert(!bank_literals);
#endif
  assert(!bank_literals);
  assert(!(lzi->chars_in_match - lz_left_to_process(lzi)));
  return 0;
}

