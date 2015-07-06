#ifndef LKL_ASM_DISK_PORTABLE__
#define LKL_ASM_DISK_PORTABLE__

#define LKL_DISK_CS_SUCCESS 0
#define LKL_DISK_CS_ERROR   1

struct lkl_disk_cs {
	void *linux_cookie;
	int error, sync;
};
void lkl_disk_do_rw(void *f, unsigned long sector, unsigned long nsect,
		    char *buffer, int dir, struct lkl_disk_cs *cs);

#endif//LKL_ASM_DISK_PORTABLE__
