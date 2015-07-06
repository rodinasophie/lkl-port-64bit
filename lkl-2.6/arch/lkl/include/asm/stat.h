#ifndef _ASM_LKL_STAT_H
#define _ASM_LKL_STAT_H

#ifdef __KERNEL__
#define __kernel_stat stat
#define __kernel_stat64 stat64
#endif

struct __kernel_stat {
	unsigned long  st_dev;
	unsigned long  st_ino;
	unsigned short st_mode;
	unsigned short st_nlink;
	unsigned short st_uid;
	unsigned short st_gid;
	unsigned long  st_rdev;
	unsigned long  st_size;
	unsigned long  st_blksize;
	unsigned long  st_blocks;
	unsigned long  st_atime;
	unsigned long  st_atime_nsec;
	unsigned long  st_mtime;
	unsigned long  st_mtime_nsec;
	unsigned long  st_ctime;
	unsigned long  st_ctime_nsec;
	unsigned long  __unused4;
	unsigned long  __unused5;
};

/* This matches struct stat64 in glibc2.1, hence the absolutely
 * insane amounts of padding around dev_t's.
 */
struct __kernel_stat64 {
	unsigned long long	st_dev;
	unsigned int	st_mode;
	unsigned int	st_nlink;

	unsigned long	st_uid;
	unsigned long	st_gid;

	unsigned long long	st_rdev;

	long long	st_size;
	unsigned long	st_blksize;

	unsigned long long	st_blocks;	/* Number 512-byte blocks allocated. */

	unsigned long	st_atime;
	unsigned long	st_atime_nsec;

	unsigned long	st_mtime;
	unsigned int	st_mtime_nsec;

	unsigned long	st_ctime;
	unsigned long	st_ctime_nsec;

	unsigned long long	st_ino;
};

#define STAT_HAVE_NSEC 1

#undef __kernel_stat64
#undef __kernel_stat

#endif

