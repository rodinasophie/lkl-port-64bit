#ifndef _ASM_LKL_UACCESS_H
#define _ASM_LKL_UACCESS_H

#include <linux/string.h>

#define VERIFY_READ 0
#define VERIFY_WRITE 1

#define KERNEL_DS	((mm_segment_t){0})
#define USER_DS		((mm_segment_t){0})

typedef struct {
    long seg;
} mm_segment_t;

#define get_ds()	((mm_segment_t){0})
#define get_fs()	((mm_segment_t){0})
#define set_fs(x)	((void)x)

#define segment_eq(a,b)	((a).seg == (b).seg)


static inline int access_ok(int type, const void __user *addr, int size)
{
    return 1;
}


static inline int copy_from_user(void *to, const void __user *from, int n)
{
        memcpy(to, from, n);
        return 0;
}


static inline int copy_to_user(void __user *to, const void *from, int  len)
{
        memcpy(to, from, len);
        return 0;
}

static inline int strncpy_from_user(char *dst, const char __user *src, int count)
{
        strncpy(dst, src, count);
        return count>strlen(src)?strlen(src):count;
}

/*
 * strlen_user returns the length of all characters + the trailing NUL
 */
static inline int strnlen_user(const void __user *str, long len)
{
        return strnlen(str, len) + 1;
}

static inline int clear_user(void __user *mem, int len)
{
        memset(mem, 0, len);
        return 0;
}

#define __clear_user(mem, len) clear_user(mem, len)
#define __copy_from_user(to, from, n) copy_from_user(to, from, n)
#define __copy_to_user(to, from, n) copy_to_user(to, from, n)
#define __copy_to_user_inatomic(to, from, n) copy_to_user(to, from, n)
#define __copy_from_user_inatomic(to, from, n) copy_from_user(to, from, n)

#define get_user(x, ptr) ({ ((x) = *(ptr)); 0; })
#define put_user(x, ptr) ({ (*(ptr) = (x)); 0; })

#define __put_user(x, ptr) put_user(x, ptr)
#define __get_user(x, ptr) get_user(x, ptr)

struct exception_table_entry
{
	unsigned long insn, fixup;
};

#endif
