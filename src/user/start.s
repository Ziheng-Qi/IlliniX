# start.s - User application startup
#

        .text
        .extern _exit
        .type   _exit, @function
        .global _start
        .type   _start, @function
_start:
        la      ra, _exit
        j       main
        .end
