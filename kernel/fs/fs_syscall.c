/* This file use for NCTU OSDI course */


// It's handel the file system APIs 
#include <inc/stdio.h>
#include <inc/syscall.h>
#include "fs.h"

/*TODO: Lab7, file I/O system call interface.*/
/*Note: Here you need handle the file system call from user.
 *       1. When user open a new file, you can use the fd_new() to alloc a file object(struct fs_fd)
 *       2. When user R/W or seek the file, use the fd_get() to get file object.
 *       3. After get file object call file_* functions into VFS level
 *       4. Update the file objet's position or size when user R/W or seek the file.(You can find the useful marco in ff.h)
 *       5. Remember to use fd_put() to put file object back after user R/W, seek or close the file.
 *       6. Handle the error code, for example, if user call open() but no fd slot can be use, sys_open should return -STATUS_ENOSPC.
 *
 *  Call flow example:
 *        ┌──────────────┐
 *        │     open     │
 *        └──────────────┘
 *               ↓
 *        ╔══════════════╗
 *   ==>  ║   sys_open   ║  file I/O system call interface
 *        ╚══════════════╝
 *               ↓
 *        ┌──────────────┐
 *        │  file_open   │  VFS level file API
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │   fat_open   │  fat level file operator
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │    f_open    │  FAT File System Module
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │    diskio    │  low level file operator
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │     disk     │  simple ATA disk dirver
 *        └──────────────┘
 */

// Below is POSIX like I/O system call 
int sys_open(const char *file, int flags, int mode)
{
	//We dont care the mode.
	int fd = fd_new();
	if (fd == -1)
		return -STATUS_ENOSPC;

	struct fs_fd *p = fd_get(fd);
	int err = file_open(p, file, flags);

	fd_put(p);
	if (err < 0) {
		fd_put(p); // clean fd
		return err;
	}
	return fd;
}

int sys_close(int fd)
{
	struct fs_fd *p = fd_get(fd);
	if (!p)
		return -STATUS_EINVAL;
	int err = file_close(p);

	fd_put(p);
	if (err < 0)
		return err;
	fd_put(p);
	return 0;
}

int sys_read(int fd, void *buf, size_t len)
{
	struct fs_fd *p = fd_get(fd);
	if (!p)
		return -STATUS_EBADF;
	if (!buf || len <= 0)
		return -STATUS_EINVAL;
	if (len > p->size)
		len = p->size;
	int ret = file_read(p, buf, len);
	fd_put(p);
	return ret;
}

int sys_write(int fd, const void *buf, size_t len)
{
	struct fs_fd *p = fd_get(fd);
	if (!p)
		return -STATUS_EBADF;
        if (!buf || len <= 0)
                return -STATUS_EINVAL;
	int ret = file_write(p, buf, len);
	fd_put(p);
	return ret;
}

/* Note: Check the whence parameter and calcuate the new offset value before do file_seek() */
off_t sys_lseek(int fd, off_t offset, int whence)
{
	struct fs_fd *p = fd_get(fd);
	if (!p)
		return -STATUS_EBADF;
	if (whence == SEEK_END)
		offset += p->size;
	else if (whence == SEEK_CUR)
		offset += p->pos;
	else if (whence != SEEK_SET)
		return -STATUS_EINVAL;
	int err = file_lseek(p, offset);
	fd_put(p);
	if (err < 0)
		return err;
	else
		return offset;
}

int sys_unlink(const char *pathname)
{
	return file_unlink(pathname);
}

int sys_readdir(const char *pathname)
{
	return file_readdir(pathname);
}
