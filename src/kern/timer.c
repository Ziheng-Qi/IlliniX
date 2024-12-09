// timer.c
//

#include "timer.h"
#include "thread.h"
#include "csr.h"
#include "intr.h"
#include "halt.h" // for assert

#include "config.h"
#include <limits.h>

// INTERNAL CONSTANTS
//

#ifndef TICK_FREQ
#define TICK_FREQ 50
#endif /* TICK_FREQ */

#define TICK_PERIOD (TIMER_FREQ/TICK_FREQ)



// EXPORTED GLOBAL VARIABLE DEFINITIONS
// 

char timer_initialized = 0;

// INTERNVAL GLOBAL VARIABLE DEFINITIONS
//

static struct alarm * sleep_list;
static uint64_t next_tick;

// INTERNAL FUNCTION DECLARATIONS
//

static void enable_mmode_timer_intr(void);

static inline uint64_t get_mtime(void);
static inline void set_mtime(uint64_t val);
static inline uint64_t get_mtcmp(void);
static inline void set_mtcmp(uint64_t val);

// EXPORTED FUNCTION DEFINITIONS
//

void timer_init(void) {
    set_mtime(0);
    set_mtcmp(TICK_PERIOD);
    csrs_sie(RISCV_SIE_STIE);
    enable_mmode_timer_intr();

    timer_initialized = 1;
}

void alarm_init(struct alarm * al, const char * name) {
    condition_init(&al->cond, name ? name : "alarm");
    al->twake = get_mtime();
    al->next = NULL;
}

void alarm_sleep(struct alarm * al, uint64_t tcnt) {
    struct alarm * prev;
    int saved_intr_state;
    uint64_t now;

    now = get_mtime();

    // If the tcnt is so large it wraps around, set it to UINT64_MAX

    if (UINT64_MAX - al->twake < tcnt)
        al->twake = UINT64_MAX;
    else
        al->twake += tcnt;
    
    // If the wake-up time has already passed, return

    if (al->twake < now)
        return;
    
    saved_intr_state = intr_disable();

    if (sleep_list == NULL || al->twake <= sleep_list->twake) {
        debug("[%lu] Inserting alarm %s at head of list", now, al->cond.name);
        // Insert alarm at head of sleep list
        al->next = sleep_list;
        sleep_list = al;
        // If current alarm occurs before next tick, update mtcmp

        if (al->twake < next_tick) {
            set_mtcmp(al->twake);
            csrs_sie(RISCV_SIE_STIE);
            enable_mmode_timer_intr();
        }


    } else {
        // Insert current alarm in list in order of wake-up time. Ideally, we
        // should not keep interrupts disabled while we iterate through the
        // list. The right way would be to restore interrupt state, find the
        // place on the list where we want to insert, the disable interrupts and
        // re-check the insert point.

        for (prev = sleep_list; prev->next != NULL; prev = prev->next) {
            if (al->twake <= prev->next->twake) {
                debug("[%lu] Inserting alarm %s after %s",
                    now, al->cond.name, prev->cond.name);
                al->next = prev->next;
                prev->next = al;
                break;
            }
        }

        // End of list, insert at tail
        if (prev->next == NULL) {
            debug("%[lu] Inserting alarm %s at tail", al->cond.name);
            al->next = NULL;
            prev->next = al;
        }
    }

    debug("[%lu] Next timer interrupt set for %lu ticks", now, get_mtcmp());

    // Note: condition_wait must be *inside* intr_disable/intr_restore block to
    // prevent a race condition where an alarm is signalled before we call
    // condition_wait.

    condition_wait(&al->cond);

    intr_restore(saved_intr_state);
}

// Resets the alarm so that the next sleep increment is relative to the time
// alarm_reset is called.

void alarm_reset(struct alarm * al) {
    al->twake = get_mtime();
}

// timer_handle_interrupt() is dispatched from intr_handler in intr.c

void timer_intr_handler(struct trap_frame * tfr) {
    struct alarm * head = sleep_list;
    struct alarm * next;
    uint64_t now;

    now = get_mtime();

    trace("[%lu] %s()", now, __func__);
    debug("[%lu] mtcmp = %lu", now, get_mtcmp());

    while (head != NULL && head->twake <= now) {
        debug("[%lu] Broadcasting alarm for %s", now, head->cond.name);
        condition_broadcast(&head->cond);
        next = head->next;
        head->next = NULL;
        head = next;
    }

    if (next_tick < now)
        next_tick += TICK_PERIOD;

    sleep_list = head;

    if (head != NULL && head->twake < next_tick)
        set_mtcmp(head->twake);
    else
        set_mtcmp(next_tick);


    debug("[%lu] Next timer interrupt set for %lu ticks", now, get_mtcmp());
    enable_mmode_timer_intr();

}

void enable_mmode_timer_intr(void) {
    // see _mmode_trap_handler in trapasm.s
    asm ("ecall" ::: "memory");
}

#define MTIME_ADDR 0x200BFF8
#define MTCMP_ADDR 0x2004000

static inline uint64_t get_mtime(void) {
    return *(volatile uint64_t*)MTIME_ADDR;
}

static inline void set_mtime(uint64_t val) {
    *(volatile uint64_t*)MTIME_ADDR = val;
}

static inline uint64_t get_mtcmp(void) {
    return *(volatile uint64_t*)MTCMP_ADDR;
}

static inline void set_mtcmp(uint64_t val) {
    *(volatile uint64_t*)MTCMP_ADDR = val;
}
