#ifndef _PIPE_H_
#define _PIPE_H_
#include "io.h"

#define PIPE_SIZE 512
#define PIPE_WAIT_EMPTY 8

extern int pipe_open(struct io_intf ** ioptr);

// _PIPE_H_
#endif