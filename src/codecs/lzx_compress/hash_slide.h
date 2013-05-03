/*
    File hash_slide.h, part of lzxcomp library
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
typedef struct hash_info_t hash_info_t;
typedef unsigned char SYMB;
typedef unsigned char u_char;

int inithash(int max_window_size, SYMB *buffer, hash_info_t **htip);
void releasehash(hash_info_t *hti);
void resethash(hash_info_t *hti);
void advancefront(hash_info_t *hti, int positions);
void advancetail(hash_info_t *hti, int positions);
int longestmatch(hash_info_t *hti, 
		 SYMB *pattern, int maxlen, int *matchlen,
                 SYMB *wrappos, SYMB *wrapto);

