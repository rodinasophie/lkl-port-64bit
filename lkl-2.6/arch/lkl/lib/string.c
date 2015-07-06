#include <linux/module.h>
#include <linux/string.h>

/*
 * Suprisingly, this tiny optimization does matter.
 */

#define do_copy(dest, src, count, type) \
	while (count >= sizeof(type)) { \
		*(type*)dest=*(type*)src; \
		dest+=sizeof(type); src+=sizeof(type); count-=sizeof(type); \
	}

void *memcpy(void *dest, const void *src, size_t count)
{
	void *ret=dest;

	do_copy(dest, src, count, long);
	do_copy(dest, src, count, char);

	return ret;
}

EXPORT_SYMBOL(memcpy);
