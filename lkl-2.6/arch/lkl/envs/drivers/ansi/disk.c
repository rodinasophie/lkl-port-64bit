#include <stdio.h>
#include <assert.h>

#include <asm/disk_portable.h>

/*
 * FIXME: This ANSI version works with < 2GB files. Get rid of it and create
 * proper versions for POSIX and NT.
 */
void lkl_disk_do_rw(void *data, unsigned long sector, unsigned long nsect,
		   char *buffer, int dir, struct lkl_disk_cs *cs)
{
	int err;
	FILE *f=(FILE*)data;

	cs->sync=1;
	if (fseek(f, sector * 512, SEEK_SET) != 0) {
		cs->error=LKL_DISK_CS_ERROR;
		return;
	}

        if (dir)
                err=fwrite(buffer, 512, nsect, f);
        else
                err=fread(buffer, 512, nsect, f);

	if (err != nsect) 
		cs->error=LKL_DISK_CS_ERROR;
	else
		cs->error=LKL_DISK_CS_SUCCESS;

	return;
}

