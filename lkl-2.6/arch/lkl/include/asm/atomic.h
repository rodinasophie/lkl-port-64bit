#ifndef _ASM_LKL_ATOMIC_H
#define _ASM_LKL_ATOMIC_H

#include <asm/cmpxchg-local.h>
#include <asm-generic/atomic.h>

static __inline__ int atomic_dec_if_positive(atomic_t *v)
{
	int ret;
	
	ret = v->counter - 1;
	if (ret >= 0)
	    v->counter = ret;
	return ret;
}


/*
 * cmpxchg_local and cmpxchg64_local are atomic wrt current CPU. Always make
 * them available.
 */
#define cmpxchg_local(ptr, o, n)					\
  ((__typeof__(*(ptr)))__cmpxchg_local_generic((ptr), (unsigned long)(o), \
					       (unsigned long)(n), sizeof(*(ptr))))
#define cmpxchg64_local(ptr, o, n) __cmpxchg64_local_generic((ptr), (o), (n))

/* this include will make us fail on SMP. */
#include <asm-generic/cmpxchg.h>


#endif
