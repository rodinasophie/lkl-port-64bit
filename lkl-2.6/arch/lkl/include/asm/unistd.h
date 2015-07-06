#ifndef _ASM_LKL_UNISTD_H
#define _ASM_LKL_UNISTD_H



#define __NR_ni_syscall 0
#define __NR_sync 1
#define __NR_reboot 2
#define __NR_write 3
#define __NR_close 4
#define __NR_unlink 5
#define __NR_open 6
#define __NR_poll 7
#define __NR_read 8
#define __NR_lseek 9
#define __NR_rename 10
#define __NR_flock 11
#define __NR_newfstat 12
#define __NR_chmod 13
#define __NR_newlstat 14
#define __NR_mkdir 15
#define __NR_rmdir 16
#define __NR_getdents 17
#define __NR_newstat 18
#define __NR_utimes 19
#define __NR_utime 20
#define __NR_nanosleep 21
#define __NR_mknod 22
#define __NR_safe_mount 23
#define __NR_umount 24
#define __NR_chdir 25
#define __NR_statfs 26
#define __NR_chroot 27
#define __NR_getcwd 28
#define __NR_chown 29
#define __NR_umask 30
#define __NR_getuid 31
#define __NR_getgid 32
#define __NR_socketcall 33
#define __NR_ioctl 34
#define __NR_call 35
#define __NR_access 36
#define __NR_truncate 37
#define __NR_pread64 38
#define __NR_lkl_pread64 38
#define __NR_pwrite64 39
#define __NR_lkl_pwrite64 39
#define __NR_getpid 40
#define NR_syscalls 41


/* TODO */
#define __NR_restart_syscall 0
#define __NR_exit 0
#define __NR_fork 0
#define __NR_creat 0
#define __NR_link 0
#define __NR_execve 0
#define __NR_time 0
#define __NR_lchown 0
#define __NR_mount 0
#define __NR_setuid 0
#define __NR_ptrace 0
#define __NR_alarm 0
#define __NR_pause 0
#define __NR_kill 0
#define __NR_dup 0
#define __NR_pipe 0
#define __NR_times 0
#define __NR_brk 0
#define __NR_setgid 0
#define __NR_geteuid 0
#define __NR_getegid 0
#define __NR_acct 0
#define __NR_fcntl 0
#define __NR_setpgid 0
#define __NR_ustat 0
#define __NR_dup2 0
#define __NR_getppid 0
#define __NR_getpgrp 0
#define __NR_setsid 0
#define __NR_setreuid 0
#define __NR_setregid 0
#define __NR_sethostname 0
#define __NR_setrlimit 0
#define __NR_getrlimit 0
#define __NR_getrusage 0
#define __NR_gettimeofday 0
#define __NR_settimeofday 0
#define __NR_getgroups 0
#define __NR_setgroups 0
#define __NR_select 0
#define __NR_symlink 0
#define __NR_readlink 0
#define __NR_uselib 0
#define __NR_swapon 0
#define __NR_mmap 0
#define __NR_munmap 0
#define __NR_ftruncate 0
#define __NR_fchmod 0
#define __NR_fchown 0
#define __NR_getpriority 0
#define __NR_setpriority 0
#define __NR_fstatfs 0
#define __NR_syslog 0
#define __NR_setitimer 0
#define __NR_getitimer 0
#define __NR_stat 0
#define __NR_lstat 0
#define __NR_fstat 0
#define __NR_vhangup 0
#define __NR_wait4 0
#define __NR_swapoff 0
#define __NR_sysinfo 0
#define __NR_fsync 0
#define __NR_clone 0
#define __NR_setdomainname 0
#define __NR_uname 0
#define __NR_adjtimex 0
#define __NR_mprotect 0
#define __NR_init_module 0
#define __NR_quotactl 0
#define __NR_getpgid 0
#define __NR_fchdir 0
#define __NR_sysfs 0
#define __NR_personality 0
#define __NR_setfsuid 0
#define __NR_setfsgid 0
#define __NR_msync 0
#define __NR_readv 0
#define __NR_writev 0
#define __NR_getsid 0
#define __NR_fdatasync 0
#define __NR__sysctl 0
#define __NR_mlock 0
#define __NR_munlock 0
#define __NR_mlockall 0
#define __NR_munlockall 0
#define __NR_sched_setparam 0
#define __NR_sched_getparam 0
#define __NR_sched_setscheduler 0
#define __NR_sched_getscheduler 0
#define __NR_sched_yield 0
#define __NR_sched_get_priority_max 0
#define __NR_sched_get_priority_min 0
#define __NR_sched_rr_get_interval 0
#define __NR_mremap 0
#define __NR_setresuid 0
#define __NR_getresuid 0
#define __NR_nfsservctl 0
#define __NR_setresgid 0
#define __NR_getresgid 0
#define __NR_prctl 0
#define __NR_rt_sigreturn 0
#define __NR_rt_sigaction 0
#define __NR_rt_sigprocmask 0
#define __NR_rt_sigpending 0
#define __NR_rt_sigtimedwait 0
#define __NR_rt_sigqueueinfo 0
#define __NR_rt_sigsuspend 0
#define __NR_capget 0
#define __NR_capset 0
#define __NR_sigaltstack 0
#define __NR_sendfile 0
#define __NR_vfork 0
#define __NR_truncate64 0
#define __NR_ftruncate64 0
#define __NR_stat64 0
#define __NR_lstat64 0
#define __NR_fstat64 0
#define __NR_pivot_root 0
#define __NR_mincore 0
#define __NR_madvise 0
#define __NR_getdents64 0
#define __NR_fcntl64 0
#define __NR_gettid 0
#define __NR_readahead 0
#define __NR_setxattr 0
#define __NR_lsetxattr 0
#define __NR_fsetxattr 0
#define __NR_getxattr 0
#define __NR_lgetxattr 0
#define __NR_fgetxattr 0
#define __NR_listxattr 0
#define __NR_llistxattr 0
#define __NR_flistxattr 0
#define __NR_removexattr 0
#define __NR_lremovexattr 0
#define __NR_fremovexattr 0
#define __NR_tkill 0
#define __NR_sendfile64 0
#define __NR_futex 0
#define __NR_sched_setaffinity 0
#define __NR_sched_getaffinity 0
#define __NR_io_setup 0
#define __NR_io_destroy 0
#define __NR_io_getevents 0
#define __NR_io_submit 0
#define __NR_io_cancel 0
#define __NR_fadvise64 0
#define __NR_exit_group 0
#define __NR_lookup_dcookie 0
#define __NR_epoll_create 0
#define __NR_epoll_ctl 0
#define __NR_epoll_wait 0
#define __NR_remap_file_pages 0
#define __NR_set_tid_address 0
#define __NR_timer_create 0
#define __NR_timer_settime 0
#define __NR_timer_gettime 0
#define __NR_timer_getoverrun 0
#define __NR_timer_delete 0
#define __NR_clock_settime 0
#define __NR_clock_gettime 0
#define __NR_clock_getres 0
#define __NR_clock_nanosleep 0
#define __NR_statfs64 0
#define __NR_fstatfs64 0
#define __NR_tgkill 0
#define __NR_fadvise64_64 0
#define __NR_mbind 0
#define __NR_get_mempolicy 0
#define __NR_set_mempolicy 0
#define __NR_mq_open 0
#define __NR_mq_unlink 0
#define __NR_mq_timedsend 0
#define __NR_mq_timedreceive 0
#define __NR_mq_0
#define __NR_mq_getsetattr 0
#define __NR_kexec_load 0
#define __NR_waitid 0
#define __NR_add_key 0
#define __NR_request_key 0
#define __NR_keyctl 0
#define __NR_ioprio_set 0
#define __NR_ioprio_get 0
#define __NR_migrate_pages 0
#define __NR_openat 0
#define __NR_mkdirat 0
#define __NR_mknodat 0
#define __NR_fchownat 0
#define __NR_futimesat 0
#define __NR_fstatat64 0
#define __NR_unlinkat 0
#define __NR_renameat 0
#define __NR_linkat 0
#define __NR_symlinkat 0
#define __NR_readlinkat 0
#define __NR_fchmodat 0
#define __NR_faccessat 0
#define __NR_pselect6 0
#define __NR_ppoll 0
#define __NR_unshare 0
#define __NR_set_robust_list 0
#define __NR_get_robust_list 0
#define __NR_splice 0
#define __NR_sync_file_range 0
#define __NR_tee 0
#define __NR_vmsplice 0
#define __NR_move_pages 0
#define __NR_getcpu 0
#define __NR_epoll_pwait 0
#define __NR_utimensat 0
#define __NR_signalfd 0
#define __NR_timerfd 0
#define __NR_eventfd 0
#define __NR_mq_notify 0
#define __NR_inotify_init 0
#define __NR_inotify_add_watch 0
#define __NR_inotify_rm_watch 0


#define __ARCH_WANT_SYS_UTIME
#define __ARCH_WANT_SYS_SOCKETCALL
#define __ARCH_WANT_STAT64
#define __ARCH_WANT_SYS_FADVISE64

//FIXME
#define cond_syscall(x)

#endif
