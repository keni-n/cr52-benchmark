/*
 * ee_types.h - fixed-width integer typedefs shared by the CR52 BSP.
 *
 * The EEMBC benchmarks (CoreMark) define the same ee_* typedefs in
 * coremark.h / core_portme.h, and Dhrystone defines them in
 * dhry_portme.h. These definitions are identical, so a translation
 * unit that pulls in both a benchmark header and this one simply
 * redefines each typedef to the same type, which ISO C11 permits.
 */
#ifndef BSP_EE_TYPES_H
#define BSP_EE_TYPES_H

#include <stddef.h>

typedef unsigned char  ee_u8;
typedef unsigned short ee_u16;
typedef signed   short ee_s16;
typedef signed   int   ee_s32;
typedef unsigned int   ee_u32;
typedef size_t         ee_size_t;

#endif /* BSP_EE_TYPES_H */
