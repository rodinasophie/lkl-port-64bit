#ifndef _ASM_LKL_FCNTL_H
#define _ASM_LKL_FCNTL_H


#ifdef __KERNEL__
#define __kernel_flock flock
#define __kernel_flock64 flock64
#endif

#define HAVE_ARCH_STRUCT_FLOCK
#define HAVE_ARCH_STRUCT_FLOCK64


struct __kernel_flock {
	short l_type;
	short l_whence;
	__kernel_off_t l_start;
	__kernel_off_t	l_len;
	__kernel_pid_t l_pid;
};


struct __kernel_flock64 {
	short  l_type;
	short  l_whence;
	__kernel_loff_t l_start;
	__kernel_loff_t l_len;
	__kernel_pid_t  l_pid;
};

#undef __kernel_flock
#undef __kernel_flock64

#include <asm-generic/fcntl.h>

#endif
