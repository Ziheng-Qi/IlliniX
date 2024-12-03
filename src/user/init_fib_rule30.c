#include "syscall.h"
#include "string.h"

void main(void) {
    int result;

    if (_fork()) {
        // exec fibonacci

        result = _fsopen(1, "fib");

        if (result < 0) {
            _msgout("_fsopen failed");
            _exit();
        }

        _exec(1);
    
    } else {
        // Open ser1 device as fd=0
        result = _devopen(0, "ser", 1);

        if (result < 0) {
            _msgout("_devopen failed");
            _exit();
        }

        // exec rule30

        result = _fsopen(1, "rule30");

        if (result < 0) {
            _msgout("_fsopen failed");
            _exit();
        }

        _exec(1);
    }
}

