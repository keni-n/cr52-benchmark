/*
 * bsp.h - Cortex-R52 bare-metal board support package (V4H / X5H).
 *
 * Single include for the platform layer shared by CoreMark and
 * Dhrystone: console output (ee_printf / cvt), the SCMT free-running
 * timer, CPU cache / TCM / MPU bring-up, the EL2->EL1 transition and
 * the PMU wrappers. Define V4H or X5H on the compiler command line to
 * select the target SoC.
 */
#ifndef BSP_CR52_H
#define BSP_CR52_H

#include "ee_types.h"

/* ------------------------------------------------------------------ */
/* Console (ee_printf.c / cvt.c)                                       */
/* ------------------------------------------------------------------ */

/* Enable ee_printf's %f/%e/%g support. It is implemented in software via
   cvt.c (ecvtbuf/fcvtbuf, see below), so no libm is required. CoreMark's
   core_portme.h also defines this; the #ifndef keeps the two in sync. */
#ifndef HAS_FLOAT
#define HAS_FLOAT 1
#endif

int   ee_printf(const char *fmt, ...);

char *ecvt   (double arg, int ndigits, int *decpt, int *sign);
char *ecvtbuf(double arg, int ndigits, int *decpt, int *sign, char *buf);
char *fcvt   (double arg, int ndigits, int *decpt, int *sign);
char *fcvtbuf(double arg, int ndigits, int *decpt, int *sign, char *buf);

/* ------------------------------------------------------------------ */
/* SCMT free-running 32-bit timer                                      */
/*   OSCCLK/1 = 130.20kHz (EXTAL=16.66MHz, MD13=0, MD14=0).            */
/* ------------------------------------------------------------------ */
#ifdef V4H
#define SCMT_BASE 0xe6040000u
#endif
#ifdef X5H
#define SCMT_BASE 0xc6500000u
#endif

#define CMSSTR (*((volatile ee_u16 *)(SCMT_BASE + 0x00)))  /* 0x20 = start */
#define CMSCSR (*((volatile ee_u16 *)(SCMT_BASE + 0x40)))
#define CMSCNT (*((volatile ee_u32 *)(SCMT_BASE + 0x44)))  /* counter      */
#define CMSCOR (*((volatile ee_u32 *)(SCMT_BASE + 0x48)))  /* overflow val */

#define CLOCKS_PER_SEC 130200

void  scmt_init(void);   /* stop, free-run config, start                */
ee_u32 scmt_read(void);  /* current CMSCNT value                        */
void  delay_sec(ee_u32 sec); /* busy-wait N seconds using the SCMT timer */

/* ------------------------------------------------------------------ */
/* CPU bring-up (cr52_hw.c)                                            */
/* ------------------------------------------------------------------ */
void cache_enable_el2(void);
void cache_enable_el1_el0(void);
void modify_imp_bpctlr(void);
void disable_mem_protect(void);
void init_atcm(void);
void init_btcm(void);
void init_ctcm(void);
void set_pmu(void);
void read_ccsidr(ee_u32 *id, ee_u32 *dd);
void read_cp15_c15(ee_u32 *low, ee_u32 *high);

/* Drop from EL2 (HYP) to EL1 (SVC). Must be called once before
   pmu_enable(): on this part the PMU does not count in EL2. */
void drop_to_el1(void);

/* ------------------------------------------------------------------ */
/* Cortex-R52 PMU wrappers (pmu.c)                                     */
/* ------------------------------------------------------------------ */
void pmu_enable(void);   /* reset + start cycle counter + 4 events      */
void pmu_disable(void);  /* freeze all counters                         */
void pmu_report(void);   /* print cycle counter + each event counter    */

#endif /* BSP_CR52_H */
