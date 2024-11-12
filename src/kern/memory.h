// memory.h - Physical and virtual memory management
//

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "csr.h"

#include <stddef.h> // size_t
#include <stdint.h> // uint_fast32_t

#define PAGE_ORDER 12 // the number of bits needed to index through the smallest page
#define PAGE_SIZE (size_t)(1 << PAGE_ORDER) // the size of the smallest page

// PTE flags. Note: V, A, and D are managed internally; they should not be set
// in any flags parameter passed to the functions below.

#define PTE_V (1 << 0)
#define PTE_R (1 << 1)
#define PTE_W (1 << 2)
#define PTE_X (1 << 3)
#define PTE_U (1 << 4)
#define PTE_G (1 << 5)
#define PTE_A (1 << 6)
#define PTE_D (1 << 7)

// COMPILE-TIME CONFIGURATION
//

// Minimum amount of memory in the initial heap block.

#ifndef HEAP_INIT_MIN
#define HEAP_INIT_MIN 256
#endif

// CONSTANT DEFINITIONS
//

#define MEGA_SIZE ((1UL << 9) * PAGE_SIZE) // megapage size
#define GIGA_SIZE ((1UL << 9) * MEGA_SIZE) // gigapage size

#define PTE_CNT (PAGE_SIZE/8) // number of PTEs per page table

// EXPORTED TYPE DEFINITIONS
//

// EXPORTED VARIABLE DECLARATIONS
//

extern uintptr_t main_mtag;

// EXPORTED FUNCTION DECLARATIONS
//

// void memory_init(void)
// Initializes the memory manager. Must be called before calling any other
// functions of the memory manager.

extern void memory_init(void);
extern char memory_initialized;

// uintptr_t memory_space_create(void)
// Creates a new memory space and makes it the currently active space. Returns a
// memory space tag (type uintptr_t) that may be used to refer to the memory
// space. The created memory space contains the same identity mapping of MMIO
// address space and RAM as the main memory space. This function never fails; if
// there are not enough physical memory pages to create the new memory space, it
// panics.

extern uintptr_t memory_space_create(uint_fast16_t asid);

// void memory_space_reclaim(uintptr_t mtag)
// Switches the active memory space to the main memory space and reclaims the
// memory space that was active on entry. All physical pages mapped by a user
// mapping are reclaimed.

extern void memory_space_reclaim(void);

// uintptr_t active_memory_space(void)
// Returns the memory space tag of the current memory space.

static inline uintptr_t active_memory_space(void);

// uintptr_t memory_space_switch(uintptr_t mtag)
// Switches to another memory space and returns the memory space tag of the
// previously active memory space.

static inline uintptr_t memory_space_switch(uintptr_t mtag);

// void * memory_alloc_page(void)
// Allocates a physical page of memory. Returns a pointer to the direct-mapped
// address of the page. Does not fail; panics if there are no free pages available.

extern void * memory_alloc_page(void);

// void memory_free_page(void * ptr)
// Returns a physical memory page to the physical page allocator. The page must
// have been previously allocated by memory_alloc_page.

extern void memory_free_page(void * pp);

// void * memory_alloc_and_map_page (
//        uintptr_t vma, uint_fast8_t rwxug_flags)
// Allocates and maps a physical page.
// Maps a virtual page to a physical page in the current memory space. The /vma/
// argument gives the virtual address of the page to map. The /pp/ argument is a
// pointer to the physical page to map. The /rwxug_flags/ argument is an OR of
// the PTE flags, of which only a combination of R, W, X, U, and G should be
// specified. (The D, A, and V flags are always added by memory_map_page.) The
// function returns a pointer to the mapped virtual page, i.e., (void*)vma.
// Does not fail; panics if the request cannot be satsified.

extern void * memory_alloc_and_map_page (
    uintptr_t vma, uint_fast8_t rwxug_flags);

// void * memory_alloc_and_map_range (
//        uintptr_t vma, size_t size, uint_fast8_t rwxug_flags)

// Allocates and maps multiple physical pages in an address range. Equivalent to
// calling memory_alloc_and_map_page for every page in the range.

extern void * memory_alloc_and_map_range (
    uintptr_t vma, size_t size, uint_fast8_t rwxug_flags);

// void memory_unmap_and_free_range(void * vp, size_t size)

// void memory_unmap_and_free_user(void)
// Unmaps and frees all pages with the U bit set in the PTE flags.

extern void memory_unmap_and_free_user(void);

// void memory_set_range_flags (
//      const void * vp, size_t size, uint_fast8_t rwxug_flags)
// Chnages the PTE flags for all pages in a mapped range.

extern void memory_set_range_flags (
const void * vp, size_t size, uint_fast8_t rwxug_flags);

// int memory_validate_vptr_len (
//     const void * vp, size_t len, uint_fast8_t rwxug_flags);
// Checks if a virtual address range is mapped with specified flags. Returns 1
// if and only if every virtual page containing the specified virtual address
// range is mapped with the at least the specified flags.

extern int memory_validate_vptr_len (
    const void * vp, size_t len, uint_fast8_t rwxug_flags);

// int memory_validate_vstr (
//     const char * vs, uint_fast8_t ug_flags)
// Checks if the virtual pointer points to a mapped range containing a
// null-terminated string. Returns 1 if and only if the virtual pointer points
// to a mapped readable page with the specified flags, and every byte starting
// at /vs/ up until the terminating null byte is also mapped with the same
// permissions.

extern int memory_validate_vstr (
    const char * vs, uint_fast8_t ug_flags);

// Called from excp.c to handle a page fault at the specified address. Either
// maps a page containing the faulting address, or calls process_exit().

extern void memory_handle_page_fault(const void * vptr);

// INLINE FUNCTION DEFINITIONS
//

static inline uintptr_t active_memory_space(void) {
    return csrr_satp();
}

static inline uintptr_t memory_space_switch(uintptr_t mtag) {
    return csrrw_satp(mtag);
}

#endif // _MEMORY_H_
