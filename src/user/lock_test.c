#include "syscall.h"
#include "string.h"
#include "stdlib.h"
#include "termutils.h"

void main() {
    int result;
    char str[10];
    int i;
    result = _fsopen(0, "ioctl.txt");
    assert(result >= 0);
    size_t ref;
    // result = _ioctl(0, IOCTL_GETREFCNT, &ref);
    if (result < 0)
    {
        _msgout("_fsioctl failed");
        _exit();
    }
    char ref_str[10];
    itoa(ref, ref_str, 10);
    // _msgout("Ref count before fork:");
    // _msgout(ref_str);
    if (result < 0)
    {
        _msgout("_ioctl failed");
        _exit();
    }
    int tid;
    tid = _fork();
    assert(tid >= 0);
    if (tid == 1)
    {

        size_t ref;
        result = _ioctl(0, IOCTL_GETREFCNT, &ref);
        if (result < 0)
        {
            _msgout("_fsioctl failed");
            _exit();
        }
        char ref_str[10];
        itoa(ref, ref_str, 10);
        _msgout("Ref count after fork 1:");
        _msgout(ref_str);
        assert(ref == 2); // after fork, ref count should be 2 for the file "ioctl.txt"
    }
    if (tid == 0)
    {
        size_t ref;
        // result = _ioctl(0, IOCTL_GETREFCNT, &ref);
        if (result < 0)
        {
            _msgout("_fsioctl failed");
            _exit();
        }
        char ref_str[10];
        itoa(ref, ref_str, 10);
        _msgout("Ref count after fork 2:");
        _msgout(ref_str);
        assert(ref == 2); // after fork, ref count should be 2 for the file "ioctl.txt"
    }
    if (tid)
    {
        assert(tid == 1);
        // size_t pos = 8;
        // result = _ioctl(0, IOCTL_SETPOS, &pos);
        char pos_str[10];
        size_t ref_cnt = 0;
        result = _ioctl(0, IOCTL_GETREFCNT, &ref_cnt);
        if (result < 0)
        {
            _msgout("_fsioctl failed");
            _exit();
        }
        char ref_str[10];
        itoa(ref_cnt, ref_str, 10);
        // _msgout("Ref count at parent before exit:");
        // _msgout(ref_str);
        // assert(ref_cnt == 2); // after fork, ref count should be 2 for the file "ioctl.txt"
        for (i = 4; i < 8; i++)
        {
            itoa(i, str, 10);
            _msgout("Parent writes line:");
            // _msgout(str);
            size_t pos;
            result = _ioctl(0, IOCTL_GETPOS, &pos);
            if (result < 0)
            {
                itoa(result, pos_str, 10);

                _msgout("_ioctl failed");
                _msgout(str);
                _exit();
            }
            // _msgout("Current position:");
            // itoa(pos, pos_str, 10);
            // _msgout(pos_str);
            result = _write(0, str, 1);
            if (result < 0)
            {
                itoa(result, str, 10);

                _msgout("_write failed");
                _msgout(str);
                _exit();
            }
        }
        // _close(0);
        _wait(1);
        // _close(0);
        size_t ref;
        result = _ioctl(0, IOCTL_GETREFCNT, &ref);
        assert(ref == 1); // after child exits, ref count should be 1 for the file "ioctl.txt"
        char read_buf[256];
        result = _fsopen(1, "ioctl.txt");
        _read(1, read_buf, sizeof(read_buf));
        _msgout("File contents:\n");
        _msgout(read_buf);
        // _close(0);
        _exit();
    }
    else
    {
        size_t ref;
        result = _ioctl(0, IOCTL_GETREFCNT, &ref);
        if (result < 0)
        {
            _msgout("_fsioctl failed");
            _exit();
        }
        char ref_str[10];
        itoa(ref, ref_str, 10);
        _msgout("Ref count at child:");
        _msgout(ref_str);
        for (i = 1; i < 4; i++)
        {
            itoa(i, str, 10);
            _msgout("Child writes line:");
            // _msgout(str);
            result = _write(0, str, 1);
            if (result < 0)
            {
                itoa(result, str, 10);

                _msgout("_write failed");
                _msgout(str);
                _exit();
            }
        }
        _exit();
    }
}