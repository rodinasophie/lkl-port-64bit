#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/init_task.h>
#include <linux/fs.h>
#include <linux/mqueue.h>
#include <linux/seq_file.h>
#include <linux/start_kernel.h>
#include <linux/syscalls.h>
#include <linux/reboot.h>

#include <asm/callbacks.h>

struct lkl_native_operations *lkl_nops;

static struct signal_struct init_signals = INIT_SIGNALS(init_signals);
static struct sighand_struct init_sighand = INIT_SIGHAND(init_sighand);

union thread_union init_thread_union = { INIT_THREAD_INFO(init_task) };

/*
 * Initial task structure.
 *
 * All other task structs will be allocated on slabs in fork.c
 */
struct task_struct init_task = INIT_TASK(init_task);

EXPORT_SYMBOL(init_task);


long arch_ptrace(struct task_struct *child, long request, long addr, long data)
{
        return -EINVAL;
}

void (*pm_power_off)(void)=NULL;

int linux_halted=0;

void kill_all_threads(void)
{
	struct task_struct *p;

	for_each_process(p) {
		BUG_ON(p->state == TASK_RUNNING);
		free_thread_info(task_thread_info(p));
	}
}

void machine_halt(void)
{
	linux_halted=1;
}

void machine_power_off(void)
{
	machine_halt();
}

void machine_restart(char * __unused)
{
	machine_halt();
}

void flush_thread(void)
{
}

struct seq_operations cpuinfo_op;

extern void mem_init_0(void);
extern void mem_cleanup(void);

static char cmd_line[128];

void __init setup_arch(char **cl)
{
        *cl=cmd_line;
        panic_blink=lkl_nops->panic_blink;
	threads_init();

        mem_init_0();
}

extern int run_syscalls(void*);

static int init_err;

int kernel_execve(const char *filename, char *const argv[], char *const envp[])
{
	/* 
	 * Don't pass through zombie, go dead directly. We do this to clean up
	 * khelper threads when hotplug is enabled. I wonder why we don't have
	 * khelper zombies on regular systems :-?
	 */
	current->exit_signal=-1;

        if (strcmp(filename, "/init") == 0) {
		/* 
		 * Run any pending irqs, softirqs and rcus. We do this
		 * here to:
		 * - cleanup the khelper zombies
		 * - install the clock source (and switch to NO_HZ mode)
		 */
		synchronize_rcu();

		if (!lkl_nops->init || (init_err=lkl_nops->init()) == 0)
			run_syscalls(NULL);

		kernel_halt();

		/* 
		 * We want to kill init without panic()ing
		 */
		init_pid_ns.child_reaper=0;
		do_exit(0);

		return 0;
	}
	

	return -1;
}

extern void *halt_sem;

extern void free_IRQ(void);
extern void free_syscall(void);

int __init lkl_start_kernel(struct lkl_native_operations *nops, const char *fmt, ...)
{
	va_list ap;

	lkl_nops=nops;

	va_start(ap, fmt);
	vsnprintf(cmd_line, sizeof(cmd_line), fmt, ap);
	va_end(ap);

	start_kernel();

	kill_all_threads();

	threads_cleanup();

	/*
	 * Stop the timer.
	 */
	lkl_nops->timer(LKL_TIMER_SHUTDOWN);


	free_syscall();
	free_IRQ();
	/*
	 * We are almost dead, announce application.
	 */
	if (lkl_nops->halt)
		lkl_nops->halt();

	mem_cleanup();

	/* 
	 * Finish the halt system call, if any. 
	 */
	if (halt_sem)
		lkl_nops->sem_up(halt_sem);

	return init_err;
}


static int __init fs_setup(void)
{
	int fd;

	/* 
	 * To skip mounting the "real" rootfs. The current one (ramfs) is good
	 * enough for us.
	 */
	fd=sys_open("/init", O_CREAT, 0600);
	BUG_ON(fd < 0);
	sys_close(fd);
	
	return 0;
}

late_initcall(fs_setup);

