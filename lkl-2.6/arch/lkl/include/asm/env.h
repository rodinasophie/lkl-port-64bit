#ifndef _LKL_ENV_H
#define _LKL_ENV_H

#include <asm/lkl.h>

LKLAPI int lkl_env_init(unsigned long mem_size);
LKLAPI int lkl_env_fini();

LKLAPI long lkl_mount(char *dev, char *mnt, int flags, void *data);
LKLAPI long lkl_mount_dev(__kernel_dev_t dev, char *fs_type, int flags, void *data,
		   char *mnt_str, int mnt_str_len);
LKLAPI long lkl_umount_dev(__kernel_dev_t dev, int flags);


LKLAPI int lkl_if_up(int ifindex);
LKLAPI int lkl_if_down(int ifindex);
LKLAPI int lkl_if_set_ipv4(int ifindex, unsigned int addr, unsigned int netmask_len);
LKLAPI int lkl_set_gateway(unsigned int addr);

LKLAPI int lkl_printf(const char * fmt, ...) __attribute__ ((format (printf, 1, 2)));


#endif
