#include "syscall.h"
#include "string.h"
#include "stdint.h"

#define BLOCK_SIZE 4096
#define MAX_DIR_ENTRIES 63
#define MAX_INODES 1023
#define BOOT_RESERVED_SPACE_SZ 52
#define MAX_FILE_NAME_LENGTH 32 // 32 bytes
#define DENTRY_RESERVED_SPACE_SZ 28
#define MAX_FILE_OPEN 32
#define INUSE 1
#define UNUSE 0

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

void main(void)
{

  // list all files in the current file system
  int result;

  result = _devopen(0, "blk", 0);

  if (result != 0)
  {
    _msgout("Error opening device\n");
    _exit();
  }

  boot_block_t *boot_block;

  result = _read(0, boot_block, sizeof(boot_block_t));

  if (result != sizeof(boot_block_t))
  {
    _msgout("Error reading boot block\n");
    _exit();
  }

  for (int i = 0; i < boot_block->num_dentry; i++)
  {
    char *file_name = boot_block->dir_entries[i].file_name;
    _msgout(file_name);
    _msgout(" ");
  }
  _msgout("\n");

  _exit();
}