#include "live_os.h"
#include <stdlib.h>
#include <stdio.h>
#include <asm/lkl.h>
#include <asm/env.h>
#include <asm/disk.h>

static __kernel_dev_t devno;
static int fd;

// Posix implementation, config kernel as POSIX_ENV
int lkl_add_disk(const char *image) {
	fd = live_lib_open(image, 0);
	if (fd < 0) {
		printf("Cannot open\n");
		return -1;
	}
	//FILE *f = fdopen(fd, "r+");
	size_t size = live_lib_get_size(fd);
	devno = lkl_disk_add_disk(&fd, size / 512);
	if (devno == 0) {
		live_lib_close(fd);
		return -1;
	}
	return 0;
}

// TODO: add to arguments fs_type and mount_point
int main(int argc, char *argv[]) {
	// Input: exec.out image file_to_mount
	if (argc < 3) {
		printf("Incorrect input: ./exec.out image file_to_add\n");
		return -1;
	}
	if (lkl_env_init(16*1024*1024) != 0) {
		return -1;
	}
	// argv[1] - image
	if (lkl_add_disk(argv[1]) < 0) {
		printf("Cannot add disk to lkl");
		return -1;
	}
	char *fs_type = NULL;
	char mount_point[32];
		int err;
	if (((err = lkl_mount_dev(devno, fs_type, 0, NULL, mount_point, sizeof(mount_point))) < 0) ||
			(lkl_sys_chdir(mount_point) < 0) ||
			(lkl_sys_chroot(".") < 0)) {
		live_lib_close(fd);
		lkl_sys_halt();
		return -1;
	}
	printf("Disk mounted to lkl successfully\n");
	// argv[2] - file
	int file_fd = live_lib_open(argv[2], 1);
	if (file_fd < 0) {
		lkl_sys_halt();
		live_lib_close(fd);
		return -1;
	}
	size_t size = live_lib_get_size(file_fd);
	char *file_buf = (char *)malloc(size * sizeof(char) + 1);
	live_lib_read(file_fd, file_buf, size);
	int lkl_fd = lkl_sys_open(argv[2], O_CREAT | O_RDWR, 0777);
	if (lkl_fd < 0) {
		lkl_sys_halt();
		live_lib_close(fd);
		live_lib_close(file_fd);
		return -1;
	}
	lkl_sys_write(lkl_fd, file_buf, size);
	lkl_sys_close(lkl_fd);
	lkl_sys_umount("/", 0);
	live_lib_close(fd);
	live_lib_close(file_fd);
	free(file_buf);
	return 0;
}
