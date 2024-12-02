// syscall.h - System calls
//

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <stddef.h>

extern void __attribute__ ((noreturn)) _exit(void);
extern void _msgout(const char * msg);
extern int _close(int fd);
extern long _read(int fd, void * buf, size_t bufsz);
extern long _write(int fd, const void * buf, size_t len);
extern int _ioctl(int fd, const int cmd, void * arg);
extern int _devopen(int fd, const char * name, int instno);
extern int _fsopen(int fd, const char * name);
extern int _exec(int fd);

#endif // _SYSCALL_H_
