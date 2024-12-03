#include "syscall.h"
#include "string.h"

static unsigned int fib(unsigned int n);

void main(void) {
    char linebuf[64];
    unsigned int n;

    for (n = 0; n < 60; n++) {
        snprintf(linebuf, sizeof(linebuf), "f(%u) = %u\n", n, fib(n));
        _msgout(linebuf);
    }

    _exit();
}

unsigned int fib(unsigned int n) {
    if (n == 0)
        return 0;
    else if (n == 1)
        return 1;
    else
        return fib(n-2) + fib(n-1);
}
