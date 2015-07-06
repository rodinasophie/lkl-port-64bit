#include "live_os.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

int live_lib_open(const char *image, int mode) {
	if (mode)
		return open(image, O_RDWR | O_CREAT, 0777);
	else
		return open(image, O_RDWR, 0777);
}

void live_lib_close(int fd) {
	close(fd);
}


long unsigned int live_lib_get_size(int fd) {
	struct stat buf;
	fstat(fd, &buf);
	return buf.st_size;
}

int live_lib_read(int file_fd, char *file_buf, long unsigned size) {
	return read(file_fd, file_buf, size);
}
