#ifndef __LIVE_OS_H_
#define __LIVE_OS_H_

int live_lib_open(const char *image, int mode);
long unsigned int live_lib_get_size(int fd);
int live_lib_read(int file_fd, char *file_buf, long unsigned size);
void live_lib_close(int fd);
#endif
