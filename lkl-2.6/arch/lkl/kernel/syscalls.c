#include <linux/syscalls.h>
#include <asm/stat.h>
#include <linux/stat.h>
#include <asm/lkl.h>
#include <asm/unistd.h>
#include <asm/callbacks.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/jhash.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/syscalls.h>
#include <linux/net.h>

typedef long (*syscall_handler_t)(long arg1, ...);

syscall_handler_t syscall_table[NR_syscalls];

static struct syscall_queue {
	struct list_head lh;
	wait_queue_head_t wqh;
	void *thread;
	struct hlist_node hn;
} *default_queue;

#define SYSCALL_QUEUE_HASH_SIZE 113
static struct hlist_head syscall_queue_hash[SYSCALL_QUEUE_HASH_SIZE];
static void* syscall_queue_hash_lock;

static inline struct hlist_head *syscall_queue_hash_head(void *thread)
{
	int i=jhash_2words(((u64)(u32)thread) >> 32,
			   ((u64)(u32)thread)&0xffffffff,
			   0) % SYSCALL_QUEUE_HASH_SIZE;
	return &syscall_queue_hash[i];
}

static struct syscall_queue* get_syscall_queue(void *thread)
{	
	struct hlist_node *i;
	struct hlist_head *head=syscall_queue_hash_head(thread);


	lkl_nops->sem_down(syscall_queue_hash_lock);
        hlist_for_each(i, head) {
		struct syscall_queue *sq=hlist_entry(i, struct syscall_queue,
						      hn);

		if (sq->thread == thread) {
			lkl_nops->sem_up(syscall_queue_hash_lock);
			return sq;
		}
	}
	lkl_nops->sem_up(syscall_queue_hash_lock);

	return default_queue;
}

static struct syscall_queue* new_syscall_queue(void *thread)
{
	struct syscall_queue *sq=kmalloc(sizeof(*sq), GFP_KERNEL);

	if (!sq)
		return NULL;

	INIT_LIST_HEAD(&sq->lh);
	init_waitqueue_head(&sq->wqh);
	sq->thread=thread;
	
	lkl_nops->sem_down(syscall_queue_hash_lock);
	hlist_add_head(&sq->hn, syscall_queue_hash_head(thread));
	lkl_nops->sem_up(syscall_queue_hash_lock);

	return sq;
}

static void del_syscall_queue(struct syscall_queue *sq)
{
	lkl_nops->sem_down(syscall_queue_hash_lock);
	hlist_del(&sq->hn);
	lkl_nops->sem_up(syscall_queue_hash_lock);

	kfree(sq);
}

struct syscall {
	long no, params[6], ret;
	void *sem;
	struct syscall_queue *queue;
	union {
		struct list_head lh;
		struct work_struct work;
	};
};

static struct syscall* dequeue_syscall(struct list_head *head)
{
	struct syscall *s=NULL;
	if (!list_empty(head)) {
		s=list_first_entry(head, typeof(*s), lh);
		list_del(&s->lh);
	}

	return s;
}

static long run_syscall(struct syscall *s)
{
	int ret;

	if (s->no < 0 || s->no >= NR_syscalls ||
	    syscall_table[s->no] == NULL) 
		ret=-ENOSYS;
	else
		ret=syscall_table[s->no](s->params[0], s->params[1],
					 s->params[2], s->params[3],
					 s->params[4], s->params[5]);
	s->ret=ret;
	if (s->sem)
		lkl_nops->sem_up(s->sem);
	return ret;
}

static void run_syscall_work(struct work_struct *work)
{
	struct syscall *s=container_of(work, struct syscall, work);

	run_syscall(s);
}

int run_syscalls(void *_sq)
{
	struct syscall_queue *sq;
	struct syscall *s;

	if (!_sq) 
		sq=default_queue;
	else
		sq=(struct syscall_queue*)_sq;

	snprintf(current->comm, sizeof(current->comm), "ksyscalld%d",
		 current->pid);

	while (1) {
		wait_event(sq->wqh, (s=dequeue_syscall(&sq->lh)) != NULL);

		if (s->no ==__NR_reboot)
			break;

		run_syscall(s);
	}

	del_syscall_queue(sq);
	s->ret=0;
	lkl_nops->sem_up(s->sem);

	if (sq != default_queue) 
		do_exit(0);

	return 0;
}

static irqreturn_t syscall_irq(int irq, void *dev_id)
{
	struct pt_regs *regs=get_irq_regs();
	struct syscall *s=regs->irq_data;

	if (!s->queue) {
		INIT_WORK(&s->work, run_syscall_work);
		schedule_work(&s->work);
	} else {
		list_add(&s->lh, &s->queue->lh);
		wake_up(&s->queue->wqh);
	}

        return IRQ_HANDLED;
}

static struct irqaction syscall_irqaction  = {
	.handler = syscall_irq,
	.flags = IRQF_DISABLED | IRQF_NOBALANCING,
        .dev_id = &syscall_irqaction,
	.name = "syscall"
};

#define SYSCALL_VIA_QUEUE(_queue, _syscall, _params...)			\
({									\
	struct syscall s = {						\
		.no = __NR_##_syscall,					\
		.params = { _params },					\
		.queue = _queue						\
	};								\
	if (!(s.sem=lkl_nops->sem_alloc(0)))				\
		return -ENOMEM;						\
	lkl_trigger_irq_with_data(SYSCALL_IRQ, &s);			\
	lkl_nops->sem_down(s.sem);					\
	lkl_nops->sem_free(s.sem);					\
	s.ret;								\
})

#define SYSCALL(_syscall, _params...)					\
({									\
	int ret;							\
	if (lkl_nops->thread_id)					\
		ret=SYSCALL_VIA_QUEUE(get_syscall_queue(lkl_nops->thread_id()), _syscall, _params); \
	else								\
		ret=SYSCALL_VIA_QUEUE(default_queue, _syscall, _params); \
	ret;								\
})

#define SYSCALL_VIA_WORKQUEUE(_syscall, _params...) \
	SYSCALL_VIA_QUEUE(NULL, _syscall, _params)


static asmlinkage long sys_call(long _f, long arg1, long arg2, long arg3,
				long arg4, long arg5)
{
	long (*f)(long arg1, long arg2, long arg3, long arg4, long arg5)=
		(long (*)(long, long, long, long, long))_f;
	return f(arg1, arg2, arg3, arg4, arg5);
}

/*
 * sys_mount (is sloppy?? and) copies a full page from dev_name, type and data
 * which can trigger page faults (which a normal kernel can safely
 * ignore). Make sure we don't trigger page fauls.
 */
asmlinkage long sys_safe_mount(char __user *dev_name, char __user *dir_name,
				char __user *type, unsigned long flags,
				void __user *data)
{
	int err;
	unsigned long _dev_name=0, _type=0, _data=0;

	err=-ENOMEM;
	if (dev_name) {
		_dev_name=__get_free_page(GFP_KERNEL);
		if (!_dev_name)
			goto out_free;
		strcpy((char*)_dev_name, dev_name);
	}

	if (type) {
		_type=__get_free_page(GFP_KERNEL);
		if (!_type)
			goto out_free;
		strcpy((char*)_type, type);
	}

	if (data) {
		_data=__get_free_page(GFP_KERNEL);
		if (!_data)
			goto out_free;
		strcpy((char*)_data, data);
	}

	err=sys_mount((char __user*)_dev_name, dir_name, (char __user*)_type,
		      flags, (char __user*)_data);

out_free:
	if (_dev_name)
		free_page(_dev_name);
	if (_type)
		free_page(_type);
	if (_data)
		free_page(_data);

	return err;
}

ssize_t sys_lkl_pwrite64(unsigned int fd, const char *buf, size_t count,
		       off_t pos_hi, off_t pos_lo)
{
	return sys_pwrite64(fd, buf, count, ((loff_t)pos_hi<<32)+pos_lo);
}

ssize_t sys_lkl_pread64(unsigned int fd, char *buf, size_t count,
		       off_t pos_hi, off_t pos_lo)
{
	return sys_pread64(fd, buf, count, ((loff_t)pos_hi<<32)+pos_lo);
}


#define INIT_STE(x) syscall_table[__NR_##x]=(syscall_handler_t)sys_##x;

void init_syscall_table(void)
{
	INIT_STE(ni_syscall);
	INIT_STE(sync);
	INIT_STE(reboot);
	INIT_STE(write);
	INIT_STE(close);
	INIT_STE(unlink);
	INIT_STE(open);
	INIT_STE(poll);
	INIT_STE(read);
	INIT_STE(lseek);
	INIT_STE(rename);
	INIT_STE(flock);
	INIT_STE(newfstat);
	//INIT_STE(fstat64);
	INIT_STE(chmod);
	INIT_STE(newlstat);
	//INIT_STE(lstat64);
	INIT_STE(mkdir);
	INIT_STE(rmdir);
	INIT_STE(getdents);
	INIT_STE(newstat);
	//INIT_STE(stat64);
	INIT_STE(utimes);
	INIT_STE(utime);
	INIT_STE(nanosleep);
	INIT_STE(mknod);
	INIT_STE(safe_mount);
	INIT_STE(umount);
	INIT_STE(chdir);
	INIT_STE(statfs);
	INIT_STE(chroot);
	INIT_STE(getcwd);
	INIT_STE(chown);
	INIT_STE(umask);
	INIT_STE(getuid);
	INIT_STE(getgid);
#ifdef CONFIG_NET
	INIT_STE(socketcall);
#endif
	INIT_STE(ioctl);
	INIT_STE(call);
	INIT_STE(access);
	INIT_STE(truncate);
	INIT_STE(lkl_pwrite64);
	INIT_STE(lkl_pread64);
	INIT_STE(getpid);
}

LKLAPI long lkl_sys_sync(void)
{
	return SYSCALL(sync);
}

LKLAPI long lkl_sys_umount(const char *path, int flags)
{
	return SYSCALL(umount, (long)path, flags);
}

LKLAPI ssize_t lkl_sys_write(unsigned int fd, const char *buf, size_t count)
{
	return SYSCALL(write, fd, (long)buf, count);
}

LKLAPI long lkl_sys_close(unsigned int fd)
{
	return SYSCALL(close, fd);
}

LKLAPI long lkl_sys_unlink(const char *pathname)
{
	return SYSCALL(unlink, (long)pathname);
}

LKLAPI long lkl_sys_open(const char *filename, int flags, int mode)
{
       return SYSCALL(open, (long)filename, flags, mode);
}

LKLAPI long lkl_sys_poll(struct pollfd *ufds, unsigned int nfds, long timeout)
{
	return SYSCALL(poll, (long)ufds, nfds, timeout);
}

LKLAPI ssize_t lkl_sys_read(unsigned int fd, char *buf, size_t count)
{
	return SYSCALL(read, fd, (long)buf, count);
}

LKLAPI off_t lkl_sys_lseek(unsigned int fd, off_t offset, unsigned int origin)
{
	return SYSCALL(lseek, fd, offset, origin);
}

LKLAPI long lkl_sys_rename(const char *oldname, const char *newname)
{
	return SYSCALL(rename, (long)oldname, (long)newname);
}

LKLAPI long lkl_sys_flock(unsigned int fd, unsigned int cmd)
{
	return SYSCALL(flock, fd, cmd);
}

LKLAPI long lkl_sys_newfstat(unsigned int fd, struct __kernel_stat *statbuf)
{
	return SYSCALL(newfstat, fd, (long)statbuf);
}

LKLAPI long lkl_sys_fstat64(unsigned int fd, struct __kernel_stat64 *statbuf)
{
	return SYSCALL(fstat64, fd, (long)statbuf);
}

LKLAPI long lkl_sys_chmod(const char *filename, mode_t mode)
{
	return SYSCALL(chmod, (long)filename, mode);
}

LKLAPI long lkl_sys_newlstat(char *filename, struct __kernel_stat *statbuf)
{
	return SYSCALL(newlstat, (long)filename, (long)statbuf);
}

LKLAPI long lkl_sys_lstat64(char *filename, struct __kernel_stat64 *statbuf)
{
	return SYSCALL(lstat64, (long)filename, (long)statbuf);
}

LKLAPI long lkl_sys_mkdir(const char *pathname, int mode)
{
	return SYSCALL(mkdir, (long)pathname, mode);
}

LKLAPI long lkl_sys_rmdir(const char *pathname)
{
	return SYSCALL(rmdir, (long)pathname);
}

LKLAPI long lkl_sys_getdents(unsigned int fd, struct __kernel_dirent *dirent, unsigned int count)
{
	return SYSCALL(getdents, fd, (long)dirent, count);
}

LKLAPI long lkl_sys_newstat(char *filename, struct __kernel_stat *statbuf)
{
	return SYSCALL(newstat, (long)filename, (long)statbuf);
}

LKLAPI long lkl_sys_stat64(char *filename, struct __kernel_stat64 *statbuf)
{
	return SYSCALL(stat64, (long)filename, (long)statbuf);
}

LKLAPI long lkl_sys_utimes(const char *filename, struct __kernel_timeval *utimes)
{
	return SYSCALL(utime, (long)filename, (long)utimes);
}

LKLAPI long lkl_sys_mount(const char *dev, const char *mnt_point,
				const char *fs, unsigned long flags, void *data)
{
	return SYSCALL(safe_mount, (long)dev, (long)mnt_point, (long)fs, flags, (long)data);
}


LKLAPI long lkl_sys_chdir(const char *dir)
{
	return SYSCALL(chdir, (long)dir);
}


LKLAPI long lkl_sys_mknod(const char *filename, int mode, unsigned dev)
{
	return SYSCALL(mknod, (long)filename, mode, dev);
}


LKLAPI long lkl_sys_chroot(const char *dir)
{
	return SYSCALL(chroot, (long)dir);
}

LKLAPI long lkl_sys_nanosleep(struct __kernel_timespec *rqtp, struct __kernel_timespec *rmtp)
{
	return SYSCALL(nanosleep, (long)rqtp, (long)rmtp);
}

LKLAPI long lkl_sys_getcwd(char *buf, unsigned long size)
{
	return SYSCALL(getcwd, (long)buf, (long) size);
}

LKLAPI long lkl_sys_utime(const char *filename, const struct __kernel_utimbuf *buf)
{
        return SYSCALL(utime, (long)filename, (long)buf);
}

LKLAPI long lkl_sys_socket(int family, int type, int protocol)
{
	long args[6]={family, type, protocol};
	return SYSCALL(socketcall, SYS_SOCKET, (long)args);
}

LKLAPI long lkl_sys_send(int sock, void *buffer, size_t size, unsigned flags)
{
	long args[6]={sock, (long)buffer, size, flags};
	return SYSCALL(socketcall, SYS_SEND, (long)args);
}

LKLAPI long lkl_sys_recv(int sock, void *buffer, size_t size, unsigned flags)
{
	long args[6]={sock, (long)buffer, size, flags};
	return SYSCALL(socketcall, SYS_RECV, (long)args);
}


LKLAPI long lkl_sys_connect(int sock, struct sockaddr *saddr, int len)
{
	long args[6]={sock, (long)saddr, len};
	return SYSCALL(socketcall, SYS_CONNECT, (long)args);
}

LKLAPI long lkl_sys_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg)
{
	return SYSCALL(ioctl, fd, cmd, arg);
}

LKLAPI long lkl_sys_umask(int mask)
{
	return SYSCALL(umask, mask);
}

LKLAPI long lkl_sys_getuid(void)
{
	return SYSCALL(getuid);
}

LKLAPI long lkl_sys_getgid(void)
{
	return SYSCALL(getgid);
}

LKLAPI long lkl_sys_call(long f, long arg1, long arg2, long arg3, long arg4, long arg5)
{
	return SYSCALL(call, f, arg1, arg2, arg3, arg4, arg5);
}

LKLAPI long lkl_sys_statfs(const char *path, struct __kernel_statfs *buf)
{
	return SYSCALL(statfs, (long)path, (long)buf);
}

LKLAPI long lkl_sys_access(const char *filename, int mode)
{
	return SYSCALL(access, (long)filename, mode);
}

LKLAPI long lkl_sys_truncate(const char *path, unsigned long length)
{
	return SYSCALL(truncate, (long)path, length);
}

LKLAPI ssize_t lkl_sys_pwrite64(unsigned int fd, const char *buf, size_t count, loff_t pos)
{
	return SYSCALL(lkl_pwrite64, fd, (long)buf, count, (pos >> 32), (pos & 0xffffffff));
}

LKLAPI ssize_t lkl_sys_pread64(unsigned int fd, char *buf, size_t count, loff_t pos)
{
	return SYSCALL(lkl_pread64, fd, (long)buf, count, (pos >> 32), (pos & 0xffffffff));
}

LKLAPI long lkl_sys_getpid(void)
{
	return SYSCALL(getpid);
}

LKLAPI long lkl_sys_bind(int sock, struct sockaddr *saddr, int len)
{
	long args[6]={sock, (long)saddr, len};
	return SYSCALL(socketcall, SYS_BIND, (long)args);
}


/* 
 * Halt is special as we want to call syscall_done after the kernel has been
 * halted, in linux_start_kernel. 
 */
void *halt_sem;

LKLAPI long lkl_sys_halt(void)
{
	int err;

	if (!(halt_sem=lkl_nops->sem_alloc(0)))
		return -ENOMEM;
	err=SYSCALL_VIA_QUEUE(default_queue, reboot, 0, 0, 0, 0, 0, 0);
	if (err < 0)
		return err;
	lkl_nops->sem_down(halt_sem);
	lkl_nops->sem_free(halt_sem);
	return 0;
}

/* this is not used! syscalls are run in the '/init' kthread. */
LKLAPI int lkl_syscall_thread_init(void)
{
	void *thread;
	int err;
	struct syscall_queue *sq;

	if (!lkl_nops->thread_id)
		return -EINVAL;

	thread=lkl_nops->thread_id();

	if (get_syscall_queue(thread) != default_queue) 
		return -EBUSY;

	if (!(sq=new_syscall_queue(thread)))
		return -ENOMEM;

	err=SYSCALL_VIA_WORKQUEUE(call, (long)kernel_thread,
				  (long)run_syscalls, (long)sq, 0, 0);
	if (err < 0) {
		del_syscall_queue(sq);
		return err;
	}

	return 0;
}

/* this is not used! syscalls are run in the '/init' kthread. */
LKLAPI int lkl_syscall_thread_cleanup(void)
{
	struct syscall_queue *sq;
	void *thread;

	if (!lkl_nops->thread_id)
		return -EINVAL;

	thread=lkl_nops->thread_id();

	if ((sq=get_syscall_queue(thread)) == default_queue) 
		return -EINVAL;

	return SYSCALL_VIA_QUEUE(sq, reboot, 0, 0, 0, 0, 0, 0);
}


int __init syscall_init(void)
{
	init_syscall_table();
	BUG_ON((syscall_queue_hash_lock=lkl_nops->sem_alloc(1)) == NULL);
	setup_irq(SYSCALL_IRQ, &syscall_irqaction);
	BUG_ON((default_queue=new_syscall_queue(NULL)) == NULL);
	printk(KERN_INFO "lkl: syscall interface initialized\n");
	return 0;
}

late_initcall(syscall_init);

void free_syscall(void)
{
	lkl_nops->sem_free(syscall_queue_hash_lock);
}
