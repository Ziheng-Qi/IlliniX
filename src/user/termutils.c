#include "termutils.h"
#include "error.h"
#include "syscall.h"
#include "string.h"
#include "stdlib.h"
#include "termio.h"

int cat(char *filename)
{
  // _close(1);
  int result = _fsopen(1, filename);
  if (result < 0)
  { // Print error code
    if (result == -ENOENT)
      printf("%s: File not found\n", filename);
    else
      printf("%s: Error %d\n", filename, result);
    return result;
  }
  size_t n;
  result = _ioctl(1, IOCTL_GETLEN, &n);
  if (result < 0)
  {
    puts("Failed to get file length");
    printf("Error %d\n", -result);
    return result;
  }
  // n += 1;
  char cat_buf[n];
  result = _read(1, cat_buf, n);
  if (result < 0)
  {
    puts("Failed to read file");
    printf("Error %d\n", -result);
    return result;
  }
  puts(cat_buf);
  puts("\n");
  _close(1);
  return 0;
}

int ls()
{
  _fsopen(1, "shell");
  size_t den_num;
  _ioctl(1, IOCTL_GETDENTRY_NUM, &den_num);
  if (den_num == 0)
  {
    puts("No files in directory");
    _close(1);
    return 0;
  }

  dentry_t dir_entries[den_num];
  int result = _ioctl(1, IOCTL_GETDENTRY, dir_entries);
  if (result < 0)
  {
    puts("Failed to get directory entries");
    printf("Error %d\n", -result);
    return result;
  }
  for (int i = 0; i < den_num; i++)
  {
    // _msgout(dir_entries[i].file_name);
    puts(dir_entries[i].file_name);
    // puts("\n");
  }
  _close(1);
  return 0;
}

int edit(char *filename)
{
  int result = _fsopen(1, filename);
  if (result < 0)
  { // Print error code
    if (result == -ENOENT)
      printf("%s: File not found\n", filename);
    else
      printf("%s: Error %d\n", filename, -result);
    return result;
  }
  size_t n;
  result = _ioctl(1, IOCTL_GETLEN, &n);
  if (result < 0)
  {
    puts("Failed to get file length");
    printf("Error %d\n", -result);
    return result;
  }
  n += 1;
  char edit_buf[n];
  result = _read(1, edit_buf, n - 1);
  if (result < 0)
  {
    puts("Failed to read file");
    printf("Error %d\n", -result);
    return result;
  }
  edit_buf[n] = '\0';
  while (1)
  {
    char buf[256];
    getsn(buf, 256);
    if (strcmp(buf, "^[[C"))
    {
      // move cursor right
      size_t pos;
      _ioctl(1, IOCTL_GETPOS, &pos);
      pos += 1;
      _ioctl(1, IOCTL_SETPOS, &pos);
    }
    if (strcmp(buf, "^[[D"))
    {
      // move cursor left
      size_t pos;
      _ioctl(1, IOCTL_GETPOS, &pos);
      pos -= 1;
      _ioctl(1, IOCTL_SETPOS, &pos);
    }
    // other cases just normally write to the file
    result = _write(1, buf, strlen(buf));
    if (result < 0)
    {
      puts("Failed to write to file");
      printf("Error %d\n", -result);
      return result;
    }
    if (strcmp(buf, "q"))
    {
      _write(1, edit_buf, n);
      break;
    }
  }

  _close(1);
  return 0;
}