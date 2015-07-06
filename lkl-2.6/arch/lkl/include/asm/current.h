#ifndef _ASM_LKL_CURRENT_H
#define _ASM_LKL_CURRENT_H

#ifndef __ASSEMBLY__

#include <linux/thread_info.h>

#define current (current_thread_info()->task)

#endif /* __ASSEMBLY__ */

#endif
