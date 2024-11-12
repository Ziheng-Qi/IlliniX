# start.s - User application startup
#

        .text

        .global _start
        .type   _start, @function
_start:
        la      ra, _exit
        j       main
        .end
