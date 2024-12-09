
#include "syscall.h"
#include "string.h"

void main(void) {

    _usleep(3000000);
    _msgout("Hello, world!");

    int c = 0;
    
    (void) c;
}
