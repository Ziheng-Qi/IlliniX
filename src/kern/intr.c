// intr.c - Interrupt management
// 

#ifdef INTR_TRACE
#define TRACE
#endif

#ifdef INTR_DEBUG
#define DEBUG
#endif

#include "intr.h"
#include "trap.h"
#include "halt.h"
#include "csr.h"
#include "plic.h"
#include "timer.h"

#include <stddef.h>

// INTERNAL COMPILE-TIME CONSTANT DEFINITIONS
//

#ifndef NIRQ
#define NIRQ 32
#endif

// EXPORTED GLOBAL VARIABLE DEFINITIONS
// 

char intr_initialized = 0;

// INTERNAL GLOBAL VARIABLE DEFINITIONS
//

static struct {
    void (*isr)(int,void*);
    void * isr_aux;
    int prio;
} isrtab[NIRQ];

// INTERNAL FUNCTION DECLARATIONS
//

static void extern_intr_handler(void);

// EXPORTED FUNCTION DEFINITIONS
//

void intr_init(void) {
    trace("%s()", __func__);

    intr_disable(); // should be disabled already
    plic_init();

    csrw_sip(0); // clear all pending interrupts
    csrw_sie(RISCV_SIE_SEIE); //enable interrupts from plic

    intr_initialized = 1;
}

void intr_register_isr (
    int irqno, int prio,
    void (*isr)(int irqno, void * aux),
    void * isr_aux)
{
    if (irqno < 0 || NIRQ <= irqno)
        panic("irqno out of bounds");

    if (prio <= 0)
        prio = 1;

    isrtab[irqno].isr = isr;
    isrtab[irqno].isr_aux = isr_aux;
    isrtab[irqno].prio = prio;
}


void intr_enable_irq(int irqno) {
    if (isrtab[irqno].isr == NULL)
        panic("intr_enable_irq with no isr");
    plic_enable_irq(irqno, isrtab[irqno].prio);
}

void intr_disable_irq(int irqno) {
    plic_disable_irq(irqno);
}

// void intr_handler(int code, struct trap_frame * tfr)
// Called from trapasm.s to handle an interrupt. Dispataches to
// timer_intr_handler and extern_intr_handler.

void intr_handler(int code, struct trap_frame * tfr) {
    switch (code) {
    case RISCV_SCAUSE_INTR_EXCODE_SEI:
        extern_intr_handler();
        break;
    case RISCV_SCAUSE_INTR_EXCODE_STI:
        timer_intr_handler(tfr);
        break;
    default:
        panic("unhandled interrupt");
        break;
    }

    // If we were running user mode, yield thread.

    if ((tfr->sstatus & RISCV_SSTATUS_SPP) == 0)
        thread_yield();
}

// INTERNAL FUNCTION DEFINITIONS
//

void extern_intr_handler(void) {
    int irqno;

    irqno = plic_claim_irq();

    if (irqno < 0 || NIRQ <= irqno)
        panic("invalid irq");
    
    if (irqno == 0)
        return;
    
    if (isrtab[irqno].isr == NULL)
        panic("unhandled irq");
    
    isrtab[irqno].isr(irqno, isrtab[irqno].isr_aux);

    plic_close_irq(irqno);
}
