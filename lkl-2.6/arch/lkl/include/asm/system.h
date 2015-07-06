#ifndef _ASM_LKL_SYSTEM_H
#define _ASM_LKL_SYSTEM_H

#include <asm/target.h>
#include <asm/irqflags.h>
#include <asm/cache.h>
#include <asm/irq.h>

#ifdef __KERNEL__
#ifndef __ASSEMBLY__

extern unsigned long __local_save_flags(void);
extern void __local_irq_restore(unsigned long flags);
extern void local_irq_enable(void);
extern void local_irq_disable(void);
#define local_save_flags(flags) do { (flags) = __local_save_flags(); } while(0)
#define local_irq_restore(flags) do { __local_irq_restore(flags); } while(0)

#define local_irq_save(flags) do { local_save_flags(flags); \
                                   local_irq_disable(); } while(0)

#define irqs_disabled()                 \
({                                      \
        unsigned long flags;            \
        local_save_flags(flags);        \
        (flags == 0);                   \
})

#endif /* __ASSEMBLY__ */

#include <asm-generic/system.h>
#include <asm-generic/cmpxchg.h>
#include <asm-generic/cmpxchg-local.h>

#endif /* __KERNEL__ */
#endif /* _ASM_LKL_SYSTEM_H */
