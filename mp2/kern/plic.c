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
    volatile uint32_t *priority_addr = (volatile uint32_t *)(PLIC_IOBASE + (srcno * 4));
    //console_printf("Priority Set for source %u at address: %p with level: %u,\n", srcno, priority_addr,level);
    *priority_addr = level;
}

int plic_source_pending(uint32_t srcno) {
    // FIXME your code goes here
    uint32_t word_index = srcno / 32;
    uint32_t bit_index = srcno % 32;

    volatile uint32_t *pending_base = (volatile uint32_t *)(PLIC_IOBASE + 0x1000);
    volatile uint32_t *pending_addr = pending_base + word_index;

    //console_printf("Pending Status for source %u at address: %p with bit pos: %u,\n", srcno, pending_base,bit_index);

    if (*pending_addr & (1 << bit_index))
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
    uint32_t word_index = srcno / 32;
    uint32_t bit_index = srcno % 32;
    volatile uint32_t *enable_base = (volatile uint32_t *)(PLIC_IOBASE + 0x2000 + (ctxno * 0x80));
    volatile uint32_t *enable_addr = enable_base + word_index;

    //console_printf("Disable Source for source %u for context %u at address:%p\n", srcno, ctxno,enable_base);
    *enable_addr |= 1 << bit_index;
}

void plic_disable_source_for_context(uint32_t ctxno, uint32_t srcid) {
    // FIXME your code goes here
    uint32_t word_index = srcid / 32;
    uint32_t bit_index = srcid % 32;
    volatile uint32_t *enable_base = (volatile uint32_t *)(PLIC_IOBASE + 0x2000 + (ctxno * 0x80));
    volatile uint32_t *enable_addr = enable_base + word_index;
    //console_printf("Enable Source for source %u for context %u at address:%p\n", srcid, ctxno,enable_base);
    *enable_addr &= ~(1 << bit_index);
}

void plic_set_context_threshold(uint32_t ctxno, uint32_t level) {
    // FIXME your code goes here
    volatile uint32_t *threshold_addr = (volatile uint32_t *)(PLIC_IOBASE + 0x200000 + (ctxno * 0x1000 ));
    //console_printf("Set threshold for context %u at address:%p\n",ctxno,threshold_addr);
    *threshold_addr = level;
}

uint32_t plic_claim_context_interrupt(uint32_t ctxno) {
    // FIXME your code goes here
    volatile uint32_t *claim_addr = (volatile uint32_t *)(PLIC_IOBASE + 0x200004 + (ctxno * 0x1000));
    //console_printf("Claim interrupt for context %u at address:%p\n",ctxno,claim_addr);

    return *claim_addr;
}

void plic_complete_context_interrupt(uint32_t ctxno, uint32_t srcno) {
    // FIXME your code goes here
    volatile uint32_t *complete_addr = (volatile uint32_t *)(PLIC_IOBASE + 0x200004 + (ctxno * 0x1000));
    //console_printf("Complete interrupt source %u for context %u at address:%p\n", srcno, ctxno,complete_addr);
    return *complete_addr = srcno;
}