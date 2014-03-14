/*
 * cyclic redundancy check 32 bit
 * Copyright (C) 1999 Christian Wolff, 2004,2005 Oskar Schirmer
 * for Convergence Integrated Media GmbH (http://www.convergence.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __Z3CRC32_H__
#define __Z3CRC32_H__

#define LSBFCOEFF(x) | ((1UL << 31) >> x)

#define POLYNOMIAL_32_LSBF 0 \
    LSBFCOEFF(0)  \
    LSBFCOEFF(1)  \
    LSBFCOEFF(2)  \
    LSBFCOEFF(4)  \
    LSBFCOEFF(5)  \
    LSBFCOEFF(7)  \
    LSBFCOEFF(8)  \
    LSBFCOEFF(10) \
    LSBFCOEFF(11) \
    LSBFCOEFF(12) \
    LSBFCOEFF(16) \
    LSBFCOEFF(22) \
    LSBFCOEFF(23) \
    LSBFCOEFF(26)

#define CRC_INIT_32   ((__u32)(-1))

#define update_crc_32(crc, data) \
    (((crc) >> 8) ^ crc_32_table[(unsigned char)((crc) ^ (data))])

// generate the tables of CRC-32 remainders for all possible bytes
static inline void gen_crc32_table(__u32 *table) {
  register int i,j;
  register __u32 crc32;
  for (i=0; i<256; i++) {
    crc32=(__u32)i;
    for (j=0; j<8; j++) {
      crc32 = (crc32 >> 1) ^ ((crc32 & 1) ? POLYNOMIAL_32_LSBF : 0);
    }
    table[i] = crc32;
  }
}

// update the CRC on the data block one byte at a time
static inline __u32 update_crc_32_block(__u32 *table, __u32 crc,
                char *data_block_ptr, int data_block_size)
{
  register int i;
  for (i=data_block_size; i>0; i--) {
    crc = (crc >> 8) ^ table[(unsigned char)(crc ^ (*data_block_ptr++))];
  }
  return crc;
}

#endif /* __Z3CRC32_H__ */
