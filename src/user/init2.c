
#include "syscall.h"
#include "string.h"

void main(void) {
    int result;

    // Open ser1 device as fd=0

    result = _devopen(0, "ser", 1);

    if (result < 0) {
        _msgout("_devopen failed");
        _exit();
    }

    // ... run trek

    result = _fsopen(1, "trek");

    if (result < 0) {
        _msgout("_fsopen failed");
        _exit();
    }

    _exec(1);
}
