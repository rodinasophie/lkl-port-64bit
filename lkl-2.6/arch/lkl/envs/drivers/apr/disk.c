#include <apr_file_io.h>
#include <asm/disk_portable.h>

void lkl_disk_do_rw(void *_file, unsigned long sector, unsigned long nsect,
		   char *buffer, int dir, struct lkl_disk_cs *cs)
{
	apr_off_t offset=(unsigned long long)sector*512;
	apr_size_t len=nsect*512;
	apr_file_t *file=(apr_file_t*)_file;
	apr_status_t status;

	cs->sync=1;

	if (apr_file_seek(file, APR_SET, &offset) != APR_SUCCESS) {
		cs->error=LKL_DISK_CS_ERROR;
		return;
	}

	if (dir) 
		status=apr_file_write_full(file, buffer, len, NULL);
	else
		status=apr_file_read_full(file, buffer, len, NULL);

	if (status == APR_SUCCESS)
		cs->error=LKL_DISK_CS_SUCCESS;
	else
		cs->error=LKL_DISK_CS_ERROR;
	
}



