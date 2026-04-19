#include <stdio.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define FS_SIG 0xf0f03410

#define MAX_FILES 64
#define MAX_FILDES 32
#define MAX_FNAME 15

#define DATA_BLOCKS 4096

#define SUPER_BLOCK 0
#define FAT_START 1
#define FAT_BLOCKS 4
#define DIR_START 5
#define DIR_BLOCKS 1
#define DATA_START 6

#define FAT_FREE -1
#define FAT_EOC -2

/*
 * Simple layout plan for now:
 * block 0      : super block
 * block 1 - 4  : FAT (4096 entries, int each)
 * block 5      : root directory
 * block 6 ...  : file data blocks
 */

typedef struct
{
    int sig;
    int fat_start;
    int fat_blocks;
    int dir_start;
    int dir_blocks;
    int data_start;
    int data_blocks;
} SuperBlock;

typedef struct
{
    int used;
    char name[MAX_FNAME + 1];
    int size;
    int first_blk;
} DirEnt;

typedef struct
{
    int used;
    int dir_i;
    int offset;
} FdEnt;

static int mounted = 0;
static SuperBlock sb;
static int fat[DATA_BLOCKS];
static DirEnt dir[MAX_FILES];
static FdEnt fd_table[MAX_FILDES];

int make_fs(char *disk_name)
{
    int i;

    (void)disk_name;

    sb.sig = FS_SIG;
    sb.fat_start = FAT_START;
    sb.fat_blocks = FAT_BLOCKS;
    sb.dir_start = DIR_START;
    sb.dir_blocks = DIR_BLOCKS;
    sb.data_start = DATA_START;
    sb.data_blocks = DATA_BLOCKS;

    for (i = 0; i < DATA_BLOCKS; i++)
        fat[i] = FAT_FREE;

    memset(dir, 0, sizeof(dir));
    memset(fd_table, 0, sizeof(fd_table));
    mounted = 0;

    return -1;
}

int mount_fs(char *disk_name)
{
    (void)disk_name;

    if (mounted)
        return -1;

    return -1;
}

int umount_fs(char *disk_name)
{
    (void)disk_name;

    if (!mounted)
        return -1;

    return -1;
}

int fs_open(char *fname)
{
    (void)fname;
    return -1;
}

int fs_close(int fildes)
{
    (void)fildes;
    return -1;
}

int fs_create(char *fname)
{
    (void)fname;
    return -1;
}

int fs_delete(char *fname)
{
    (void)fname;
    return -1;
}

int fs_read(int fildes, void *buf, size_t nbyte)
{
    (void)fildes;
    (void)buf;
    (void)nbyte;
    return -1;
}

int fs_write(int fildes, void *buf, size_t nbyte)
{
    (void)fildes;
    (void)buf;
    (void)nbyte;
    return -1;
}

int fs_get_filesize(int fildes)
{
    (void)fildes;
    return -1;
}

int fs_lseek(int fildes, off_t offset)
{
    (void)fildes;
    (void)offset;
    return -1;
}

int fs_truncate(int fildes, off_t length)
{
    (void)fildes;
    (void)length;
    return -1;
}