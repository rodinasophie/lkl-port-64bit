#ifndef _ASM_LKL_TIMEX_H
#define _ASM_LKL_TIMEX_H

typedef unsigned long cycles_t;

static inline cycles_t get_cycles (void)
{
	return 0;
}

#define CLOCK_TICK_RATE (HZ)

#endif
