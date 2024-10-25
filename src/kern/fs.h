//           fs.h - File system interface
//          

#ifndef _FS_H_
#define _FS_H_

#include "io.h"

extern char fs_initialized;

extern void fs_init(void);
extern int fs_mount(struct io_intf * blkio);
extern int fs_open(const char * name, struct io_intf ** ioptr);

//           _FS_H_
#endif
