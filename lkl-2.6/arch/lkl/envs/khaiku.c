#include <KernelExport.h>
#include <OS.h>
#include <kernel/thread.h>
#include <fs_interface.h>
#include <stdlib.h>
#include <asm/callbacks.h>



static void khaiku_env_print(char *str, int len)
{
	char c = str[len];
	str[len] = '\0';
	dprintf("[lkl-console] %s", str);
	str[len] = c;
}


static void* khaiku_env_sem_alloc(int count)
{
	sem_id sem = create_sem(count, "lkl-generic-sem");
	if (sem == B_NO_MORE_SEMS)
		return NULL;

	return (void*) sem;
}

static void khaiku_env_sem_free(void *sem)
{
	delete_sem((sem_id) sem);
}

static void khaiku_env_sem_up(void *sem)
{
	release_sem_etc((sem_id) sem, 1, B_DO_NOT_RESCHEDULE);
}

static void khaiku_env_sem_down(void *sem)
{
	acquire_sem((sem_id) sem);
}

static void* khaiku_env_thread_create(void (*fn)(void*), void *arg)
{
	thread_id tid;

	tid = spawn_kernel_thread((int32 (*)(void*)) fn, "lkl-generic-thread",
		B_NORMAL_PRIORITY, arg);
	if ((tid == B_NO_MORE_THREADS) || (tid == B_NO_MEMORY))
			return NULL;

	/* initially threads are suspended. */
	resume_thread(tid);

	return (void*) tid;
}

static void khaiku_env_thread_exit(void *thread)
{
	/* TODO: I could not find documentation regarding thread exit
	   code values. As no one is waiting to check the status of
	   this thread, the value shouldn't matter. */
	exit_thread(0);
}

static void* khaiku_env_thread_id(void)
{
	return (void*) thread_get_current_thread_id();
}

static unsigned long long khaiku_env_time_ns(void)
{
        return real_time_clock_usecs() * 1000ULL;
}




static int32 env_timer_interrupt_generator(timer *t)
{
	lkl_trigger_irq(TIMER_IRQ);
}

static timer * g_timer;
static void khaiku_env_timer(unsigned long delta)
{

	switch (delta)
	{
	case LKL_TIMER_INIT:
		dprintf("khaiku_env_timer:: LKL_TIMER_INIT\n");
		return;


	case LKL_TIMER_SHUTDOWN:
		dprintf("khaiku_env_timer:: LKL_TIMER_SHUTDOWN\n");
		// if any previous timer existed, wait until it fires
		// so we don't unload LKL before the callback we setup is called,
		// If that were to happen, the code of the callback would be unloaded
		// and the timer (which runs in interrupt context with interrupts disabled)
		// would generate an irrecuperable page fault.
		if (g_timer)
		{
			cancel_timer(g_timer);
			free(g_timer);
			g_timer = NULL;
		}
		return;


	default:
		if (g_timer == NULL)
		{
			g_timer = malloc(sizeof(*g_timer));
		}
		else
		{
			// wait for the previous timer to fire and queue another one
			// TODO: deltas are not absolute, but relative.
			// Because we wait for previous timers to fire before inserting new ones
			cancel_timer(g_timer);
		}

		add_timer(g_timer, env_timer_interrupt_generator,
				  (bigtime_t) delta / 1000, B_ONE_SHOT_RELATIVE_TIMER);
	}
}


static long khaiku_env_panic_blink(long time)
{
	panic("khaiku_env_panic_blink :: lkl-triggered-panic-blink\n");
	return 0;
}


/* semaphore used to synchronize the init thread */
sem_id g_init_sem;

static int khaiku_env_init(void)
{
	/* this signals lkl_env_init() that the kernel has finished booting.
	   Users can now start making system calls */

	/* return 0 on success */
	return release_sem_etc(g_init_sem, 1, B_DO_NOT_RESCHEDULE) != B_OK;
}

static void khaiku_env_dump_stack(void)
{
	dprintf("khaiku_env_dump_stack :: not-implemented\n");
}

static void khaiku_env_halt(void)
{
	dprintf("lkl: halt user callback called\n");
}

static struct lkl_native_operations nops = {
	.panic_blink   = khaiku_env_panic_blink,
	.thread_create = khaiku_env_thread_create,
	.thread_exit   = khaiku_env_thread_exit,
	.thread_id     = khaiku_env_thread_id,
	.sem_alloc     = khaiku_env_sem_alloc,
	.sem_free      = khaiku_env_sem_free,
	.sem_up	       = khaiku_env_sem_up,
	.sem_down      = khaiku_env_sem_down,
	.time	       = khaiku_env_time_ns,
	.timer	       = khaiku_env_timer,
	.init	       = khaiku_env_init,
	.print	       = khaiku_env_print,
	.mem_alloc     = malloc,
	.mem_free      = free,
	.dump_stack    = khaiku_env_dump_stack,
	.halt	       = khaiku_env_halt,
};


static int32 init_thread(void *unused_arg)
{
	lkl_start_kernel(&nops, "");
	return 0;
}


thread_id g_init_thread;
int lkl_env_init(unsigned long mem_size)
{
	int rc = 0;
	nops.phys_mem_size = mem_size;

	g_init_sem = create_sem(0, "lkl-init-sem");
	if ((g_init_sem == B_NO_MEMORY) || (g_init_sem == B_NO_MORE_SEMS))
		return g_init_sem;

	g_init_thread = spawn_kernel_thread(init_thread, "lkl-init-thread",
					    B_NORMAL_PRIORITY, NULL);
	if ((g_init_thread == B_NO_MORE_THREADS) || (g_init_thread == B_NO_MEMORY))
	{
		delete_sem(g_init_sem);
		return g_init_thread;
	}

	/* all Haiku threads are initially suspended */
	resume_thread(g_init_thread);

	/* remain blocked until the kernel finishes booting */
	acquire_sem(g_init_sem);
	return 0;
}

int lkl_env_fini()
{
	status_t status = B_OK;

	lkl_sys_halt();
	wait_for_thread(g_init_thread, &status);
	delete_sem(g_init_sem);
	return 0;
}

