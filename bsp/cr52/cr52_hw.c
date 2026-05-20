/* --------------------------------------------------------------------------
 * cr52_hw.c - Cortex-R52 (V4H / X5H) CPU bring-up primitives.
 *
 * SCMT free-running timer, cache / TCM / MPU configuration, branch-predictor
 * tweaks and the EL2 (HYP) -> EL1 (SVC) transition. The orchestration of
 * these (which to call, and in what order) lives in each benchmark's
 * writer_main(); this file only provides the individual operations.
 * -------------------------------------------------------------------------- */

#include "bsp.h"

/* ------------------------------------------------------------------ */
/* SCMT free-running 32-bit timer                                      */
/* ------------------------------------------------------------------ */
void scmt_init(void)
{
    CMSSTR = 0x00;          /* Timer stop                              */
    CMSCOR = 0xFFFFFFFFu;   /* Overflow value (free-running 32-bit)    */
    CMSCNT = 0;
    CMSCSR = 0x10f;         /* 32bit freerun, no irq, OSCCLK/1=130.20kHz
                              (EXTAL=16.66MHz, MD13=0, MD14=0)         */
    CMSSTR = 0x20;          /* Timer start                             */
}

ee_u32 scmt_read(void)
{
    return CMSCNT;
}

/* Busy-wait for `sec` seconds using the SCMT free-running counter
   (OSCCLK/1 = CLOCKS_PER_SEC ticks/sec). Independent of the (unknown)
   CPU clock. This (re)configures the timer; the benchmark re-initialises
   it via scmt_init() before its own measurement. */
void delay_sec(ee_u32 sec)
{
    ee_u32 ticks = sec * (ee_u32)CLOCKS_PER_SEC;
    ee_u32 start;

    scmt_init();
    start = scmt_read();
    while ((scmt_read() - start) < ticks)
    {
        /* spin */
    }
}

/* ------------------------------------------------------------------ */
/* Cache / TCM / MPU / branch predictor                                */
/* ------------------------------------------------------------------ */
void read_cp15_c15(ee_u32 *low, ee_u32 *high)
{
    __asm__ volatile (
        "MRRC p15, 0, %0, %1, c15"
        : "=r"(*low), "=r"(*high)
    );
    ee_printf("Read ");

    *low &= ~(3 << 25); /* clear */
    *low |= 0 << 25;
    ee_printf("Modify ");

    __asm__ volatile (
        "MCRR p15, 0, %0, %1, c15\n\t"
        :
        : "r" (*low), "r"(*high)
        : "memory"
    );
    ee_printf("Write ");

    __asm__ volatile (
        "MRRC p15, 0, %0, %1, c15"
        : "=r"(*low), "=r"(*high)
    );
    ee_printf("Read\n");
}

void cache_enable_el1_el0(void)
{
  __asm__("MRC   p15, 0, R1, c1, c0, 0   ;# Read System Control Register configuration data");
  __asm__("ORR   R1, R1, #0x1 <<12       ;# instruction cache enable");
  __asm__("ORR   R1, R1, #0x4  ;# data cache enable");
  __asm__("MCR   p15, 0, r0, c7, c5, 0   ;# Invalidate entire instruction cache");
  __asm__("MCR   p15, 0, R1, c1, c0, 0   ;# enabled instruction cache");
  __asm__("ISB");
}

void cache_enable_el2(void)
{
  __asm__("MRC   p15, 4, R1, c1, c0, 0   ;# Read System Control Register configuration data");
  __asm__("ORR   R1, R1, #0x1 <<2       ;# data cache enable");
  __asm__("ORR   R1, R1, #0x1 <<12       ;# instruction cache enable");
#ifdef X5H
  /* MPU On using background region setting */
  __asm__("ORR   R1, R1, #0x1 <<17       ;# Back ground region ");
  __asm__("ORR   R1, R1, #0x1 <<0        ;# MPU on ");
#endif
  __asm__("MCR   p15, 4, R1, c1, c0, 0   ;# Write to HSCTLR");
  __asm__("ISB");
}

void modify_imp_bpctlr(void) {
    unsigned int val;

    __asm__ volatile (
        "MRC p15, 1, %0, c9, c1, 1\n\t"   /* IMP_BPCTLR read  */
        : "=r" (val) : : "memory"
    );
    ee_printf("before IMP_BPCTLR = 0x%08x\n", val);

    val &= 0xFFFFFFF8;                    /* clear bits [2:0] */

    __asm__ volatile (
        "MCR p15, 1, %0, c9, c1, 1\n\t"   /* IMP_BPCTLR write */
        : : "r" (val) : "memory"
    );
}

void init_atcm(void) {
    ee_u32 atcm_config;
    /* base 0x20000000, size 32KB, enable at EL2/EL1/EL0 */
    atcm_config  = (0x20000000 >> 12) << 12;
    atcm_config |= (0x6 << 2);
    atcm_config |= 0x3;
    __asm__ volatile (
        "MCR p15, 0, %0, c9, c1, 0\n\t"   /* ATCM Region Register */
        : : "r" (atcm_config) : "memory"
    );
}

void init_btcm(void) {
    ee_u32 btcm_config;
    /* base 0x20008000, size 32KB, enable at EL2/EL1/EL0 */
    btcm_config  = (0x20008000 >> 12) << 12;
    btcm_config |= (0x6 << 2);
    btcm_config |= 0x3;
    __asm__ volatile (
        "MCR p15, 0, %0, c9, c1, 1\n\t"   /* BTCM Region Register */
        : : "r" (btcm_config) : "memory"
    );
}

void init_ctcm(void) {
    ee_u32 ctcm_config;
    /* base 0x20010000, size 32KB, enable at EL2/EL1/EL0 */
    ctcm_config  = (0x20010000 >> 12) << 12;
    ctcm_config |= (0x6 << 2);
    ctcm_config |= 0x3;
    __asm__ volatile (
        "MCR p15, 0, %0, c9, c1, 2\n\t"   /* CTCM Region Register */
        : : "r" (ctcm_config) : "memory"
    );
}

void disable_mem_protect(void)
{
    unsigned int val;

    __asm__ volatile (
        "MRC p15, 1, %0, c9, c1, 2\n\t"   /* IMP_MEMPROTCTLR read */
        : "=r" (val) : : "memory"
    );
    ee_printf("before IMP_MEMPROTCTLR = 0x%08x\n", val);

    val = 0;

    __asm__ volatile (
        "MCR p15, 1, %0, c9, c1, 2\n\t"   /* IMP_MEMPROTCTLR write */
        : : "r" (val) : "memory"
    );
}

void set_pmu(void)
{
  __asm__("MRC	p15,0,r0,c10,c2"); /* MAIR0  */
  __asm__("MRC	p15,4,r1,c10,c2"); /* HMAIR0 */
  __asm__("ORR	r0, r0, #0xff00");
  __asm__("BIC	r0, r0, #0xff");   /* 0000 = Device memory nGnRnE */
  __asm__("BIC	r1, r1, #0xff00");
  __asm__("ORR	r1, r1, #0xff00"); /* Normal WB non-transient (inner/outer) */
  __asm__("MCR  p15,0,r0,c10,c2");
  __asm__("MCR  p15,0,r0,c10,c2");
  __asm__("MCR  p15,4,r1,c10,c2");

  __asm__("LDR	r0, =#0xeb200002"); /* AP=R/W, XN=0                 */
  __asm__("LDR	r1, =#0xeb2fffc3"); /* AttrIndex = 00001, enable    */
  __asm__("MCR	p15,0,r0,c6,c13,0");
  __asm__("MCR	p15,0,r1,c6,c13,1");
  __asm__("MCR	p15,4,r0,c6,c13,0");
  __asm__("MCR	p15,4,r1,c6,c13,1");
}

void read_ccsidr(ee_u32 *id, ee_u32 *dd)
{
    /* Instruction cache */
    __asm__ volatile ("MOV r0, #1");
    __asm__ volatile ("MCR p15, 2, r0, c0, c0, 0");
    __asm__ volatile ("MRC p15, 1, %0, c0, c0, 0\n\t" : "=r"(*id));

    /* Data cache */
    __asm__ volatile ("MOV r0, #0");
    __asm__ volatile ("MCR p15, 2, r0, c0, c0, 0");
    __asm__ volatile ("MRC p15, 1, %0, c0, c0, 0\n\t" : "=r"(*dd));
}

/* ------------------------------------------------------------------ */
/* EL2 (HYP) -> EL1 (SVC) transition                                   */
/*                                                                    */
/* Naked: ERET resumes the caller at LR, but now in SVC mode. SP_svc  */
/* is primed with the current SP_hyp so the caller's stack frame      */
/* remains valid across the mode switch. After this point HDCR.HPMN   */
/* defaults to N and counters count EL1/EL0 events without HPME.      */
/* ------------------------------------------------------------------ */
__attribute__((naked))
void
drop_to_el1(void)
{
    __asm__ volatile(
        /* Configure EL1 SCTLR: I/D caches on, mirror EL2 setup. */
        "mrc    p15, 0, r1, c1, c0, 0   \n\t"
        "orr    r1, r1, #(1 << 2)        \n\t"  /* C: D-cache  */
        "orr    r1, r1, #(1 << 12)       \n\t"  /* I: I-cache  */
#ifdef X5H
        "orr    r1, r1, #(1 << 17)       \n\t"  /* BR: background region */
        "orr    r1, r1, #(1 << 0)        \n\t"  /* M:  MPU enable */
#endif
        "mcr    p15, 0, r1, c1, c0, 0   \n\t"
        "isb                              \n\t"

        /* SP_svc <- current SP_hyp, so the caller's stack stays usable. */
        "mov    r0, sp                    \n\t"
        "msr    sp_svc, r0                \n\t"

        /* SPSR_hyp = SVC mode (0x13), A=I=F masked => 0x1D3.
           ELR_hyp = LR  -> ERET resumes at the caller's continuation. */
        "mov    r0, #0x1D3                \n\t"
        "msr    spsr_cxsf, r0             \n\t"
        "msr    elr_hyp, lr               \n\t"
        "eret                             \n\t"
    );
}
