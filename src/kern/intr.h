// intr.h - Interrupt management
// 

#ifndef _INTR_H_
#define _INTR_H_

#include "csr.h"
#include "plic.h"

// EXPORTED CONSTANT DEFINITIONS
//

#define INTR_PRIO_MIN PLIC_PRIO_MIN
#define INTR_PRIO_MAX PLIC_PRIO_MAX

// EXPORTED GLOBAL VARIABLE DECLARATIONS
// 

extern char intr_initialized;

// EXPORTED FUNCTION DECLARATIONS
// 

extern void intr_init(void);

static inline int intr_enable(void);
static inline int intr_disable(void);
static inline void intr_restore(int saved);

// intr_enabled and intr_disabled return the current status of interrupts

static inline int intr_enabled(void);
static inline int intr_disabled(void);

extern void intr_register_isr (
    int irqno, int prio, void (*isr)(int irqno, void * aux), void * isr_aux);

extern void intr_enable_irq(int irqno);
extern void intr_disable_irq(int irqno);

// INLINE FUNCTION DEFINITIONS
//

static inline int intr_enable(void) {
    int64_t sstatus;

    asm volatile (
    "csrrsi  %0, sstatus, %1"
    :   "=r" (sstatus)
    :   "I" (RISCV_SSTATUS_SIE)
    :   "memory");

    return (int)sstatus;
}

static inline int intr_disable(void) {
    int64_t sstatus;

    asm volatile (
    "csrrci %0, sstatus, %1"
    :   "=r" (sstatus)
    :   "I" (RISCV_SSTATUS_SIE)
    :   "memory");

    return (int)sstatus;
}

// Restores sstatus.SIE bit to previous state; assumes sstatus.SIE = 0.

static inline void intr_restore(int saved_intr_state) {
    asm volatile (
    "csrci sstatus, %0" "\n\t"
    "csrs  sstatus, %1"
    ::  "I" (RISCV_SSTATUS_SIE),
        "r" (saved_intr_state & RISCV_SSTATUS_SIE)
    :   "memory");
}

static int intr_enabled(void) {
    return ((csrr_sstatus() & RISCV_SSTATUS_SIE) != 0);
}

static int intr_disabled(void) {
    return ((csrr_sstatus() & RISCV_SSTATUS_SIE) == 0);
}

#endif // _INTR_H_
