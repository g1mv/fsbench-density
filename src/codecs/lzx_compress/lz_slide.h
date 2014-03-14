/*
    File lz_slide.h, part of lzxcomp library
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
typedef struct lz_info lz_info;
typedef int (*get_chars_t)(lz_info *lzi, int n, u_char *buf);
typedef int (*output_match_t)(lz_info *lzi, int match_pos, int match_len);
typedef void (*output_literal_t)(lz_info *lzi, u_char ch);

struct lz_info
{
  int wsize; /* window size in bytes */
  int max_match; /* size of longest match in bytes */
  int min_match;
  int match_buf_size;
#ifdef LZ_ONEBUFFER
  int slide_buf_size;
#else
  #define slide_buf_size wsize
#endif
  u_char *slide_buf;      /* circular buffer for sliding window pattern matching */
  u_char *slide_bufe;     /* end of circular buffer */
#ifdef LZ_ONEBUFFER
  #define slide_bufp match_bufp
#else
  u_char *slide_bufp;     /* current location in slide buffer */
#endif
  u_char *match_buf;      /* circular buffer for new characters to be matched */
  u_char *match_bufe;     /* end of circular buffer for new characters to be matched */
  u_char *match_bufp;     /* current location in match buffer */
  int chars_in_slide;     /* current size of suffix tree */
  int chars_in_match;     /* current size of match buffer */
  int front_offset;       /* location within slide_buf of front of suffix tree */
  int cur_loc;            /* location within stream */
  int loc_in_frame;
  int frame_size;
  short stop;

#ifdef SHORTSLIDE
  /* ignore this.  It was used when I was using suffix trees instead
     of hash tables */
  u_char *short_slide;
  u_char *short_slidep;
  u_char *short_slidee;
  int short_front_offset;
  int chars_in_short_slide;
  suffix_tree_info_t      *short_tree;
#endif

  hash_info_t      *main_tree;

  get_chars_t get_chars;
  output_match_t output_match;
  output_literal_t output_literal;
  void *user_data;
};

void lz_init(lz_info *lzi, int wsize, int max_match, int min_match,
	     int frame_size,
	     get_chars_t get_chars,
	     output_match_t output_match,
	     output_literal_t output_literal, void *user_data);

void lz_release(lz_info *lzi);

void lz_reset(lz_info *lzi);
void lz_stop_compressing(lz_info *lzi);
int lz_compress(lz_info *lzi, int nchars);
int lz_left_to_process(lz_info *lzi); /* returns # chars read in but unprocessed */
