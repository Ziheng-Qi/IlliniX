//           fs.h - File system interface
//          

#ifndef _FS_H_
#define _FS_H_

#include "heap.h"
#include "io.h"
#include "virtio.h"
#include "intr.h"
#include "string.h"
#include "console.h"

#define BLOCK_SIZE 4096
#define MAX_DIR_ENTRIES 63
#define MAX_INODES 1023
#define BOOT_RESERVED_SPACE_SZ 52
#define MAX_FILE_NAME_LENGTH 32 // 32 bytes
#define DENTRY_RESERVED_SPACE_SZ 28
#define MAX_FILE_OPEN 32
#define INUSE 1
#define UNUSE 0

typedef struct file_t
{
  struct io_intf *io;
  uint64_t file_position;
  uint64_t file_size;
  uint64_t inode_num;
  uint64_t flag;
} __attribute((packed)) file_t;

typedef struct dentry_t
{
  char file_name[MAX_FILE_NAME_LENGTH];
  uint32_t inode;
  uint8_t reserved[DENTRY_RESERVED_SPACE_SZ];
} __attribute((packed)) dentry_t;

typedef struct boot_block_t
{
  uint32_t num_dentry;
  uint32_t num_inodes;
  uint32_t num_data;
  uint8_t reserved[BOOT_RESERVED_SPACE_SZ];
  dentry_t dir_entries[MAX_DIR_ENTRIES];
} __attribute((packed)) boot_block_t;

typedef struct inode_t
{
  uint32_t byte_len;
  uint32_t data_block_num[MAX_INODES];
} __attribute((packed)) inode_t;

typedef struct data_block_t
{
  uint8_t data[BLOCK_SIZE];
} __attribute((packed)) data_block_t;

extern char fs_initialized;

extern void fs_init(void);

extern int fs_mount(struct io_intf * blkio);

extern int fs_open(const char * name, struct io_intf ** ioptr);

extern void fs_close(struct io_intf *io);

extern long fs_read(struct io_intf *io, void *buf, unsigned long n);

extern long fs_write(struct io_intf *io, const void *buf, unsigned long n);

extern int fs_ioctl(struct io_intf *io, int cmd, void *arg);

extern int fs_getlen(file_t *file, void *arg);

extern int fs_getpos(file_t *file, void *arg);

extern int fs_setpos(file_t *file, void *arg);

extern int fs_getblksz(file_t *file, void *arg);

//           _FS_H_
#endif
