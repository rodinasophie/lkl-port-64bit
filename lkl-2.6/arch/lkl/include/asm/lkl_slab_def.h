#ifndef __LINUX_LKL_SLAB_DEF_H
#define __LINUX_LKL_SLAB_DEF_H

void *__kmalloc(size_t size, gfp_t flags);

/**
 * kmalloc - allocate memory
 * @size: how many bytes of memory are required.
 * @flags: the type of memory to allocate (see kcalloc).
 *
 * kmalloc is the normal method of allocating memory
 * in the kernel.
 */
static inline void *kmalloc(size_t size, gfp_t flags)
{
	return __kmalloc(size, flags);
}

#endif /* __LINUX_LKL_SLAB_DEF_H */
