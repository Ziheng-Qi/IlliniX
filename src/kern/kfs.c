#include "virtio.h"
#include "intr.h"
#include "io.h"
#include "stdio.h"
#include "string.h"

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

boot_block_t *boot_block = NULL;

struct io_intf *fs_io = NULL;

file_t file_desc_tab[MAX_FILE_OPEN];

size_t fs_base = 0;

int fs_mount(struct io_intf *io)
{
  fs_io = io;
  // Read the boot block
  ioread(fs_io, boot_block, BLOCK_SIZE);
  // get the boot block, the boot block won't be changed after mounting
  fs_base = fs_io->ops->ctl(fs_io, IOCTL_GETPOS, NULL);

  for (int i = 0; i < MAX_FILE_OPEN; i++)
  {
    file_desc_tab[i].flag = UNUSE;
    // mark all file as UNUSE for initialization, all UNUSED files can be flushed by fs_open with new file opened
  }
  return 0;
}

int fs_open(const char *name, struct io_intf **io)
{
  // search the file in the directory
  struct file_t *file = NULL;
  struct io_intf *file_io;
  file_io->ops = fs_io->ops;
  for (int i = 0; i < boot_block->num_dentry; i++)
  {

    if (strcmp(boot_block->dir_entries[i].file_name, name) == 0)
    {
      // file found
      // set up a new instance of io_interface for the file struct
      *io = file_io;
      file->io = file_io;
      // check if the file has unique io interface
      printf("file_io: %p, fs_io: %p\n", file_io, fs_io);
      // set inode_num to be the inode number of the file
      file->inode_num = boot_block->dir_entries[i].inode;
      // seek to the inode position
      size_t position = fs_base + BLOCK_SIZE + boot_block->dir_entries[i].inode * BLOCK_SIZE;
      file->file_position = 0;
      ioseek(fs_io, position);
      inode_t file_inode;
      ioread(fs_io, &file_inode, BLOCK_SIZE);
      file->file_size = file_inode.byte_len;
      file->flag = INUSE;
      for (int i = 0; i < MAX_FILE_OPEN; i++)
      {
        if (file_desc_tab[i].flag == UNUSE)
        {
          file_desc_tab[i] = *file;
          return 0;
        }
      }
    }
  }
  return 0;
}

void fs_close(struct io_intf *io)
{
  for (int i = 0; i < MAX_FILE_OPEN; i++)
  {
    if (file_desc_tab[i].io == io)
    {
      file_desc_tab[i].flag = UNUSE;
      return;
    }
  }
}

long fs_write(struct io_intf *io, const void *buf, unsigned long n)
{
  for (int i = 0; i < MAX_FILE_OPEN; i++)
  {
    if (io == file_desc_tab[i].io)
    {
      // found the file
      file_t file = file_desc_tab[i];
      uint64_t file_position = file.file_position;
      uint64_t file_size = file.file_size;
      uint64_t inode_num = file.inode_num;

      // Seek to the inode position
      ioseek(fs_io, fs_base + BLOCK_SIZE + inode_num * BLOCK_SIZE);

      // Read the inode
      inode_t file_inode;
      file.io->ops->read(io, &file_inode, BLOCK_SIZE);

      // Calculate the number of blocks written based on the file position
      uint64_t written_blocks = file_position / BLOCK_SIZE;
      uint64_t written_bytes = file_position % BLOCK_SIZE;

      // Seek to the data block position
      ioseek(fs_io, fs_base + BLOCK_SIZE + boot_block->num_inodes * BLOCK_SIZE + file_inode.data_block_num[written_blocks] * BLOCK_SIZE + written_bytes);

      // Read the data block
      data_block_t data_block;
      ioread(fs_io, &data_block, BLOCK_SIZE);

      uint64_t bytes_written = 0;

      // Write data to the blocks
      while (bytes_written < n)
      {
        if (written_bytes == BLOCK_SIZE)
        {
          // Write the current data block to disk
          iowrite(fs_io, &data_block, BLOCK_SIZE);

          // Move to the next block
          written_blocks++;
          written_bytes = 0;

          // Check if the file is full
          if (written_blocks == MAX_INODES)
          {
            return -1;
          }

          // Seek to the next data block position
          ioseek(fs_io, fs_base + BLOCK_SIZE + boot_block->num_inodes * BLOCK_SIZE + file_inode.data_block_num[written_blocks] * BLOCK_SIZE);

          // Read the next data block
          ioread(fs_io, &data_block, BLOCK_SIZE);
        }

        // Write the byte to the data block
        data_block.data[written_bytes] = ((char *)buf)[bytes_written];
        written_bytes++;
        bytes_written++;
      }

      // Write the last data block to disk
      iowrite(fs_io, &data_block, BLOCK_SIZE);

      // Update the file position
      file.file_position += n;
      return n;
    }
  }
  return -1;
}

long fs_read(struct io_intf *io, void *buf, unsigned long n)
{
  // Loop through the file descriptor table to find the matching io interface
  for (int i = 0; i < MAX_FILE_OPEN; i++)
  {
    if (io == file_desc_tab[i].io)
    {
      // Found the file descriptor
      file_t file = file_desc_tab[i];
      uint64_t file_position = file.file_position; // Current position in the file
      uint64_t file_size = file.file_size;         // Size of the file
      uint64_t inode_num = file.inode_num;         // Inode number of the file

      // Seek to the inode location in the filesystem
      ioseek(fs_io, fs_base + BLOCK_SIZE + inode_num * BLOCK_SIZE);
      inode_t file_inode;
      file.io->ops->read(io, &file_inode, BLOCK_SIZE); // Read the inode data

      // Calculate the number of blocks and bytes to read based on the file position
      uint64_t read_blocks = file_position / BLOCK_SIZE;
      uint64_t read_bytes = file_position % BLOCK_SIZE;

      // Seek to the data block that contains the file data
      data_block_t data_block;
      ioseek(fs_io, fs_base + BLOCK_SIZE + boot_block->num_inodes * BLOCK_SIZE + file_inode.data_block_num[read_blocks] * BLOCK_SIZE + read_bytes);
      ioread(fs_io, &data_block, BLOCK_SIZE); // Read the data block

      uint64_t bytes_read = 0; // Counter for the number of bytes read

      // Read data from the file until the requested number of bytes is read
      while (bytes_read < n)
      {
        if (read_bytes == BLOCK_SIZE)
        {
          // Move to the next block if the current block is fully read
          read_blocks++;
          read_bytes = 0;
          if (read_blocks == MAX_INODES)
          {
            // If the file is full, return an error
            return -1;
          }
          // Seek to the next data block
          ioseek(fs_io, fs_base + BLOCK_SIZE + boot_block->num_inodes * BLOCK_SIZE + file_inode.data_block_num[read_blocks] * BLOCK_SIZE);
          ioread(fs_io, &data_block, BLOCK_SIZE); // Read the next data block
        }
        // Copy data from the data block to the buffer
        ((char *)buf)[bytes_read] = data_block.data[read_bytes];
        read_bytes++;
        bytes_read++;
      }
      // Update the file position after reading
      file.file_position += n;
      return n; // Return the number of bytes read
    }
  }
  // If the file descriptor is not found, return an error
  return -1;
}

int fs_ioctl(struct io_intf *io, int cmd, void *arg)
{
  for (int i = 0; i < MAX_FILE_OPEN; i++)
  {
    file_t file = file_desc_tab[i];
    struct io_intf *file_io = file.io;
    // check if the file_io is the same `io_intf` as the argument provided
    if (io == file_io)
    {
      switch (cmd)
      {
      case IOCTL_GETLEN:
        return fs_getlen(&file, arg);
      case IOCTL_SETPOS:
        return fs_setpos(&file, arg);
      case IOCTL_GETPOS:
        return fs_getpos(&file, arg);
      case IOCTL_GETBLKSZ:
        return fs_getblksz(&file, arg);
      default:
        return -1;
      }
    }
  }
}

int fs_getlen(struct file_t *file, void *arg)
{
  return file->file_size;
}

int fs_getpos(struct file_t *file, void *arg)
{
  return file->file_position;
}

int fs_setpos(struct file_t *file, void *arg)
{
  file->file_position = *(uint64_t *)arg;
  return 0;
}

int fs_getblksz(struct file_t *file, void *arg)
{
  return BLOCK_SIZE;
}