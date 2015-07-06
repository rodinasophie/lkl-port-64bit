#ifndef _ASM_LKL_TLBFLUSH_H
#define _ASM_LKL_TLBFLUSH_H


static inline void flush_tlb_all(void)
{
}

static inline void flush_tlb_mm(struct mm_struct *mm)
{
}

static inline 
void flush_tlb_range(struct vm_area_struct *vma, unsigned long start, 
                     unsigned long end)
{
}


static inline 
void  flush_tlb_page(struct vm_area_struct *vma, unsigned long vmaddr)
{
}

static inline
void flush_tlb_kernel_vm(void)
{
}

static inline
void flush_tlb_kernel_range(unsigned long start, unsigned long end)
{
}

static inline
void __flush_tlb_one(unsigned long addr)
{
}

static inline 
void flush_tlb_pgtables(struct mm_struct *mm,
                        unsigned long start, unsigned long end)
{
}


#endif
