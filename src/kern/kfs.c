#include "fs.h"
// boot blocks for the file system
static boot_block_t boot_block;
// io interface for the file system
static struct io_intf *fs_io = NULL;
// file descriptor table
static file_t file_desc_tab[MAX_FILE_OPEN];
// base address of the file system, basically just zero, everything operates using offsets
static size_t fs_base = 0;

/**
 * @brief Mounts the filesystem by initializing the file descriptor table and reading the boot block.
 *
 * This function sets up the filesystem by associating it with the provided I/O interface,
 * reading the boot block into memory, and initializing the file descriptor table.
 *
 * @param io Pointer to the I/O interface to be used for filesystem operations.
 * @return 0 on success, non-zero error code on failure.
 */
int fs_mount(struct io_intf *io)
{
  fs_io = io;
  // Allocate memory for the boot block
  ioread(fs_io, &boot_block, BLOCK_SIZE);
  // Read the boot block
  // get the boot block, the boot block won't be changed after mounting
  for (int i = 0; i < MAX_FILE_OPEN; i++)
  {
    file_desc_tab[i].flag = UNUSE;
    // mark all file as UNUSE for initialization, all UNUSED files can be flushed by fs_open with new file opened
  }
  return 0;
}

/**
 * @brief Opens a file and sets up an I/O interface for it.
 *
 * This function searches for a file by its name in the directory entries of the boot block.
 * If the file is found, it allocates memory for a new I/O interface, sets up the interface,
 * and initializes a file descriptor for the file.
 *
 * @param name The name of the file to open.
 * @param io A pointer to a pointer to an I/O interface structure. This will be set to the newly created I/O interface.
 * @return 0 on success, -1 on failure (e.g., memory allocation failure).
 */

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
      for (int j = 0; j < MAX_FILE_OPEN; j++)
      {
        if (file_desc_tab[j].flag == UNUSE)
        {
          // console_printf("File descriptor index: %d\n", i);
          console_printf("file %s opened\n", boot_block.dir_entries[i].file_name);
          file_desc_tab[j].file_position = file_position;
          file_desc_tab[j].file_size = file_size;
          file_desc_tab[j].inode_num = inode_num;
          file_desc_tab[j].flag = flag;
          file_desc_tab[j].io = file_io;
          return 0;
        }
      }
    }
  }
  // console_printf("File not found\n");
  return -ENOENT;
}

/**
 * @brief Closes a file associated with the given I/O interface.
 *
 * This function iterates through the file descriptor table to find the entry
 * that matches the provided I/O interface. Once found, it marks the file
 * descriptor as unused and frees the associated I/O interface memory.
 *
 * @param io Pointer to the I/O interface to be closed.
 */
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

/**
 * @brief Writes data to a file in the filesystem.
 *
 * This function writes up to `n` bytes from the buffer `buf` to the file
 * associated with the given `io` interface. It updates the file's position
 * accordingly and handles block-level operations to ensure data is written
 * correctly to the filesystem.
 *
 * @param io Pointer to the I/O interface representing the file.
 * @param buf Pointer to the buffer containing the data to be written.
 * @param n Number of bytes to write from the buffer.
 * @return The number of bytes successfully written, or -1 if an error occurs.
 *
 * @note The function assumes that the file descriptor table and other
 *       filesystem structures are properly initialized and accessible.
 *       It also assumes that the file is not full and has enough space
 *       to accommodate the data being written.
 */

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

      if (file_position + n > file_inode.byte_len)
      {
        // writing past the end of file
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

/**
 * @brief Reads data from a file into a buffer.
 *
 * This function reads up to `n` bytes of data from the file associated with the given
 * I/O interface (`io`) into the provided buffer (`buf`). It searches through the file
 * descriptor table to find the matching I/O interface and reads the file data starting
 * from the current file position.
 *
 * @param io Pointer to the I/O interface associated with the file.
 * @param buf Pointer to the buffer where the read data will be stored.
 * @param n The number of bytes to read from the file.
 * @return The number of bytes read on success, or -1 if an error occurs (e.g., if the
 *         file descriptor is not found or if the file is full).
 */

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
      int curr_pos = 0;
      ioctl(fs_io, IOCTL_GETPOS, &curr_pos);
      console_printf("Seeking to pos: %d\n", curr_pos);
      // Read the inode data
      inode_t file_inode;

      ioread(fs_io, &file_inode, BLOCK_SIZE); // Read the inode data
      // Calculate the number of blocks and bytes to read based on the file position
      uint64_t read_blocks = file_position / BLOCK_SIZE;
      uint64_t read_bytes = file_position % BLOCK_SIZE;

      // Seek to the data block that contains the file data
      data_block_t data_block;
      if (read_blocks == sizeof(file_inode.data_block_num) / sizeof(file_inode.data_block_num[0]))
      {
        return -1;
      }
      // check if the file_position is greater than the file size

      if (file_position > file_inode.byte_len)
      {
        console_printf("File Position: %d\n", file_position);
        return -1;
      }

      ioseek(fs_io, fs_base + BLOCK_SIZE + boot_block.num_inodes * BLOCK_SIZE + file_inode.data_block_num[read_blocks] * BLOCK_SIZE);
      console_printf("Reading from block: %d\n", file_inode.data_block_num[read_blocks]);
      ioctl(fs_io, IOCTL_GETPOS, &curr_pos);
      console_printf("reading from: %d\n", curr_pos);
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
          // Check if the file is full
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
        // console_printf("currently reading at %d\n", read_bytes);
        // console_putchar(data_block.data[read_bytes]);
        read_bytes++;
        bytes_read++;
      }
      // Update the file position after reading
      console_printf("added %d bytes to the buffer\n", bytes_read);
      file->file_position += n;
      return n; // Return the number of bytes read
    }
  }
  // If the file descriptor is not found, return an error
  return -1;
}

/**
 * @brief Perform an I/O control operation on a file.
 *
 * This function iterates through the file descriptor table to find the file
 * associated with the provided I/O interface (`io`). Once found, it performs
 * the specified I/O control command (`cmd`) on the file.
 *
 * @param io Pointer to the I/O interface structure.
 * @param cmd The I/O control command to be performed. Supported commands are:
 *            - IOCTL_GETLEN: Get the length of the file.
 *            - IOCTL_SETPOS: Set the position within the file.
 *            - IOCTL_GETPOS: Get the current position within the file.
 *            - IOCTL_GETBLKSZ: Get the block size of the file.
 * @param arg Pointer to the argument for the I/O control command.
 *
 * @return The result of the I/O control command, or -1 if the command is not supported,
 *         or -ENOTSUP if the provided I/O interface does not match any file.
 */

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

/**
 * @brief Get the length of the file.
 *
 * This function returns the size of the file in bytes.
 *
 * @param file Pointer to the file structure.
 * @param arg Unused argument.
 * @return The size of the file in bytes.
 */
int fs_getlen(file_t *file, void *arg)
{
  return file->file_size;
}
/**
 * @brief Get the current position in the file.
 *
 * This function returns the current position in the file.
 *
 * @param file Pointer to the file structure.
 * @param arg Unused argument.
 * @return The current position in the file.
 */
int fs_getpos(file_t *file, void *arg)
{
  return file->file_position;
}

/**
 * @brief Set the current position in the file.
 *
 * This function sets the current position in the file to the specified value.
 *
 * @param file Pointer to the file structure.
 * @param arg The new position to set in the file.
 * @return Always returns 0.
 */
int fs_setpos(file_t *file, void *arg)
{

  file->file_position = (uint64_t)arg;
  return 0;
}
/**
 * @brief Get the block size of the file system.
 *
 * This function returns the block size of the file system.
 *
 * @param file Pointer to the file structure.
 * @param arg Unused argument.
 * @return The block size of the file system.
 */
int fs_getblksz(file_t *file, void *arg)
{
  return BLOCK_SIZE;
}
