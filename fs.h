#ifndef _FS_H_
#define _FS_H_

#include <sys/types.h>
#include <stddef.h>

int make_fs(char *disk_name);
int mount_fs(char *disk_name);
int umount_fs(char *disk_name);

int fs_open(char *fname);
int fs_close(int fildes);
int fs_create(char *fname);
int fs_delete(char *fname);
int fs_read(int fildes, void *buf, size_t nbyte);
int fs_write(int fildes, void *buf, size_t nbyte);
int fs_get_filesize(int fildes);
int fs_lseek(int fildes, off_t offset);
int fs_truncate(int fildes, off_t length);

#endif