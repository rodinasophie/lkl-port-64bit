#include <pthread.h>
#include <malloc.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <execinfo.h>

#include <asm/callbacks.h>



static void print(const char *str, int len)
{
	write(1, str, len);
}


typedef struct {
	pthread_mutex_t lock;
	int count;
	pthread_cond_t cond;
} pthread_sem_t;

static void* sem_alloc(int count)
{
	pthread_sem_t *sem=malloc(sizeof(*sem));

	if (!sem)
		return NULL;

	pthread_mutex_init(&sem->lock, NULL);
	sem->count=count;
	pthread_cond_init(&sem->cond, NULL);

	return sem;
}

static void sem_free(void *sem)
{
	free(sem);
}

static void sem_up(void *_sem)
{
	pthread_sem_t *sem=(pthread_sem_t*)_sem;

	pthread_mutex_lock(&sem->lock);
	sem->count++;
	if (sem->count > 0)
		pthread_cond_signal(&sem->cond);
	pthread_mutex_unlock(&sem->lock);
}

static void sem_down(void *_sem)
{
	pthread_sem_t *sem=(pthread_sem_t*)_sem;
	
	pthread_mutex_lock(&sem->lock);
	while (sem->count <= 0)
		pthread_cond_wait(&sem->cond, &sem->lock);
	sem->count--;
	pthread_mutex_unlock(&sem->lock);
}

static void* thread_create(void (*fn)(void*), void *arg)
{
        int ret;
	pthread_t *thread=malloc(sizeof(*thread));

	if (!thread)
		return NULL;

        ret=pthread_create(thread, NULL, (void* (*)(void*))fn, arg);
	if (ret != 0) {
		free(thread);
		return NULL;
	}

        return thread;
}

static void thread_exit(void *thread)
{
	free(thread);
	pthread_exit(NULL);
}

static void* thread_id(void)
{
	return (void*)pthread_self();
}

static unsigned long long time_ns(void)
{
        struct timeval tv;

        gettimeofday(&tv, NULL);

        return tv.tv_sec*1000000000ULL+tv.tv_usec*1000ULL;
}

static int timer_pipe[2];

static void timer(unsigned long delta)
{
	write(timer_pipe[1], &delta, sizeof(delta));
}

static void* timer_thread(void *arg)
{
	long timeout_ms=-1;
	unsigned long timeout_ns;
	struct pollfd pf = {
		.events = POLLIN,
	};
	int err;


	if (pipe(timer_pipe) < 0) {
		printf("lkl: unable to create timer pipe\n");
		return NULL;
	}
	pf.fd=timer_pipe[0];

	/* wait for timer init */
	read(timer_pipe[0], &timeout_ns, sizeof(unsigned long));

	while (1) {
		err=poll(&pf, 1, timeout_ms);
		timeout_ms=-1;

		switch (err) {
		case 1:
			read(timer_pipe[0], &timeout_ns, sizeof(unsigned long));
			if (timeout_ns == LKL_TIMER_SHUTDOWN)
				return NULL;
			timeout_ms=timeout_ns/1000000; 
			/* 
			 * while(1){poll(,,0);} is really while(1);. Not to
			 * mention that we will generate a zillion interrupts
			 * while doing it.
			 */
			if (!timeout_ms)
				timeout_ms++;
			break;
		default:
			printf("lkl: timer error: %d %s!\n", err, strerror(errno));
			/* fall through */
		case 0:
			lkl_trigger_irq(TIMER_IRQ);
			break;
		}
	}

	return NULL;
}

static long panic_blink(long time)
{
	assert(0);
	return 0;
}

static pthread_mutex_t init_mutex=PTHREAD_MUTEX_INITIALIZER;

static int init(void)
{
	pthread_mutex_unlock(&init_mutex);
	return 0;
}

static void dump_stack(void)
{
#define DUMP_STACK_MAX_FRAMES 20
	void *frame_pointers[DUMP_STACK_MAX_FRAMES];
	size_t size;
	size_t i;

	/* This breaks the claim to be POSIX only in this file.
	 * backtrace() & co. are GNU extensions, but it's not clear
	 * how to do feature detection on it so it can be compiled out
	 * when not available.
	 *
	 * Use the -rdynamic linker option to get usable symbols.  The
	 * lkl Makefile strips symbols that do not start with _lkl or
	 * lkl with objdump. Skip that phase to get kernel symbols in dumps.
	 */
	size = backtrace(frame_pointers, DUMP_STACK_MAX_FRAMES);
	backtrace_symbols_fd(frame_pointers, size, STDOUT_FILENO);
}

static void halt(void)
{
	printf("lkl: halt user callback called\n");
}

static struct lkl_native_operations nops = {
	.panic_blink = panic_blink,
	.thread_create = thread_create,
	.thread_exit = thread_exit,
	.thread_id = thread_id,
	.sem_alloc = sem_alloc,
	.sem_free = sem_free,
	.sem_up = sem_up,
	.sem_down = sem_down,
	.time = time_ns,
	.timer = timer,
	.init = init,
	.print = print,
	.mem_alloc = malloc,
	.mem_free = free,
	.dump_stack = dump_stack,
	.halt = halt,
};


static void* init_thread(void *arg)
{
	lkl_start_kernel(&nops, "");
	return NULL;
}

/* FIXME: check for errors */
int lkl_env_init(unsigned long mem_size)
{
	/* don't really need them */
	pthread_t a, b;

	nops.phys_mem_size=mem_size;

        pthread_create(&b, NULL, timer_thread, NULL);
	pthread_mutex_lock(&init_mutex);
        pthread_create(&a, NULL, init_thread, NULL);
	pthread_mutex_lock(&init_mutex);

	return 0;
}

int lkl_env_fini()
{
	lkl_sys_halt();
        pthread_join(&timer_thread, NULL);
        pthread_join(&init_thread,  NULL);
	return 0;
}

