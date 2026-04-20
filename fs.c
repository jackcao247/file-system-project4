#include <stdio.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define FS_SIG 0xf0f03410

#define MAX_FILES 64
#define MAX_FNAME 15
#define MAX_FILDES 32

#define DATA_BLOCKS 4096

#define SUPER_BLOCK 0
#define FAT_START 1
#define FAT_BLOCKS 4
#define DIR_START 5
#define DIR_BLOCKS 1
#define DATA_START 6

#define FAT_FREE -1
#define FAT_EOC -2

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

static void initSB()
{
    sb.sig = FS_SIG;
    sb.fat_start = FAT_START;
    sb.fat_blocks = FAT_BLOCKS;
    sb.dir_start = DIR_START;
    sb.dir_blocks = DIR_BLOCKS;
    sb.data_start = DATA_START;
    sb.data_blocks = DATA_BLOCKS;
}

static void clrMeta()
{
    int i;

    initSB();

    for (i = 0; i < DATA_BLOCKS; i++)
        fat[i] = FAT_FREE;

    memset(dir, 0, sizeof(dir));
    memset(fd_table, 0, sizeof(fd_table));
}

static int saveMeta()
{
    char buf[BLOCK_SIZE];
    int i;

    memset(buf, 0, BLOCK_SIZE);
    memcpy(buf, &sb, sizeof(sb));
    if (block_write(SUPER_BLOCK, buf) < 0)
        return -1;

    for (i = 0; i < FAT_BLOCKS; i++)
    {
        memcpy(buf, ((char *)fat) + (i * BLOCK_SIZE), BLOCK_SIZE);
        if (block_write(FAT_START + i, buf) < 0)
            return -1;
    }

    memset(buf, 0, BLOCK_SIZE);
    memcpy(buf, dir, sizeof(dir));
    if (block_write(DIR_START, buf) < 0)
        return -1;

    return 0;
}

static int loadMeta()
{
    char buf[BLOCK_SIZE];
    int i;

    if (block_read(SUPER_BLOCK, buf) < 0)
        return -1;
    memcpy(&sb, buf, sizeof(sb));

    if (sb.sig != (int)FS_SIG)
    {
        fprintf(stderr, "Unfound file system\n");
        return -1;
    }

    for (i = 0; i < FAT_BLOCKS; i++)
    {
        if (block_read(FAT_START + i, buf) < 0)
            return -1;
        memcpy(((char *)fat) + (i * BLOCK_SIZE), buf, BLOCK_SIZE);
    }

    if (block_read(DIR_START, buf) < 0)
        return -1;
    memcpy(dir, buf, sizeof(dir));

    memset(fd_table, 0, sizeof(fd_table));

    return 0;
}

static int findDir(char *fname)
{
    int i;

    for (i = 0; i < MAX_FILES; i++)
    {
        if (dir[i].used && strcmp(dir[i].name, fname) == 0)
            return i;
    }

    return -1;
}

static int findFreeDir()
{
    int i;

    for (i = 0; i < MAX_FILES; i++)
    {
        if (!dir[i].used)
            return i;
    }

    return -1;
}

static int findFreeFd()
{
    int i;

    for (i = 0; i < MAX_FILDES; i++)
    {
        if (!fd_table[i].used)
            return i;
    }

    return -1;
}

static int isOpen(int dir_i)
{
    int i;

    for (i = 0; i < MAX_FILDES; i++)
    {
        if (fd_table[i].used && fd_table[i].dir_i == dir_i)
            return 1;
    }

    return 0;
}

static void freeChain(int first_blk)
{
    int now;
    int next;

    now = first_blk;
    while (now >= 0)
    {
        next = fat[now];
        fat[now] = FAT_FREE;
        now = next;
    }
}

int make_fs(char *disk_name)
{
    if (!disk_name)
    {
        fprintf(stderr, "Unfound filename\n");
        return -1;
    }

    if (make_disk(disk_name) < 0)
        return -1;

    if (open_disk(disk_name) < 0)
        return -1;

    clrMeta();

    if (saveMeta() < 0)
    {
        close_disk();
        return -1;
    }

    if (close_disk() < 0)
        return -1;

    return 0;
}

int mount_fs(char *disk_name)
{
    if (!disk_name)
    {
        fprintf(stderr, "Unfound filename\n");
        return -1;
    }

    if (mounted)
    {
        fprintf(stderr, "File system mounted\n");
        return -1;
    }

    if (open_disk(disk_name) < 0)
        return -1;

    if (loadMeta() < 0)
    {
        close_disk();
        return -1;
    }

    mounted = 1;
    return 0;
}

int umount_fs(char *disk_name)
{
    (void)disk_name;

    if (!mounted)
    {
        fprintf(stderr, "No file mounted\n");
        return -1;
    }

    memset(fd_table, 0, sizeof(fd_table));

    if (saveMeta() < 0)
        return -1;

    if (close_disk() < 0)
        return -1;

    mounted = 0;
    return 0;
}

int fs_open(char *fname)
{
    int dir_i;
    int fd_i;

    if (!mounted)
    {
        fprintf(stderr, "No file mounted\n");
        return -1;
    }

    if (!fname)
    {
        fprintf(stderr, "Unfound filename\n");
        return -1;
    }

    dir_i = findDir(fname);
    if (dir_i < 0)
    {
        fprintf(stderr, "Unfound file\n");
        return -1;
    }

    fd_i = findFreeFd();
    if (fd_i < 0)
    {
        fprintf(stderr, "Too many opened files\n");
        return -1;
    }

    fd_table[fd_i].used = 1;
    fd_table[fd_i].dir_i = dir_i;
    fd_table[fd_i].offset = 0;

    return fd_i;
}

int fs_close(int fildes)
{
    if (!mounted)
    {
        fprintf(stderr, "No file mounted\n");
        return -1;
    }

    if (fildes < 0 || fildes >= MAX_FILDES)
    {
        fprintf(stderr, "Wrong file id\n");
        return -1;
    }

    if (!fd_table[fildes].used)
    {
        fprintf(stderr, "File not opened\n");
        return -1;
    }

    memset(&fd_table[fildes], 0, sizeof(FdEnt));

    return 0;
}

int fs_create(char *fname)
{
    int dir_i;

    if (!mounted)
    {
        fprintf(stderr, "No file mounted\n");
        return -1;
    }

    if (!fname)
    {
        fprintf(stderr, "Unfound filename\n");
        return -1;
    }

    if ((int)strlen(fname) > MAX_FNAME)
    {
        fprintf(stderr, "Filename too long\n");
        return -1;
    }

    if (findDir(fname) >= 0)
    {
        fprintf(stderr, "File already exists\n");
        return -1;
    }

    dir_i = findFreeDir();
    if (dir_i < 0)
    {
        fprintf(stderr, "Directory full\n");
        return -1;
    }

    dir[dir_i].used = 1;
    strcpy(dir[dir_i].name, fname);
    dir[dir_i].size = 0;
    dir[dir_i].first_blk = FAT_EOC;

    return 0;
}

int fs_delete(char *fname)
{
    int dir_i;

    if (!mounted)
    {
        fprintf(stderr, "No file mounted\n");
        return -1;
    }

    if (!fname)
    {
        fprintf(stderr, "Unfound filename\n");
        return -1;
    }

    dir_i = findDir(fname);
    if (dir_i < 0)
    {
        fprintf(stderr, "Unfound file\n");
        return -1;
    }

    if (isOpen(dir_i))
    {
        fprintf(stderr, "File opened\n");
        return -1;
    }

    freeChain(dir[dir_i].first_blk);
    memset(&dir[dir_i], 0, sizeof(DirEnt));
    dir[dir_i].first_blk = FAT_EOC;

    return 0;
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
