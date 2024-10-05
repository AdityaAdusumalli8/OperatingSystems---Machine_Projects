// serial.c - NS16550a serial port
// 

#include "serial.h"
#include "intr.h"
#include "halt.h"

#include <stddef.h>
#include <stdint.h>

// COMPILE-TIME CONSTANT DEFINITIONS
//

#ifndef UART0_MMIO_BASE
#define UART0_MMIO_BASE 0x10000000
#endif

#ifndef UART1_MMIO_BASE
#define UART1_MMIO_BASE 0x10000100
#endif

#ifndef UART1_IRQNO
#define UART1_IRQNO 11
#endif

#ifndef SERIAL_RBUFSZ
#define SERIAL_RBUFSZ 64
#endif

// EXPORTED VARIABLE DEFINITIONS
//

char com0_initialized = 0;
char com1_initialized = 0;

// INTERNAL TYPE DEFINITIONS
// 

struct ns16550a_regs {
    union {
        char rbr; // DLAB=0 read
        char thr; // DLAB=0 write
        uint8_t dll; // DLAB=1
    };
    
    union {
        uint8_t ier; // DLAB=0
        uint8_t dlm; // DLAB=1
    };
    
    union {
        uint8_t iir; // read
        uint8_t fcr; // write
    };

    uint8_t lcr;
    uint8_t mcr;
    uint8_t lsr;
    uint8_t msr;
    uint8_t scr;
};

#define LCR_DLAB (1 << 7)
#define LSR_OE (1 << 1)
#define LSR_DR (1 << 0)
#define LSR_THRE (1 << 5)
#define IER_DRIE (1 << 0)
#define IER_THREIE (1 << 1)


struct ringbuf {
    uint16_t hpos; // head of queue (from where elements are removed)
    uint16_t tpos; // tail of queue (where elements are inserted)
    char data[SERIAL_RBUFSZ];
};

// INTERAL FUNCTION DECLARATIONS
//

static void uart1_isr(int irqno, void * aux);

static void rbuf_init(struct ringbuf * rbuf);
static int rbuf_empty(const struct ringbuf * rbuf);
static int rbuf_full(const struct ringbuf * rbuf);
static void rbuf_put(struct ringbuf * rbuf, char c);
static char rbuf_get(struct ringbuf * rbuf);

static void rbuf_wait_not_empty(const struct ringbuf * rbuf);
static void rbuf_wait_not_full(const struct ringbuf * rbuf);


// INTERNAL MACRO DEFINITIONS
//

#define UART0 (*(volatile struct ns16550a_regs*)UART0_MMIO_BASE)
#define UART1 (*(volatile struct ns16550a_regs*)UART1_MMIO_BASE)

// INTERNAL GLOBAL VARIABLES
//

static struct ringbuf uart1_rxbuf;
static struct ringbuf uart1_txbuf;

// EXPORTED FUNCTION DEFINITIONS
//

void com0_init(void) {
    // Configure UART0. We set the baud rate divisor to 1, the lowest value,
    // for the fastest baud rate. In a physical system, the actual baud rate
    // depends on the attached oscillator frequency. In a virtualized system,
    // it doesn't matter.
    
    UART0.lcr = LCR_DLAB;
    UART0.dll = 0x01;
    UART0.dlm = 0x00;
    UART0.lcr = 0; // DLAB=0

    UART0.rbr; // flush receive buffer
    UART0.ier = 0x00;
}

void com0_putc(char c) {
    // Spin until THR is empty
    while (!(UART0.lsr & LSR_THRE))
        continue;

    UART0.thr = c;
}

char com0_getc(void) {
    // Spin until RBR contains a byte
    while (!(UART0.lsr & LSR_DR))
        continue;
    
    return UART0.rbr;
}

void com1_init(void) {
    // FIXME your code goes here
}

void com1_putc(char c) {
    // FIXME your code goes here
}

char com1_getc(void) {
    // FIXME your code goes here
}

static void uart1_isr (int irqno, void * aux) {
    const uint_fast8_t line_status = UART1.lsr;

    if (line_status & LSR_OE)
        panic("Receive buffer overrun");
    
    // FIXME your code goes here
}

void rbuf_init(struct ringbuf * rbuf) {
    rbuf->hpos = 0;
    rbuf->tpos = 0;
}

int rbuf_empty(const struct ringbuf * rbuf) {
    const volatile struct ringbuf * const vrbuf = rbuf;
    return (rbuf->hpos == vrbuf->tpos);
}

int rbuf_full(const struct ringbuf * rbuf) {
    const volatile struct ringbuf * const vrbuf = rbuf;
    return (rbuf->tpos - vrbuf->hpos == SERIAL_RBUFSZ);
}

void rbuf_put(struct ringbuf * rbuf, char c) {
    rbuf->data[rbuf->tpos++ % SERIAL_RBUFSZ] = c;
}

char rbuf_get(struct ringbuf * rbuf) {
    return rbuf->data[rbuf->hpos++ % SERIAL_RBUFSZ];
}

// The rbuf_wait_not_empty and rbuf_wait_not_full functions until a ring buffer
// is not empty and not full, respectively, using the wait-for-interrupt
// intruction. (This is more efficient than spin-waiting.)

void rbuf_wait_not_empty(const struct ringbuf * rbuf) {
    // Note: we need to disable interrupts to avoid a race condition where the
    // ISR runs and places a character in the buffer after we check if its empty
    // but before we execute the wfi instruction.

    intr_disable();

    while (rbuf_empty(rbuf)) {
        asm ("wfi"); // wait until interrupt pending
        intr_enable();
        intr_disable();
    }

    intr_enable();
}

void rbuf_wait_not_full(const struct ringbuf * rbuf) {
    // See comment in rbuf_wait_not_empty().

    intr_disable();

    while (rbuf_full(rbuf)) {
        asm ("wfi"); // wait until interrupt pending
        intr_enable();
        intr_disable();
    }

    intr_enable();
}