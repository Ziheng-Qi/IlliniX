#include "syscall.h"
#include "string.h"
#include "scnum.h"
#include "stdlib.h"

void main(void)
{
    int result;

    // Open ser1 device as fd=0
    char c;
    result = _devopen(0, "ser", 1);

    if (result < 0)
    {
        _msgout("_devopen failed");
        _exit();
    }
    int fd = _fsopen(1, "helloworld.txt");
    if (fd < 0)
    {
        _msgout("_fsopen failed");
        _exit();
    }
    size_t len = 0;
    result = _ioctl(fd, IOCTL_GETLEN, &len);
    if (result < 0)
    {
        _msgout("ioctl failed");
        _exit();
    }
    char num[10];
    itoa(len, num, 10);
    _msgout("Length of file: ");
    _msgout(num);
    // len = 456;
    size_t blksz;
    result = _ioctl(fd, IOCTL_GETBLKSZ, &blksz);
    if (result < 0)
    {
        _msgout("ioctl failed");
        _exit();
    }
    itoa(blksz, num, 10);
    _msgout("Block size: ");
    _msgout(num);
    char buf[len];
    c = ' ';
    while (1)
    {
        _read(0, &c, 1);
        if (c == '\r')
        {
            break;
        }
    }

    result = _read(fd, buf, len);
    if (result < 0)
    {
        _msgout("read failed");
        _exit();
    }
    _msgout("read successful");

    result = _write(0, buf, len);
    if (result < 0)
    {
        _msgout("write failed");
        _exit();
    }
    _write(0, "\n\r", 2);
    size_t pos = 0;
    _ioctl(fd, IOCTL_SETPOS, &pos);

    result = _read(fd, buf, len);

    c = ' ';
    while (1)
    {
        _read(0, &c, 1);
        if (c == '\r')
        {
            break;
        }
    }

    if (result < 0)
    {
        _msgout("read failed");
        _exit();
    }

    result = _write(0, buf, len);
    _write(0, "\n\r", 2);
    if (result < 0)
    {
        _msgout("write failed");
        _exit();
    }

    c = ' ';
    while (1)
    {
        _read(0, &c, 1);
        if (c == '\r')
        {
            break;
        }
    }

    _close(fd);
}
