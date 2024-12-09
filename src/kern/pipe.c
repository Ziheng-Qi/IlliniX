#include "lock.h"
#include "io.h"
#include "device.h"
#include "heap.h"
#include "string.h"

#define PIPESIZE 512
#define PIPE_WAIT_EMPTY 8

struct pipe {
    struct io_intf io_intf;
    struct lock buf_lock;
    char data[PIPESIZE];
    size_t size_read;
    size_t size_written;
    struct condition not_empty;
    struct condition empty;
};


// INTERNAL FUNCTION DEFINITIONS
// 


int pipe_open(struct io_intf ** ioptr);
static void pipe_close(struct io_intf * io);
static long pipe_read(struct io_intf * io, void * buf, unsigned long bufsz);
static long pipe_write(struct io_intf * io, const void * buf, unsigned long n);
static int pipe_ioctl(struct io_intf * io, int cmd, void * arg);


// EXPORTED FUNCTION DEFINITIONS
static const struct io_ops pipe_ops = {
    .close = pipe_close,
    .read = pipe_read,
    .write = pipe_write,
    .ctl = pipe_ioctl
};

int pipe_open(struct io_intf ** ioptr) {
    struct pipe * pi;
    pi = kmalloc(sizeof(struct pipe));
    pi->io_intf.ops = &pipe_ops;

    pi->size_read = 0;
    pi->size_written = 0;
    memset(pi->data, 0, PIPESIZE);
    lock_init(&pi->buf_lock, "pipe_lock");

    *ioptr = &pi->io_intf;
    (*ioptr)->refcnt = 1;
    return 0;
}

long pipe_read(struct io_intf * io, void * buf, unsigned long bufsz) {

    lock_acquire(&((struct pipe *)io)->buf_lock); // wait for read to complete
    
    struct pipe * pi = (struct pipe *)io; // io_intf is the first member of struct pipe

    if(bufsz > PIPESIZE) {
        lock_release(&pi->buf_lock);
        return -EINVAL;
    }

    size_t initial_size_read = pi->size_read;
    if(pi->size_read == pi->size_written) {
        lock_release(&pi->buf_lock);
        condition_wait(&pi->not_empty);
        lock_acquire(&pi->buf_lock);
    }

    while(pi->size_read != pi->size_written) {
        // copy data from pipe to buf
        for(size_t i = 0; i < pi->size_written-pi->size_read; i++) {
            ((char *)buf)[i] = pi->data[i];
            pi->size_read ++;
        }
    }

    condition_broadcast(&pi->empty);
    lock_release(&pi->buf_lock);
    return pi->size_read - initial_size_read;    
}

long pipe_write(struct io_intf * io, const void * buf, unsigned long n){
    lock_acquire(&((struct pipe *)io)->buf_lock); // wait for write to complete

    struct pipe * pi = (struct pipe *)io; // io_intf is the first member of struct pipe

    if(n > PIPESIZE) {
        lock_release(&pi->buf_lock);
        return -EINVAL;
    }

    if(pi->size_read != pi->size_written) { // reader has not read previous data yet
        lock_release(&pi->buf_lock);
        condition_wait(&pi->empty);
        lock_acquire(&pi->buf_lock);
    }

    for(size_t i = 0; i < n; i++) {
        pi->data[i] = ((char *)buf)[i];
        pi->size_written ++;
    }

    condition_broadcast(&pi->not_empty);
    lock_release(&pi->buf_lock);
    return n;
}

int pipe_ioctl(struct io_intf * io, int cmd, void * arg) {
    struct pipe * pi = (struct pipe *)io; // io_intf is the first member of struct pipe

    if(cmd == PIPE_WAIT_EMPTY){
        if(pi->size_read != pi->size_written)
            condition_wait(&(pi->empty));
        return 0;
    }
    return -ENOTSUP;
}

void pipe_close(struct io_intf * io) {
    struct pipe * pi = (struct pipe *)io; // io_intf is the first member of struct pipe
    lock_acquire(&pi->buf_lock);
    kfree(pi);
}

