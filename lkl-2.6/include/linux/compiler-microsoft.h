#ifndef __LINUX_COMPILER_H
#error "Please don't include <linux/compiler-intel.h> directly, include <linux/compiler.h> instead."
#endif


#undef __weak
#define __weak

#define __aligned__(x)
#define __attribute__(x)
#define __i386__
#define LITTLE_ENDIAN

