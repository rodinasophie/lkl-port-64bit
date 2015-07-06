#ifndef _ASM_LKL_TLB_H
#define _ASM_LKL_TLB_H

#define tlb_start_vma(tlb, vma)	do { } while (0)
#define tlb_end_vma(tlb, vma)	do { } while (0)
#define __tlb_remove_tlb_entry(tlb, ptep, address)	do { } while (0)

#define tlb_flush(tlb) flush_tlb_mm((tlb)->mm)

static inline void global_flush_tlb(void)
{
}

static inline 
int change_page_attr(struct page *page, int numpages, pgprot_t prot)
{
    return 0;
}


#include <asm-generic/tlb.h>

#endif
