// ezheap.c - Leaking memory manager for small allocations
//

#ifndef TRACE
#ifdef HEAP_TRACE
#define TRACE
#endif
#endif

#ifndef DEBUG
#ifdef HEAP_DEBUG
#define DEBUG
#endif
#endif

#include "heap.h"

#include "console.h"
#include "string.h"
#include "halt.h"
#include "memory.h"

#include <stdint.h>

// EXPORTED GLOBAL VARIABLES
//

char heap_initialized = 0;

// INTERNAL GLOBAL VARIABLES
//

static void * heap_start;
static void * heap_end;

// EXPORTED FUNCTION DEFINITIONS
//

void heap_init(void * start, void * end) {
    trace("%s(%p,%p)", __func__, start, end);
    assert (start < end);
    heap_start = start;
    heap_end = end;
    heap_initialized = 1;
}

void * kmalloc(size_t size) {
    void * new_block;

    trace("%s(%zu)", __func__, size);

    // Round up to multiple of 16 bytes
    size = (size + 16-1) / 16 * 16;

    if (PAGE_SIZE < size)
        panic("heap alloc request too large");
    
    // If the request fits in the current heap block, allocate from it.

    if (size <= heap_end - heap_start) {
        heap_end -= size;
        return heap_end;
    }

    // The request is no more than a page, but we don't have room for it in the
    // current block of heap memory. Get a direct-mapped page of physical memory
    // from the memory manager.

    new_block = memory_alloc_page();

    // Do we have more free space left if we abandon the current block and
    // switch to the new one, or just use the new block for this request and
    // stay with the current block?

    if (heap_end - heap_start < PAGE_SIZE - size) {
        // switch to new block
        heap_start = new_block;
        heap_end = new_block + PAGE_SIZE - size;
        return heap_end;
    } else
        return new_block;
}

void * kcalloc(size_t n, size_t size) {
    void * ptr;

    trace("%s(%zu,%zu)", __func__, n, size);

    if (SIZE_MAX / size < n)
        panic("heap alloc request too large");

    ptr = kmalloc(n * size);
    memset(ptr, 0, n * size);
    return ptr;
}

void * krealloc(void * ptr, size_t size) {
    panic("krealloc not implemented");
}

void kfree(void * ptr) {
    trace("%s(%p)", __func__, ptr);
    // do nothing
}
