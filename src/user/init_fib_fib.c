#include "syscall.h"
#include "string.h"

void main(void) {
    int result;

    _fork();
    
    result = _fsopen(0, "fib");

    if (result < 0) {
        _msgout("_fsopen failed");
        _exit();
    }

    _exec(0);
}
