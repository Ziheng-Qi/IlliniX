// scnum.h - System call numbers
//

#ifndef _SCNUM_H_
#define _SCNUM_H_

#define SYSCALL_EXIT    0
#define SYSCALL_MSGOUT  1

#define SYSCALL_DEVOPEN 10
#define SYSCALL_FSOPEN  11

#define SYSCALL_CLOSE   20
#define SYSCALL_READ    21
#define SYSCALL_WRITE   22
#define SYSCALL_IOCTL   23

#define SYSCALL_EXEC    30
#define SYSCALL_FORK    31

//            arg is pointer to uint64_t
#define IOCTL_GETLEN 1
//            arg is pointer to uint64_t
#define IOCTL_SETLEN 2
//            arg is pointer to uint64_t
#define IOCTL_GETPOS 3
//            arg is pointer to uint64_t
#define IOCTL_SETPOS 4
//            arg is ignored
#define IOCTL_FLUSH 5
//            arg is pointer to uint32_t
#define IOCTL_GETBLKSZ 6
#endif // _SCNUM_H_
