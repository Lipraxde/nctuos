#ifndef USR_SYSCALL_H
#define USR_SYSCALL_H
#include <inc/types.h>

/* system call numbers */
enum {
	SYS_puts = 0,
	SYS_getc,
	SYS_getpid,
	SYS_getcid,
	SYS_fork,
	SYS_kill,
	SYS_sleep,
	SYS_get_num_used_page,
	SYS_get_num_free_page,
	SYS_get_ticks,
	SYS_settextcolor,
	SYS_cls,
	SYS_open,
	SYS_close,
	SYS_read,
	SYS_write,
	SYS_lseek,
	SYS_unlink,
	SYS_readdir,
	NSYSCALLS
};


void puts(const char *s, size_t len);
int getc(void);
int32_t getpid(void);
int32_t getcid(void);
int32_t fork(void);
int32_t kill(uint32_t pid);
uint32_t get_num_used_page(void);
uint32_t get_num_free_page(void);
unsigned long get_ticks(void);
uint32_t sleep(uint32_t ticks);
void settextcolor(unsigned char forecolor, unsigned char backcolor);
int32_t cls(void);

/*********** Lab7 ************/
int open(const char *file, int flags, int mode);
int close(int d);
int read(int fd, void *buf, size_t len);
int write(int fd, const void *buf, size_t len);
off_t lseek(int fd, off_t offset, int whence);
int unlink(const char *pathname);
int readdir(const char *pathname);

#endif
