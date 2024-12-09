#ifndef TERMUTILS_H
#define TERMUTILS_H

#include <stddef.h>
#include "syscall.h"
#include "termio.h"
#define BLOCK_SIZE 4096
#define MAX_DIR_ENTRIES 63
#define MAX_INODES 1023
#define BOOT_RESERVED_SPACE_SZ 52
#define MAX_FILE_NAME_LENGTH 32 // 32 bytes
#define DENTRY_RESERVED_SPACE_SZ 28
#define MAX_FILE_OPEN 32
typedef struct dentry_t
{
  char file_name[MAX_FILE_NAME_LENGTH];
  uint32_t inode;
  uint8_t reserved[DENTRY_RESERVED_SPACE_SZ];
} __attribute((packed)) dentry_t;

extern int cat(char *filename);
extern int ls();
extern int edit(char *filename);
#define assert(c)                                                              \
  do                                                                           \
  {                                                                            \
    if (!(c))                                                                  \
    {                                                                          \
      char message[100];                                                       \
      snprintf(message, 100, "Assertion failed at %s:%d", __FILE__, __LINE__); \
      _msgout(message);                                                        \
      _exit();                                                                 \
    }                                                                          \
  } while (0)

#endif // TERMUTILS_H