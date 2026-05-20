/*
 * dhry_portme.c - Dhrystone port for Cortex-R52 (V4H) bare-metal.
 *
 * The boot code (bsp/cr52/boot_mon.s) clears BSS, copies .data, then
 * calls writer_main(), which performs CPU/cache/PMU bring-up (via the
 * shared CR52 BSP) and finally invokes dhrystone(). Timing uses the
 * SCMT free-running counter (CMSCNT) at OSCCLK/1 = 130.20kHz on V4H.
 *
 * Everything board-level (SCMT registers, cache/TCM init, drop_to_el1,
 * the PMU wrappers) lives in the BSP; only the Dhrystone-specific timer
 * conversion, writer_main orchestration and freestanding libc shims
 * remain here.
 */

#include "dhry_portme.h"

extern void dhrystone(void);

/* ------------------------------------------------------------------ */
/* High-level timer: convert SCMT ticks to seconds                     */
/* ------------------------------------------------------------------ */
double secs;
static ee_u32 start_tick;

void start_time(void)
{
    start_tick = scmt_read();
}

void end_time(void)
{
    ee_u32 now = scmt_read();
    ee_u32 elapsed;

    /* SCMT counts UP on this port: elapsed = now - start (wrap-safe). */
    if (now >= start_tick)
    {
        elapsed = now - start_tick;
    }
    else
    {
        elapsed = (0xFFFFFFFFu - start_tick) + now + 1u;
    }

    secs = (double)elapsed / (double)CLOCKS_PER_SEC;
}

/* ------------------------------------------------------------------ */
/* Entry point called from boot_mon.s                                  */
/* ------------------------------------------------------------------ */
void writer_main(void)
{
    /* Settle time before the first console output / the benchmark. */
    delay_sec(1);

	ee_printf("\n");

    modify_imp_bpctlr();
    disable_mem_protect();
    cache_enable_el2();

    __asm__ volatile ("DSB");
    __asm__ volatile ("ISB");

    /* Initialise SP to the top of the linker-defined stack region. */
    __asm__ volatile ("LDR sp, =__STACKS_END__");

    scmt_init();

    ee_printf("Dhrystone start (initial SCMT count = %08x)\n",
              (unsigned)CMSCNT);

    /* Switch from EL2 (HYP) to EL1 (SVC) before the benchmark so the
       PMU counters count EL1 events with default HDCR. */
    ee_printf("Dropping to EL1 (SVC mode) for PMU measurement.\n");
    drop_to_el1();
    /* Beyond this point we are in SVC mode. */

    pmu_enable();
    dhrystone();
    pmu_disable();

    ee_printf("Dhrystone end\n");
    pmu_report();

    /* Don't fall through to whatever follows in ROM. */
    for (;;) { __asm__ volatile ("wfi"); }
}

/* ------------------------------------------------------------------ */
/* Minimal C library helpers (no libc / no -lc in this build)         */
/* ------------------------------------------------------------------ */

void *memcpy(void *dst, const void *src, size_t len)
{
    unsigned char       *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (len--) { *d++ = *s++; }
    return dst;
}

void *memset(void *dst, int val, size_t len)
{
    unsigned char *d = (unsigned char *)dst;
    unsigned char  v = (unsigned char)val;
    while (len--) { *d++ = v; }
    return dst;
}

size_t strlen(const char *str)
{
    const char *p = str;
    while (*p) { p++; }
    return (size_t)(p - str);
}

char *strcpy(char *dst, const char *src)
{
    char *p = dst;
    while ((*p++ = *src++) != '\0') { /* copy */ }
    return dst;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return (int)((unsigned char)*s1 - (unsigned char)*s2);
}

/* Simple bump allocator (no free). Big enough for two Rec_Type plus
   a little slack. dhrystone only allocates twice. */
#define MALLOC_POOL_SIZE 0x1000
static unsigned char malloc_pool[MALLOC_POOL_SIZE];
static size_t        malloc_offset;

void *malloc(size_t size)
{
    /* 8-byte align allocations so doubles/pointers behave. */
    size = (size + 7u) & ~(size_t)7u;
    if (malloc_offset + size > MALLOC_POOL_SIZE) { return 0; }
    void *p = &malloc_pool[malloc_offset];
    malloc_offset += size;
    return p;
}

void free(void *ptr) { (void)ptr; }
