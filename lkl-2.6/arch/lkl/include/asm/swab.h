#ifndef _ASM_LKL_SWAB_H
#define _ASM_LKL_SWAB_H

#include <asm/types.h>
#include <linux/compiler.h>

#if !defined(__STRICT_ANSI__) || defined(__KERNEL__)
#  define __SWAB_64_THRU_32__
#endif

static __inline__ __attribute_const__ __u16 ___arch__swab16(__u16 x)
{
	return ((x&0xff)<<8)+(x>>8);
}
#define __arch__swab16 ___arch__swab16

static __inline__ __attribute_const__ __u32 ___arch__swab32(__u32 x)
{
	return (___arch__swab16(x&0xffff)<<16)+___arch__swab16(x>>16);
}
#define __arch__swab32 ___arch__swab32

static __inline__ __attribute_const__ __u64 ___arch__swab64(__u64 x)
{
	return ((__u64)___arch__swab32(x&0xffffffff)<<32)+___arch__swab32(x>>32);
}
#define __arch__swab64 ___arch__swab64

#endif//_ASM_LKL_SWAB_H
