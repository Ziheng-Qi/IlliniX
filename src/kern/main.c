// main.c - Main function: runs shell to load executable
//

#ifdef MAIN_TRACE
#define TRACE
#endif

#ifdef MAIN_DEBUG
#define DEBUG
#endif

#define INIT_PROC "init0" // name of init process executable

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

    kprintf("         ####### VirtMem Rubric_3 #######\n");
    uintptr_t test_vma = 0xD0000000;
    memory_alloc_and_map_page(test_vma, PTE_G | PTE_R | PTE_W | PTE_U);
    struct pte* pte = walk_pt(active_space_root(), test_vma, 0);
    kprintf("unmapped vma in user program: %x\nmapping to pma: %x\nwith pte: %x\n",test_vma,(pte->ppn)<<12,pte);
    *((volatile uint64_t *)test_vma) = 3026;
    uint64_t value = (uint64_t)((pte->ppn) << 12);
    if (value == 3026)
        kprintf("Demamd paging read/write pass!\n");
    else
        kprintf("Demand paging read/write fail!\n");
    
    kprintf("         ####### VirtMem Rubric_4 #######\n");
    uintptr_t base_vma = 0xC0001000;
    memory_alloc_and_map_page(base_vma, PTE_G | PTE_R | PTE_W | PTE_U);
    //pte = walk_pt(active_space_root(), test_vma, 0);
    volatile uint32_t* ptr;
    uintptr_t size = sizeof(uint32_t);
    for (uintptr_t k = 0; k < 0x1000; k += size) {
        ptr = (volatile uint32_t *)(base_vma + k);
        *ptr = (uint32_t)(k / size);
    }
    for (uintptr_t j = 0; j < 0x1000; j += size) {
        ptr = (volatile uint32_t *)(base_vma + j);
        pte = walk_pt(active_space_root(), (uintptr_t)ptr, 0);
        uint32_t val = (uint32_t)((pte->ppn) << 12);
        val = *ptr;
        if (val != (uint32_t)(j / size))
            panic("Paging implementation with repeated pointer arithmetic operations fail!\n");
    }
    kprintf("Paging implementation with repeated pointer arithmetic operations pass!\n");



    // kprintf("         ####### VirtMem Rubric_5 #######\n");
    // uintptr_t stack_vma = 0x80032000;
    // memory_alloc_and_map_page(stack_vma, PTE_R | PTE_W);
    // pte = walk_pt(active_space_root(), stack_vma, 0);
    // kprintf("unmapped vma in user program: %x\nmapping to pma: %x\nwith pte: %x\n",stack_vma,(pte->ppn)<<12,pte);
    // *((volatile uint64_t *)stack_vma) = 3026;
    // value = *(uint64_t*)((pte->ppn)<<12);


    // Attach NS16550a serial devices

    for (i = 0; i < 2; i++) {
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
    // while (1);
    if (result < 0)
        panic(INIT_PROC ": process image not found");

    result = process_exec(initio);
    // process_exec will never return here because it's in user stack, and its exit will call process_exit
    panic(INIT_PROC ": process_exec failed");
}
