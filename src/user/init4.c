#include "syscall.h"
#include "string.h"

void main(void) {
    const char * const greeting = "Hello, world!\r\n";
    size_t slen;
    int result;

    struct io_intf* fs_io1;
    result = _fsopen("ioctl.txt", fs_io1);
    if (result < 0)
        return;

    slen = strlen(greeting);
    _write(0, greeting, slen);
    
    _close(0);
}