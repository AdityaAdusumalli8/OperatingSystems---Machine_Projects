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

#define PLIC_SRCCNT 0x400
#define PLIC_CTXCNT 1

#define Enable_Array_Size 0x80
#define Pending_Bits_BASE 0x1000
#define Enable_Bits_BASE 0x2000
#define MIN_Inter_Priority 0x200000
#define CLAIM_COMPLETE_REG 0x200004

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

    for (i = 0; i < PLIC_SRCCNT; i++) {
        plic_set_source_priority(i, 0);
        plic_enable_source_for_context(0, i);
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
    // Hardwired context 0 (M mode on hart 0)
    trace("%s()", __func__);
    return plic_claim_context_interrupt(0);
}

extern void plic_close_irq(int irqno) {
    // Hardwired context 0 (M mode on hart 0)
    trace("%s(irqno=%d)", __func__, irqno);
    plic_complete_context_interrupt(0, irqno);
}

// INTERNAL FUNCTION DEFINITIONS
//

void plic_set_source_priority(uint32_t srcno, uint32_t level) {
    // FIXME your code goes here
    if ((level> PLIC_PRIO_MAX) || (srcno >= PLIC_SRCCNT)){
        return;
    }
    volatile uint32_t *priority_addr = (volatile uint32_t *)(PLIC_IOBASE + (srcno * 4));
    *priority_addr = level;
}

int plic_source_pending(uint32_t srcno) {
    // FIXME your code goes here
    if ((srcno>= PLIC_SRCCNT)){
        return;
    }
    uint32_t word_index = (srcno / 32)*4;
    volatile uint32_t *pending_addr = (volatile uint32_t *)(PLIC_IOBASE + Pending_Bits_BASE + word_index);

    if (*pending_addr & (1 << (srcno % 32)))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void plic_enable_source_for_context(uint32_t ctxno, uint32_t srcno) {
    // FIXME your code goes here
    if ((ctxno >= PLIC_CTXCNT) || (srcno>= PLIC_SRCCNT)){
        return;
    }
    uint32_t word_index = (srcno / 32)*4;
    volatile uint32_t *enable_addr = (volatile uint32_t *)(PLIC_IOBASE + Enable_Bits_BASE + (ctxno * Enable_Array_Size) + word_index);

    *enable_addr |= 1 << (srcno % 32);
}

void plic_disable_source_for_context(uint32_t ctxno, uint32_t srcid) {
    // FIXME your code goes here
    if ((ctxno >= PLIC_CTXCNT) || (srcid>= PLIC_SRCCNT)){
        return;
    }
    uint32_t word_index = (srcid / 32)*4;
    volatile uint32_t *enable_addr = (volatile uint32_t *)(PLIC_IOBASE + Enable_Bits_BASE + (ctxno * Enable_Array_Size) + word_index);
    *enable_addr &= ~(1 << (srcid % 32));
}

void plic_set_context_threshold(uint32_t ctxno, uint32_t level) {
    // FIXME your code goes here
    if ((level> PLIC_PRIO_MAX )||(ctxno >= PLIC_CTXCNT)){
        return;
    }
    volatile uint32_t *thresh = (volatile uint32_t *)(PLIC_IOBASE + MIN_Inter_Priority + (ctxno * Pending_Bits_BASE ));
    *thresh = level;
}

uint32_t plic_claim_context_interrupt(uint32_t ctxno) {
    // FIXME your code goes here
    if ((ctxno >= PLIC_CTXCNT)){
        return;
    }
    volatile uint32_t *claim = (volatile uint32_t *)(PLIC_IOBASE + CLAIM_COMPLETE_REG + (ctxno * Pending_Bits_BASE));

    return *claim;
}

void plic_complete_context_interrupt(uint32_t ctxno, uint32_t srcno) {
    // FIXME your code goes here
    if ((ctxno >= PLIC_CTXCNT) || (srcno>= PLIC_SRCCNT)){
        return;
    }
    volatile uint32_t *complete_addr = (volatile uint32_t *)(PLIC_IOBASE + CLAIM_COMPLETE_REG + (ctxno * Pending_Bits_BASE));
    return *complete_addr = srcno;
}