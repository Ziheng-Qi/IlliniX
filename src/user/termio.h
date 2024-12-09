// termio.h - Terminal I/O
//

#ifndef _TERMIO_H_
#define _TERMIO_H_

#include <stddef.h>

extern char getchar(void);
extern void putchar(char c);
extern void puts(const char *s);
extern char *getsn(char *buf, size_t n);
extern void printf(const char *fmt, ...);
extern void wfent();

#endif // _TERMIO_H_