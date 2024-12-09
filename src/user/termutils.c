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
  puts(edit_buf);
  char *new_buf = getsn(edit_buf, n);
  result = _write(1, new_buf, n);
  if (result < 0)
  {
    puts("Failed to write file");
    printf("Error %d\n", -result);
    return result;
  }
  _close(1);
  return 0;
}