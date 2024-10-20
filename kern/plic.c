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

/*
Inputs - uint32_t srcno: 4 byte source number value
uint32_t level: Priority Value assigned to Interrupt Source. 

Outputs - None

Purpose - Set priority level of interrupt source. This function modifies the priority array in PLIC memory. 

Effect - Writes the passed priority level to the memory-mapped priority array for the specific interrupt source. 
If the source number or the priority level are out of bounds, then function returns without making changes.
*/
void plic_set_source_priority(uint32_t srcno, uint32_t level) {
    // FIXME your code goes here
    // Ensure the priority level and source number are within valid range
    if ((level> PLIC_PRIO_MAX) || (srcno >= PLIC_SRCCNT)){
        return;
    }
    // Calculate memory address of the priority register for given interrupt source. 
    // We source number by 4 because each source number is a 4 byte value  
    volatile uint32_t *priorityAdd = (volatile uint32_t *)(PLIC_IOBASE + (srcno * 4));
    // Set the value at the memory address to the level.
    *priorityAdd = level;
}


/*
Inputs - uint32_t srcno: 4 byte source number value

Outputs - 1: Interrupt source has a pending request, 0: If otherwise

Purpose - To check if an interrupt source has a pending request for an interrupt. Represented in PLIC's pending array.

Effect - Read the pending array in PLIC's memory-mapped data and check if the pending bit is set.
If the source number is out of bounds, then function returns without making changes.
*/
int plic_source_pending(uint32_t srcno) {
    // FIXME your code goes here
    // Check if source number is within valid range
    if ((srcno>= PLIC_SRCCNT)){
        return;
    }
    // Calculates the index of the 32-bit word in the pending array. Each source number takes 4 bytes in the array. 
    uint32_t word_index = (srcno / 32)*4;
    // Calculate the memory address of the pending register for the given source number
    // Add PLIC base memory address , word index, and Base address for the pending array
    volatile uint32_t *pendingAddr = (volatile uint32_t *)(PLIC_IOBASE + Pending_Bits_BASE + word_index);

    // Check if the pending bit for the given source is set. Return 1 if true and 0 if false.
    if (*pendingAddr & (1 << (srcno % 32)))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}


/*
Inputs - uint32_t srcno: 4 byte source number value
uint32_t ctxno: 4 byte context number value

Outputs - None

Purpose - To set the enable bit for an interrupt source for a given context number

Effect - Sets the bit in enable register for context and source to enable the interrupt source. Updates the PLIC enable array.
If the source number or context number is out of bounds, then function returns without making changes.
*/
void plic_enable_source_for_context(uint32_t ctxno, uint32_t srcno) {
    // FIXME your code goes here
    // Check if context number and source number in range
    if ((ctxno >= PLIC_CTXCNT) || (srcno>= PLIC_SRCCNT)){
        return;
    }
    // Calculate word index of 32-bit source number (4 bytes)
    uint32_t word_index = (srcno / 32)*4;
    // Calculate memory address of source number in Enable Address.
    // Calculate by adding base address of PLIC registers, base address of enable array, context number * size of enable array, and word index.
    volatile uint32_t *enableAdd = (volatile uint32_t *)(PLIC_IOBASE + Enable_Bits_BASE + (ctxno * Enable_Array_Size) + word_index);
    // Set the bit related to the source number in the enable array.
    *enableAdd |= 1 << (srcno % 32);
}


/*
Inputs - uint32_t srcid: 4 byte source number value
uint32_t ctxno: 4 byte context number value

Outputs - None

Purpose - To disable the interrupt source for a given context number by clearing its bit in enable array

Effect - Clears the bit in enable array to update the PLIC enable array and disable the interrupt source.
If the source number or context number is out of bounds, then function returns without making changes.
*/
void plic_disable_source_for_context(uint32_t ctxno, uint32_t srcid) {
    // FIXME your code goes here
    // Check context number and source id are in range.
    if ((ctxno >= PLIC_CTXCNT) || (srcid>= PLIC_SRCCNT)){
        return;
    }
    // Calculate the 32-bit word_index. Multiply by 4 as each source number is bytes long.
    uint32_t word_index = (srcid / 32)*4;

    // Calculate the address in enable array for the source number and context.
    // Calculate by adding the PLIC base address, Enable Base Array address, context number * Enable array size, word index.
    volatile uint32_t *enableAdd = (volatile uint32_t *)(PLIC_IOBASE + Enable_Bits_BASE + (ctxno * Enable_Array_Size) + word_index);
    // Disable the interrupt source by clearing bit in the enable address array.
    *enableAdd &= ~(1 << (srcid % 32));
}


/*
Inputs - uint32_t level: 4 byte Priority Level value
uint32_t ctxno: 4 byte context number value

Outputs - None

Purpose - Sets the Interrupt Priority Threshold for a context in the PLIC priority threshold array. 
These threshold values will help determine what interrupts will be handled.

Effect - Updates the PLIC threshold register for the context by setting the minimum threshold level. 
If the priority level value or context number is out of bounds, then function returns without making changes.
*/
void plic_set_context_threshold(uint32_t ctxno, uint32_t level) {
    // FIXME your code goes here
    // Check if the priority level and context number in range. 
    if ((level> PLIC_PRIO_MAX )||(ctxno >= PLIC_CTXCNT)){
        return;
    }
    // Calculate address of context threshold register in PLIC.
    // Calculate by adding PLIC base address, Base address of Priority Registers, context number * Offset between Contexts' Registers.
    volatile uint32_t *thresh = (volatile uint32_t *)(PLIC_IOBASE + MIN_Inter_Priority + (ctxno * Pending_Bits_BASE ));
    // Set the threshold level for the context 
    *thresh = level;
}

/*
Inputs -  uint32_t ctxno: 4 byte context number value

Outputs - uint32_t - Returns interrupt ID of highest-priority pending interrupt.

Purpose - Claims highest priority interrupt that is pending for given context.  

Effect - Reads and returns the ID of highest-priority interrupt located in the claim register.
If the context number is out of bounds, then function returns without making changes.
*/
uint32_t plic_claim_context_interrupt(uint32_t ctxno) {
    // FIXME your code goes here
    // Check if context number in range
    if ((ctxno >= PLIC_CTXCNT)){
        return;
    }
    // Calculate the memory address of the claim register for the context.
    // Calculate by adding PLIC Base Address, base address of Claim/Complete Reg, context number * Offset between Contexts' Registers.
    volatile uint32_t *claim = (volatile uint32_t *)(PLIC_IOBASE + CLAIM_COMPLETE_REG + (ctxno * Pending_Bits_BASE));
    // Return the value at claim register.
    return *claim;
}

/*
Inputs -  uint32_t ctxno: 4 byte context number value
uint32_t srcno: 4 byte source number value

Outputs - None

Purpose - Signals to the PLIC complete register that the interrupt has been handled. 

Effect - Writes the source number back to the copmlete register to indicate the interrupt has been serviced, Now the PLIC can get new interrupts for this source,
If the context number and source number is out of bounds, then function returns without making changes.
*/
void plic_complete_context_interrupt(uint32_t ctxno, uint32_t srcno) {
    // FIXME your code goes here
    // Check if context number and source number are in range.
    if ((ctxno >= PLIC_CTXCNT) || (srcno>= PLIC_SRCCNT)){
        return;
    }
    // Calculate the memory address of the complete register for the given context.
    // Calculate by adding PLIC Base Address, base address of Claim/Complete Reg, context number * Offset between Contexts' Registers.
    volatile uint32_t *complete = (volatile uint32_t *)(PLIC_IOBASE + CLAIM_COMPLETE_REG + (ctxno * Pending_Bits_BASE));
    // Write the interrupt source number to complete register to mark the interrupt has been handled.
    *complete = srcno;
}