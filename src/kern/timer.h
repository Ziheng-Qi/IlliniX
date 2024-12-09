// timer.h
// 

#ifndef _TIMER_H_
#define _TIMER_H_

#include <stdint.h>
#include "thread.h" // for struct condition
#include "trap.h" // for struct trap_frame

#define TIMER_FREQ 10000000UL // from QEMU include/hw/intc/riscv_aclint.h

struct alarm {
    struct condition cond;
    struct alarm * next;
    uint64_t twake;
};

// EXPORTED FUNCTION DECLARATIONS
//

extern char timer_initialized;
extern void timer_init(void);

// Initializes an alarm. The /name/ argument is optional.

extern void alarm_init(struct alarm * al, const char * name);

// Puts the current thread to sleep for some number of ticks. The /tcnt/
// argument specifies the number of timer ticks relative to the most recent
// alarm event, either init, wake-up, or reset.

extern void alarm_sleep(struct alarm * al, uint64_t tcnt);

// Resets the alarm so that the next sleep increment is relative to the time
// of this function call.

extern void alarm_reset(struct alarm * al);

extern void timer_intr_handler(struct trap_frame * tfr); // called from intr.c

static inline void alarm_sleep_sec(struct alarm * al, unsigned int sec);
static inline void alarm_sleep_ms(struct alarm * al, unsigned long ms);
static inline void alarm_sleep_us(struct alarm * al, unsigned long us);

// INLINE FUNCTION DEFINITIONS
//

static inline void alarm_sleep_sec(struct alarm * al, unsigned int sec) {
    alarm_sleep(al, sec * TIMER_FREQ);
}

static inline void alarm_sleep_ms(struct alarm * al, unsigned long ms) {
    alarm_sleep(al, ms * (TIMER_FREQ / 1000));
}

static inline void alarm_sleep_us(struct alarm * al, unsigned long us) {
    alarm_sleep(al, us * (TIMER_FREQ / 1000 / 1000));
}


#endif // _TIMER_H_
