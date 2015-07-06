#ifndef _ASM_LKL_SMP_H
#define _ASM_LKL_SMP_H

#define smp_wmb() wmb()
#define smp_rmb() rmb()
#define smp_mb() mb()
#define smp_read_barrier_depends() mb()
#define smp_mb__before_clear_bit() mb()
#define smp_mb__after_clear_bit() mb()
#define set_mb(var, value) do { var = value; barrier(); } while (0)

#endif
