#include "syscall.h"
#include "string.h"

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

void main(void) {
    ckprintf("         ####### VirtMem Rubric_5 #######\n");
    int stack_vma = 0x80032000;
    memory_alloc_and_map_page(stack_vma, PTE_R | PTE_W);
    struct pte* pte = walk_pt(active_space_root(), stack_vma, 0);
    kprintf("unmapped vma in user program: %x\nmapping to pma: %x\nwith pte: %x\n",stack_vma,(pte->ppn)<<12,pte);
    *((volatile uint64_t *)stack_vma) = 3026;
    uint64_t value = *(uint64_t*)((pte->ppn)<<12);
    if (value == 3026)
        kprintf("Demamd paging read/write pass!\n");
    else
        kprintf("Demand paging read/write fail!\n");
}
