#include "syscall.h"
#include "string.h"
#include "stdlib.h"
#include "termio.h"
#include "termutils.h"

void main()
{
  int result;
  char str[10];
  int i;

  result = _fsopen(0, "helloworld.txt");
  assert(result >= 0);
  if (_fork())
  {
    _close(0);
    _msgout("file closed by parent");
    _wait(1);
    _exit();
  }
  else
  {
    size_t size;
    result = _ioctl(0, IOCTL_GETLEN, &size);
    assert(size != 0);
    char read_buf[size];
    result = _read(0, &read_buf, size);
    assert(result == size);
    _msgout("File contents before write:");
    _msgout(read_buf);
    size_t pos = 0;
    result = _ioctl(0, IOCTL_SETPOS, &pos);
    assert(result >= 0);
    for (i = 0; i < 10; i++)
    {
      itoa(i, str, 10);
      result = _write(0, str, 1);
      assert(result == 1);
    }
    // size_t pos = 0;
    result = _ioctl(0, IOCTL_SETPOS, &pos);
    assert(result >= 0);
    result = _read(0, &read_buf, size);
    assert(result == size);
    _msgout("File contents after write 0-9 to start:");
    _msgout(read_buf);
  }
}