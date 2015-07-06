#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <asm/disk_portable.h>

/*
 * FIXME: This ANSI version works with < 2GB files. Get rid of it and create
 * proper versions for POSIX and NT.
 */
void lkl_disk_do_rw(void *data, unsigned long sector, unsigned long nsect,
		   char *buffer, int dir, struct lkl_disk_cs *cs)
{
	int err;
	int *fd = (int *)data;
	cs->sync=1;

	if (lseek(*fd, 512*sector, SEEK_SET) < 0) {
		cs->error=LKL_DISK_CS_ERROR;
		return;
	}

  if (dir) {
		err = write(*fd, buffer, 512*nsect);
	} else {
		err = read(*fd, buffer, 512*nsect);
	}

	if (err != (nsect * 512))
		cs->error=LKL_DISK_CS_ERROR;
	else
		cs->error=LKL_DISK_CS_SUCCESS;

	return;
}

