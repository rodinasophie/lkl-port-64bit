#ifndef _ASM_LKL_MMU_H
#define _ASM_LKL_MMU_H

typedef struct {
	struct vm_list_struct	*vmlist;
	unsigned long		end_brk;
} mm_context_t;

/*
 * We don't have strict user/kernel spaces. 
 */
#define TASK_SIZE	((unsigned long)-1)

/* This decides where the kernel will search for a free chunk of vm
 * space during mmap's.
 */
#define TASK_UNMAPPED_BASE	0xdead0020

#endif /* _ASM_LKL_MMU_H */
