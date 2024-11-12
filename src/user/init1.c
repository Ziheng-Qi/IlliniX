
#include "syscall.h"
#include "string.h"

void main(void) {
    const char * const greeting = "Hello, world!\r\n";
    size_t slen;
    int result;

    // Open ser1 device as fd=0

    result = _devopen(0, "ser", 1);

    if (result < 0)
        return;

    slen = strlen(greeting);
    _write(0, greeting, slen);

    _close(0);
}
