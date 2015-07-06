#ifndef _ASM_LKL_CALLBACKS_H
#define _ASM_LKL_CALLBACKS_H

#include "irq.h"

struct lkl_native_operations {
	/*
	 * All printks go out through this callback.
	 */
	void (*print)(const char *str, int len);

	/*
	 * Called to print non-LKL stack dumps [optional]
	 */
	void (*dump_stack)(void);

	/*
	 * Called during a kernel panic. 	 
	 */
	long (*panic_blink)(long time);

	/*
	 * Semaphore operations.
	 */
	void* (*sem_alloc)(int count);
	void (*sem_free)(void *sem);
	void (*sem_up)(void *sem);
	void (*sem_down)(void *sem);

	/*
	 * Create a new thread and starts running f passing arg as a
	 * parameter. Returns NULL or the thread handle.
	 */
	void* (*thread_create)(void (*f)(void*), void *arg);

	/*
	 * Terminate the currently running thread. thread is the handle returned
	 * by thread_create.
	 */
	void (*thread_exit)(void *thread);

	void* (*thread_id)(void);

	/*
	 * Memory operations.
	 */
	void* (*mem_alloc)(unsigned int);
	void (*mem_free)(void *);
	unsigned int phys_mem_size;

	/*
	 * This routine is called after the kernel initialization is
	 * complete and before the syscall thread is started. The application
	 * should do here any initialization it requires. 
	 */
	int (*init)(void);

	#define LKL_TIMER_INIT 0
	#define LKL_TIMER_SHUTDOWN 1
	#define LKL_TIMER_LAST_OP 1
        /*
         * Request a timer interrupt in delta nanoseconds, i.e. a
         * linux_trigger_irq(TIMER_IRQ) call. If delta <= LKL_TIMER_LAST_OP then
         * it is a special timer request. 
         */
	void (*timer)(unsigned long delta);
        
        /*
         * Return the current time in nanoseconds 
         */
        unsigned long long (*time)(void);

	/*
	 * Kernel has been halted, linux_start_kernel will exit soon. This is
	 * the chance for the app to clean-up any resources allocated so far. Do
	 * not forget to:
	 *
	 * - free the memory allocated via mem_init
	 * 
	 */
	void (*halt)(void);
};

extern struct lkl_native_operations *lkl_nops;

/*
 * Signal an interrupt. Can be called at any time, including early boot time. 
 */
void lkl_trigger_irq(int irq);


/*
 * Signal an interrupt with data to be passed in irq_data of the pt_regs
 * structure. (see get_irq_regs). For device driver convenience. Can't be called
 * during early boot time, before the SLAB is initialized.
 */
int lkl_trigger_irq_with_data(int irq, void *data);

/*
 * Clears all pending irqs.
 */
void lkl_purge_irq_queue(int irq);

/*
 * Register the native operations and start the kernel. The function returns
 * only after the kernel shutdowns. 
 */
int lkl_start_kernel(struct lkl_native_operations *lkl_nops, const char *cmd_line, ...);

#endif
