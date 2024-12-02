// plic.c - RISC-V PLIC
//

#include "plic.h"
#include "console.h"

#include <stdint.h>

// COMPILE-TIME CONFIGURATION
//

// *** Note to student: you MUST use PLIC_IOBASE for all address calculations,
// as this will be used for testing!

#ifndef PLIC_IOBASE
#define PLIC_IOBASE 0x0C000000
#endif

#define INTERRUPT_PENDING 0x1000
#define ENABLING_OFFSET 0x2000
#define THRESHOLD_OFFSET 0x200000
#define THRESHOLD_DIFF 0x1000
#define CLAIM_OFFSET 0x200004
#define COMPLETE_OFFSET 0x200004
#define PLIC_SRCCNT 0x400
#define PLIC_CTXCNT 1
#define DATA_SIZE 32
#define PRIORITY_OFFSET 0x4

// INTERNAL FUNCTION DECLARATIONS
//

// *** Note to student: the following MUST be declared extern. Do not change these
// function delcarations!

extern void plic_set_source_priority(uint32_t srcno, uint32_t level);
extern int plic_source_pending(uint32_t srcno);
extern void plic_enable_source_for_context(uint32_t ctxno, uint32_t srcno);
extern void plic_disable_source_for_context(uint32_t ctxno, uint32_t srcno);
extern void plic_set_context_threshold(uint32_t ctxno, uint32_t level);
extern uint32_t plic_claim_context_interrupt(uint32_t ctxno);
extern void plic_complete_context_interrupt(uint32_t ctxno, uint32_t srcno);

// Currently supports only single-hart operation. The low-level PLIC functions
// already understand contexts, so we only need to modify the high-level
// functions (plic_init, plic_claim, plic_complete).

// EXPORTED FUNCTION DEFINITIONS
// 

void plic_init(void) {
    int i;

    // Disable all sources by setting priority to 0, enable all sources for
    // context 0 (M mode on hart 0).
    // context 1 (S mode on hart 0)

    for (i = 0; i < PLIC_SRCCNT; i++) {
        plic_set_source_priority(i, 0);
        plic_enable_source_for_context(1, i);
    }
}

extern void plic_enable_irq(int irqno, int prio) {
    trace("%s(irqno=%d,prio=%d)", __func__, irqno, prio);
    plic_set_source_priority(irqno, prio);
}

extern void plic_disable_irq(int irqno) {
    if (0 < irqno)
        plic_set_source_priority(irqno, 0);
    else
        debug("plic_disable_irq called with irqno = %d", irqno);
}

extern int plic_claim_irq(void) {
    // Hardwired context 1 (S mode on hart 0)
    trace("%s()", __func__);
    return plic_claim_context_interrupt(1);
}

extern void plic_close_irq(int irqno) {
    // Hardwired context 1 (S mode on hart 0)
    trace("%s(irqno=%d)", __func__, irqno);
    plic_complete_context_interrupt(1, irqno);
}

// INTERNAL FUNCTION DEFINITIONS
//
// REMINDER: THE SIZE OF THE DATA IS STILL 32-BIT, SO WE NEED TO USE 32-BIT, BUT THE ADDRESS IS 64-BIT
/**
 * @brief Sets the priority level for a given interrupt source in the PLIC.
 *
 * This function sets the priority level for the specified interrupt source.
 * The priority level must be within the valid range defined by PLIC_PRIO_MIN
 * and PLIC_PRIO_MAX. If the level is invalid, the function will trigger a panic.
 *
 * @param srcno The interrupt source number.
 * @param level The priority level to set for the interrupt source.
 *
 * @note The priority level must be between PLIC_PRIO_MIN and PLIC_PRIO_MAX.
 *       If the level is outside this range, the function will call panic().
 */
void plic_set_source_priority(uint32_t srcno, uint32_t level) {
    // FIXME your code goes here

    // Get the address of the priority register for the source number
    uint64_t priority_addr = PLIC_IOBASE + PRIORITY_OFFSET * srcno;    
    *((volatile uint32_t*)(priority_addr)) = level;
}

/**
 * @brief Checks if a specific interrupt source is pending.
 *
 * This function checks the pending status of a given interrupt source number (srcno)
 * by accessing the PLIC's pending array. It calculates the address of the pending array
 * and retrieves the pending bit for the specified source number.
 *
 * @param srcno The interrupt source number to check.
 * @return Non-zero if the interrupt source is pending, zero otherwise.
 */
int plic_source_pending(uint32_t srcno) {
    // FIXME your code goes here
    if (srcno >= PLIC_SRCCNT || srcno == 0) {
        return 0;
    } 
    // check for the pending array:
    uint64_t pending_array_addr = PLIC_IOBASE + 0x1000 + srcno / DATA_SIZE * PRIORITY_OFFSET;
    uint32_t pending_bit_array = *(uint32_t*)pending_array_addr;
    uint32_t pending_bit = 1 << (srcno % DATA_SIZE);
    return pending_bit_array & pending_bit;
}


/**
 * @brief Enable a specific interrupt source for a given context in the PLIC.
 *
 * This function enables an interrupt source for a specified context in the 
 * Platform-Level Interrupt Controller (PLIC). It ensures that the source 
 * number and context number are within valid ranges before enabling the 
 * interrupt.
 *
 * @param ctxno The context number for which the interrupt source is to be enabled.
 *              Must be in the range [0, PLIC_CTXCNT).
 * @param srcno The source number of the interrupt to be enabled.
 *              Must be in the range [1, PLIC_SRCCNT).
 *
 * @note The function does nothing if the source number is 0 or out of range,
 *       or if the context number is out of range.
 */
void plic_enable_source_for_context(uint32_t ctxno, uint32_t srcno) {
    // FIXME your code goes here
    
    if (srcno >= PLIC_SRCCNT || srcno == 0) {
        return;
    }

    if (ctxno < 0||ctxno > PLIC_CTXCNT) {
        return;
    }


    uint64_t context_offset = ctxno * (PLIC_SRCCNT / DATA_SIZE * PRIORITY_OFFSET);
    uint64_t enable_addr = PLIC_IOBASE + ENABLING_OFFSET + context_offset + srcno / DATA_SIZE * PRIORITY_OFFSET;
    uint32_t enable_bit = 1 << (srcno % DATA_SIZE);

    *(volatile uint32_t*)enable_addr |= enable_bit;
}


/**
 * @brief Disables a specific interrupt source for a given context in the PLIC.
 *
 * This function disables the interrupt source identified by `srcno` for the context
 * identified by `ctxno` in the Platform-Level Interrupt Controller (PLIC).
 *
 * @param ctxno The context number for which the interrupt source should be disabled.
 *              Must be in the range [0, PLIC_CTXCNT-1].
 * @param srcno The source number of the interrupt to be disabled.
 *              Must be in the range [1, PLIC_SRCCNT-1].
 *
 * @note If `srcno` is 0 or greater than or equal to PLIC_SRCCNT, the function returns
 *       without making any changes. Similarly, if `ctxno` is less than 0 or greater than
 *       or equal to PLIC_CTXCNT, the function returns without making any changes.
 */
void plic_disable_source_for_context(uint32_t ctxno, uint32_t srcno) {
    // FIXME your code goes here
    if (srcno >= PLIC_SRCCNT || srcno == 0) {
        return;
    }

    if (ctxno < 0||ctxno > PLIC_CTXCNT) {
        return;
    }

    uint64_t context_offset = ctxno * (PLIC_SRCCNT / DATA_SIZE * PRIORITY_OFFSET);
    uint64_t enable_addr = PLIC_IOBASE + ENABLING_OFFSET + context_offset + srcno / DATA_SIZE * PRIORITY_OFFSET;
    uint32_t enable_bit = 1 << (srcno % DATA_SIZE);
    uint32_t disable_bit = ~enable_bit;

    *(volatile uint32_t*)enable_addr &= disable_bit;

}

/*
 * @brief Implementation of functions to interact with the Platform-Level Interrupt Controller (PLIC).
 */

/**
 * @brief Sets the interrupt priority threshold for a given context in the PLIC.
 *
 * This function sets the priority threshold for the specified context number (ctxno) to the given level.
 * The threshold determines the minimum priority level of interrupts that will be forwarded to the context.
 *
 * @param ctxno The context number for which the threshold is being set. Must be between 0 and PLIC_CTXCNT-1.
 * @param level The priority threshold level to set for the specified context.
 */


void plic_set_context_threshold(uint32_t ctxno, uint32_t level) {
    // FIXME your code goes here

    if (ctxno < 0||ctxno > PLIC_CTXCNT) {
        return;
    }
    if (level < PLIC_PRIO_MIN || level > PLIC_PRIO_MAX) {
        return;
    }
    uint64_t threshold_addr = PLIC_IOBASE + THRESHOLD_OFFSET + ctxno * THRESHOLD_DIFF;

    *(volatile uint32_t*)threshold_addr = level;
}

/**
 * @brief Claims an interrupt for a given context number.
 *
 * This function checks if the provided context number is within the valid range.
 * If the context number is valid, it calculates the address for the claim register
 * and reads the interrupt source ID from that address.
 *
 * @param ctxno The context number for which to claim the interrupt.
 * @return The source ID of the claimed interrupt, or 0 if the context number is invalid.
 */


uint32_t plic_claim_context_interrupt(uint32_t ctxno) {
    // FIXME your code goes here

    if (ctxno < 0||ctxno > PLIC_CTXCNT) {
        return 0;
    }

    uint64_t claim_addr = PLIC_IOBASE + CLAIM_OFFSET + ctxno * THRESHOLD_DIFF;

    uint32_t claim = *(uint32_t*)claim_addr; // claim the interrupt, now claim is the source ID

    return claim;
}



/**
 * @brief Completes the interrupt for a given context and source.
 *
 * This function marks the interrupt as completed for the specified context and source number.
 * It ensures that the context and source numbers are within valid ranges before proceeding.
 *
 * @param ctxno The context number for which the interrupt is being completed. Must be within the range [0, PLIC_CTXCNT).
 * @param srcno The source number of the interrupt being completed. Must be within the range [1, PLIC_SRCCNT).
 */
void plic_complete_context_interrupt(uint32_t ctxno, uint32_t srcno) {
      // FIXME your code goes here

    if (ctxno < 0||ctxno > PLIC_CTXCNT) {
        return;
    }

    if (srcno >= PLIC_SRCCNT || srcno == 0) {
        return;
    }

    uint64_t complete_addr = PLIC_IOBASE + COMPLETE_OFFSET + ctxno * THRESHOLD_DIFF;

    *(volatile uint32_t*)complete_addr = srcno;
    
}