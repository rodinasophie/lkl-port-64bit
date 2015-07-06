#ifndef __LKL_IO_H
#define __LKL_IO_H

#include <asm/page.h>

#define IO_SPACE_LIMIT 0xdeadbeef /* Sure hope nothing uses this */


/*
 * Convert a physical pointer to a virtual kernel pointer for /dev/mem
 * access
 */
#define xlate_dev_mem_ptr(p)	__va(p)

/*
 * Convert a virtual cached pointer to an uncached pointer
 */
#define xlate_dev_kmem_ptr(p)	(p)



static inline unsigned long virt_to_phys(volatile void *address)
{
	return __pa((unsigned long)address);
}

static inline void *phys_to_virt(unsigned long address)
{
	return __va(address);
}


#endif /* __LKL_IO_H */
