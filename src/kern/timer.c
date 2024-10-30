// timer.c
//

#include "timer.h"
#include "thread.h"
#include "csr.h"
#include "intr.h"
#include "halt.h" // for assert

// EXPORTED GLOBAL VARIABLE DEFINITIONS
// 

char timer_initialized = 0;

struct condition tick_1Hz;
struct condition tick_10Hz;

uint64_t tick_1Hz_count;
uint64_t tick_10Hz_count;

#define MTIME_FREQ 10000000 // from QEMU include/hw/intc/riscv_aclint.h
#define PERIOD 10           // 10Hz
// INTERNVAL GLOBAL VARIABLE DEFINITIONS
//

// INTERNAL FUNCTION DECLARATIONS
//

static inline uint64_t get_mtime(void);
static inline void set_mtime(uint64_t val);
static inline uint64_t get_mtimecmp(void);
static inline void set_mtimecmp(uint64_t val);

// EXPORTED FUNCTION DEFINITIONS
//

void timer_init(void) {
    assert (intr_initialized);
    condition_init(&tick_1Hz, "tick_1Hz");
    condition_init(&tick_10Hz, "tick_10Hz");

    // Set mtimecmp to maximum so timer interrupt does not fire

    set_mtime(0);
    set_mtimecmp(UINT64_MAX);
    csrc_mie(RISCV_MIE_MTIE);

    timer_initialized = 1;
}

void timer_start(void) {
    set_mtime(0);
    set_mtimecmp(MTIME_FREQ / 10);
    csrs_mie(RISCV_MIE_MTIE);
}

// timer_handle_interrupt() is dispatched from intr_handler in intr.c

/**
 * @brief Timer interrupt handler.
 *
 * This function is called when a timer interrupt occurs. It broadcasts
 * conditions at 1Hz and 10Hz intervals and updates the timer compare
 * register to schedule the next interrupt.
 *
 * @note The function assumes the existence of global variables
 *       `tick_10Hz_count`, `tick_1Hz`, `tick_1Hz_count`, and functions
 *       `condition_broadcast`, `set_mtimecmp`, and `get_mtime`.
 */

void timer_intr_handler(void) {
    // FIXME your code goes here

    if (tick_10Hz_count % PERIOD == 0)
    {
        condition_broadcast(&tick_1Hz);
        tick_1Hz_count++;
    }
    condition_broadcast(&tick_10Hz);
    tick_10Hz_count++;
    set_mtimecmp(get_mtime() + MTIME_FREQ / PERIOD);
}

// Hard-coded MTIMER device addresses for QEMU virt device

#define MTIME_ADDR 0x200BFF8
#define MTCMP_ADDR 0x2004000

static inline uint64_t get_mtime(void) {
    return *(volatile uint64_t*)MTIME_ADDR;
}

static inline void set_mtime(uint64_t val) {
    *(volatile uint64_t*)MTIME_ADDR = val;
}

static inline uint64_t get_mtimecmp(void) {
    return *(volatile uint64_t*)MTCMP_ADDR;
}

static inline void set_mtimecmp(uint64_t val) {
    *(volatile uint64_t*)MTCMP_ADDR = val;
}
