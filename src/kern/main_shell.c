//           main.c - Main function: runs shell to load executable
//          

#include "console.h"
#include "thread.h"
#include "device.h"
#include "uart.h"
#include "timer.h"
#include "intr.h"
#include "heap.h"
#include "virtio.h"
#include "halt.h"
#include "elf.h"
#include "fs.h"
#include "stdlib.h"
#include "string.h"
#include "fs.h" // Assuming boot_block is defined in fs.h

extern struct boot_block_t boot_block; // Ensure boot_block is declared

//           end of kernel image (defined in kernel.ld)
extern char _kimg_end[];

#define RAM_SIZE (8*1024*1024)
#define RAM_START 0x80000000UL
#define KERN_START RAM_START
#define USER_START 0x80100000UL

#define UART0_IOBASE 0x10000000
#define UART1_IOBASE 0x10000100
#define UART0_IRQNO 10

#define VIRT0_IOBASE 0x10001000
#define VIRT1_IOBASE 0x10002000
#define VIRT0_IRQNO 1

static void shell_main(struct io_intf * termio);

void main(void) {
    struct io_intf * termio;
    struct io_intf * blkio;
    void * mmio_base;
    int result;
    int i;

    

    heap_init(_kimg_end, (void*)USER_START);

    //           Attach NS16550a serial devices

    for (i = 0; i < 2; i++) {
        mmio_base = (void*)UART0_IOBASE;
        mmio_base += (UART1_IOBASE-UART0_IOBASE)*i;
        uart_attach(mmio_base, UART0_IRQNO+i);
    }
    
    //           Attach virtio devices

    for (i = 0; i < 8; i++) {
        mmio_base = (void*)VIRT0_IOBASE;
        mmio_base += (VIRT1_IOBASE-VIRT0_IOBASE)*i;
        virtio_attach(mmio_base, VIRT0_IRQNO+i);
    }

    intr_enable();
    timer_start();

   result = device_open(&blkio, "blk", 0);

    if (result != 0)
        panic("device_open failed");
    
    result = fs_mount(blkio);

    debug("Mounted blk0");

    if (result != 0)
        panic("fs_mount failed");

    //           Open terminal for trek

    result = device_open(&termio, "ser", 1);

    if (result != 0)
        panic("Could not open ser1");
    
    shell_main(termio);
}

void shell_main(struct io_intf * termio_raw) {
    struct io_term ioterm;
    struct io_intf * termio;
    void (*exe_entry)(struct io_intf*);
    struct io_intf * exeio;
    char cmdbuf[128];
    int tid;
    int result;

    termio = ioterm_init(&ioterm, termio_raw);

    ioputs(termio, "Enter executable name or \"exit\" to exit");
    

    for (;;) {
        ioprintf(termio, "CMD> ");
        ioterm_getsn(&ioterm, cmdbuf, sizeof(cmdbuf));

        char *argv[10];
        int argc = 0;
        char *token = strtok(cmdbuf, " ");

        while (token != NULL && argc < 10)
        {
            argv[argc++] = token;
            token = strtok(NULL, " ");
        }
        argv[argc] = NULL;

        if (cmdbuf[0] == '\0')
        {
            ioprintf(termio, "Enter a command\n");
            ioprintf(termio, "Usage: exec <filename>\n");
            ioprintf(termio, "Usage: cat <filename>\n");
            ioprintf(termio, "Usage: write <filename> <startpos>\n");
            continue;
        }

        if (strcmp("exit", argv[0]) == 0)
            return;

        char *cmd = argv[0];

        if (strcmp("exec", cmd) == 0)
        {
            if (argc < 2)
            {
                ioputs(termio, "Usage: exec <filename>");
                continue;
            }

            result = fs_open(argv[1], &exeio);

            if (result != 0)
            {
                ioputs(termio, "Could not open file");
                continue;
            }

            result = elf_load(exeio, &exe_entry);

            if (result != 0)
            {
                ioputs(termio, "Could not load executable");
                continue;
            }

            tid = thread_spawn(argv[1], exe_entry, termio);
            ioclose(exeio);
            ioprintf(termio, "Spawned thread %d\n", tid);
            if (tid < 0)
            {
                ioputs(termio, "Could not spawn thread");
                continue;
            }
            else
            {
                thread_join(tid);
            }
        }
        else if (strcmp("cat", cmd) == 0)
        {
            if (argc < 2)
            {
                ioputs(termio, "Usage: cat <filename>");
                continue;
            }
            else
            {
                struct io_intf *fs_io;
                result = fs_open(argv[1], &fs_io);
                if (result < 0)
                {
                    ioputs(termio, "Could not open file");
                    continue;
                }
                size_t fil_sz = 0;
                result = ioctl(fs_io, IOCTL_GETLEN, &fil_sz);
                if (result != 0)
                {
                    ioputs(termio, "Could not get file size");
                    continue;
                }
                char buf[fil_sz + 1];
                result = ioread_full(fs_io, &buf, fil_sz);
                if (result < 0)
                {
                    ioputs(termio, "Could not read file");
                    continue;
                }
                buf[fil_sz] = '\0';
                ioprintf(termio, "%s\n", buf);
                ioclose(fs_io);
            }
        }
        else if (strcmp("write", cmd) == 0)
        {
            if (argc < 3)
            {
                /*argv: `cmd` `file_name` `startpos`*/
                ioputs(termio, "Usage: write <filename> <data>");
                continue;
            }
            else
            {
                struct io_intf *fs_io;
                result = fs_open(argv[1], &fs_io);
                if (result < 0)
                {
                    ioputs(termio, "Could not open file");
                    continue;
                }
                size_t fil_sz = 0;
                result = ioctl(fs_io, IOCTL_GETLEN, &fil_sz);
                if (result != 0)
                {
                    ioputs(termio, "Could not get file size");
                    continue;
                }
                size_t startpos = atoi(argv[2]);
                ioprintf(termio, "Enter txt from position %d:\n", startpos);
                char data[fil_sz - startpos];
                result = ioseek(fs_io, startpos);
                if (result < 0)
                {
                    ioputs(termio, "Could not seek to position");
                    continue;
                }
                ioterm_getsn(&ioterm, data, sizeof(data));
                result = iowrite(fs_io, data, sizeof(data));
                if (result < 0)
                {
                    ioputs(termio, "Could not write to file");
                    continue;
                }
                char buf[fil_sz + 1];
                size_t pos = 0;
                result = ioseek(fs_io, pos);
                if (result < 0)
                {
                    ioputs(termio, "Could not set position");
                    continue;
                }
                result = ioread_full(fs_io, &buf, fil_sz);
                if (result < 0)
                {
                    ioputs(termio, "Could not read file");
                    continue;
                }
                buf[fil_sz] = '\0';
                ioprintf(termio, "%s\n", buf);

                ioclose(fs_io);
            }
        }
        else
        {
            ioprintf(termio, "Unknown command: %s\n", cmd);
        }
    }
}
