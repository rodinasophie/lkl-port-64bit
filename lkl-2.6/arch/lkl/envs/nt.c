#include <windows.h>
#include <assert.h>
#include <unistd.h>

#include <asm/callbacks.h>
#include <asm/env.h>

static void* sem_alloc(int count)
{
        return CreateSemaphore(NULL, count, 100, NULL);
}

static void sem_up(void *sem)
{
	ReleaseSemaphore(sem, 1, NULL);
}

static void sem_down(void *sem)
{
        WaitForSingleObject(sem, INFINITE);
}

static void sem_free(void *sem)
{
	CloseHandle(sem);
}

static void* thread_create(void (*fn)(void*), void *arg)
{
        return CreateThread(NULL, 0, (DWORD WINAPI (*)(LPVOID arg))fn, arg, 0, NULL);
}

static void thread_exit(void *arg)
{
	ExitThread(0);
}


/*
 * With 64 bits, we can cover about 583 years at a nanosecond resolution.
 * Windows counts time from 1601 so we do have about 100 years before we
 * overflow.
 */
static unsigned long long time(void)
{
	SYSTEMTIME st;
	FILETIME ft;
	LARGE_INTEGER li;

	GetSystemTime(&st);
	SystemTimeToFileTime(&st, &ft);
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;
	
        return li.QuadPart*100;
}

static HANDLE timer;
static int timer_done;

static void set_timer(unsigned long delta)
{
	LARGE_INTEGER li = {
		.QuadPart = -((long)(delta/100)),
	};

	if (delta == LKL_TIMER_INIT)
		return;

	if (delta == LKL_TIMER_SHUTDOWN) {
		timer_done=1;
		li.QuadPart=0;
	}
        
	SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE);
}

static DWORD WINAPI timer_thread(LPVOID arg)
{
	while (1) {
		WaitForSingleObject(timer, INFINITE);
		if (timer_done)
			return 0;
		lkl_trigger_irq(TIMER_IRQ);
	}
}

static long panic_blink(long time)
{
	assert(0);
	return 0;
}

static void print(const char *str, int len)
{
	write(1, str, len);
}

static HANDLE init_sem;

static int init(void)
{
	ReleaseSemaphore(init_sem, 1, NULL);
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
	.mem_alloc = malloc,
	.mem_free = free
};


static DWORD WINAPI init_thread(LPVOID arg)
{
	lkl_start_kernel(&nops, "");
	return 0;
}

/* FIXME: check for errors */
int lkl_env_init(unsigned long mem_size)
{
	nops.phys_mem_size=mem_size;

	timer=CreateWaitableTimer(NULL, FALSE, NULL);
	init_sem=CreateSemaphore(NULL, 0, 100, NULL);
        CreateThread(NULL, 0, timer_thread, NULL, 0, NULL);
        CreateThread(NULL, 0, init_thread, NULL, 0, NULL);
        WaitForSingleObject(init_sem, INFINITE);

	return 0;
}

