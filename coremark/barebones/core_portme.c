/*
Copyright 2018 Embedded Microprocessor Benchmark Consortium (EEMBC)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Original Author: Shay Gal-on
*/
#include "coremark.h"
#include "core_portme.h"
#include "bsp.h"   /* CR52 BSP: ee_printf, SCMT timer, cache/PMU bring-up */

#if VALIDATION_RUN
volatile ee_s32 seed1_volatile = 0x3415;
volatile ee_s32 seed2_volatile = 0x3415;
volatile ee_s32 seed3_volatile = 0x66;
#endif
#if PERFORMANCE_RUN
volatile ee_s32 seed1_volatile = 0x0;
volatile ee_s32 seed2_volatile = 0x0;
volatile ee_s32 seed3_volatile = 0x66;
#endif
#if PROFILE_RUN
volatile ee_s32 seed1_volatile = 0x8;
volatile ee_s32 seed2_volatile = 0x8;
volatile ee_s32 seed3_volatile = 0x8;
#endif
volatile ee_s32 seed4_volatile = ITERATIONS;
volatile ee_s32 seed5_volatile = 0;

/* Porting : Timing functions
        Time is captured from the SCMT free-running counter (CMSCNT); the
   register access and clock rate live in the BSP (bsp.h / cr52_hw.c).
*/
CORETIMETYPE
barebones_clock()
{
    return CMSCNT;
}

#define GETMYTIME(_t)              (*_t = barebones_clock())
#define MYTIMEDIFF(fin, ini)       ((fin) - (ini))
#define TIMER_RES_DIVIDER          1
#define SAMPLE_TIME_IMPLEMENTATION 1
#define EE_TICKS_PER_SEC           (CLOCKS_PER_SEC / TIMER_RES_DIVIDER)

/** Define Host specific (POSIX), or target specific global time variables. */
static CORETIMETYPE start_time_val, stop_time_val;

/* Function : start_time
        This function will be called right before starting the timed portion of
   the benchmark.
*/
void
start_time(void)
{
    GETMYTIME(&start_time_val);
}
/* Function : stop_time
        This function will be called right after ending the timed portion of the
   benchmark.
*/
void
stop_time(void)
{
    GETMYTIME(&stop_time_val);
}
/* Function : get_time
        Return an abstract "ticks" number that signifies time on the system.
*/
CORE_TICKS
get_time(void)
{
    CORE_TICKS elapsed
        = (CORE_TICKS)(MYTIMEDIFF(stop_time_val, start_time_val));
    return elapsed;
}
/* Function : time_in_secs
        Convert the value returned by get_time to seconds.
*/
secs_ret
time_in_secs(CORE_TICKS ticks)
{
    secs_ret retval = ((secs_ret)ticks) / (secs_ret)EE_TICKS_PER_SEC;
    return retval;
}

ee_u32 default_num_contexts = 1;

/* Function : portable_init
        Target specific initialization code
        Test for some common mistakes.
*/
void
portable_init(core_portable *p, int *argc, char *argv[])
{
    (void)argc; // prevent unused warning
    (void)argv; // prevent unused warning

    if (sizeof(ee_ptr_int) != sizeof(ee_u8 *))
    {
        ee_printf(
            "ERROR! Please define ee_ptr_int to a type that holds a "
            "pointer!\n");
    }
    if (sizeof(ee_u32) != 4)
    {
        ee_printf("ERROR! Please define ee_u32 to a 32b unsigned type!\n");
    }
    p->portable_id = 1;

    ee_printf("Timer Initialized\n");
    // Timer Init (SCMT)
    CMSSTR = 0x00; // Timer Stop
    CMSCOR = 0xffffffff;
    CMSCNT = 0;
    CMSCSR = 0x10f; // 32bit freerun, disable interrupt, OSCCLK/1=130.20KHz(EXTAL=16.66MHz,MD13=0,MD14=0
    ee_printf("Init count = %08x\n",  barebones_clock());
    CMSSTR = 0x20; // Timer Start
}
/* Function : portable_fini
        Target specific final code
*/
void
portable_fini(core_portable *p)
{
    p->portable_id = 0;
}

extern void main(void);

/* V4H Write Buffer Enable register. NOTE: same address as SCMT_BASE on
   V4H; this is written here before the SCMT timer is configured (in
   portable_init), matching the historical ordering. */
#define WBCTLR (*((volatile ee_u32 *)0xe6040000))

// From boot_mon.o
void writer_main(void)
{
	ee_u32 low, high;
	ee_u32 val;
	ee_u32 val2;

	read_ccsidr(&val, &val2);
	ee_printf("CCSIDR I=%08x, D=%08x\n", val, val2);

	read_cp15_c15( &low, &high);
	ee_printf("CPUACTLR High=%08x, Low=%08x\n", high, low);

	modify_imp_bpctlr();
	disable_mem_protect();

	cache_enable_el2();

	set_pmu();

    __asm__("DSB");
    __asm__("ISB");

#ifdef V4H
	// Write Buffer Enable
	WBCTLR = 1;
#endif

	__asm__("LDR sp, =__STACKS_END__");

	/* Switch from EL2 (HYP) to EL1 (SVC) before running the benchmark so
	   the PMU counters can count EL1 events with default HDCR. */
	ee_printf("Dropping to EL1 (SVC mode) for PMU measurement.\n");
	drop_to_el1();
	/* Beyond this point we are in SVC mode. */

	for( val=1; val<=1; val++){
		ee_printf("No.%d\n", val);
		(void)main();
		ee_printf("\n\n");
	}
	ee_printf("End\n");
}
