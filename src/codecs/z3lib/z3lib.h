/*
 * z3lib (c)1993,2000,2005,2006/GPL,BSD Oskar Schirmer, schirmer@scara.com
 * combined huffman/lz compression library
 */

#ifndef __Z3LIB_H__
#define __Z3LIB_H__

#ifdef Z3LIB_WITHOUT_LIBC
typedef unsigned char __u8;
typedef unsigned short __u16;
typedef unsigned int __u32;
typedef signed char __s8;
typedef signed short __s16;
typedef signed int __s32;
#else
#include <string.h>
#include <stdint.h>
typedef uint8_t __u8;
typedef uint16_t __u16;
typedef uint32_t __u32;
typedef int8_t __s8;
typedef int16_t __s16;
typedef int32_t __s32;
#endif

enum z3errno {
  z3err_none,
  z3err_bd_notbfinal,
  z3err_bd_btype3,
  z3err_bd_nlenmismatch,
  z3err_bd_hlitexceed,
  z3err_bd_hdistexceed,
  z3err_bd_codelengthtable,
  z3err_bd_codeliteralundefined,
  z3err_bd_codeliteralexceed,
  z3err_bd_codeliteralnoprevious,
  z3err_bd_codeliteraltoomany,
  z3err_bd_codeliteraltable,
  z3err_bd_codedistancesundefined,
  z3err_bd_codedistancesexceed,
  z3err_bd_codedistancesnoprevious,
  z3err_bd_codedistancestoomany,
  z3err_bd_codedistancestable,
  z3err_bd_dataundefined,
  z3err_bd_distanceundefined
};

#endif /* __Z3LIB_H__ */
