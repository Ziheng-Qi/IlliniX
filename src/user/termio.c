// termio.c - Terminal I/O
//

#include "termio.h"
#include "syscall.h"
#include "string.h"

static char getchar_raw(void);
static void putchar_raw(char c);

char getchar_raw(void)
{
    long n;
    char c;

    n = _read(0, &c, 1);

    if (n != 1)
    {
        _msgout("getchar_raw() failed");
        _exit();
    }

    return c;
}

char getchar(void)
{
    static char cprev = '\0';
    char c;

    // Convert \r followed by any number of \n to just \n

    do
    {
        c = getchar_raw();
    } while (c == '\n' && cprev == '\r');

    cprev = c;

    if (c == '\r')
        return '\n';
    else
        return c;
}

void putchar_raw(char c)
{
    long n;

    n = _write(0, &c, 1);

    if (n != 1)
        _exit();
}

void putchar(char c)
{
    // Convert \n to \r\n

    if (c == '\n')
        putchar_raw('\r');
    putchar_raw(c);
}

void puts(const char *s)
{
    const char *start;

    // Try to minimize the number of device_write calls by writing multiple
    // characters that are not \n at once.

    if (s != NULL)
    {
        for (;;)
        {
            start = s;
            while (*s != '\0' && *s != '\n')
                s += 1;
            if (s != start)
                _write(0, start, s - start);
            _write(0, "\r\n", 2);
            if (*s == '\0')
                break;
            s += 1;
        }
    }
}

char *getsn(char *buf, size_t n)
{
    char *p = buf;
    char c;

    for (;;)
    {
        c = getchar();

        switch (c)
        {
        case '\r':
            break;
        case '\n':
            putchar('\n');
            *p = '\0';
            return buf;
        case '\b':
        case '\177':
            if (p != buf)
            {
                putchar('\b');
                putchar(' ');
                putchar('\b');

                p -= 1;
                n += 1;
            }
            break;
        default:
            if (n > 1)
            {
                putchar(c);
                *p++ = c;
                n -= 1;
            }
            else
                putchar('\a'); // bell
            break;
        }
    }
}

void printf(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vgprintf((void *)putchar, NULL, fmt, ap);
    va_end(ap);
}

void wfent()
{
    char c;

    c = ' ';
    while (1)
    {
        c = getchar();
        if (c == '\n')
        {
            break;
        }
    }
}