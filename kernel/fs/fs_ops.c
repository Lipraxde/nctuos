#include <inc/stdio.h>
#include "fs.h"
#include "fat/ff.h"
#include "fat/diskio.h"

extern struct fs_dev fat_fs;

/*TODO: Lab7, fat level file operator.
 *       Implement below functions to support basic file system operators by using the elmfat's API(f_xxx).
 *       Reference: http://elm-chan.org/fsw/ff/00index_e.html (or under doc directory (doc/00index_e.html))
 *
 *  Call flow example:
 *        ┌──────────────┐
 *        │     open     │
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │   sys_open   │  file I/O system call interface
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │  file_open   │  VFS level file API
 *        └──────────────┘
 *               ↓
 *        ╔══════════════╗
 *   ==>  ║   fat_open   ║  fat level file operator
 *        ╚══════════════╝
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

static int fr2err(int fr) {
	switch (fr) {
	case FR_OK:
		return 0;
	case FR_DISK_ERR:
		return -STATUS_EIO;
	case FR_INT_ERR:
		return -1;
	case FR_NOT_READY:
		return -STATUS_EAGIAN;
	case FR_NO_FILE:
		return -STATUS_ENOENT;
	case FR_NO_PATH:
		return -STATUS_ENOENT;
	case FR_INVALID_NAME:
		return -STATUS_EINVAL;
	case FR_DENIED:
		return -1;
	case FR_EXIST:
		return -STATUS_EEXIST;
	case FR_INVALID_OBJECT:
	case FR_WRITE_PROTECTED:
	case FR_INVALID_DRIVE:
	case FR_NOT_ENABLED:
	case FR_NO_FILESYSTEM:
	case FR_MKFS_ABORTED:
	case FR_TIMEOUT:
	case FR_LOCKED:
	case FR_NOT_ENOUGH_CORE:
	case FR_TOO_MANY_OPEN_FILES:
		return -1;
	case FR_INVALID_PARAMETER:
		return -STATUS_EINVAL;
	}
}

static void update_size(struct fs_fd *file) {
	file->size = f_size((FIL *)file->data);
}

static void update_pos(struct fs_fd *file) {
	file->pos = f_tell((FIL *)file->data);
}

/* Note: 1. Get FATFS object from fs->data
*        2. Check fs->path parameter then call f_mount.
*/
int fat_mount(struct fs_dev *fs, const void* data)
{
	FATFS *fat =(FATFS*) fs->data;
	return fr2err(f_mount(fat, fs->path, 1));
}

/* Note: Just call f_mkfs at root path '/' */
int fat_mkfs(const char* device_name)
{
	return  fr2err(f_mkfs("/", 0, 0));
}

/* Note: Convert the POSIX's open flag to elmfat's flag.
*        Example: if file->flags == O_RDONLY then open_mode = FA_READ
*                 if file->flags & O_APPEND then f_seek the file to end after f_open
*/
int fat_open(struct fs_fd* file)
{
	FIL *fp = (FIL*)file->data;
	int open_mode = FA_OPEN_EXISTING;

	if (file->flags == O_RDONLY)
		open_mode |= FA_READ;
	if (file->flags & O_WRONLY)
		open_mode |= FA_WRITE;
	if (file->flags & O_RDWR)
		open_mode |= ( FA_READ | FA_WRITE);
	if (file->flags & O_CREAT) {
		open_mode |= FA_OPEN_ALWAYS;
		if (file->flags & O_TRUNC && open_mode & FA_WRITE)
			open_mode |= FA_CREATE_ALWAYS;
		else
			open_mode |= FA_CREATE_NEW;
		// XXX: How to ensure file has been create?
		// if (file->flags & O_EXCL)
	}
	
	int fr = f_open(fp, file->path, open_mode);
	if (fr != FR_OK)
		return fr2err(fr);

	update_size(file);

	if (file->flags & O_APPEND)
		return fr2err(f_lseek(fp, file->size));

	update_pos(file);

	return 0;
}

int fat_close(struct fs_fd* file)
{
        FIL *fp = (FIL*)file->data;
	return fr2err(f_close(fp));
}

int fat_read(struct fs_fd* file, void* buf, size_t count)
{
        int fr;
	int ret;
        FIL *fp = (FIL*)file->data;
        fr = f_read(fp, buf, count, &ret);
	update_pos(file);
        if (fr == FR_OK)
                return ret;
        return fr2err(fr);
}

int fat_write(struct fs_fd* file, const void* buf, size_t count)
{
        int fr;
	int ret;
        FIL *fp = (FIL*)file->data;
        fr = f_write(fp, buf, count, &ret);
	update_size(file);
	update_pos(file);
        if (fr == FR_OK)
                return ret;
        return fr2err(fr);
}

int fat_lseek(struct fs_fd* file, off_t offset)
{
	int fr;
	FIL *fp = (FIL*)file->data;  
	fr = f_lseek(fp, offset);
	update_pos(file);
	if (fr == FR_OK)
		file->pos = offset;
	return fr2err(fr);
}

int fat_unlink(struct fs_fd* file, const char *pathname)
{
	return fr2err(f_unlink(pathname));
}

int fat_readdir(struct fs_fd* file, const char *pathname)
{
	DIR dp;
	FILINFO fno;
	int fr;
	fr = f_opendir(&dp, pathname);
	if (fr != FR_OK)
		return fr2err(fr);

	cprintf("type\t size\t %16s\n", "name");
	while (f_readdir(&dp, &fno) == FR_OK) {
		if (fno.fname[0] == 0)
			break;
		cprintf("%s\t ", fno.fattrib & AM_DIR ? "DIR" : "FILE");
		cprintf("%d\t ", fno.fsize);
		cprintf("%16s\n", fno.fname);
	}

	f_closedir(&dp);
	return 0;
}

struct fs_ops elmfat_ops = {
    .dev_name = "elmfat",
    .mount = fat_mount,
    .mkfs = fat_mkfs,
    .open = fat_open,
    .close = fat_close,
    .read = fat_read,
    .write = fat_write,
    .lseek = fat_lseek,
    .unlink = fat_unlink,
    .readdir = fat_readdir
};
