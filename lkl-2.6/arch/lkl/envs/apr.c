#include <apr.h>
#include <apr_atomic.h>
#include <apr_pools.h>
#include <apr_thread_proc.h>
#include <apr_thread_mutex.h>
#include <apr_thread_cond.h>
#include <apr_file_io.h>
#include <apr_time.h>
#include <apr_poll.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>

#include <asm/callbacks.h>
#include <asm/env.h>

static apr_pool_t *pool;

typedef struct {
	apr_thread_mutex_t *lock;
	int count;
	apr_thread_cond_t *cond;
} apr_thread_sem_t;

static void* sem_alloc(int count)
{
	apr_thread_sem_t *sem=malloc(sizeof(*sem));
	apr_status_t status;

	if (!sem)
		return NULL;

	sem->count=count;
        status=apr_thread_mutex_create(&sem->lock, APR_THREAD_MUTEX_UNNESTED,
				       pool);
	if (status != APR_SUCCESS) {
		free(sem);
		return NULL;
	}

	status=apr_thread_cond_create(&sem->cond, pool);
	if (status != APR_SUCCESS) {
		apr_thread_mutex_destroy(sem->lock);
		free(sem);
		return NULL;
	}

	return sem;
}

static void sem_free(void *_sem)
{
	apr_thread_sem_t *sem=(apr_thread_sem_t*)_sem;

	apr_thread_mutex_destroy(sem->lock);
	apr_thread_cond_destroy(sem->cond);
	free(sem);
}

static void sem_up(void *_sem)
{
	apr_thread_sem_t *sem=(apr_thread_sem_t*)_sem;

	apr_thread_mutex_lock(sem->lock);
	sem->count++;
	if (sem->count > 0)
		apr_thread_cond_signal(sem->cond);
	apr_thread_mutex_unlock(sem->lock);
}

static void sem_down(void *_sem)
{
	apr_thread_sem_t *sem=(apr_thread_sem_t*)_sem;

	apr_thread_mutex_lock(sem->lock);
	while (sem->count <= 0)
		apr_thread_cond_wait(sem->cond, sem->lock);
	sem->count--;
	apr_thread_mutex_unlock(sem->lock);
}

static void print(const char *str, int len)
{
	write(1, str, len);
}

static unsigned long long time(void)
{
	return apr_time_now()*1000;
}

struct bootstrap_arg {
	void (*fn)(void *arg);
	void *arg;
};

static void* APR_THREAD_FUNC bootstrap_thread(apr_thread_t *thread, void *_arg)
{
	struct bootstrap_arg *barg=(struct bootstrap_arg*)_arg;
	void (*fn)(void *arg)=barg->fn;
	void *arg=barg->arg;

	free(barg);
	fn(arg);
	return NULL;
}

static void* thread_create(void (*fn)(void*), void *arg)
{
	apr_thread_t *thread;
	apr_status_t status;
	struct bootstrap_arg *barg=malloc(sizeof(*barg));

	if (!barg)
		return NULL;

	barg->fn=fn;
	barg->arg=arg;

	status=apr_thread_create(&thread, NULL, bootstrap_thread, barg, pool);

	if (status != APR_SUCCESS)
		return NULL;

        return thread;
}

static void thread_exit(void *thread)
{
	apr_thread_exit((apr_thread_t*)thread, 0);
}

static apr_file_t *pipe_in, *pipe_out;

static void timer(unsigned long delta)
{
	apr_file_write_full(pipe_out, &delta, sizeof(delta), NULL);
}

static void* APR_THREAD_FUNC timer_thread(apr_thread_t *thr, void *arg)
{
	apr_int32_t num;
	apr_status_t status;
	const apr_pollfd_t *descriptors;
	static apr_pollset_t *pollset;
	apr_interval_time_t timeout_us=-1;
	unsigned long timeout_ns;

	apr_pollset_create(&pollset, 1, pool, 0);
	apr_file_pipe_create(&pipe_in, &pipe_out, pool);
	apr_pollfd_t apfd = {
		.p = pool,
		.desc_type = APR_POLL_FILE,
		.reqevents = APR_POLLIN,
		.desc = {
			.f = pipe_in
		}
	};
	apr_pollset_add(pollset, &apfd);

	/* wait for the Linux timer to be initialized */
	apr_file_read_full(pipe_in, &timeout_ns, sizeof(timeout_ns), NULL);

	while (1) {
		status=apr_pollset_poll(pollset, timeout_us, &num,
					&descriptors);
		timeout_us=-1;
		
		if (status != APR_SUCCESS) {
			if (!APR_STATUS_IS_TIMEUP(status) && APR_EINTR != status) {
				char buffer[128];
				printf("lkl: timer error: %s!\n", 
				       apr_strerror(status, buffer, sizeof(buffer)));
			}
			lkl_trigger_irq(TIMER_IRQ);
			continue;
		}

		apr_file_read_full(pipe_in, &timeout_ns,
				   sizeof(timeout_ns), NULL);
		if (timeout_ns == LKL_TIMER_SHUTDOWN)
			break;
		timeout_us=timeout_ns / 1000; 
		/* 
		 * apr, when using poll: timeout/=1000
		 *
		 * while(1){poll(,,0);} is really while(1);. Not to
		 * mention that we will generate a zillion interrupts
		 * while doing it.
		 */
		if (timeout_us < 1000) 
			timeout_us=1000; 
	}

	return NULL;
}

static long panic_blink(long time)
{
	assert(0);
	return 0;
}

static apr_thread_mutex_t *init_mutex;

static int init(void)
{
        apr_thread_mutex_unlock(init_mutex);
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
	.timer = timer,
	.init = init,
	.print = print,
	.mem_alloc = malloc,
	.mem_free = free
};


static void* APR_THREAD_FUNC init_thread(apr_thread_t *thr, void *arg)
{
	lkl_start_kernel(&nops, "");
	apr_thread_exit(thr, 0);
	return NULL;
}

/* FIXME: check for errors */
int lkl_env_init(unsigned long mem_size)
{
	apr_thread_t *a, *b;

	nops.phys_mem_size=mem_size;

	apr_pool_create(&pool, NULL);
	apr_thread_create(&a, NULL, timer_thread, NULL, pool);
	apr_thread_mutex_create(&init_mutex, APR_THREAD_MUTEX_UNNESTED, pool);
	apr_thread_mutex_lock(init_mutex);
	apr_thread_create(&b, NULL, init_thread, NULL, pool);
        apr_thread_mutex_lock(init_mutex);

	return 0;
}

