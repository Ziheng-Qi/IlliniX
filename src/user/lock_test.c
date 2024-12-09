#include "syscall.h"
#include "string.h"
#include "stdlib.h"

void main() {
    int result;
    char str[10];
    int i;

    if (_fork() == 0)
    {
        result = _fsopen(0, "ioctl.txt");

        if (result < 0)
        {
            _msgout("_fsopen failed");
            _exit();
        }
        size_t ref;
        result = _ioctl(0, IOCTL_GETREFCNT, &ref);
        char ref_str[10];
        itoa(ref, ref_str, 10);
        _msgout("Ref count:");
        _msgout(ref_str);
        for (i = 5; i < 10; i++)
        {
            itoa(i, str, 10);
            // _msgout("Child writes line:");
            // _msgout(str);
            _write(0, str, 1);
        }
        _close(0);
        _exit();
    }
    else
    {
        // result = _ioctl(0, IOCTL_SETPOS, 4);
        // if (result < 0)
        // {
        //     _msgout("_fsioctl failed");
        //     _exit();
        // }
        result = _fsopen(0, "ioctl.txt");

        if (result < 0)
        {
            _msgout("_fsopen failed");
            _exit();
        }

        for (i = 1; i < 10; i++)
        {
            itoa(i, str, 10);
            // _msgout("Parent writes line:");
            // _msgout(str);
            _write(0, str, 1);
        }
        _wait(0);
        // _close(0);
        size_t ref = 0;
        result = _ioctl(0, IOCTL_GETREFCNT, &ref);
        char ref_str[10];
        itoa(ref, ref_str, 10);
        // _msgout("Ref count:");
        // _msgout(ref_str);
        char read_buf[256];
        result = _fsopen(1, "ioctl.txt");
        _read(1, read_buf, sizeof(read_buf));
        _msgout("File contents:\n");
        _msgout(read_buf);
        // _close(0);
        _exit();
    }
}