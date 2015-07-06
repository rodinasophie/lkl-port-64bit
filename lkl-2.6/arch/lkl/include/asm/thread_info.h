#ifndef _ASM_LKL_THREAD_INFO_H
#define _ASM_LKL_THREAD_INFO_H


#define THREAD_SIZE            (4096)

#ifndef __ASSEMBLY__

#include <linux/errno.h>
#include <asm/processor.h>
#include <asm/types.h>
#include <asm/uaccess.h>
#include <linux/linkage.h>


struct thread_info {
	struct task_struct *task;
	struct exec_domain *exec_domain;
	unsigned long		flags;		/* low level flags */
	int preempt_count;
        mm_segment_t		addr_limit;	
	struct restart_block    restart_block;
};


struct thread_struct {
};

#define INIT_THREAD {  } 

#define PREEMPT_ACTIVE		0x10000000

#define INIT_THREAD_INFO(tsk)			\
{						\
	.task =		&tsk,			\
	.exec_domain =	&default_exec_domain,	\
	.preempt_count =	1,		\
	.addr_limit	= KERNEL_DS,		\
	.restart_block =  {			\
		.fn =  do_no_restart_syscall,	\
	},					\
}

void threads_init(void);
void threads_cleanup(void);

#define init_thread_info	(init_thread_union.thread_info)
#define init_stack		(init_thread_union.stack)

/* how to get the thread information struct from C */
extern struct thread_info *_current_thread_info;
static inline struct thread_info *current_thread_info(void)
{
        return _current_thread_info;
}

#define __HAVE_ARCH_THREAD_INFO_ALLOCATOR
/* thread information allocation */
extern struct thread_info* alloc_thread_info(struct task_struct*);
extern void free_thread_info(struct thread_info*);

static inline void cpu_relax(void)
{
}

//REVIEW: any way to do this portable?
#define current_text_addr() (NULL)

static inline unsigned long get_wchan(struct task_struct *task)
{
        return 0;
}

#define KSTK_EIP(task) (0)
#define KSTK_ESP(task) (0)

static inline void release_thread(struct task_struct *dead_task)
{
}

static inline void prepare_to_copy(struct task_struct *tsk)
{
}

static inline unsigned long thread_saved_pc(struct task_struct *tsk)
{
        return 0;
	//return ((unsigned long *)tsk->thread.esp)[3];
}

extern asmlinkage int kernel_thread(int (*fn)(void *), void * arg, unsigned long flags);

#endif



/*
 * thread information flags
 * - these are process state flags that various assembly files may need to access
 * - pending work-to-be-done flags are in LSW
 * - other flags in MSW
 */
#define TIF_SYSCALL_TRACE	0	/* syscall trace active */
#define TIF_NOTIFY_RESUME	1	/* resumption notification requested */
#define TIF_SIGPENDING		2	/* signal pending */
#define TIF_NEED_RESCHED	3	/* rescheduling necessary */
#define TIF_SINGLESTEP		4	/* restore singlestep on return to user mode */
#define TIF_IRET		5	/* return with iret */

/* TODO: LAG: see if this can be enabled in LKL (as it is present in x86 UML) */
/* #define TIF_SYSCALL_EMU		6 */	/* syscall emulation active */

#define TIF_SYSCALL_AUDIT	7	/* syscall auditing active */
#define TIF_SECCOMP		8	/* secure computing */
#define TIF_RESTORE_SIGMASK	9	/* restore signal mask in do_signal() */
#define TIF_MEMDIE		16
#define TIF_DEBUG		17	/* uses debug registers */
#define TIF_IO_BITMAP		18	/* uses I/O bitmap */
#define TIF_FREEZE		19	/* is freezing for suspend */


#endif
