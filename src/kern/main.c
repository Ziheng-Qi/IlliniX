// main.c - Main function: runs shell to load executable
//

#ifdef MAIN_TRACE
#define TRACE
#endif

#ifdef MAIN_DEBUG
#define DEBUG
#endif

#define INIT_PROC "fib" // name of init process executable

#include "console.h"
#include "thread.h"
#include "device.h"
#include "uart.h"
#include "timer.h"
#include "intr.h"
#include "memory.h"
#include "heap.h"
#include "virtio.h"
#include "halt.h"
#include "elf.h"
#include "fs.h"
#include "string.h"
#include "process.h"
#include "config.h"


void main(void) {
    struct io_intf * initio;
    struct io_intf * blkio;
    void * mmio_base;
    int result;
    int i;

    console_init();
    memory_init();
    intr_init();
    devmgr_init();
    thread_init();
    procmgr_init();
    timer_init();

    // Attach NS16550a serial devices

    for (i = 0; i < 3; i++) {
        mmio_base = (void*)UART0_IOBASE;
        mmio_base += (UART1_IOBASE-UART0_IOBASE)*i;
        uart_attach(mmio_base, UART0_IRQNO+i);
    }
    
    // Attach virtio devices

    for (i = 0; i < 8; i++) {
        mmio_base = (void*)VIRT0_IOBASE;
        mmio_base += (VIRT1_IOBASE-VIRT0_IOBASE)*i;
        virtio_attach(mmio_base, VIRT0_IRQNO+i);
    }

    intr_enable();

    result = device_open(&blkio, "blk", 0);

    if (result != 0)
        panic("device_open failed");
    
    result = fs_mount(blkio);

    debug("Mounted blk0");

    if (result != 0)
        panic("fs_mount failed");

    result = fs_open(INIT_PROC, &initio);

    if (result < 0)
        panic(INIT_PROC ": process image not found");
    
    result = process_exec(initio);
    panic(INIT_PROC ": process_exec failed");
}
