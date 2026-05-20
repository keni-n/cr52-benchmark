/*
 * dhry_portme.h - Dhrystone port for Cortex-R52 (V4H) bare-metal.
 *
 * The platform layer (ee_* types, ee_printf, SCMT timer, cache/PMU
 * bring-up) is shared with CoreMark and lives in the CR52 BSP; this
 * header only adds the Dhrystone-specific timing hooks and the tiny
 * freestanding C-runtime shims (this build links neither libc nor -lc).
 */
#ifndef DHRY_PORTME_H
#define DHRY_PORTME_H

#include "bsp.h"   /* ee_u* types, ee_printf, pmu_*, SCMT, cache init */

/* High-level timer used by dhrystone (mirrors CoreMark's start/stop). */
extern double secs;
void  start_time(void);
void  end_time(void);

/* Freestanding C-runtime helpers provided by dhry_portme.c */
void *memcpy (void *dst, const void *src, size_t len);
void *memset (void *dst, int val, size_t len);
size_t strlen(const char *str);
char  *strcpy(char *dst, const char *src);
int    strcmp(const char *s1, const char *s2);
void  *malloc(size_t size);
void   free  (void *ptr);

#endif /* DHRY_PORTME_H */
