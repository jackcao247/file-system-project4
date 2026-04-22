#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "fs.h"

#define DISK_NAME "disk"
#define FILE_A "f1"
#define FILE_B "f2"
#define FILE_C "f3"
#define DATA_LEN 8500
#define COPY_BUF 600

typedef struct
{
    char *data;
    int len;
} ThreadArg;

static void fillData(char *buf, int len)
{
    int i;

    for (i = 0; i < len; i++)
        buf[i] = 'A' + (i % 26);
}

static int copyFile(char *src_name, char *dst_name)
{
    int src_fd;
    int dst_fd;
    int got;
    char buf[COPY_BUF];

    if (fs_create(dst_name) < 0)
        return -1;

    src_fd = fs_open(src_name);
    if (src_fd < 0)
        return -1;

    dst_fd = fs_open(dst_name);
    if (dst_fd < 0)
    {
        fs_close(src_fd);
        return -1;
    }

    while (1)
    {
        got = fs_read(src_fd, buf, COPY_BUF);
        if (got < 0)
        {
            fs_close(src_fd);
            fs_close(dst_fd);
            return -1;
        }

        if (got == 0)
            break;

        if (fs_write(dst_fd, buf, got) != got)
        {
            fs_close(src_fd);
            fs_close(dst_fd);
            return -1;
        }
    }

    fs_close(src_fd);
    fs_close(dst_fd);
    return 0;
}

static void *runThread(void *arg_p)
{
    ThreadArg *arg;
    int fd;
    int got;
    char buf[64];

    arg = (ThreadArg *)arg_p;

    fd = fs_open(FILE_A);
    if (fd < 0)
    {
        printf("FAIL   : thread open\n");
        return NULL;
    }

    if (fs_read(fd, buf, 20) != 20)
    {
        printf("FAIL   : thread read 1\n");
        fs_close(fd);
        return NULL;
    }

    if (memcmp(buf, arg->data, 20) == 0)
        printf("SUCCESS : thread read start\n");
    else
        printf("FAIL   : thread read start\n");

    if (fs_lseek(fd, 4095) < 0)
    {
        printf("FAIL   : thread lseek block 1\n");
        fs_close(fd);
        return NULL;
    }

    got = fs_read(fd, buf, 30);
    if (got != 30)
    {
        printf("FAIL   : thread read 2\n");
        fs_close(fd);
        return NULL;
    }

    if (memcmp(buf, arg->data + 4095, 30) == 0)
        printf("SUCCESS : thread cross block read\n");
    else
        printf("FAIL   : thread cross block read\n");

    if (fs_lseek(fd, 8191) < 0)
    {
        printf("FAIL   : thread lseek block 2\n");
        fs_close(fd);
        return NULL;
    }

    got = fs_read(fd, buf, 40);
    if (got == 40 && memcmp(buf, arg->data + 8191, 40) == 0)
        printf("SUCCESS : thread late read\n");
    else
        printf("FAIL   : thread late read\n");

    fs_close(fd);

    if (copyFile(FILE_A, FILE_B) < 0)
        printf("FAIL   : thread copy\n");
    else
        printf("SUCCESS : thread copy\n");

    return NULL;
}

int main()
{
    pthread_t tid;
    ThreadArg arg;
    int fd;
    int size;
    int got;
    char *data;
    char tail[32];

    data = malloc(DATA_LEN);
    if (!data)
    {
        printf("FAIL   : malloc\n");
        return 1;
    }

    fillData(data, DATA_LEN);
    arg.data = data;
    arg.len = DATA_LEN;

    if (make_fs(DISK_NAME) < 0)
    {
        printf("FAIL   : make_fs\n");
        free(data);
        return 1;
    }
    printf("SUCCESS : make_fs\n");

    if (mount_fs(DISK_NAME) < 0)
    {
        printf("FAIL   : mount_fs\n");
        free(data);
        return 1;
    }
    printf("SUCCESS : mount_fs\n");

    if (fs_create(FILE_A) < 0)
    {
        printf("FAIL   : fs_create f1\n");
        free(data);
        return 1;
    }
    printf("SUCCESS : fs_create f1\n");

    fd = fs_open(FILE_A);
    if (fd < 0)
    {
        printf("FAIL   : fs_open f1\n");
        free(data);
        return 1;
    }
    printf("SUCCESS : fs_open f1 fd=%d\n", fd);

    if (fs_write(fd, data, DATA_LEN) != DATA_LEN)
    {
        printf("FAIL   : fs_write f1\n");
        free(data);
        return 1;
    }
    printf("SUCCESS : fs_write f1\n");

    size = fs_get_filesize(fd);
    printf("f1 size = %d\n", size);

    if (fs_lseek(fd, 500) < 0)
        printf("FAIL   : fs_lseek f1\n");
    else
        printf("SUCCESS : fs_lseek f1\n");

    if (fs_close(fd) < 0)
        printf("FAIL   : fs_close f1\n");
    else
        printf("SUCCESS : fs_close f1\n");

    if (pthread_create(&tid, NULL, runThread, &arg) != 0)
    {
        printf("FAIL   : pthread_create\n");
        free(data);
        return 1;
    }
    printf("thread created\n");

    if (pthread_join(tid, NULL) != 0)
    {
        printf("FAIL   : pthread_join\n");
        free(data);
        return 1;
    }
    printf("thread joined\n");

    fd = fs_open(FILE_B);
    if (fd < 0)
    {
        printf("FAIL   : fs_open f2\n");
        free(data);
        return 1;
    }

    size = fs_get_filesize(fd);
    printf("f2 size before truncate = %d\n", size);

    if (fs_truncate(fd, 5000) < 0)
        printf("FAIL   : fs_truncate f2\n");
    else
        printf("SUCCESS : fs_truncate f2\n");

    size = fs_get_filesize(fd);
    printf("f2 size after truncate = %d\n", size);

    if (fs_lseek(fd, 4995) < 0)
        printf("FAIL   : fs_lseek f2\n");
    else
    {
        memset(tail, 0, sizeof(tail));
        got = fs_read(fd, tail, 10);
        printf("f2 last read count = %d\n", got);
    }

    if (fs_close(fd) < 0)
        printf("FAIL   : fs_close f2\n");
    else
        printf("SUCCESS : fs_close f2\n");

    if (fs_delete(FILE_A) < 0)
        printf("FAIL   : fs_delete f1\n");
    else
        printf("SUCCESS : fs_delete f1\n");

    if (fs_create(FILE_C) < 0)
        printf("FAIL   : fs_create f3\n");
    else
        printf("SUCCESS : fs_create f3\n");

    fd = fs_open(FILE_C);
    if (fd < 0)
        printf("FAIL   : fs_open f3\n");
    else
    {
        got = fs_write(fd, data, DATA_LEN);
        printf("f3 write count = %d\n", got);
        fs_close(fd);
    }

    if (umount_fs(DISK_NAME) < 0)
    {
        printf("FAIL   : umount_fs\n");
        free(data);
        return 1;
    }
    printf("SUCCESS : umount_fs\n");

    if (mount_fs(DISK_NAME) < 0)
    {
        printf("FAIL   : mount_fs again\n");
        free(data);
        return 1;
    }
    printf("SUCCESS : mount_fs again\n");

    fd = fs_open(FILE_B);
    if (fd >= 0)
    {
        printf("f2 size after remount = %d\n", fs_get_filesize(fd));
        fs_close(fd);
    }

    fd = fs_open(FILE_C);
    if (fd >= 0)
    {
        printf("f3 size after remount = %d\n", fs_get_filesize(fd));
        fs_close(fd);
    }

    if (umount_fs(DISK_NAME) < 0)
        printf("FAIL   : final umount\n");
    else
        printf("SUCCESS : final umount\n");

    free(data);
    return 0;
}