#ifndef _ASM_LKL_TARGET_H
#define _ASM_LKL_TARGET_H

#include <linux/stringify.h>

/* Don't actually use ELF, but Linux needs it even if ELFs are not used. */
#define ELF_CLASS	ELFCLASS32

#ifdef __GNUC__

#if __LONG_MAX__  == 9223372036854775807L
#define BITS_PER_LONG 64
#else
#define BITS_PER_LONG 32
#endif

#ifdef __i386__

#define LKL_LITTLE_ENDIAN

#if defined(__ELF__)
#define TARGET_FORMAT "elf32-i386"
#define TARGET_ARCH   "i386"
#elif defined(__MINGW32__)
#define TARGET_FORMAT "pe-i386"
#define TARGET_ARCH   "i386"
#endif

#endif /* __i386__ */

#ifdef __x86_64__

#define LKL_LITTLE_ENDIAN

#if defined(__ELF__)
#define TARGET_FORMAT "elf64-x86-64"
#define TARGET_ARCH   "x86_64"
#elif defined(__MINGW32__)
#define TARGET_FORMAT "pe-x86"
#define TARGET_ARCH   "x86_64"
#endif

#endif /* __x86_64__ */

#ifdef __powerpc__

#ifdef __BIG_ENDIAN__
#define LKL_BIG_ENDIAN
#endif

#if BITS_PER_LONG == 32
#define TARGET_FORMAT "elf32-powerpc"
#endif
#define TARGET_ARCH "powerpc"

#endif /* __powerpc__ */

#endif /* __GNUC__ */

#ifdef _MSC_VER
#define LKL_LITTLE_ENDIAN
#define TARGET_FORMAT "pe-i386"
#define TARGET_ARCH   "i386"
#define BITS_PER_LONG 32
#define __signed__ signed
#define __inline__ __inline
#define __builtin_constant_p(x) 0

#endif /* _MSC_VER */


#if ! defined(LKL_BIG_ENDIAN) && ! defined(LKL_LITTLE_ENDIAN)
#error "unknown endianess"
#endif

#ifndef TARGET_FORMAT
#error "TARGET_FORMAT not defined"
#endif

#ifndef TARGET_ARCH
#error "TARGET_ARCH not defined"
#endif

#ifndef BITS_PER_LONG
#error "BITS_PER_LONG not defined"
#endif


#endif
