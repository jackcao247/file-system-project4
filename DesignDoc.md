# A. Volume layout

- Block 0 : super block
- Blocks 1 to 4 : FAT
- Block 5 : directory
- Block 6+ data blocks

# B. Phys directory

- entry status (used/unused)
- file name
- file size
- first block of the file

# C. Space allocation and free blocks

FAT method:

`fat[DATA_BLOCKS]`

Free blocks management:

- `FAT_FREE` : block is free
- `FAT_EOC` : end of chain

# D. Logical directory

If the user calls `fs_open("fileA")`, the file system searches the directory entries for the name `"fileA"`

Directory entry stores the file name, file size, and first data block

First block and following the FAT chain contains file data

File descriptor is only in memory and not saved on disk. File descriptor stores directory entry pointer and current offset.

This means the same file can be opened more than once, and each open file descriptor can have its own offset

## E. Relationship between logical and physical directory

Logical directory : what the user sees

Physical directory : what stored on disk

# F. Function descriptions

### Helper

`initSB`

Set the file system signature and the block locations for the FAT, directory, and data area

`clrMeta`

Set all FAT entries to free, clears the directory, and clears the file descriptor table

`saveMeta`

Write the super block, FAT, and directory

`loadMeta`

Check the signature first, then loads the FAT and directory into memory

`findDir`

Search the directory for a file name

`findFreeDir`

Find an empty directory entry for a new file

`isOpen`

Check if a file is currently open for fs_delete

`freeChain`

Free every block in the FAT chain

`getBlk`

Find a certain block inside a file start from first block

`findFreeFd`

Find an empty file descriptor slot

`findFreeBlk`

Find a free data block

`addBlk`

Find a free block and connect it to the end of the file chain

### File system management functions

`make_fs`

Create the disk, open it, clear the metadata, save the empty metadata, and close the disk

`mount_fs`

Open the virtual disk and loads the file system metadata into memory

`umount_fs`

Save the metadata back to disk and closes the disk

### File functions

`fs_create`

Create a new empty file and check that the file system is mounted, name is not too long, does not already exist and directory has room

`fs_delete`

Check that the file exist and is not open, free the file blocks and clear the directory entry

`fs_open`

Find the file in the directory, free descriptor slot and set the offset to 0

`fs_close`

Close a file descriptor by clearing its slot in descriptor table

`fs_get_filesize`

Return the size of the file

`fs_lseek`

Change the offset of an open file

`fs_read`

Find the correct data block through the FAT, copy data into the user buffer, stop at the end of the file and update the offset

`fs_write`

Write data to file starting at the current offset, can allocate more block if needed

`fs_truncate`

Free the blocks after the new file size and update the size. Move offset back if open file descriptor has offset past the new size

# Demo app

1. Create disk, then mount, then create a file, open, write data, check file size and close

2. Create one pthread. Thread open file, read from different positions, compare data, copy the file to another file

3. Truncates copied file, delete original file, create another file, write data again then unmount file system

4. Mount the disk again, check that the files are still there
