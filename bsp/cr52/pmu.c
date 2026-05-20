/* --------------------------------------------------------------------------
 * pmu.c - Cortex-R52 PMU (Performance Monitor Unit) wrappers.
 *
 *   pmu_enable()  : reset + start cycle counter and 4 event counters.
 *   pmu_disable() : freeze all counters.
 *   pmu_report()  : print cycle counter + each event counter value.
 *
 * The PMU does not count while the core runs in EL2 (HPME has no effect on
 * this part), so drop_to_el1() (see cr52_hw.c) must be called once before
 * pmu_enable(); at EL1 the default HDCR (HPMN = N, HPME = 0) makes all
 * counters count EL1/EL0 events normally.
 *
 * Counter assignment (Cortex-R52 has 4 event counters + 1 cycle counter):
 *   PMEVCNTR0 : Instruction architecturally executed (0x08)
 *   PMEVCNTR1 : L1 I-cache refill                    (0x01)
 *   PMEVCNTR2 : L1 D-cache refill                    (0x03)
 *   PMEVCNTR3 : Mispredicted/not-predicted branch    (0x10)
 *   PMCCNTR   : CPU cycles
 *
 * Notes:
 *  - PMXEVTYPER filter bits are left at 0 so events are counted in every
 *    mode (no P/U/NSK/NSU/NSH filtering). PMCCFILTR is also cleared.
 *  - 32-bit counters can wrap. Overflow flags are read from PMOVSR and
 *    annotated in the report.
 * -------------------------------------------------------------------------- */

#include "bsp.h"

#define PMU_NUM_EVENT_COUNTERS 4

static const ee_u32 pmu_event_ids[PMU_NUM_EVENT_COUNTERS] = {
    0x08, /* INST_RETIRED     */
    0x01, /* L1I_CACHE_REFILL */
    0x03, /* L1D_CACHE_REFILL */
    0x10  /* BR_MIS_PRED      */
};

static const char *pmu_event_names[PMU_NUM_EVENT_COUNTERS] = {
    "Instructions retired",
    "L1 I-cache refill   ",
    "L1 D-cache refill   ",
    "Branch mispredicted "
};

void
pmu_enable(void)
{
    ee_u32 val;
    ee_u32 i;

    /* Clear OS Lock (OSLAR_EL1). If set, PMU counters are halted. */
    val = 0;
    __asm__ volatile("MCR p14, 0, %0, c1, c0, 4" : : "r"(val));
    __asm__ volatile("ISB");

    /* DBGPRCR.CORENPDRQ = 1: keep the debug power domain alive so the
       PMU (which lives in the debug subsystem) is clocked. */
    __asm__ volatile("MRC p14, 0, %0, c1, c4, 4" : "=r"(val));
    val |= 1u;
    __asm__ volatile("MCR p14, 0, %0, c1, c4, 4" : : "r"(val));
    __asm__ volatile("ISB");

    /* HDCR is EL2-only and we now run at EL1 after drop_to_el1(); default
       HDCR (HPMN = N, HPME = 0) makes all counters count EL1/EL0 events. */

    /* PMCNTENCLR: stop all counters before reconfiguring. */
    val = 0x8000000Fu;
    __asm__ volatile("MCR p15, 0, %0, c9, c12, 2" : : "r"(val));

    /* PMOVSR: clear overflow flags (write-1-to-clear). */
    val = 0x8000000Fu;
    __asm__ volatile("MCR p15, 0, %0, c9, c12, 3" : : "r"(val));

    /* PMCCFILTR: clear all filter bits so the cycle counter counts in
       every mode. PMCCFILTR's reset value is UNKNOWN, so we must write
       it explicitly. Direct CP15 encoding is (p15, 0, Rt, c14, c15, 7);
       the PMSELR=0x1F + PMXEVTYPER indirect alias is CONSTRAINED
       UNPREDICTABLE on ARMv8 and does not reach PMCCFILTR on this core. */
    val = 0;
    __asm__ volatile("MCR p15, 0, %0, c14, c15, 7" : : "r"(val));

    /* Program each event counter via PMSELR + PMXEVTYPER. */
    for (i = 0; i < PMU_NUM_EVENT_COUNTERS; i++)
    {
        __asm__ volatile("MCR p15, 0, %0, c9, c12, 5" : : "r"(i));
        __asm__ volatile("ISB");
        val = pmu_event_ids[i]; /* filter bits = 0: count in all modes */
        __asm__ volatile("MCR p15, 0, %0, c9, c13, 1" : : "r"(val));
    }

    /* PMCR: E|P|C  (enable, reset event counters, reset cycle counter). */
    val = (1u << 0) | (1u << 1) | (1u << 2);
    __asm__ volatile("MCR p15, 0, %0, c9, c12, 0" : : "r"(val));

    /* PMCNTENSET: enable cycle counter (bit 31) + event counters 0..3. */
    val = 0x80000000u | ((1u << PMU_NUM_EVENT_COUNTERS) - 1u);
    __asm__ volatile("MCR p15, 0, %0, c9, c12, 1" : : "r"(val));

    __asm__ volatile("ISB");
}

void
pmu_disable(void)
{
    ee_u32 val;

    __asm__ volatile("ISB");
    /* PMCNTENCLR: stop cycle counter + event counters. */
    val = 0x80000000u | ((1u << PMU_NUM_EVENT_COUNTERS) - 1u);
    __asm__ volatile("MCR p15, 0, %0, c9, c12, 2" : : "r"(val));

    /* PMCR: clear E (global enable). */
    __asm__ volatile("MRC p15, 0, %0, c9, c12, 0" : "=r"(val));
    val &= ~1u;
    __asm__ volatile("MCR p15, 0, %0, c9, c12, 0" : : "r"(val));
    __asm__ volatile("ISB");
}

void
pmu_report(void)
{
    ee_u32 cycles;
    ee_u32 count;
    ee_u32 ovsr;
    ee_u32 i;

    __asm__ volatile("MRC p15, 0, %0, c9, c12, 3" : "=r"(ovsr));
    __asm__ volatile("MRC p15, 0, %0, c9, c13, 0" : "=r"(cycles));

    ee_printf("---- PMU results ----\n");
    ee_printf("  Cycle counter        : %u%s\n",
              (unsigned)cycles,
              (ovsr & 0x80000000u) ? "  (OVERFLOW)" : "");

    for (i = 0; i < PMU_NUM_EVENT_COUNTERS; i++)
    {
        __asm__ volatile("MCR p15, 0, %0, c9, c12, 5" : : "r"(i));
        __asm__ volatile("ISB");
        __asm__ volatile("MRC p15, 0, %0, c9, c13, 2" : "=r"(count));
        ee_printf("  %s (evt 0x%02x): %u%s\n",
                  pmu_event_names[i],
                  (unsigned)pmu_event_ids[i],
                  (unsigned)count,
                  (ovsr & (1u << i)) ? "  (OVERFLOW)" : "");
    }
    ee_printf("---------------------\n");
}
