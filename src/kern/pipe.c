#include "lock.h"
#include "io.h"
#include "device.h"
#include "pipe.h"
#include "heap.h"
#include "string.h"

struct pipe {
    struct io_intf io_intf;
    struct lock buf_lock;
    char data[PIPE_SIZE];
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

/**
 * @brief Open a pipe, allow communcation between two processes (from the same fork) if the pipe is created before fork,
 * called on _pipe() syscall
 * @param ioptr the pointer to the io interface to be filled in
 * @return always 0, means success
 */
int pipe_open(struct io_intf ** ioptr) {
    struct pipe * pi;
    pi = kmalloc(sizeof(struct pipe));
    pi->io_intf.ops = &pipe_ops;

    pi->size_read = 0;
    pi->size_written = 0;
    memset(pi->data, 0, PIPE_SIZE);
    lock_init(&pi->buf_lock, "pipe_lock");

    *ioptr = &pi->io_intf;
    (*ioptr)->refcnt = 1;
    return 0;
}

/**
 * @brief Read from pipe buffer, compatible with ioread
 * @param io the io interface in the pipe struct to read from
 * @param buf the buffer to read to
 * @param bufsz the size of the buffer to read, but actually not used, just used to comply with ioread
 * @note This function will block until there is data to read
 * @return number of bytes read, if negative, error code
 */
long pipe_read(struct io_intf * io, void * buf, unsigned long bufsz) {

    lock_acquire(&((struct pipe *)io)->buf_lock); // wait for read to complete
    
    struct pipe * pi = (struct pipe *)io; // io_intf is the first member of struct pipe

    if(bufsz > PIPE_SIZE) {
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

/**
 * @brief Write to a pipe buffer indicated by the io interface in parameter, compatible with iowrite
 * @param io the io interface in the pipe struct to write to
 * @param buf the buffer to write to
 * @param n number of bytes to write
 * @return number of bytes written, if negative, error code
 */
long pipe_write(struct io_intf * io, const void * buf, unsigned long n){
    lock_acquire(&((struct pipe *)io)->buf_lock); // wait for write to complete

    struct pipe * pi = (struct pipe *)io; // io_intf is the first member of struct pipe

    if(n > PIPE_SIZE) {
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

/**
 * @brief Perform ioctl on pipe, only support PIPE_WAIT_EMPTY, compatible with ioctl
 * @param io the io interface in the pipe struct to perform ioctl on
 * @param cmd the command to perform, please include pipe.h for the command
 * @param arg the argument to the command, currently no use
 */
int pipe_ioctl(struct io_intf * io, int cmd, void * arg) {
    struct pipe * pi = (struct pipe *)io; // io_intf is the first member of struct pipe

    if(cmd == PIPE_WAIT_EMPTY){
        if(pi->size_read != pi->size_written)
            condition_wait(&(pi->empty));
        return 0;
    }
    return -ENOTSUP;
}

/**
 * @brief Close the pipe, free the memory, compatible with ioclose
 * @param io the io interface in the pipe struct to close
 */
void pipe_close(struct io_intf * io) {
    struct pipe * pi = (struct pipe *)io; // io_intf is the first member of struct pipe
    lock_acquire(&pi->buf_lock);
    kfree(pi);
}

