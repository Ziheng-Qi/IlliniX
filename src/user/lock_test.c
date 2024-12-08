#include "syscall.h"
#include "string.h"
#include "stdlib.h"

void main() {
    int result;
    char str[10];
    int i;

    result = _fsopen(0, "ioctl.txt");

    if (result < 0) {
        _msgout("_fsopen failed");
        _exit();
    }

    if (_fork() == 0) {
        for (i = 1; i < 4; i++) {
            itoa(i, str, 10);
            _msgout("Child writes line:");
            _msgout(str);
            _write(0, str, 1);
        }
        _close(0);
        _exit();
    } else {
        for (i = 1; i < 4; i++) {
            itoa(i, str, 10);
            _msgout("Parent writes line:");
            _msgout(str);
            _write(0, str, 1);
        }
        _wait(0);
        char read_buf[256];
        result = _fsopen(0, "ioctl.txt");
        _read(0, read_buf, sizeof(read_buf));
        _msgout("File contents:\n");
        _msgout(read_buf);
        _close(0);
        _exit();
    }
}