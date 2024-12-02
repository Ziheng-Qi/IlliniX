// config.h - Kernel configuration
//

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stddef.h> // size_t

#ifndef RAM_SIZE
#ifndef RAM_SIZE_MB
#define RAM_SIZE ((size_t)8*1024*1024)
#else
#define RAM_SIZE ((size_t)RAM_SIZE_MB*1024*1024)
#endif
#endif

// PMA : Physical Memory Address
// VMA : Virtual Memory Address

#define RAM_START_PMA 0x80000000 // QEMU
#define RAM_START ((void*)RAM_START_PMA)
#define RAM_END_PMA (RAM_START_PMA+RAM_SIZE)
#define RAM_END (RAM_START+RAM_SIZE)

// The memory manager assumes that the user memory region starts on a gigapage
// boundary after the kernel's identity-mapped MMIO and RAM in the first three
// gigabytes of the address space, i.e., [0,0xC0000000).

#define USER_START_VMA  0xC0000000UL // User programs loaded here
#define USER_END_VMA    0xD0000000UL // End of user program space
#define USER_STACK_VMA  USER_END_VMA // starting user stack pointer

#define UART0_IOBASE 0x10000000 // PMA
#define UART1_IOBASE 0x10000100 // PMA
#define UART0_IRQNO 10

#define VIRT0_IOBASE 0x10001000 // PMA
#define VIRT1_IOBASE 0x10002000 // PMA
#define VIRT0_IRQNO 1

#endif // _CONFING_H_
