/*
    File hash_slide.c, part of lzxcomp library
    Copyright (C) 2002 Matthew T. Russotto

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; version 2.1 only.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "hash_slide.h"

/*
 * This code is part of the lzx compression library LZXCOMP. 
 * It implements a hash table for pattern-matching similar to that
 * of the 'deflate' algorithm in zlib.  
 *
 * It's not as efficient, but it supports buffer sizes other than
 * 32K.  It also lacks the speed-versus-ratio tradeoff parameters
 * though some, such as chain length cutoff and 'good enough'
 * matching could be added with little difficulty
 *
 * Nearly all the LZX compression time is spent in the longestmatch
 * and longestmatch_at routines
 */

/*
 *  Turns out my next-character-lookahead speedup was already discovered by 
 *  Gutmann in
 *  P.M. Fenwick and P.C. Gutmann, "Fast LZ77 String Matching", Dept of 
 *  Computer Science, The University of Auckland, Tech Report 102, Sep 1994 
 *
 *  I tried the half match stuff from the same paper, but performance
 *  improvement was marginal at best
 *
 */

typedef struct hash_entry_t {
  struct hash_entry_t *next; /* MUST BE FIRST */
  struct hash_entry_t *last;
} hash_entry_t;

#define HASH_CHARS 3
#define UPDATE_HASH(hti, hash, c) \
  ((hash) = (((hash) << (hti)->hash_shift) ^ c) & (hti)->hash_mask)

struct hash_info_t {
  int wsize;
  short hash_bits;
  short hash_shift;
  uint32_t hash_mask;
  uint32_t curhash;
  hash_entry_t *entries;
  hash_entry_t **hasht;
#ifdef TWOCHAR
  uint32_t *twochar;
  SYMB lastchar;
#endif
  uint32_t chars_in_window;
  uint32_t fourchar;
  SYMB *front;
  SYMB *tail;
  SYMB *buf;
  SYMB *bufe;
};

int inithash(int max_window_size, SYMB *buffer, hash_info_t **htip)
{
  hash_info_t *hti;
#ifdef TWOCHAR
  uint32_t *tcp;
#endif

  hti = malloc(sizeof(hash_info_t));
  assert(hti != NULL);
  *htip = hti;
  hti->curhash = 0;
  hti->hash_shift = 8;
  hti->hash_bits = hti->hash_shift * HASH_CHARS;
  hti->hash_mask = (1<<hti->hash_bits)-1;
  hti->entries = malloc(max_window_size * sizeof(hash_entry_t));
  assert(hti->entries != NULL);
  hti->hasht = calloc((1<<hti->hash_bits),sizeof(hash_entry_t *));
  assert(hti->hasht != NULL);
  
#ifdef TWOCHAR
  hti->twochar = malloc(65536 * sizeof(*hti->twochar));
  tcp = hti->twochar + 65536;
  while (tcp > hti->twochar) *--tcp = -1;
#endif

  hti->front = hti->tail = hti->buf = buffer;
  hti->bufe = buffer + max_window_size;
  hti->wsize = max_window_size;
  hti->chars_in_window = 0;
  return 0;
}

void releasehash(hash_info_t *hti)
{
  free(hti->entries);
  free(hti->hasht);
#ifdef TWOCHAR
  free(hti->twochar);
#endif
  free(hti);
}

void resethash(hash_info_t *hti)
{
  hti->front = hti->tail = hti->buf;
  hti->curhash = 0;

  hti->chars_in_window = 0;
  memset(hti->hasht, 0, (1<<hti->hash_bits) * sizeof(hash_entry_t *));
}

void advancetail(hash_info_t *hti, int positions)
{
  int itail;
  int i;

  /* delete hash entries relating to item(s) */
  itail = hti->tail - hti->buf + HASH_CHARS;
#ifdef DEBUG_HASH
  fprintf(stderr, "advancetail %d\n", positions);
#endif
  for (i = 0; i < positions; i++, itail++) {
    if (itail >= hti->wsize) itail -= hti->wsize;
    
    if (hti->entries[itail].last) {
      hti->entries[itail].last->next = NULL;
#ifdef DEBUG_HASH
      fprintf(stderr, "deleting stale entry %d\n", itail);
#endif
    }
  }

  hti->tail += positions;
  if (hti->tail >= hti->bufe) hti->tail -= hti->wsize;
  hti->chars_in_window -= positions;

#ifdef DEBUG_HASH
    fprintf(stderr, "positions = %d hti->buf = %08x, hti->tail = %08x, hti->bufe = %08x\n", positions,
  	  hti->buf, hti->tail, hti->bufe);
#endif
  assert(hti->tail < hti->bufe);
  assert(hti->tail >= hti->buf);
}

void advancefront(hash_info_t *hti, int positions)
{
  int i;
  int ifront;
  SYMB ch;

#ifdef DEBUG_HASH
  fprintf(stderr, "advancefront %d\n", positions);
#endif
  for (i = 0; i < positions; i++) {
    ch = *hti->front;
    UPDATE_HASH(hti, hti->curhash, ch);
    hti->front++;
    if (hti->front == hti->bufe) {
      hti->front = hti->buf;
    }
    ifront = hti->front - hti->buf;
    if (++hti->chars_in_window >= HASH_CHARS ) {
#ifdef DEBUG_HASH
      fprintf(stderr, "adding fresh entry %d\n", ifront);
#endif
      if (&hti->entries[ifront] != hti->hasht[hti->curhash])
	hti->entries[ifront].next = hti->hasht[hti->curhash];
      else 
	hti->entries[ifront].next = NULL;

      if (hti->entries[ifront].next)
	hti->entries[ifront].next->last = &hti->entries[ifront];

      hti->entries[ifront].last = /* kludge york city */ (struct hash_entry_t *)&hti->hasht[hti->curhash];
      hti->hasht[hti->curhash] = &hti->entries[ifront];
    }
#ifdef TWOCHAR
    if (hti->chars_in_window >= 2) {
      hti->twochar[(hti->lastchar << 8) | ch] = ifront;
    }
    hti->lastchar = ch;
#endif
    
  }
#ifdef DEBUG_HASH
  fprintf(stderr, "advancefront %d END\n", positions);
#endif
}

static __inline__ int longestmatch_at(/*const hash_info_t * const hti, */
				      const SYMB * const hti_buf, 
				      const SYMB * const hti_bufe, 
				      const SYMB * const hti_front, 
				      const int loc,
				      const SYMB * const pattern,
				      const int maxlen,
				      const SYMB * const wrappos, 
				      const SYMB * const wrapto, 
				      SYMB * /*const*/ pat_lookahead) 
{
  const u_char *nmatchb;
  register const u_char *c1, *c2;
  register const u_char *e1, *e2;
  register int j;

#ifdef DEBUG_HASH
  fprintf(stderr, "longestmatch_at %d\n", loc);
#endif
  nmatchb = hti_buf + loc;
#ifdef DEBUG_HASH
  /* basically, this code should never happen because 'loc' should always be
     within the hash buffer */
    if (nmatchb >= hti->bufe) {
      fprintf(stderr, "hti->buf = %08x, loc = %d, nmatchb = %08x, hti->bufe = %08x\n",
	      hti->buf, loc, nmatchb, hti->bufe);
    }
    if (nmatchb >= hti->bufe) nmatchb -= hti->wsize;
#endif
  c1 = pattern;
  c2 = nmatchb;
  e1 = wrappos;
  e2 = hti_bufe;

  if (((c2 + maxlen) < hti_front) && ((c2 + maxlen) < e2) && (c1 + maxlen < e1)) {
    /* this is a slight performance-enhancer */
    j = maxlen;
    while (j--) {
      if (*c1++ != *c2++) break;
    }
    *pat_lookahead = *--c1;
    return maxlen - j - 1;
  }

  for (j = 0; j < maxlen; j++) {
    if (c2 == hti_front) break;
    if (*c1 != *c2) break;
    if (++c1 == e1) c1 = wrapto;
    if (++c2 == e2) c2 = hti_buf;
  }
  *pat_lookahead = *c1;
#ifdef DEBUG_HASH
  fprintf(stderr, "longestmatch_at %d END\n", loc);
#endif
  return j;
}

#ifdef LZ_HALF_MATCH
#define UPDATE_HALFMATCH \
     if (max_matchlen > (HASH_CHARS + 3)) { \
       patp = pattern + ((max_matchlen - HASH_CHARS)>>1) + HASH_CHARS; \
       if (patp >= wrappos) patp -= (wrappos - wrapto); \
       half_match = *patp; \
     }
#else
#define UPDATE_HALFMATCH
#endif
int longestmatch(hash_info_t *hti, 
		 SYMB *pattern, int maxlen, int *matchlen,
                 SYMB *wrappos, SYMB *wrapto)
{
  uint32_t newhash = hti->curhash;
  int i;
  SYMB * patp;
  struct hash_entry_t *cursor;
  struct hash_entry_t *last;
  int cur_matchlen;
  int max_matchlen = 0;
  int cur_matchpos;
  int max_matchpos; /* not used uninitialized.  Trust me on this */
#if 0
  int hclen = 0;
#endif
  SYMB max_lookahead;
  SYMB cur_lookahead;
  int cur_lpos; /* position of lookahead */
#ifdef LZ_HALF_MATCH
  SYMB half_match;
  int cur_hpos = -1; /* position of half-match */
#endif
  struct hash_entry_t *hti_entries;
  int hti_wsize;
  SYMB *hti_buf;
  SYMB *hti_bufe;
  SYMB *hti_front;
  short do_scan;
#ifdef LZ_ONEBUFFER
  uint32_t hash1;
  uint32_t hash2;
#endif

  max_lookahead = *pattern;
  if (maxlen >= HASH_CHARS) {
    patp = pattern;
    for (i = 0 ; i < HASH_CHARS; i++) {
      UPDATE_HASH(hti, newhash, *patp);
#ifdef LZ_ONEBUFFER
      if (i == 0) hash2 = newhash;
      if (i == 1) hash1 = newhash;
#endif
      patp++;
      if (patp == wrappos) patp = wrapto;
    }
    cursor = hti->hasht[newhash];
    last = NULL;
    
    hti_entries = hti->entries;
    hti_wsize = hti->wsize;
    hti_buf = hti->buf;
    hti_bufe = hti->buf + hti_wsize;
#ifdef LZ_ONEBUFFER
    /* we can also match within the match buffer, thus
       abcabca -> abc(-3,4) */
      hti_front = hti->front + maxlen;
      if (hti_front >= hti->bufe) hti->front -= hti_wsize;
      if (hash1 == newhash) {
	cur_matchpos = hti->front - hti->buf - 1;
	if (cur_matchpos < 0) cur_matchpos += hti_wsize;
	cur_matchlen = longestmatch_at(/*hti,*/ hti_buf, hti_bufe, hti_front, cur_matchpos, pattern, maxlen, wrappos, wrapto, &cur_lookahead);
	if (cur_matchlen > max_matchlen) {
	  max_lookahead = cur_lookahead;
	  max_matchlen = cur_matchlen;
	  max_matchpos = cur_matchpos;
	  UPDATE_HALFMATCH;
	}
      }
      if (hash2 == newhash) {
	cur_matchpos = hti->front - hti->buf - 2;
	if (cur_matchpos < 0) cur_matchpos += hti_wsize;
	cur_lpos = cur_matchpos + max_matchlen;
	if (cur_lpos >= hti_wsize) cur_matchpos -= hti_wsize;
	do_scan = hti_buf[cur_lpos] == max_lookahead;
	if (do_scan) {
	  cur_matchlen = longestmatch_at(/*hti,*/ hti_buf, hti_bufe, hti_front, cur_matchpos, pattern, maxlen, wrappos, wrapto, &cur_lookahead);
	  if (cur_matchlen > max_matchlen) {
	    max_lookahead = cur_lookahead;
	    max_matchlen = cur_matchlen;
	    max_matchpos = cur_matchpos;
	    UPDATE_HALFMATCH;
	  }
	}
      }
#else
    hti_front = hti->front;
#endif
    while (cursor) {
#if 0
      /* chopping off the hash chains makes higher window sizes
	 less efficient */
      hclen++;
      if (hclen > 4096) break;
#endif
      cur_matchpos = cursor - hti_entries;
      cur_matchpos -= HASH_CHARS;
      if (cur_matchpos < 0) cur_matchpos += hti_wsize;
      cur_lpos = cur_matchpos + max_matchlen;
      if (cur_lpos >= hti_wsize) cur_matchpos -= hti_wsize;
      do_scan = hti_buf[cur_lpos] == max_lookahead;
#ifdef LZ_HALF_MATCH
      if (do_scan && (max_matchlen > (HASH_CHARS + 3))) {
	cur_hpos = cur_matchpos + ((max_matchlen - HASH_CHARS)>>1) + HASH_CHARS;
	if (cur_hpos >= hti_wsize) cur_hpos -= hti_wsize;
	if (hti_buf[cur_hpos] != half_match) do_scan = 0;
      }
#endif
      if (do_scan) {
	cur_matchlen = longestmatch_at(/*hti,*/ hti_buf, hti_bufe, hti_front, cur_matchpos, pattern, maxlen, wrappos, wrapto, &cur_lookahead);
	if (cur_matchlen > max_matchlen) {
	  max_lookahead = cur_lookahead;
	  max_matchlen = cur_matchlen;
	  max_matchpos = cur_matchpos;
	  if (max_matchlen == maxlen) break;
	  UPDATE_HALFMATCH;
	}
      }
      cursor = cursor->next;
    }
#if 0
    fprintf(stderr, "hclen = %d\n", hclen);
#endif
    if (max_matchlen > 0) {
      *matchlen = max_matchlen;
      return max_matchpos;
    }
  }
  /* short match search */
#ifdef TWOCHAR
  if (maxlen >= 2) {
    patp = pattern;
    newhash = (*patp++) << 8;
    if (patp == wrappos) patp = wrapto;
    newhash |= *patp;
    max_matchpos = hti->twochar[newhash] - 2;
    if (max_matchpos >= -2) {
      if (max_matchpos < 0) max_matchpos += hti->wsize;
#ifdef DEBUG_HASH
      fprintf(stderr, "twochar matchpos = %d\n", max_matchpos);
#endif
      max_matchlen = longestmatch_at(hti, hti_buf, hti_bufe, hti_front, max_matchpos, pattern, 2, wrappos, wrapto, &cur_lookahead);
      if (max_matchlen > 0) {
	*matchlen = max_matchlen;
	assert (max_matchlen <= 2);
	return max_matchpos;
      }
    }
  }
#endif
  return -1;
}
