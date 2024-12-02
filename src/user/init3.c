#include "syscall.h"
#include "string.h"
#include "memory.h"

#define PTE_V (1 << 0)
#define PTE_R (1 << 1)
#define PTE_W (1 << 2)
#define PTE_X (1 << 3)
#define PTE_U (1 << 4)
#define PTE_G (1 << 5)
#define PTE_A (1 << 6)
#define PTE_D (1 << 7)

typedef unsigned long int	uintptr_t;
typedef unsigned long int __uint64_t;
typedef __uint64_t uint64_t;

// This user program tests read/write a kernel page from user program.
// It should generate "Load page fault"

void main(void) {
    uintptr_t *stack_vma = (uintptr_t *)0x80032000;
    *stack_vma = 0xC0001000;
    // uintptr_t value = *stack_vma;
}   
