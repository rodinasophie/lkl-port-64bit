#ifndef _ASM_LKL_PAGE_H
#define _ASM_LKL_PAGE_H

/* PAGE_SHIFT determines the page size */

#include <asm/page_offset.h>

/*
 * HACK: FIXME: TODO: normally CONFIG_* macros are defined by Kconfig.
 * Here we set CONFIG_KERNEL_RAM_BASE_ADDRESS because this is used by
 * asm-generic/page.h to set PAGE_OFFSET.
 */
#define CONFIG_KERNEL_RAM_BASE_ADDRESS	(PAGE_OFFSET_RAW)

#include <asm-generic/page.h>

#endif /* _ASM_LKL_PAGE_H */
