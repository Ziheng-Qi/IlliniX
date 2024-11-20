// memory.c - Memory management
//

#ifndef TRACE
#ifdef MEMORY_TRACE
#define TRACE
#endif
#endif

#ifndef DEBUG
#ifdef MEMORY_DEBUG
#define DEBUG
#endif
#endif

#include "config.h"

#include "memory.h"
#include "console.h"
#include "halt.h"
#include "heap.h"
#include "csr.h"
#include "string.h"
#include "error.h"
#include "thread.h"
#include "process.h"

#include <stdint.h>

// EXPORTED VARIABLE DEFINITIONS
//

char memory_initialized = 0;
uintptr_t main_mtag;

// IMPORTED VARIABLE DECLARATIONS
//

// The following are provided by the linker (kernel.ld)

extern char _kimg_start[];
extern char _kimg_text_start[];
extern char _kimg_text_end[];
extern char _kimg_rodata_start[];
extern char _kimg_rodata_end[];
extern char _kimg_data_start[];
extern char _kimg_data_end[];
extern char _kimg_end[];

// INTERNAL TYPE DEFINITIONS
//

union linked_page {
    union linked_page * next;
    char padding[PAGE_SIZE];
};

struct pte {
    uint64_t flags:8;
    uint64_t rsw:2;
    uint64_t ppn:44;
    uint64_t reserved:7;
    uint64_t pbmt:2;
    uint64_t n:1;
};

// INTERNAL MACRO DEFINITIONS
//

#define VPN2(vma) (((vma) >> (9+9+12)) & 0x1FF)
#define VPN1(vma) (((vma) >> (9+12)) & 0x1FF)
#define VPN0(vma) (((vma) >> 12) & 0x1FF)
#define MIN(a,b) (((a)<(b))?(a):(b))

// INTERNAL FUNCTION DECLARATIONS
//

static inline int wellformed_vma(uintptr_t vma);
static inline int wellformed_vptr(const void * vp);
static inline int aligned_addr(uintptr_t vma, size_t blksz);
static inline int aligned_ptr(const void * p, size_t blksz);
static inline int aligned_size(size_t size, size_t blksz);

static inline uintptr_t active_space_mtag(void);
static inline struct pte * mtag_to_root(uintptr_t mtag);
static inline struct pte * active_space_root(void);

static inline void * pagenum_to_pageptr(uintptr_t n);
static inline uintptr_t pageptr_to_pagenum(const void * p);

static inline void * round_up_ptr(void * p, size_t blksz);
static inline uintptr_t round_up_addr(uintptr_t addr, size_t blksz);
static inline size_t round_up_size(size_t n, size_t blksz);
static inline void * round_down_ptr(void * p, size_t blksz);
static inline size_t round_down_size(size_t n, size_t blksz);
static inline uintptr_t round_down_addr(uintptr_t addr, size_t blksz);

static inline struct pte leaf_pte (
    const void * pptr, uint_fast8_t rwxug_flags);
static inline struct pte ptab_pte (
    const struct pte * ptab, uint_fast8_t g_flag);
static inline struct pte null_pte(void);

static inline void sfence_vma(void);

// INTERNAL GLOBAL VARIABLES
//

static union linked_page * free_list;

// Root page table: each PTE maps 1GB
static struct pte main_pt2[PTE_CNT]
    __attribute__ ((section(".bss.pagetable"), aligned(4096)));
// Level1 PT for 1GB: each PTE maps 2MB
static struct pte main_pt1_0x80000[PTE_CNT]
    __attribute__ ((section(".bss.pagetable"), aligned(4096)));
// Level0(leaf) PT for 2MB: each PTE maps 4kB
static struct pte main_pt0_0x80000[PTE_CNT]
    __attribute__ ((section(".bss.pagetable"), aligned(4096)));

// EXPORTED VARIABLE DEFINITIONS
//

// EXPORTED FUNCTION DEFINITIONS
// 

void memory_init(void) {
    const void * const text_start = _kimg_text_start;
    const void * const text_end = _kimg_text_end;
    const void * const rodata_start = _kimg_rodata_start;
    const void * const rodata_end = _kimg_rodata_end;
    const void * const data_start = _kimg_data_start;
    union linked_page * page;
    void * heap_start;
    void * heap_end;
    size_t page_cnt;
    uintptr_t pma;
    const void * pp;

    trace("%s()", __func__);

    assert (RAM_START == _kimg_start);

    kprintf("           RAM: [%p,%p): %zu MB\n",
        RAM_START, RAM_END, RAM_SIZE / 1024 / 1024);
    kprintf("  Kernel image: [%p,%p)\n", _kimg_start, _kimg_end);

    // Kernel must fit inside 2MB megapage (one level 1 PTE)
    
    if (MEGA_SIZE < _kimg_end - _kimg_start)
        panic("Kernel too large");

    // Initialize main page table with the following direct mapping:
    // 
    //         0 to RAM_START:           RW gigapages (MMIO region)
    // RAM_START to _kimg_end:           RX/R/RW pages based on kernel image
    // _kimg_end to RAM_START+MEGA_SIZE: RW pages (heap and free page pool)
    // RAM_START+MEGA_SIZE to RAM_END:   RW megapages (free page pool)
    //
    // RAM_START = 0x80000000
    // MEGA_SIZE = 2 MB
    // GIGA_SIZE = 1 GB
    
    // Identity mapping of two gigabytes (as two gigapage mappings)
    for (pma = 0; pma < RAM_START_PMA; pma += GIGA_SIZE)
        main_pt2[VPN2(pma)] = leaf_pte((void*)pma, PTE_R | PTE_W | PTE_G);
     
    // Third gigarange has a second-level page table
    main_pt2[VPN2(RAM_START_PMA)] = ptab_pte(main_pt1_0x80000, PTE_G);

    // First physical megarange of RAM is mapped as individual pages with
    // permissions based on kernel image region.

    main_pt1_0x80000[VPN1(RAM_START_PMA)] =
        ptab_pte(main_pt0_0x80000, PTE_G);

    for (pp = text_start; pp < text_end; pp += PAGE_SIZE) {
        main_pt0_0x80000[VPN0((uintptr_t)pp)] =
            leaf_pte(pp, PTE_R | PTE_X | PTE_G);
    }

    for (pp = rodata_start; pp < rodata_end; pp += PAGE_SIZE) {
        main_pt0_0x80000[VPN0((uintptr_t)pp)] =
            leaf_pte(pp, PTE_R | PTE_G);
    }

    for (pp = data_start; pp < RAM_START + MEGA_SIZE; pp += PAGE_SIZE) {
        main_pt0_0x80000[VPN0((uintptr_t)pp)] =
            leaf_pte(pp, PTE_R | PTE_W | PTE_G);
    }

    // Remaining RAM mapped in 2MB megapages

    for (pp = RAM_START + MEGA_SIZE; pp < RAM_END; pp += MEGA_SIZE) {
        main_pt1_0x80000[VPN1((uintptr_t)pp)] =
            leaf_pte(pp, PTE_R | PTE_W | PTE_G);
    }

    // Enable paging. This part always makes me nervous.

    main_mtag =  // Sv39
        ((uintptr_t)RISCV_SATP_MODE_Sv39 << RISCV_SATP_MODE_shift) |
        pageptr_to_pagenum(main_pt2);
    
    csrw_satp(main_mtag);
    sfence_vma();

    // Give the memory between the end of the kernel image and the next page
    // boundary to the heap allocator, but make sure it is at least
    // HEAP_INIT_MIN bytes.

    heap_start = _kimg_end;
    heap_end = round_up_ptr(heap_start, PAGE_SIZE);
    if (heap_end - heap_start < HEAP_INIT_MIN) {
        heap_end += round_up_size (
            HEAP_INIT_MIN - (heap_end - heap_start), PAGE_SIZE);
    }

    if (RAM_END < heap_end)
        panic("Not enough memory");
    
    // Initialize heap memory manager

    heap_init(heap_start, heap_end);

    kprintf("Heap allocator: [%p,%p): %zu KB free\n",
        heap_start, heap_end, (heap_end - heap_start) / 1024);

    free_list = heap_end; // heap_end is page aligned
    page_cnt = (RAM_END - heap_end) / PAGE_SIZE;

    kprintf("Page allocator: [%p,%p): %lu pages free\n",
        free_list, RAM_END, page_cnt); 

    // Put free pages on the free page list
    // TODO: FIXME implement this (must work with your implementation of
    // memory_alloc_page and memory_free_page).
    for (void* free_page = heap_end; free_page < RAM_END; free_page += PAGE_SIZE) {
        memory_free_page(free_page);
    }
    // Allow supervisor to access user memory. We could be more precise by only
    // enabling it when we are accessing user memory, and disable it at other
    // times to catch bugs.

    csrs_sstatus(RISCV_SSTATUS_SUM);

    memory_initialized = 1;
}

// This function takes a pointer to your active root page table and a virtual memory address. 
// It walks down the page table structure using the VPN fields of vma, and if create is non-zero, 
// it will create the appropriate page tables to walk to the leaf page table (”level 0”). 
// It returns a pointer to the page table entry that represents the 4 kB page containing vma. 
// This function will only walk to a 4 kB page, not a megapage or gigapage.
struct pte* walk_pt(struct pte* root, uintptr_t vma, int create) {
    // walk down every level and create table if table not exist
    uintptr_t pt1_ppn = root[VPN2(vma)].ppn;
    uintptr_t pt1_pma = pagenum_to_pageptr(pt1_ppn); 
    struct pte* pt1 = (struct pte*) pt1_pma;
    if (create != 0 && !(root[VPN2(vma)].flags & PTE_V)) {
        void* allocated_page = memory_alloc_page();
        root[VPN2(vma)] = ptab_pte((struct pte*)allocated_page, PTE_G);
        pt1 = allocated_page;
    }

    uintptr_t pt0_ppn = pt1[VPN1(vma)].ppn;
    uintptr_t pt0_pma = pagenum_to_pageptr(pt0_ppn); 
    struct pte* pt0 = (struct pte*) pt0_pma;
    if (create != 0 && !(pt1[VPN1(vma)].flags & PTE_V)) {
        void* allocated_page = memory_alloc_page();
        pt1[VPN1(vma)] = ptab_pte((struct pte*)allocated_page, PTE_G);
        pt0 = allocated_page;
    }

    uintptr_t ppn = pt0[VPN0(vma)].ppn;
    uintptr_t pma = (ppn << 12) | (vma & 0xFFF);
    // if (create != 0 && !(pt0[VPN0(vma)].flags & PTE_V)) {
    //     void* allocated_page = memory_alloc_page();
    //     pt0[VPN0(vma)] = leaf_pte((struct pte*)allocated_page, PTE_G | PTE_W |PTE_R);
    // }
    return &pt0[VPN0(vma)];
}

// Switches the active memory space to the main memory space and reclaims the
// memory space that was active on entry. All physical pages mapped by the memory space 
// that are not part of the global mapping are reclaimed.
// Question: what is reclaim? Are we recalling all the previously active space?
void memory_space_reclaim(void) {
    uintptr_t old_mtag = memory_space_switch(main_mtag);
    if (old_mtag == main_mtag)
        panic("try to reclaim main space\r\n");
    
    struct pte* pt2 = mtag_to_root(old_mtag);
    for (int vpn2 = 0; vpn2 < PTE_CNT; vpn2++) {
        if (!(pt2[vpn2].flags & PTE_V) || !(pt2[vpn2].flags & PTE_U) || pt2[vpn2].flags & PTE_G) 
            continue;
        struct pte* pt1 = (struct pte*) pagenum_to_pageptr(pt2[vpn2].ppn);
        for (int vpn1 = 0; vpn1 < PTE_CNT; vpn1++) {
            if (!(pt1[vpn1].flags & PTE_V) || !(pt1[vpn1].flags & PTE_U) || pt1[vpn1].flags & PTE_G)
                continue;
            struct pte* pt0 = (struct pte*) pagenum_to_pageptr(pt1[vpn1].ppn);
            for (int vpn0 = 0; vpn0 < PTE_CNT; vpn0++) {
                if (!(pt0[vpn0].flags & PTE_V) || !(pt0[vpn0].flags & PTE_U) || pt0[vpn0].flags & PTE_G)
                    continue;
                memory_free_page(pagenum_to_pageptr(pt0[vpn0].ppn));
            }
            if (pt1[vpn1].flags & PTE_G)
                continue;
            memory_free_page(pt0);
        }
        if (pt2[vpn2].flags & PTE_G)
            continue;
        memory_free_page(pt1);
    }
    memory_free_page(pt2);
    sfence_vma();
}

// Allocates a physical page from the free physical page pool and returns a pointer
// to the direct-mapped addr of the page. Return value in [RAM_START, RAM_END],
// so VMA = PMA. Panics if there are no free pages available.
void * memory_alloc_page(void) {
    if (free_list == NULL)
        panic("No free pages available!");
    union linked_page * allocated_page = free_list;
    free_list = free_list->next;
    allocated_page->next = NULL;
    if (allocated_page > RAM_END || allocated_page < RAM_START)
        panic("Invalid physical page!");
    return allocated_page;
}

// Returns a previously allocated physical page to the free page pool.
void memory_free_page(void * pp) {
    // insert the page to the head
    if (pp == NULL)
        panic("Invalid allocated physical page!");
    // union linked_page* cur_page = free_list;
    ((union linked_page* )pp)->next = free_list;
    free_list = pp;
}

// Allocates and maps a physical page.
// Maps a virtual page to a physical page in the current memory space. The /vma/
// argument gives the virtual address of the page to map. The /pp/ argument is a
// pointer to the physical page to map. The /rwxug_flags/ argument is an OR of
// the PTE flags, of which only a combination of R, W, X, U, and G should be
// specified. (The D, A, and V flags are always added by memory_map_page.) The
// function returns a pointer to the mapped virtual page, i.e., (void*)vma.
// Does not fail; panics if the request cannot be satsified.
void * memory_alloc_and_map_page (
    uintptr_t vma, uint_fast8_t rwxug_flags) {
        // allocate new physical page
        void* page = memory_alloc_page();
        if (page == NULL)
            panic("Failed to allocate new physical page!");
        // get pte of vma
        struct pte* pte = walk_pt(active_space_root(), vma, 1);
        if (pte == NULL)
            panic("Failed to allocate page table entry");
        // map the vma to the physical page
        pte->ppn = pageptr_to_pagenum(page);
        pte->flags = rwxug_flags | PTE_D | PTE_A | PTE_V;
        return vma;
    }

// Allocates and maps multiple physical pages in an address range. Equivalent to
// calling memory_alloc_and_map_page for every page in the range. Returns the mapped
// virtual memory address.
void * memory_alloc_and_map_range (
    uintptr_t vma, size_t size, uint_fast8_t rwxug_flags) {
        for (uintptr_t addr = round_up_addr(vma, PAGE_SIZE); addr < round_up_size(addr + size, PAGE_SIZE); addr += PAGE_SIZE) {
            // struct pte* pte = walk_pt(active_space_root(), addr, 1);
            // if (pte == NULL || !(pte->flags | PTE_V))
            //     continue;
            void* addr = memory_alloc_and_map_page(addr, PTE_V);
            if (addr == NULL)
                kprintf("Allocation failed!");
        }   
        return vma;
    }

// Unmaps and frees all pages with the U flag asserted.
void memory_unmap_and_free_user(void) {
    struct pte* pt2 = active_space_root();
    for (int vpn2 = 0; vpn2 < PTE_CNT; vpn2++) {
        if ((!pt2[vpn2].flags & PTE_V) || !(pt2[vpn2].flags & PTE_U)) 
            continue;
        struct pte* pt1 = (struct pte*) pagenum_to_pageptr(pt2[vpn2].ppn);
        for (int vpn1 = 0; vpn1 < PTE_CNT; vpn1++) {
            if ((!pt1[vpn1].flags & PTE_V) || !(pt1[vpn1].flags & PTE_U))
                continue;
            struct pte* pt0 = (struct pte*) pagenum_to_pageptr(pt1[vpn1].ppn);
            for (int vpn0 = 0; vpn0 < PTE_CNT; vpn0++) {
                if ((!pt0[vpn0].flags & PTE_V) || !(pt0[vpn0].flags & PTE_U))
                    continue;
                memory_free_page(pagenum_to_pageptr(pt0[vpn0].ppn));
            }
            if (pt1[vpn1].flags & PTE_U)
                continue;
            memory_free_page(pt0);
        }
        if (pt2[vpn2].flags & PTE_U)
            continue;
        memory_free_page(pt1);
    }
    memory_free_page(pt2);
    sfence_vma();
}

// Sets the flags of the PTE associated with vp. Only works with 4 kB pages.
void memory_set_page_flags(const void *vp, uint8_t rwxug_flags) {
    struct pte* pte = walk_pt(active_space_root(), vp, 0);
    pte->flags |= rwxug_flags;
}


// Changes the PTE flags for all pages in a mapped range.
void memory_set_range_flags (
const void * vp, size_t size, uint_fast8_t rwxug_flags) {
    for (uintptr_t vma = vp; vma < vp + size; vma += PAGE_SIZE) {
        struct pte* pte = walk_pt(active_space_root(), vma, 0);
        if (pte == NULL | !pte->flags | PTE_V)
            continue;
        pte->flags |= rwxug_flags;
    }
}


// Checks if a virtual address range is mapped with specified flags. Returns 1
// if and only if every virtual page containing the specified virtual address
// range is mapped with the at least the specified flags.
int memory_validate_vptr_len (
    const void * vp, size_t len, uint_fast8_t rwxug_flags) {
        return 0;
    }

// Checks if the virtual pointer points to a mapped range containing a
// null-terminated string. Returns 1 if and only if the virtual pointer points
// to a mapped readable page with the specified flags, and every byte starting
// at /vs/ up until the terminating null byte is also mapped with the same
// permissions.
int memory_validate_vstr (
    const char * vs, uint_fast8_t ug_flags) {
        return 0;
    }

// Called from excp.c to handle a page fault at the specified virtual address. Either
// maps a page containing the faulting address, or calls process_exit, depending on if the address 
// is within the user region. Must call this func when a store page fault is triggered by a user program.
void memory_handle_page_fault(const void * vptr) {
    if (vptr < USER_START_VMA || vptr > USER_END_VMA) {
        kprintf("Address outside the user region");
        process_exit();
    }
    struct pte* pte = walk_pt(active_space_root(), vptr, 1);
    if (pte == NULL)
        panic("Invalid allocated page!");
    sfence_vma();
}


// INTERNAL FUNCTION DEFINITIONS
//

static inline int wellformed_vma(uintptr_t vma) {
    // Address bits 63:38 must be all 0 or all 1
    uintptr_t const bits = (intptr_t)vma >> 38;
    return (!bits || !(bits+1));
}

static inline int wellformed_vptr(const void * vp) {
    return wellformed_vma((uintptr_t)vp);
}

static inline int aligned_addr(uintptr_t vma, size_t blksz) {
    return ((vma % blksz) == 0);
}

static inline int aligned_ptr(const void * p, size_t blksz) {
    return (aligned_addr((uintptr_t)p, blksz));
}

static inline int aligned_size(size_t size, size_t blksz) {
    return ((size % blksz) == 0);
}

static inline uintptr_t active_space_mtag(void) {
    return csrr_satp();
}

static inline struct pte * mtag_to_root(uintptr_t mtag) {
    return (struct pte *)((mtag << 20) >> 8);
}


static inline struct pte * active_space_root(void) {
    return mtag_to_root(active_space_mtag());
}

static inline void * pagenum_to_pageptr(uintptr_t n) {
    return (void*)(n << PAGE_ORDER);
}

static inline uintptr_t pageptr_to_pagenum(const void * p) {
    return (uintptr_t)p >> PAGE_ORDER;
}

static inline void * round_up_ptr(void * p, size_t blksz) {
    return (void*)((uintptr_t)(p + blksz-1) / blksz * blksz);
}

static inline uintptr_t round_up_addr(uintptr_t addr, size_t blksz) {
    return ((addr + blksz-1) / blksz * blksz);
}

static inline size_t round_up_size(size_t n, size_t blksz) {
    return (n + blksz-1) / blksz * blksz;
}

static inline void * round_down_ptr(void * p, size_t blksz) {
    return (void*)((uintptr_t)p / blksz * blksz);
}

static inline size_t round_down_size(size_t n, size_t blksz) {
    return n / blksz * blksz;
}

static inline uintptr_t round_down_addr(uintptr_t addr, size_t blksz) {
    return (addr / blksz * blksz);
}

static inline struct pte leaf_pte (
    const void * pptr, uint_fast8_t rwxug_flags)
{
    return (struct pte) {
        .flags = rwxug_flags | PTE_A | PTE_D | PTE_V,
        .ppn = pageptr_to_pagenum(pptr)
    };
}

static inline struct pte ptab_pte (
    const struct pte * ptab, uint_fast8_t g_flag)
{
    return (struct pte) {
        .flags = g_flag | PTE_V,
        .ppn = pageptr_to_pagenum(ptab)
    };
}

static inline struct pte null_pte(void) {
    return (struct pte) { };
}

static inline void sfence_vma(void) {
    asm inline ("sfence.vma" ::: "memory");
}
