#include "virtio.h"
#include "intr.h"
#include "string.h"
#include "console.h"
#include "heap.h"
#include "fs.h"

extern char _kimg_end[]; // end of kernel image (defined in kernel.ld)

#ifndef RAM_SIZE
#ifdef RAM_SIZE_MB
#define RAM_SIZE (RAM_SIZE_MB * 1024 * 1024)
#else
#define RAM_SIZE (8 * 1024 * 1024)
#endif
#endif

#ifndef RAM_START
#define RAM_START 0x80000000UL
#endif

static boot_block_t boot_block;

static struct io_intf *fs_io = NULL;

static file_t file_desc_tab[MAX_FILE_OPEN];

static size_t fs_base = 0;

int fs_mount(struct io_intf *io)
{

  fs_io = io;
  // Allocate memory for the boot block

  ioread(fs_io, &boot_block, BLOCK_SIZE);
  // Read the boot block
  // console_printf("dentries: %d, inodes: %d, data blocks: %d\n", boot_block.num_dentry, boot_block.num_inodes, boot_block.num_data);
  // get the boot block, the boot block won't be changed after mounting

  for (int i = 0; i < MAX_FILE_OPEN; i++)
  {

    file_desc_tab[i].flag = UNUSE;
    // mark all file as UNUSE for initialization, all UNUSED files can be flushed by fs_open with new file opened
  }

  heap_init(_kimg_end, (void *)RAM_START + RAM_SIZE);

  return 0;
}

int fs_open(const char *name, struct io_intf **io)
{
  // search the file in the directory

  for (int i = 0; i < boot_block.num_dentry; i++)
  {
    if (strcmp(boot_block.dir_entries[i].file_name, name) == 0)
    {
      // file found
      // set up a new instance of io_interface for the file struct
      struct io_intf *file_io = (struct io_intf *)kmalloc(sizeof(struct io_intf));
      if (file_io == NULL)
      {
        return -1; // Handle memory allocation failure
      }
      file_io->ops = fs_io->ops;

      // pass the io interface to the caller
      *io = file_io;
      // check if the file has unique io interface

      // set inode_num to be the inode number of the file
      uint64_t inode_num = boot_block.dir_entries[i].inode;
      // seek to the inode position
      uint64_t position = fs_base + BLOCK_SIZE + boot_block.dir_entries[i].inode * BLOCK_SIZE;
      // console_printf("Seeking to position: %d\n", position);
      uint64_t file_position = 0;
      ioseek(fs_io, position);
      inode_t file_inode;
      ioread(fs_io, &file_inode, BLOCK_SIZE);
      uint64_t file_size = file_inode.byte_len;
      uint64_t flag = INUSE;
      for (int i = 0; i < MAX_FILE_OPEN; i++)
      {
        if (file_desc_tab[i].flag == UNUSE)
        {

          // console_printf("File descriptor index: %d\n", i);

          file_desc_tab[i].file_position = file_position;
          file_desc_tab[i].file_size = file_size;
          file_desc_tab[i].inode_num = inode_num;
          file_desc_tab[i].flag = flag;
          file_desc_tab[i].io = file_io;
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
      kfree(file_desc_tab[i].io);
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
      file_t *file = &file_desc_tab[i];
      uint64_t file_position = file->file_position;
      uint64_t inode_num = file->inode_num;

      // Seek to the inode position
      ioseek(fs_io, fs_base + BLOCK_SIZE + inode_num * BLOCK_SIZE);

      // Read the inode
      inode_t file_inode;

      ioread(fs_io, &file_inode, BLOCK_SIZE);

      // Calculate the number of blocks written based on the file position
      uint64_t written_blocks = file_position / BLOCK_SIZE;
      uint64_t written_bytes = file_position % BLOCK_SIZE;

      if (written_blocks == file_inode.byte_len / BLOCK_SIZE)
      {
        return -1;
      }

      // Seek to the data block position
      ioseek(fs_io, fs_base + BLOCK_SIZE + boot_block.num_inodes * BLOCK_SIZE + file_inode.data_block_num[written_blocks] * BLOCK_SIZE + written_bytes);
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
          ioseek(fs_io, fs_base + BLOCK_SIZE + boot_block.num_inodes * BLOCK_SIZE + file_inode.data_block_num[written_blocks] * BLOCK_SIZE);
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
          ioseek(fs_io, fs_base + BLOCK_SIZE + boot_block.num_inodes * BLOCK_SIZE + file_inode.data_block_num[written_blocks] * BLOCK_SIZE);

          // Read the next data block
          ioread(fs_io, &data_block, BLOCK_SIZE);
        }

        // Write the byte to the data block
        data_block.data[written_bytes] = ((char *)buf)[bytes_written];
        written_bytes++;
        bytes_written++;
      }

      // Write the last data block to disk
      for (int i = 0; i < BLOCK_SIZE; i++)
      {
        // console_putchar(data_block.data[i]);
      }

      ioseek(fs_io, fs_base + BLOCK_SIZE + boot_block.num_inodes * BLOCK_SIZE + file_inode.data_block_num[written_blocks] * BLOCK_SIZE);

      iowrite(fs_io, &data_block, BLOCK_SIZE);

      // Update the file position
      // console_printf("n: %d\n", n);
      file->file_position += n;
      // console_printf("file position: %d\n", file->file_position);
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
      console_printf("Found the file descriptor\n");
      // Found the file descriptor
      file_t *file = &file_desc_tab[i];
      uint64_t file_position = file->file_position; // Current position in the file
      uint64_t inode_num = file->inode_num;         // Inode number of the file
      // Seek to the inode location in the filesystem
      ioseek(fs_io, fs_base + BLOCK_SIZE + inode_num * BLOCK_SIZE);
      console_printf("Inode number: %d\n", inode_num);
      console_printf("Seeking to inode: %d\n", fs_io->ops->ctl(fs_io, IOCTL_GETPOS, NULL));
      // Read the inode data
      inode_t file_inode;

      ioread(fs_io, &file_inode, BLOCK_SIZE); // Read the inode data
      // Calculate the number of blocks and bytes to read based on the file position
      uint64_t read_blocks = file_position / BLOCK_SIZE;
      uint64_t read_bytes = file_position % BLOCK_SIZE;

            // Seek to the data block that contains the file data
      data_block_t data_block;
      ioseek(fs_io, fs_base + BLOCK_SIZE + boot_block.num_inodes * BLOCK_SIZE + file_inode.data_block_num[read_blocks] * BLOCK_SIZE);
      console_printf("Reading from block: %d\n", file_inode.data_block_num[read_blocks]);
      console_printf("reading from: %d\n", fs_io->ops->ctl(fs_io, IOCTL_GETPOS, NULL));
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
          ioseek(fs_io, fs_base + BLOCK_SIZE + boot_block.num_inodes * BLOCK_SIZE + file_inode.data_block_num[read_blocks] * BLOCK_SIZE);
          ioread(fs_io, &data_block, BLOCK_SIZE); // Read the next data block
        }
        // Copy data from the data block to the buffer

        ((char *)buf)[bytes_read] = data_block.data[read_bytes];

        // console_putchar(data_block.data[read_bytes]);
        read_bytes++;
        bytes_read++;
      }
      // Update the file position after reading

      file->file_position += n;
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
    file_t *file = &file_desc_tab[i];
    struct io_intf *file_io = file->io;
    // check if the file_io is the same `io_intf` as the argument provided
    if (io == file_io)
    {
      switch (cmd)
      {
      case IOCTL_GETLEN:
        return fs_getlen(file, arg);
      case IOCTL_SETPOS:
        return fs_setpos(file, arg);
      case IOCTL_GETPOS:
        return fs_getpos(file, arg);
      case IOCTL_GETBLKSZ:
        return fs_getblksz(file, arg);
      default:
        return -1;
      }
    }
  }
  return -ENOTSUP;
}

int fs_getlen(file_t *file, void *arg)
{
  return file->file_size;
}

int fs_getpos(file_t *file, void *arg)
{
  return file->file_position;
}

int fs_setpos(file_t *file, void *arg)
{

  file->file_position = (uint64_t)arg;
  return 0;
}

int fs_getblksz(file_t *file, void *arg)
{
  return BLOCK_SIZE;
}