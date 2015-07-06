
#include <KernelExport.h>
#include <asm/disk_portable.h>

void lkl_disk_do_rw(void *data, unsigned long sector, unsigned long nsect,
		   char *buffer, int dir, struct lkl_disk_cs *cs)
{
	unsigned long len = sector * nsect;
	int rc;
	int fd = (int) data;
	off_t off = sector * 512;

	cs->sync = 1;

	if (dir)
		rc = _kern_write(fd, off, buffer, 512 * nsect);
	else
		rc = _kern_read(fd, off, buffer, 512 * nsect);

	if (rc < B_OK)
		dprintf("[lkl-disk-driver]: %s returned %d, [%s]\n",
				(dir) ? "_kern_write" : "_kern_read",
				rc, strerror(rc));

	cs->error = (rc < 0) ? LKL_DISK_CS_ERROR : LKL_DISK_CS_SUCCESS;
}

