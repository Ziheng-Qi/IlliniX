#include "scnum.h"

        .text

        .global _exit
        .type   _exit, @function
_exit:
        li      a7, SYSCALL_EXIT
        ecall
        ret

        .global _msgout
        .type   _msgout, @function
_msgout:
        li      a7, SYSCALL_MSGOUT
        ecall
        ret

        .global _devopen
        .type   _devopen, @function
_devopen:
        li      a7, SYSCALL_DEVOPEN
        ecall
        ret

        .global _fsopen
        .type   _fsopen, @function
_fsopen:
        li      a7, SYSCALL_FSOPEN
        ecall
        ret

        .global _close
        .type   _close, @function
_close:
        li      a7, SYSCALL_CLOSE
        ecall
        ret

        .global _read
        .type   _read, @function
_read:
        li      a7, SYSCALL_READ
        ecall
        ret

        .global _write
        .type   _write, @function
_write:
        li      a7, SYSCALL_WRITE
        ecall
        ret

        .global _ioctl
        .type   _ioctl, @function
_ioctl:
        li      a7, SYSCALL_IOCTL
        ecall
        ret

        .global _exec
        .type   _exec, @function
_exec:
        li      a7, SYSCALL_EXEC
        ecall
        ret

        .end
