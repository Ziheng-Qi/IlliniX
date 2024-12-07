//           io.c 
//          

#include "io.h"
#include "error.h"

#include <stddef.h>
#include <string.h>
#include <stdint.h>

//           INTERNAL TYPE DEFINITIONS
//          

struct iovprintf_state {
    struct io_intf * io;
    int err;
};

//           INTERNAL FUNCTION DECLARATIONS
//          

static void ioterm_close(struct io_intf * io);
static long ioterm_read(struct io_intf * io, void * buf, size_t len);
static long ioterm_write(struct io_intf * io, const void * buf, size_t len);
static int ioterm_ioctl(struct io_intf * io, int cmd, void * arg);

static void iovprintf_putc(char c, void * aux);

//           EXPORTED FUNCTION DEFINITIONS
//          

long ioread_full(struct io_intf * io, void * buf, unsigned long bufsz) {
    long cnt, acc = 0;

    if (io->ops->read == NULL)
        return -ENOTSUP;

    while (acc < bufsz) {
        cnt = io->ops->read(io, buf+acc, bufsz-acc);
        if (cnt < 0)
            return cnt;
        else if (cnt == 0)
            return acc;
        acc += cnt;
    }

    return acc;
}

long iowrite(struct io_intf * io, const void * buf, unsigned long n) {
    long cnt, acc = 0;

    if (io->ops->write == NULL)
        return -ENOTSUP;

    while (acc < n) {
        cnt = io->ops->write(io, buf+acc, n-acc);
        if (cnt < 0)
            return cnt;
        else if (cnt == 0)
            return acc;
        acc += cnt;
    }

    return acc;
}

long io_lit_read(struct io_intf *io, void *buf, unsigned long bufsz);
void lit_io_close(struct io_intf *io);
long io_lit_write(struct io_intf *io, const void *buf, unsigned long n);
int io_lit_ioctl(struct io_intf *io, int cmd, void *arg);

//            Initialize an io_lit. This function should be called with an io_lit, a buffer, and the size of the device.
//            It should set up all fields within the io_lit struct so that I/O operations can be performed on the io_lit
//            through the io_intf interface. This function should return a pointer to an io_intf object that can be used
//            to perform I/O operations on the device.

//            An I/O literal object allows a block of memory to be treated as file. I/O
//            operations should be able to be performed via the io_intf associated with
//            the io_lit.

struct io_intf *iolit_init(
    struct io_lit *lit, void *buf, size_t size)
{
    static const struct io_ops ops = {
        .close = lit_io_close,
        .read = io_lit_read,
        .write = io_lit_write,
        .ctl = io_lit_ioctl};
    lit->io_intf.ops = &ops;
    lit->buf = buf;
    lit->size = size;
    lit->pos = 0;
    return &(lit->io_intf);
}

/**
 * @brief Reads data from an io_lit interface into a buffer.
 *
 * This function reads up to `bufsz` bytes from the `io` interface into the
 * provided buffer `buf`. It adjusts the number of bytes read if the end of
 * the buffer is reached.
 *
 * @param io A pointer to the io_intf structure representing the I/O interface.
 * @param buf A pointer to the buffer where the read data will be stored.
 * @param bufsz The maximum number of bytes to read into the buffer.
 * @return Returns 0 on success, or -EINVAL if the end of the buffer is reached.
 */

long io_lit_read(struct io_intf *io, void *buf, unsigned long bufsz)
{
    struct io_lit *lit = (struct io_lit *)io;
    if (lit->pos >= lit->size)
    {
        return -EINVAL; // End of buffer
    }

    size_t bytes_to_read = bufsz;
    if (lit->pos + bufsz > lit->size)
    {
        bytes_to_read = lit->size - lit->pos; // Adjust to remaining bytes
    }

    memcpy(buf, (char *)lit->buf + lit->pos, bytes_to_read);
    lit->pos += bytes_to_read;
    return 0;
}

/**
 * @brief Closes the given I/O interface.
 *
 * This function is a placeholder for closing an I/O interface.
 * Currently, it does not perform any operations.
 *
 * @param io A pointer to the I/O interface to be closed.
 */

void lit_io_close(struct io_intf *io)
{
    // Nothing to do
}

/**
 * @brief Writes data to an io_lit interface.
 *
 * This function writes up to `n` bytes from the buffer `buf` to the io_lit
 * interface `io`. If there is not enough space to write all `n` bytes, it
 * writes as many bytes as possible.
 *
 * @param io Pointer to the io_intf structure representing the io_lit interface.
 * @param buf Pointer to the buffer containing the data to be written.
 * @param n Number of bytes to write from the buffer.
 * @return Returns 0 on success, or -EINVAL if there is no space left to write.
 */

long io_lit_write(struct io_intf *io, const void *buf, unsigned long n)
{
    struct io_lit *lit = (struct io_lit *)io;
    if (lit->pos >= lit->size)
    {
        return -EINVAL; // No space left to write
    }

    size_t bytes_to_write = n;
    if (lit->pos + n > lit->size)
    {
        bytes_to_write = lit->size - lit->pos; // Adjust to remaining space
    }

    memcpy((char *)lit->buf + lit->pos, buf, bytes_to_write);
    lit->pos += bytes_to_write;
    return 0;
}

/**
 * @brief Handle IO control commands for a given IO interface.
 *
 * This function processes various IO control commands (ioctl) for a given
 * IO interface. It supports commands to get the length, set the position,
 * get the position, and get the block size of the IO interface.
 *
 * @param io Pointer to the IO interface structure.
 * @param cmd The ioctl command to be executed.
 * @param arg Pointer to the argument for the ioctl command, used for passing arguments and returning results.
 *
 * @return 0 on success, -1 on unsupported command, or -ENOTSUP if the command is not supported.
 *
 * Supported commands:
 * - IOCTL_GETLEN: Get the length of the IO interface. The length is stored in the location pointed to by arg.
 * - IOCTL_SETPOS: Set the position of the IO interface. The new position is obtained from the location pointed to by arg.
 * - IOCTL_GETPOS: Get the current position of the IO interface. The position is stored in the location pointed to by arg.
 * - IOCTL_GETBLKSZ: Get the block size of the IO interface. The block size (4096) is stored in the location pointed to by arg.
 */
int io_lit_ioctl(struct io_intf *io, int cmd, void *arg)
{

    struct io_lit *lit = (struct io_lit *)io;

    switch (cmd)
    {
    case IOCTL_GETLEN:
        *(uint64_t *)arg = lit->size;
        return 0;
    case IOCTL_SETPOS:
        lit->pos = *(uint64_t *)arg;

        return 0;
    case IOCTL_GETPOS:
        *(uint64_t *)arg = lit->pos;
        return 0;
    case IOCTL_GETBLKSZ:
        *(uint64_t *)arg = 4096;
        return 0;
    default:
        return -1;
    }
    return -ENOTSUP;
}

//           I/O term provides three features:
//          
//               1. Input CRLF normalization. Any of the following character sequences in
//                  the input are converted into a single \n:
//          
//                      (a) \r\n,
//                      (b) \r not followed by \n,
//                      (c) \n not preceeded by \r.
//          
//               2. Output CRLF normalization. Any \n not preceeded by \r, or \r not
//                  followed by \n, is written as \r\n. Sequence \r\n is written as \r\n.
//          
//               3. Line editing. The ioterm_getsn function provides line editing of the
//                  input.
//          
//           Input CRLF normalization works by maintaining one bit of state: cr_in.
//           Initially cr_in = 0. When a character ch is read from rawio:
//           
//           if cr_in = 0 and ch == '\r': return '\n', cr_in <- 1;
//           if cr_in = 0 and ch != '\r': return ch;
//           if cr_in = 1 and ch == '\r': return \n;
//           if cr_in = 1 and ch == '\n': skip, cr_in <- 0;
//           if cr_in = 1 and ch != '\r' and ch != '\n': return ch, cr_in <- 0.
//          
//           Ouput CRLF normalization works by maintaining one bit of state: cr_out.
//           Initially, cr_out = 0. When a character ch is written to I/O term:
//          
//           if cr_out = 0 and ch == '\r': output \r\n to rawio, cr_out <- 1;
//           if cr_out = 0 and ch == '\n': output \r\n to rawio;
//           if cr_out = 0 and ch != '\r' and ch != '\n': output ch to rawio;
//           if cr_out = 1 and ch == '\r': output \r\n to rawio;
//           if cr_out = 1 and ch == '\n': no ouput, cr_out <- 0;
//           if cr_out = 1 and ch != '\r' and ch != '\n': output ch, cr_out <- 0.

struct io_intf * ioterm_init(struct io_term * iot, struct io_intf * rawio) {
    static const struct io_ops ops = {
        .close = ioterm_close,
        .read = ioterm_read,
        .write = ioterm_write,
        .ctl = ioterm_ioctl
    };

    iot->io_intf.ops = &ops;
    iot->rawio = rawio;
    iot->cr_out = 0;
    iot->cr_in = 0;

    return &iot->io_intf;
};

int ioputs(struct io_intf * io, const char * s) {
    const char nl = '\n';
    size_t slen;
    long wlen;

    slen = strlen(s);

    wlen = iowrite(io, s, slen);
    if (wlen < 0)
        return wlen;

    //           Write newline

    wlen = iowrite(io, &nl, 1);
    if (wlen < 0)
        return wlen;
    
    return 0;
}

long ioprintf(struct io_intf * io, const char * fmt, ...) {
	va_list ap;
	long result;

	va_start(ap, fmt);
	result = iovprintf(io, fmt, ap);
	va_end(ap);
	return result;
}

long iovprintf(struct io_intf * io, const char * fmt, va_list ap) {
    //           state.nout is number of chars written or negative error code
    struct iovprintf_state state = { .io = io, .err = 0 };
    size_t nout;

	nout = vgprintf(iovprintf_putc, &state, fmt, ap);
    return state.err ? state.err : nout;
}

char * ioterm_getsn(struct io_term * iot, char * buf, size_t n) {
    char * p = buf;
    int result;
    char c;

    for (;;) {
        //           already CRLF normalized
        c = iogetc(&iot->io_intf);

        switch (c) {
        //           escape
        case '\133':
            iot->cr_in = 0;
            break;
        //           should not happen
        case '\r':
        case '\n':
            result = ioputc(iot->rawio, '\r');
            if (result < 0)
                return NULL;
            result = ioputc(iot->rawio, '\n');
            if (result < 0)
                return NULL;
            *p = '\0';
            return buf;
        //           backspace
        case '\b':
        //           delete
        case '\177':
            if (p != buf) {
                p -= 1;
                n += 1;
                
                result = ioputc(iot->rawio, '\b');
                if (result < 0)
                    return NULL;
                result = ioputc(iot->rawio, ' ');
                if (result < 0)
                    return NULL;
                result = ioputc(iot->rawio, '\b');
            } else
                //           beep
                result = ioputc(iot->rawio, '\a');
            
            if (result < 0)
                return NULL;
            break;

        default:
            if (n > 1) {
                result = ioputc(iot->rawio, c);
                *p++ = c;
                n -= 1;
            } else
                //           beep
                result = ioputc(iot->rawio, '\a');
            
            if (result < 0)
                return NULL;
        }
    }
}

//           INTERNAL FUNCTION DEFINITIONS
//          

void ioterm_close(struct io_intf * io) {
    struct io_term * const iot = (void*)io - offsetof(struct io_term, io_intf);
    ioclose(iot->rawio);
}

long ioterm_read(struct io_intf * io, void * buf, size_t len) {
    struct io_term * const iot = (void*)io - offsetof(struct io_term, io_intf);
    char * rp;
    char * wp;
    long cnt;
    char ch;

    do {
        //           Fill buffer using backing io interface

        cnt = ioread(iot->rawio, buf, len);

        if (cnt < 0)
            return cnt;
        
        //           Scan though buffer and fix up line endings. We may end up removing some
        //           characters from the buffer.  We maintain two pointers /wp/ (write
        //           position) and and /rp/ (read position). Initially, rp = wp, however, as
        //           we delete characters, /rp/ gets ahead of /wp/, and we copy characters
        //           from *rp to *wp to shift the contents of the buffer.
        //           
        //           The processing logic is as follows:
        //           if cr_in = 0 and ch == '\r': return '\n', cr_in <- 1;
        //           if cr_in = 0 and ch != '\r': return ch;
        //           if cr_in = 1 and ch == '\r': return \n;
        //           if cr_in = 1 and ch == '\n': skip, cr_in <- 0;
        //           if cr_in = 1 and ch != '\r' and ch != '\n': return ch, cr_in <- 0.

        wp = rp = buf;
        while ((void*)rp < buf+cnt) {
            ch = *rp++;

            if (iot->cr_in) {
                switch (ch) {
                case '\r':
                    *wp++ = '\n';
                    break;
                case '\n':
                    iot->cr_in = 0;
                    break;
                default:
                    iot->cr_in = 0;
                    *wp++ = ch;
                }
            } else {
                switch (ch) {
                case '\r':
                    iot->cr_in = 1;
                    *wp++ = '\n';
                    break;
                default:
                    *wp++ = ch;
                }
            }
        }

    //           We need to return at least one character, however, it is possible that
    //           the buffer is still empty. (This would happen if it contained a single
    //           '\n' character and cr_in = 1.) If this happens, read more characters.
    } while (wp == buf);

    return (wp - (char*)buf);
}

long ioterm_write(struct io_intf * io, const void * buf, size_t len) {
    struct io_term * const iot = (void*)io - offsetof(struct io_term, io_intf);
    //           how many bytes from the buffer have been written
    long acc = 0;
    //           everything up to /wp/ in buffer has been written out
    const char * wp;
    //           position in buffer we're reading
    const char * rp;
    long cnt;
    char ch;

    //           Scan through buffer and look for cases where we need to modify the line
    //           ending: lone \r and lone \n get converted to \r\n, while existing \r\n
    //           are not modified. We can't modify the buffer, so mwe may need to do
    //           partial writes.
    //           The strategy we want to implement is:
    //           if cr_out = 0 and ch == '\r': output \r\n to rawio, cr_out <- 1;
    //           if cr_out = 0 and ch == '\n': output \r\n to rawio;
    //           if cr_out = 0 and ch != '\r' and ch != '\n': output ch to rawio;
    //           if cr_out = 1 and ch == '\r': output \r\n to rawio;
    //           if cr_out = 1 and ch == '\n': no ouput, cr_out <- 0;
    //           if cr_out = 1 and ch != '\r' and ch != '\n': output ch, cr_out <- 0.

    wp = rp = buf;

    while ((void*)rp < buf+len) {
        ch = *rp++;
        switch (ch) {
        case '\r':
            //           We need to emit a \r\n sequence. If it already occurs in the
            //           buffer, we're all set. Otherwise, we need to write what we have
            //           from the buffer so far, then write \n, and then continue.
            if ((void*)rp < buf+len && *rp == '\n') {
                //           The easy case: buffer already contains \r\n, so keep going.
                iot->cr_out = 0;
                rp += 1;
            } else {
                //           Next character is not '\n' or we're at the end of the buffer.
                //           We need to write out what we have so far and add a \n.
                cnt = iowrite(iot->rawio, wp, rp - wp);
                if (cnt < 0)
                    return cnt;
                else if (cnt == 0)
                    return acc;
                
                acc += cnt;
                wp += cnt;

                //           Now output \n, which does not count toward /acc/.
                cnt = ioputc(iot->rawio, '\n');
                if (cnt < 0)
                    return cnt;
                
                iot->cr_out = 1;
            }
                
            break;
        
        case '\n':
            //           If last character was \r, skip the \n. This should only occur at
            //           the beginning of the buffer, because we check for a \n after a
            //           \r, except if \r is the last character in the buffer. Since we're
            //           at the start of the buffer, we don't have to write anything out.
            if (iot->cr_out) {
                iot->cr_out = 0;
                wp += 1;
                break;
            }
            
            //           Previous character was not \r, so we need to write a \r first,
            //           then the rest of the buffer. But before that, we need to write
            //           out what we have so far, up to, but not including the \n we're
            //           processing.
            if (wp != rp-1) {
                cnt = iowrite(iot->rawio, wp, rp-1 - wp);
                if (cnt < 0)
                    return cnt;
                else if (cnt == 0)
                    return acc;
                acc += cnt;
                wp += cnt;
            }
            
            cnt = ioputc(iot->rawio, '\r');
            if (cnt < 0)
                return cnt;
            
            //           wp should now point to \n. We'll write it when we drain the
            //           buffer later.

            iot->cr_out = 0;
            break;
            
        default:
            iot->cr_out = 0;
        }
    }

    if (rp != wp) {
        cnt = iowrite(iot->rawio, wp, rp - wp);

        if (cnt < 0)
            return cnt;
        else if (cnt == 0)
            return acc;
        acc += cnt;
    }

    return acc;
}

int ioterm_ioctl(struct io_intf * io, int cmd, void * arg) {
    struct io_term * const iot = (void*)io - offsetof(struct io_term, io_intf);

    //           Pass ioctls through to backing io interface. Seeking is not supported,
    //           because we maintain state on the characters output so far.
    if (cmd != IOCTL_SETPOS)
        return ioctl(iot->rawio, cmd, arg);
    else
        return -ENOTSUP;
}

void iovprintf_putc(char c, void * aux) {
    struct iovprintf_state * const state = aux;
    int result;

    if (state->err == 0) {
        result = ioputc(state->io, c);
        if (result < 0)
            state->err = result;
    }
}
