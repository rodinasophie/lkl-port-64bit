#ifndef _LKL_LKL_H
#define _LKL_LKL_H

#define __KERNEL_STRICT_NAMES

#include <linux/compiler.h>
#include <linux/posix_types.h>
#include <asm/types.h>
#include <asm/stat.h>

/* Microsoft compilers need to decorate exported/imported functions */
#ifdef _MSC_VER
/* TODO: add switch to add __declspec(dllexport) or __declspec(dllimport) */
# define LKLAPI __stdcall
#else
# define LKLAPI
#endif

/* horible hack, but i'll have it over hacks in apps */
#ifdef __GLIBC__
# define __LKL_GLIBC__ __GLIBC__
# undef __GLIBC__
#endif

#include <linux/stat.h>

#ifdef __LKL_GLIBC__
# define __GLIBC__ __LKL_GLIBC__
# undef __LKL_GLIBC__
#endif

#include <asm/statfs.h>
#include <linux/fcntl.h>

struct sockaddr;
struct pollfd;
struct __kernel_stat;
struct __kernel_statfs;
struct __kernel_stat64;

/*
 * FIXME: these structs are duplicated from within the kernel. Find a way
 * to remove duplicates. (maybe define them in arch??)
 *
 * For now, these must be kept in sync with their Linux counterparts.
 */
struct __kernel_timespec {
	__kernel_time_t	tv_sec;
	long tv_nsec;
};
struct __kernel_timeval {
	__kernel_time_t tv_sec;
	__kernel_suseconds_t tv_usec;
};

struct __kernel_timezone {
	int	tz_minuteswest;
	int	tz_dsttime;
};

struct __kernel_utimbuf {
	__kernel_time_t actime;
	__kernel_time_t modtime;
};

#ifndef __KERNEL__
typedef __u32 __kernel_dev_t;
#endif

struct __kernel_dirent {
	unsigned long	d_ino;
	unsigned long	d_off;
	unsigned short	d_reclen;
	char		d_name[1];
};

LKLAPI long lkl_sys_sync(void);
LKLAPI long lkl_sys_reboot(int magic1, int magic2, unsigned int cmd,  void *arg);
LKLAPI __kernel_ssize_t lkl_sys_write(unsigned int fd, const char *buf, __kernel_size_t count);
LKLAPI long lkl_sys_close(unsigned int fd);
LKLAPI long lkl_sys_unlink(const char *pathname);
LKLAPI long lkl_sys_open(const char *filename, int flags, int mode);
LKLAPI long lkl_sys_poll(struct pollfd *ufds, unsigned int nfds, long timeout);
LKLAPI __kernel_ssize_t lkl_sys_read(unsigned int fd, char *buf,  __kernel_size_t count);
LKLAPI __kernel_off_t lkl_sys_lseek(unsigned int fd, __kernel_off_t offset,  unsigned int origin);
LKLAPI long lkl_sys_rename(const char *oldname,  const char *newname);
LKLAPI long lkl_sys_flock(unsigned int fd, unsigned int cmd);
LKLAPI long lkl_sys_newfstat(unsigned int fd, struct __kernel_stat *statbuf);
LKLAPI long lkl_sys_chmod(const char *filename, __kernel_mode_t mode);
LKLAPI long lkl_sys_newlstat(char *filename, struct __kernel_stat *statbuf);
LKLAPI long lkl_sys_lstat64(char *filename, struct __kernel_stat64 *statbuf);
LKLAPI long lkl_sys_mkdir(const char *pathname, int mode);
LKLAPI long lkl_sys_rmdir(const char *pathname);
LKLAPI long lkl_sys_getdents(unsigned int fd, struct __kernel_dirent *dirent, unsigned int count);
LKLAPI long lkl_sys_newfstat(unsigned int fd, struct __kernel_stat *statbuf);
LKLAPI long lkl_sys_fstat64(unsigned int fd, struct __kernel_stat64 *statbuf);
LKLAPI long lkl_sys_newstat(char *filename, struct __kernel_stat *statbuf);
LKLAPI long lkl_sys_stat64(char *filename, struct __kernel_stat64 *statbuf);
LKLAPI long lkl_sys_utimes(const char *filename, struct __kernel_timeval *utimes);
LKLAPI long lkl_sys_nanosleep(struct __kernel_timespec *rqtp, struct __kernel_timespec *rmtp);
LKLAPI long lkl_sys_mknod(const char *filename, int mode, unsigned dev);
LKLAPI long lkl_sys_umount(const char *name, int flags);
LKLAPI long lkl_sys_chdir(const char *filename);
LKLAPI long lkl_sys_statfs(const char * path, struct __kernel_statfs *buf);
LKLAPI long lkl_sys_chroot(const char *filename);
LKLAPI long lkl_sys_mount(const char *dev_name, const char *dir_name,
		    const char *type, unsigned long flags, void *data);
LKLAPI long lkl_sys_halt(void);
LKLAPI long lkl_sys_getcwd(char *buf, unsigned long size);
LKLAPI long lkl_sys_utime(const char *filename, const struct __kernel_utimbuf *buf);
LKLAPI long lkl_sys_socket(int family, int type, int protocol);
LKLAPI long lkl_sys_send(int sock, void *buffer, __kernel_size_t size, unsigned flags);
LKLAPI long lkl_sys_recv(int sock, void *buffer, __kernel_size_t size, unsigned flags);
LKLAPI long lkl_sys_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg);
LKLAPI long lkl_sys_connect(int sock, struct sockaddr *saddr, int len);
LKLAPI long lkl_sys_umask(int mask);
LKLAPI long lkl_sys_getuid(void);
LKLAPI long lkl_sys_getgid(void);
LKLAPI long lkl_sys_call(long f, long arg1, long arg2, long arg3, long arg4, long arg5);
LKLAPI long lkl_sys_access(const char *filename, int mode);
LKLAPI long lkl_sys_truncate(const char *path, unsigned long length);
LKLAPI __kernel_ssize_t lkl_sys_pwrite64(unsigned int fd, const char *buf,
				  __kernel_size_t count, __kernel_loff_t pos);
LKLAPI __kernel_ssize_t lkl_sys_pread64(unsigned int fd, char *buf,
				 __kernel_size_t count, __kernel_loff_t pos);
LKLAPI long lkl_sys_getpid(void);
LKLAPI long lkl_sys_bind(int sock, struct sockaddr *saddr, int len);


int sprintf(char * buf, const char * fmt,
	    ...) __attribute__ ((format (printf, 2, 3)));
int snprintf(char * buf, __kernel_size_t size, const char * fmt,
	     ...) __attribute__ ((format (printf, 3, 4)));
int sscanf(const char *, const char *,
	   ...) __attribute__ ((format (scanf, 2, 3)));


/* GCC implements strchr/strlen as preprocessor macros.
 *
 * If we're on GCC but don't see strchr/strlen preprocessor symbols,
 * we add regular declarations for them.
 */
#if defined(__GNUC__)
# ifndef stdchr
  extern char * strchr(const char *, int);
# endif /* strchr */

# ifndef strlen
  extern __kernel_size_t strlen(const char *);
# endif /* strlen */
#endif


/*
 * All subsequent system calls from the current native thread will be executed
 * in a newly created Linux system call thread. System calls are still
 * serialized during execution, but this allows running other system calls while
 * one is blocked, if the application is multithreaded.
 */
LKLAPI int lkl_syscall_thread_init(void);

/*
 * After this point, all system calls issued by this thread will be executed by
 * the default Linux system call thread, as before calling
 * lkl_syscall_thread_init().
 */
LKLAPI int lkl_syscall_thread_cleanup(void);

#endif
