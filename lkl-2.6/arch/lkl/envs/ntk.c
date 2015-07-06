#include <ddk/ntddk.h>
#include <asm/callbacks.h>
#include <asm/lkl.h>

static void* sem_alloc(int count)
{
	KSEMAPHORE *sem=ExAllocatePoolWithTag(NonPagedPool, sizeof(*sem), 'LKLS');

	if (!sem)
		return NULL;

        KeInitializeSemaphore(sem, count, 100);	

	return sem;
}

static void sem_up(void *sem)
{
        KeReleaseSemaphore((KSEMAPHORE*)sem, 0, 1, 0);
}

static void sem_down(void *sem)
{
        KeWaitForSingleObject((KSEMAPHORE*)sem, Executive, KernelMode, FALSE, NULL);
}

static void sem_free(void *sem)
{
	ExFreePoolWithTag(sem, 'LKLS');
}

static void* thread_create(void (*fn)(void*), void *arg)
{
	void *thread;
	if (PsCreateSystemThread(&thread, THREAD_ALL_ACCESS, NULL, NULL, NULL,
				 (void DDKAPI (*)(void*))fn, arg) != STATUS_SUCCESS)
		return NULL;
	return thread;
}

static void thread_exit(void *arg)
{
	PsTerminateSystemThread(0);
}

/*
 * With 64 bits, we can cover about 583 years at a nanosecond resolution.
 * Windows counts time from 1601 so we do have about 100 years before we
 * overflow.
 */
static unsigned long long time(void)
{
	LARGE_INTEGER li;

	KeQuerySystemTime(&li);

        return li.QuadPart*100;
}

static KTIMER timer;
static KSEMAPHORE timer_killer_sem;
static volatile int timer_done;

static void set_timer(unsigned long delta)
{
	if (delta == LKL_TIMER_INIT)
		return;

	if (delta == LKL_TIMER_SHUTDOWN) {
		/* should not deliver timer shutdown twice */
		if(timer_done) {
			DbgPrint("*** LKL_TIMER_SHUTDOWN called when timer_done ***");
			while(1)
				;
		}

		/* deque the timer so it won't be put in signaled state */
		KeCancelTimer(&timer);
		/* timers run on DPCs. This returns after all active
		 * DPCs have executed, which means the timer is
		 * certainly not running nor being schduled after this
		 * point. */
		KeFlushQueuedDpcs();


		/* signal the timer interrupt we're done */
		timer_done = 1;
		/* the memory barrier is needed because it may be
		 * possible for the compiler/cpu to call
		 * KeReleaseSemaphore before assigning
		 * timer_done. That would make the timer_thread wake
		 * from the wait-for-multiple-objs without noticing
		 * out signalling */
		KeMemoryBarrier();
		KeReleaseSemaphore(&timer_killer_sem, 0, 1, 0);
		return;
	}

	KeSetTimer(&timer, RtlConvertLongToLargeInteger((unsigned long)(-(delta/100))), NULL);
}



#define WaitAll 0
#define WaitAny 1

static void *timer_wait_objs[] = {&timer, &timer_killer_sem};
static void DDKAPI timer_thread(LPVOID arg)
{
	while (1) {
		KeWaitForMultipleObjects(2, timer_wait_objs, WaitAny, Executive, KernelMode, FALSE, NULL, NULL);
		if (timer_done)
			break;
		lkl_trigger_irq(TIMER_IRQ);
	}

	PsTerminateSystemThread(STATUS_SUCCESS);
}

static long panic_blink(long time)
{
    DbgPrint("***Kernel panic!***");
    while (1)
	    ;
    return 0;
}

static void* mem_alloc(unsigned int size)
{
	return ExAllocatePoolWithTag(NonPagedPool, size, 'LKLP');
}

static void mem_free(void *data)
{
	ExFreePoolWithTag(data, 'LKLP');
}

static void print(const char *str, int len)
{
	DbgPrint("%s", str);
}

static KSEMAPHORE init_sem;

static int init(void)
{
        KeReleaseSemaphore(&init_sem, 0, 1, 0);
	return 0;
}

static struct lkl_native_operations nops = {
	.panic_blink = panic_blink,
	.thread_create = thread_create,
	.thread_exit = thread_exit,
	.sem_alloc = sem_alloc,
	.sem_free = sem_free,
	.sem_up = sem_up,
	.sem_down = sem_down,
	.time = time,
	.timer = set_timer,
	.init = init,
	.print = print,
	.mem_free = mem_free,
	.mem_alloc = mem_alloc,
};


static void DDKAPI init_thread(LPVOID arg)
{
	lkl_start_kernel(&nops, "");
	PsTerminateSystemThread(STATUS_SUCCESS);
}

PVOID init_thread_obj;
PVOID timer_thread_obj;

int lkl_env_init(unsigned long mem_size)
{
	HANDLE init_thread_handle, timer_thread_handle;
	NTSTATUS status;

	nops.phys_mem_size=mem_size;

	KeInitializeTimerEx(&timer, SynchronizationTimer);
        KeInitializeSemaphore(&init_sem, 0, 100);
        KeInitializeSemaphore(&timer_killer_sem, 0, 100);

	/* create the initial thread */
	status = PsCreateSystemThread(&init_thread_handle, THREAD_ALL_ACCESS,
				      NULL, NULL, NULL, init_thread, NULL);
	if (status != STATUS_SUCCESS)
		goto err;


	/* wait for the initial thread to complete initialization to
	 * be able to interact with it */
        status = KeWaitForSingleObject(&init_sem, Executive, KernelMode, FALSE, NULL);
	if (status != STATUS_SUCCESS)
		goto close_init_thread;

	/* create the timer thread responsible with delivering timer interrupts */
	status = PsCreateSystemThread(&timer_thread_handle, THREAD_ALL_ACCESS,
				      NULL, NULL, NULL, timer_thread, NULL);
	if (status != STATUS_SUCCESS)
		goto close_init_thread;


	/* get references to the init and timer threads to be able to wait on them */
	status = ObReferenceObjectByHandle(init_thread_handle, THREAD_ALL_ACCESS,
					   NULL, KernelMode, &init_thread_obj, NULL);
	if (!NT_SUCCESS(status))
		goto close_timer_thread;

	status = ObReferenceObjectByHandle(timer_thread_handle, THREAD_ALL_ACCESS,
					   NULL, KernelMode, &timer_thread_obj, NULL);
	if (!NT_SUCCESS(status))
		goto deref_init_thread_obj;

	/* we don't need the handles, we have access to the objects */
	ZwClose(timer_thread_handle);
	ZwClose(init_thread_handle);
	return STATUS_SUCCESS;


deref_init_thread_obj:
	ObDereferenceObject(init_thread_obj);
close_timer_thread:
	ZwClose(timer_thread_handle);
close_init_thread:
	ZwClose(init_thread_handle);
err:
	return status;
}

static void lkl_wait_init(void)
{
	KeWaitForSingleObject(init_thread_obj, Executive, KernelMode, FALSE, NULL);
	ObDereferenceObject(init_thread_obj);
}

static void lkl_wait_timer(void)
{
	KeWaitForSingleObject(timer_thread_obj, Executive, KernelMode, FALSE, NULL);
	ObDereferenceObject(timer_thread_obj);
}


int lkl_env_fini(void)
{
	lkl_sys_halt();
	lkl_wait_init();
	lkl_wait_timer();
	return 0;
}
