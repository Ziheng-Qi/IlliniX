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

// INTERNAL MACRO DEFINITIONS
//

#define VPN2(vma) (((vma) >> (9 + 9 + 12)) & 0x1FF)
#define VPN1(vma) (((vma) >> (9 + 12)) & 0x1FF)
#define VPN0(vma) (((vma) >> 12) & 0x1FF)
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

// INTERNAL FUNCTION DECLARATIONS
//

static inline int wellformed_vma(uintptr_t vma);
static inline int wellformed_vptr(const void *vp);
static inline int aligned_addr(uintptr_t vma, size_t blksz);
static inline int aligned_ptr(const void *p, size_t blksz);
static inline int aligned_size(size_t size, size_t blksz);

static inline void *pagenum_to_pageptr(uintptr_t n);
static inline uintptr_t pageptr_to_pagenum(const void *p);

static inline void *round_up_ptr(void *p, size_t blksz);
static inline uintptr_t round_up_addr(uintptr_t addr, size_t blksz);
static inline size_t round_up_size(size_t n, size_t blksz);
static inline void *round_down_ptr(void *p, size_t blksz);
static inline size_t round_down_size(size_t n, size_t blksz);
static inline uintptr_t round_down_addr(uintptr_t addr, size_t blksz);

static inline struct pte leaf_pte(
    const void *pptr, uint_fast8_t rwxug_flags);
static inline struct pte ptab_pte(
    const struct pte *ptab, uint_fast8_t g_flag);
static inline struct pte null_pte(void);

static inline void sfence_vma(void);

// INTERNAL GLOBAL VARIABLES
//

static union linked_page *free_list;

// Root page table: each PTE maps 1GB
static struct pte main_pt2[PTE_CNT]
    __attribute__((section(".bss.pagetable"), aligned(4096)));
// Level1 PT for 1GB: each PTE maps 2MB
static struct pte main_pt1_0x80000[PTE_CNT]
    __attribute__((section(".bss.pagetable"), aligned(4096)));
// Level0(leaf) PT for 2MB: each PTE maps 4kB
static struct pte main_pt0_0x80000[PTE_CNT]
    __attribute__((section(".bss.pagetable"), aligned(4096)));

// EXPORTED VARIABLE DEFINITIONS
//

// EXPORTED FUNCTION DEFINITIONS
//

/**
 * @brief Initializes the memory management system.
 *
 * This function sets up the memory layout and initializes the page tables
 * for the kernel. It ensures that the kernel image fits within a 2MB megapage,
 * sets up identity mapping for the first 2GB of memory, and configures the
 * main page table with appropriate permissions for different regions of the
 * kernel image. It also initializes the heap memory manager and the free page
 * list for the page allocator.
 *
 * Memory layout:
 * - 0 to RAM_START: RW gigapages (MMIO region)
 * - RAM_START to _kimg_end: RX/R/RW pages based on kernel image
 * - _kimg_end to RAM_START + MEGA_SIZE: RW pages (heap and free page pool)
 * - RAM_START + MEGA_SIZE to RAM_END: RW megapages (free page pool)
 *
 * @note This function must be called during the system initialization process.
 *       It assumes that the kernel image is loaded at RAM_START and that the
 *       memory regions are properly defined.
 *
 * @warning If the kernel image is too large to fit within a 2MB megapage,
 *          the function will panic.
 *
 * @param None
 * @return None
 */
void memory_init(void)
{
    const void *const text_start = _kimg_text_start;
    const void *const text_end = _kimg_text_end;
    const void *const rodata_start = _kimg_rodata_start;
    const void *const rodata_end = _kimg_rodata_end;
    const void *const data_start = _kimg_data_start;
    union linked_page *page;
    void *heap_start;
    void *heap_end;
    size_t page_cnt;
    uintptr_t pma;
    const void *pp;

    trace("%s()", __func__);

    assert(RAM_START == _kimg_start);

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
        main_pt2[VPN2(pma)] = leaf_pte((void *)pma, PTE_R | PTE_W | PTE_G);

    // Third gigarange has a second-level page table
    main_pt2[VPN2(RAM_START_PMA)] = ptab_pte(main_pt1_0x80000, PTE_G);

    // First physical megarange of RAM is mapped as individual pages with
    // permissions based on kernel image region.

    main_pt1_0x80000[VPN1(RAM_START_PMA)] =
        ptab_pte(main_pt0_0x80000, PTE_G);

    for (pp = text_start; pp < text_end; pp += PAGE_SIZE)
    {
        main_pt0_0x80000[VPN0((uintptr_t)pp)] =
            leaf_pte(pp, PTE_R | PTE_X | PTE_G);
    }

    for (pp = rodata_start; pp < rodata_end; pp += PAGE_SIZE)
    {
        main_pt0_0x80000[VPN0((uintptr_t)pp)] =
            leaf_pte(pp, PTE_R | PTE_G);
    }

    for (pp = data_start; pp < RAM_START + MEGA_SIZE; pp += PAGE_SIZE)
    {
        main_pt0_0x80000[VPN0((uintptr_t)pp)] =
            leaf_pte(pp, PTE_R | PTE_W | PTE_G);
    }

    // Remaining RAM mapped in 2MB megapages

    for (pp = RAM_START + MEGA_SIZE; pp < RAM_END; pp += MEGA_SIZE)
    {
        main_pt1_0x80000[VPN1((uintptr_t)pp)] =
            leaf_pte(pp, PTE_R | PTE_W | PTE_G);
    }

    // Enable paging. This part always makes me nervous.

    main_mtag = // Sv39
        ((uintptr_t)RISCV_SATP_MODE_Sv39 << RISCV_SATP_MODE_shift) |
        pageptr_to_pagenum(main_pt2);

    csrw_satp(main_mtag);
    sfence_vma();

    // Give the memory between the end of the kernel image and the next page
    // boundary to the heap allocator, but make sure it is at least
    // HEAP_INIT_MIN bytes.

    heap_start = _kimg_end;
    heap_end = round_up_ptr(heap_start, PAGE_SIZE);
    if (heap_end - heap_start < HEAP_INIT_MIN)
    {
        heap_end += round_up_size(
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
    for (void *free_page = heap_end; free_page < RAM_END; free_page += PAGE_SIZE)
    {
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
/**
 * walk_pt - Walks through the page table and optionally creates new page tables if they do not exist.
 * @root: Pointer to the root page table entry.
 * @vma: Virtual memory address to be translated.
 * @create: Flag indicating whether to create new page tables if they do not exist (non-zero to create).
 *
 * This function traverses the multi-level page table structure starting from the root page table entry.
 * It walks down each level of the page table hierarchy (level 2, level 1, and level 0) and optionally
 * allocates new page tables if they do not exist and the create flag is set. The function returns a pointer
 * to the page table entry corresponding to the given virtual memory address.
 *
 * Return: Pointer to the page table entry corresponding to the given virtual memory address.
 */
struct pte *walk_pt(struct pte *root, uintptr_t vma, int create)
{
    // walk down every level and create table if table not exist
    uintptr_t pt1_ppn = root[VPN2(vma)].ppn;
    uintptr_t pt1_pma = pagenum_to_pageptr(pt1_ppn);
    struct pte *pt1 = (struct pte *)pt1_pma;
    if (create != 0 && !(root[VPN2(vma)].flags & PTE_V))
    {
        void *allocated_page = memory_alloc_page();
        root[VPN2(vma)] = ptab_pte((struct pte *)allocated_page, 0);
        pt1 = allocated_page;
    }

    uintptr_t pt0_ppn = pt1[VPN1(vma)].ppn;
    uintptr_t pt0_pma = pagenum_to_pageptr(pt0_ppn);
    struct pte *pt0 = (struct pte *)pt0_pma;
    if (create != 0 && !(pt1[VPN1(vma)].flags & PTE_V))
    {
        void *allocated_page = memory_alloc_page();
        pt1[VPN1(vma)] = ptab_pte((struct pte *)allocated_page, 0);
        pt0 = allocated_page;
    }

    uintptr_t ppn = pt0[VPN0(vma)].ppn;
    uintptr_t pma = (ppn << 12) | (vma & 0xFFF);

    return &pt0[VPN0(vma)];
}

// Switches the active memory space to the main memory space and reclaims the
// memory space that was active on entry. All physical pages mapped by the memory space
// that are not part of the global mapping are reclaimed.
// Question: what is reclaim? Are we recalling all the previously active space?
/**
 * @brief Reclaims memory space by traversing and freeing unused pages.
 *
 * This function switches to the main memory tag and iterates through the page tables
 * to identify and free pages that are not marked as global and are valid. It updates
 * the free list with the reclaimed pages.
 *
 * The function performs the following steps:
 * 1. Switches to the main memory tag.
 * 2. Iterates through the second-level page table entries.
 * 3. For each valid and non-global entry, iterates through the first-level page table entries.
 * 4. For each valid and non-global entry, iterates through the zero-level page table entries.
 * 5. For each valid and non-global entry, adds the page to the free list and clears the page table entry.
 * 6. Frees the zero-level page table if it is not marked as global.
 * 7. Frees the first-level page table if it is not marked as global.
 * 8. Ensures the changes are protected by calling sfence_vma to flush TLB.
 *
 * @note This function assumes that the page tables are organized in a hierarchical manner
 *       with three levels (second-level, first-level, and zero-level).
 */
void memory_space_reclaim(void)
{
    uintptr_t old_mtag = memory_space_switch(main_mtag);

    union linked_page *page = free_list;

    struct pte *pt2 = pagenum_to_pageptr(old_mtag);
    for (int vpn2 = 0; vpn2 < PTE_CNT; vpn2++)
    {
        if (!(pt2[vpn2].flags & PTE_V) || !(pt2[vpn2].flags & PTE_X) || !(pt2[vpn2].flags & PTE_R) || !(pt2[vpn2].flags & PTE_W) || pt2[vpn2].flags & PTE_G)
            continue;
        struct pte *pt1 = (struct pte *)pagenum_to_pageptr(pt2[vpn2].ppn);
        for (int vpn1 = 0; vpn1 < PTE_CNT; vpn1++)
        {
            if (!(pt1[vpn1].flags & PTE_V) || !(pt1[vpn1].flags & PTE_X) || !(pt1[vpn1].flags & PTE_R) || !(pt1[vpn1].flags & PTE_W) || pt1[vpn1].flags & PTE_G)
                continue;
            struct pte *pt0 = (struct pte *)pagenum_to_pageptr(pt1[vpn1].ppn);
            for (int vpn0 = 0; vpn0 < PTE_CNT; vpn0++)
            {
                if ((pt0[vpn0].flags & PTE_V))
                {
                    if (!(pt0[vpn0].flags & PTE_G))
                    {
                        if (page == NULL)
                        {
                            page = pagenum_to_pageptr(pt0[vpn0].ppn);
                            free_list = page;
                        }
                        else
                        {
                            page->next = pagenum_to_pageptr(pt0[vpn0].ppn);
                            page = page->next;
                        }
                        pt0[vpn0].ppn &= 0;
                        pt0[vpn0].flags &= 0;
                        sfence_vma();
                    }
                }
            }
            if (pt1[vpn1].flags & PTE_G)
                continue;
            memory_free_page(pt0);
            sfence_vma();
        }
        if (pt2[vpn2].flags & PTE_G)
            continue;
        memory_free_page(pt1);
        sfence_vma();
    }
    sfence_vma();
}

// Allocates a physical page from the free physical page pool and returns a pointer
// to the direct-mapped addr of the page. Return value in [RAM_START, RAM_END],
// so VMA = PMA. Panics if there are no free pages available.
/**
 * @brief Allocates a page of memory from the free list.
 *
 * This function allocates a page of memory by removing the first page from the free list.
 * If there are no free pages available, it triggers a panic with an appropriate message.
 * It also checks if the allocated page is within the valid physical memory range and
 * triggers a panic if the page is invalid.
 *
 * @return A pointer to the allocated page of memory.
 *
 * @note This function assumes that `free_list` is a global variable pointing to the head
 *       of the free list of pages, and that `RAM_START` and `RAM_END` define the valid
 *       range of physical memory addresses.
 */
void *memory_alloc_page(void)
{
    if (free_list == NULL)
        panic("No free pages available!");
    union linked_page *allocated_page = free_list;
    free_list = free_list->next;
    if (allocated_page > RAM_END || allocated_page < RAM_START)
        panic("Invalid physical page!");
    return (void *)allocated_page;
}

// Returns a previously allocated physical page to the free page pool.
/**
 * @brief Frees a previously allocated memory page and adds it back to the free list.
 *
 * This function takes a pointer to a previously allocated memory page and inserts it
 * at the head of the free list, making it available for future allocations.
 *
 * @param pp Pointer to the memory page to be freed. Must not be NULL.
 *
 * @note If the provided pointer is NULL, the function will trigger a panic with the
 *       message "Invalid allocated physical page!".
 */
void memory_free_page(void *pp)
{
    // insert the page to the head
    if (pp == NULL)
        panic("Invalid allocated physical page!");
    // union linked_page* cur_page = free_list;
    ((union linked_page *)pp)->next = free_list;
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
/**
 * @brief Allocates a new physical page and maps it to the specified virtual memory address (vma).
 *
 * This function allocates a new physical page and maps it to the given virtual memory address (vma).
 * It also sets the appropriate page table entry (pte) flags based on the provided rwxug_flags.
 *
 * @param vma The virtual memory address to map the new physical page to.
 * @param rwxug_flags The flags to set for the page table entry, indicating read/write/execute/user/global permissions.
 *
 * @return The virtual memory address (vma) that was mapped to the new physical page.
 *
 * @note If the free list is empty, a message with the current virtual memory address is printed.
 * @note If memory allocation for the new physical page fails, the function will panic.
 * @note If allocation of the page table entry fails, the function will panic.
 */
void *memory_alloc_and_map_page(
    uintptr_t vma, uint_fast8_t rwxug_flags)
{
    // allocate new physical page
    if (free_list == NULL)
    {
        kprintf("current vmaddr: %x\n", vma);
    }
    void *page = memory_alloc_page();
    if (page == NULL)
        panic("Failed to allocate new physical page!");
    // get pte of vma
    struct pte *pte = walk_pt(active_space_root(), vma, 1);
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
/**
 * @brief Allocates and maps a range of memory pages.
 *
 * This function allocates and maps memory pages starting from the given virtual memory address (vma)
 * and spanning the specified size. The pages are allocated and mapped with the provided read/write/execute/user/global
 * (rwxug) flags.
 *
 * @param vma The starting virtual memory address for the allocation.
 * @param size The size of the memory range to allocate and map, in bytes.
 * @param rwxug_flags The flags indicating the permissions and attributes for the allocated pages.
 *
 * @return The starting virtual memory address (vma) of the allocated and mapped range.
 */
void *memory_alloc_and_map_range(
    uintptr_t vma, size_t size, uint_fast8_t rwxug_flags)
{
    uintptr_t initial_vma = vma;
    uintptr_t temp_addr;
    uintptr_t rounded_addr = round_up_addr(vma, PAGE_SIZE);
    uintptr_t round_up_bound = round_up_size(rounded_addr + size, PAGE_SIZE);
    for (uintptr_t addr = rounded_addr; addr < round_up_bound; addr += PAGE_SIZE)
    {
        temp_addr = memory_alloc_and_map_page(addr, rwxug_flags);
        if (temp_addr == NULL)
            kprintf("Allocation failed!");
    }
    return vma;
}

// Unmaps and frees all pages with the U flag asserted.
/**
 * @brief Unmaps and frees user memory pages.
 *
 * This function traverses the page table entries (PTEs) at three levels (pt2, pt1, pt0)
 * and frees the memory pages that are mapped and marked as user pages.
 *
 * The function performs the following steps:
 * 1. Retrieves the root page table (pt2).
 * 2. Iterates through the second-level PTEs (vpn2).
 * 3. Checks if the PTE is valid (PTE_V) and a user page (PTE_U).
 * 4. Retrieves the first-level page table (pt1) and iterates through its PTEs (vpn1).
 * 5. Checks if the PTE is valid (PTE_V) and a user page (PTE_U).
 * 6. Retrieves the zero-level page table (pt0) and iterates through its PTEs (vpn0).
 * 7. Checks if the PTE is valid (PTE_V) and a user page (PTE_U).
 * 8. Frees the memory page and clears the PTE.
 * 9. Frees the first-level page table if no user pages remain.
 * 10. Use sfence_vma to flush the TLB and prevent bad memory accesses.
 */
void memory_unmap_and_free_user(void)
{
    struct pte *pt2 = active_space_root();
    for (int vpn2 = 0; vpn2 < PTE_CNT; vpn2++)
    {
        if ((!pt2[vpn2].flags & PTE_V) || !(pt2[vpn2].flags & PTE_U))
            continue;
        struct pte *pt1 = (struct pte *)pagenum_to_pageptr(pt2[vpn2].ppn);
        for (int vpn1 = 0; vpn1 < PTE_CNT; vpn1++)
        {
            if ((!pt1[vpn1].flags & PTE_V) || !(pt1[vpn1].flags & PTE_U))
                continue;
            struct pte *pt0 = (struct pte *)pagenum_to_pageptr(pt1[vpn1].ppn);
            for (int vpn0 = 0; vpn0 < PTE_CNT; vpn0++)
            {
                if ((!pt0[vpn0].flags & PTE_V) || !(pt0[vpn0].flags & PTE_U))
                    continue;
                memory_free_page(pagenum_to_pageptr(pt0[vpn0].ppn));
                pt0[vpn0].ppn &= 0;
                pt0[vpn0].flags &= 0;
            }
            if (pt1[vpn1].flags & PTE_U)
                continue;
            memory_free_page(pt0);
        }
        if (pt2[vpn2].flags & PTE_U)
            continue;
        memory_free_page(pt1);
    }
    pt2[3].flags &= ~PTE_V;
    sfence_vma();
}

// Sets the flags of the PTE associated with vp. Only works with 4 kB pages.
/**
 * @brief Sets the page table entry flags for a given virtual address.
 *
 * This function updates the flags of the page table entry corresponding to the
 * provided virtual address. The flags are set based on the provided rwxug_flags
 * and additional flags for dirty (PTE_D), accessed (PTE_A), and valid (PTE_V) states.
 *
 * @param vp The virtual address for which the page table entry flags are to be set.
 * @param rwxug_flags The flags to be set for the page table entry, including read, write,
 *                    execute, user, and global permissions.
 */
void memory_set_page_flags(const void *vp, uint8_t rwxug_flags)
{
    struct pte *pte = walk_pt(active_space_root(), vp, 0);

    pte->flags = 0x0;
    pte->flags |= rwxug_flags | PTE_D | PTE_A | PTE_V;
}

// Changes the PTE flags for all pages in a mapped range.
/**
 * @brief Sets the memory range flags for a given virtual address range.
 *
 * This function sets the specified flags for each page table entry (PTE)
 * within the given virtual address range. The range is rounded up to the
 * nearest page size boundary.
 *
 * @param vp Pointer to the start of the virtual address range.
 * @param size Size of the memory range in bytes.
 * @param rwxug_flags Flags to set for the memory range. These flags typically
 *                    include read, write, execute, user, and global permissions.
 */
void memory_set_range_flags(
    const void *vp, size_t size, uint_fast8_t rwxug_flags)
{
    size = round_up_size(size, PAGE_SIZE);
    for (uintptr_t vma = round_up_ptr(vp, PAGE_SIZE); vma < round_up_ptr(vp, PAGE_SIZE) + size; vma += PAGE_SIZE)
    {
        struct pte *pte = walk_pt(active_space_root(), vma, 0);
        if (pte == NULL | !(pte->flags | PTE_V))
            continue;
        pte->flags = 0x0;
        pte->flags |= rwxug_flags | PTE_D | PTE_A | PTE_V;
    }
}

// Checks if a virtual address range is mapped with specified flags. Returns 1
// if and only if every virtual page containing the specified virtual address
// range is mapped with the at least the specified flags.
/**
 * @brief Validates a memory region specified by a virtual pointer and length.
 *
 * This function checks if the memory region starting at the virtual pointer `vp`
 * and spanning `len` bytes is valid according to the specified access flags.
 *
 * @param vp The starting virtual pointer of the memory region to validate.
 * @param len The length of the memory region in bytes.
 * @param rwxug_flags The access flags indicating the permissions for the memory region.
 *                     - Read (R)
 *                     - Write (W)
 *                     - Execute (X)
 *                     - User (U)
 *                     - Global (G)
 *
 * @return Returns 0 if the memory region is valid, otherwise returns an error code.
 */
int memory_validate_vptr_len(
    const void *vp, size_t len, uint_fast8_t rwxug_flags)
{
    struct pte *pte = walk_pt(active_space_root(), (uintptr_t)vp, 0);
    if (pte == NULL)
        return -EINVAL;
    for (uintptr_t vma = (uintptr_t)vp; vma < (uintptr_t)vp + len; vma += PAGE_SIZE)
    {
        struct pte *pte = walk_pt(active_space_root(), vma, 0);
        if (pte == NULL || !(pte->flags & PTE_V) || !(pte->flags & rwxug_flags))
            return -EINVAL;
    }
    return 0;
}

// Checks if the virtual pointer points to a mapped range containing a
// null-terminated string. Returns 1 if and only if the virtual pointer points
// to a mapped readable page with the specified flags, and every byte starting
// at /vs/ up until the terminating null byte is also mapped with the same
// permissions.
int memory_validate_vstr(
    const char *vs, uint_fast8_t ug_flags)
{
    struct pte *pte = walk_pt(active_space_root(), (uintptr_t)vs, 0);
    if (pte == NULL)
        return 0;
    for (const char *s = vs; *s != '\0'; s++)
    {
        struct pte *pte = walk_pt(active_space_root(), (uintptr_t)s, 0);
        if (pte == NULL || !(pte->flags & PTE_V) || !(pte->flags & ug_flags))
            return 0;
    }
    return 1;
}

// Called from excp.c to handle a page fault at the specified virtual address. Either
// maps a page containing the faulting address, or calls process_exit, depending on if the address
// is within the user region. Must call this func when a store page fault is triggered by a user program.
/**
 * @brief Handles a page fault by allocating and mapping a new page.
 *
 * This function is called when a page fault occurs. It checks if the faulting
 * address is within the user region. If the address is outside the user region,
 * it prints an error message and terminates the process. Otherwise, it allocates
 * and maps a new page for the faulting address with read, write, and user permissions.
 * Finally, it flushes the TLB for the updated virtual memory area.
 *
 * @param vptr The faulting virtual address.
 */
void memory_handle_page_fault(const void *vptr)
{
    if (vptr < USER_START_VMA || vptr > USER_END_VMA)
    {
        kprintf("Address outside the user region\n");
        process_exit();
    }
    memory_alloc_and_map_page(vptr, PTE_R | PTE_W | PTE_U);
    sfence_vma();
}

// INTERNAL FUNCTION DEFINITIONS
//

static inline int wellformed_vma(uintptr_t vma)
{
    // Address bits 63:38 must be all 0 or all 1
    uintptr_t const bits = (intptr_t)vma >> 38;
    return (!bits || !(bits + 1));
}

static inline int wellformed_vptr(const void *vp)
{
    return wellformed_vma((uintptr_t)vp);
}

static inline int aligned_addr(uintptr_t vma, size_t blksz)
{
    return ((vma % blksz) == 0);
}

static inline int aligned_ptr(const void *p, size_t blksz)
{
    return (aligned_addr((uintptr_t)p, blksz));
}

static inline int aligned_size(size_t size, size_t blksz)
{
    return ((size % blksz) == 0);
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
