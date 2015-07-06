#ifndef _ASM_LKL_HARDIRQ_H
#define _ASM_LKL_HARDIRQ_H


typedef struct {
	unsigned int __softirq_pending;
	unsigned long idle_timestamp;
} ____cacheline_aligned irq_cpustat_t;


#include <linux/irq_cpustat.h>

static inline void ack_bad_irq(unsigned int irq)
{
	printk(KERN_ERR "unexpected IRQ %02x\n", irq);
	BUG();
}


#endif
