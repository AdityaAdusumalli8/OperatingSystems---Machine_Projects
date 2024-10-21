// serial.c - NS16550a serial port
//

#include "serial.h"
#include "intr.h"
#include "halt.h"
#include "thread.h"

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

#ifndef UART0_IRQNO
#define UART0_IRQNO 10
#endif

#ifndef SERIAL_RBUFSZ
#define SERIAL_RBUFSZ 64
#endif

// EXPORTED VARIABLE DEFINITIONS
//

char serial_initialized = 0;

char com0_initialized = 0;
char com1_initialized = 0;

// INTERNAL TYPE DEFINITIONS
//

struct ns16550a_regs
{
    union
    {
        char rbr;    // DLAB=0 read
        char thr;    // DLAB=0 write
        uint8_t dll; // DLAB=1
    };

    union
    {
        uint8_t ier; // DLAB=0
        uint8_t dlm; // DLAB=1
    };

    union
    {
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

struct ringbuf
{
    uint16_t hpos; // head of queue (from where elements are removed)
    uint16_t tpos; // tail of queue (where elements are inserted)
    char data[SERIAL_RBUFSZ];
};

struct uart
{
    volatile struct ns16550a_regs *regs;
    void (*putc)(struct uart *uart, char c);
    char (*getc)(struct uart *uart);
    struct condition rxbuf_not_empty;
    struct condition txbuf_not_full;
    struct ringbuf rxbuf;
    struct ringbuf txbuf;
};

// INTERAL FUNCTION DECLARATIONS
//

struct uart *com_init_common(int k);

static void com_putc_sync(struct uart *uart, char c);
static char com_getc_sync(struct uart *uart);

static void com_putc_async(struct uart *uart, char c);
static char com_getc_async(struct uart *uart);

static void uart_isr(int irqno, void *aux);

static void rbuf_init(struct ringbuf *rbuf);
static int rbuf_empty(const struct ringbuf *rbuf);
static int rbuf_full(const struct ringbuf *rbuf);
static void rbuf_put(struct ringbuf *rbuf, char c);
static char rbuf_get(struct ringbuf *rbuf);

static void rbuf_wait_not_empty(const struct ringbuf *rbuf);
static void rbuf_wait_not_full(const struct ringbuf *rbuf);

// INTERNAL MACRO DEFINITIONS
//

// INTERNAL GLOBAL VARIABLES
//

static struct uart uarts[NCOM];

// EXPORTED FUNCTION DEFINITIONS
//

void com_init_sync(int k)
{
    struct uart *uart;

    assert(0 <= k && k < NCOM);

    uart = com_init_common(k);
    uart->putc = &com_putc_sync;
    uart->getc = &com_getc_sync;
}

void com_init_async(int k)
{
    struct uart *uart;

    assert(0 <= k && k < NCOM);

    uart = com_init_common(k);
    uart->putc = &com_putc_async;
    uart->getc = &com_getc_async;

    rbuf_init(&uart->rxbuf);
    rbuf_init(&uart->txbuf);

    condition_init(&uart->rxbuf_not_empty, "rxbuf_not_empty");
    condition_init(&uart->txbuf_not_full, "txbuf_not_full");

    // Register ISR and enable the IRQ
    intr_register_isr(UART0_IRQNO + k, 1, uart_isr, uart);
    intr_enable_irq(UART0_IRQNO + k);

    // Enable data ready interrupts
    uart->regs->ier = IER_DRIE;
}

int com_initialized(int k)
{
    assert(0 <= k && k < NCOM);
    return (uarts[k].putc != NULL);
}

void com_putc(int k, char c)
{
    assert(0 <= k && k < NCOM);
    uarts[k].putc(uarts + k, c);
}

char com_getc(int k)
{
    assert(0 <= k && k < NCOM);
    return uarts[k].getc(uarts + k);
}

// The following four functions are for compatibility with CP1.

void com0_init(void)
{
    com_init_sync(0);
    com0_initialized = 1;
}

void com0_putc(char c)
{
    com_putc(0, c);
}

char com0_getc(void)
{
    return com_getc(0);
}

void com1_init(void)
{
    com_init_async(1);
    com1_initialized = 1;
}

void com1_putc(char c)
{
    com_putc(1, c);
}

char com1_getc(void)
{
    return com_getc(1);
}

// INTERNAL FUNCTION DEFINITIONS
//

// Common initialization for sync and async com ports. Specifically:
//
// 1. Initializes uart->regs,
// 2. Configures UART hardware divisor register, and
// 3. Flushes RBR and clears IER.
//
// Returns a pointer to the uart object (for convenience).
//

struct uart *com_init_common(int k)
{
    struct uart *const uart = uarts + k;
    assert(0 <= k && k < NCOM);

    uart->regs =
        (void *)UART0_MMIO_BASE +
        k * (UART1_MMIO_BASE - UART0_MMIO_BASE);

    // Configure UART0. We set the baud rate divisor to 1, the lowest value,
    // for the fastest baud rate. In a physical system, the actual baud rate
    // depends on the attached oscillator frequency. In a virtualized system,
    // it doesn't matter.

    uart->regs->lcr = LCR_DLAB;
    // fence o,o
    uart->regs->dll = 0x01;
    uart->regs->dlm = 0x00;
    // fence o,o
    uart->regs->lcr = 0; // DLAB=0

    uart->regs->rbr; // flush receive buffer
    uart->regs->ier = 0;

    return uart;
}

void com_putc_sync(struct uart *uart, char c)
{
    // Spin until THR is empty (THRE=1)
    while (!(uart->regs->lsr & LSR_THRE))
        continue;

    uart->regs->thr = c;
}

char com_getc_sync(struct uart *uart)
{
    // Spin until RBR contains a byte (DR=1)
    while (!(uart->regs->lsr & LSR_DR))
        continue;

    return uart->regs->rbr;
}

/*
Inputs -  struct uart *uart : Pointer to uart structure containing specific information about the specific UART device.
char c : The character to be placed into the transmit buffer

Outputs - None

Purpose -  The purpose of this function is to place a character into the uart transmit buffer. The function makes sure
the character is only placed when space is in transmit buffer.

Effect - The effect of this function is that it waits until the buffer has space and then places the character in
the buffer. Then the Transmit Interrupt signal is enabled to allow for more characters to be written. Disable interrupts
and restores the state at the end to avoid simultaneous access to critical data.
*/
void com_putc_async(struct uart *uart, char c)
{
    // FIXME your code goes here
    // Disable interrupts to protect data and restore at end of function.
    int savedIntrState = intr_disable();
    // WHile the transmit buffer is full, wait for the condition txbuf_not_fll to be signaled before writing.
    while(rbuf_full(&uart->txbuf)){
        condition_wait(&uart->txbuf_not_full);
    }
    // If the buffer isn't full, then write character to buffer
    rbuf_put(&uart->txbuf,c);
    // Enable the Transmit Interrupt to allow for more characters to move to buffer
    uart->regs->ier |= IER_THREIE;

    intr_restore(savedIntrState);
}

/*
Inputs -  struct uart *uart : Pointer to uart structure containing specific information about the specific UART device.

Outputs - char : The character read from the recieve buffer

Purpose -  The purpose of this function is to read a character from the uart recieve buffer. The function makes sure
the character is only read when the reiceve buffer is not empty.

Effect - The effect of this function is that it waits until the recieve buffer isn't empty and then reads the character in
the buffer. Then the Recieve Interrupt signal is enabled to allow for more characters to be read. Disable interrupts
and restores the state at the end to avoid simultaneous access to critical data.
*/
char com_getc_async(struct uart *uart)
{
    // FIXME your code goes here
    // Disable interrupts to protect data and restore at end of function.
    int savedIntrState = intr_disable();

    // WHile the recieve buffer is empty, wait for the condition rxbuf_not_empty to be signaled before reading.
    while (rbuf_empty(&uart->rxbuf))
    {
        condition_wait(&uart->rxbuf_not_empty);
    }
    // Read character from recieve buffer if the recive buffer is not empty
    char character = rbuf_get(&uart->rxbuf);

    // Enable the Recieve Interrupt to notify when data is available
    uart->regs->ier |= IER_DRIE;

    intr_restore(savedIntrState);
    return character;
}

/*
Inputs -  int irqno : This is the interrupt request number.
void *aux: Auxiliary data passed to the ISR.
Outputs - char : None

Purpose -  The purpose of this function is to handle the UART ISR. 

Effect - The effect of this function is that it reads data from recieve buffer if available and 
writes data to transmit buffer if buffer isn't full. Handles by checking and setting the buffer status
*/
static void uart_isr(int irqno, void *aux)
{
    struct uart *const uart = aux;
    uint_fast8_t line_status;

    line_status = uart->regs->lsr;

    if (line_status & LSR_OE)
        panic("Receive buffer overrun");

    // FIXME your code goes here
    // Check if data is available to be read from recieve buffer
    if (line_status & LSR_DR)
    {
        // Read character from the recieve buffer register
        char buffer_char = uart->regs->rbr;
        // If the recieve buffer has space, place the character in buffer
        if (!rbuf_full(&uart->rxbuf))
        {
            rbuf_put(&uart->rxbuf, buffer_char);
            condition_broadcast(&uart->rxbuf_not_empty);
        }
        else
        {
            // If buffer is full , disable the Data Ready Interrupt
            uart->regs->ier &= ~IER_DRIE;
        }
    }
    // Ensure THR is empty and there is data to send
    if ((line_status & LSR_THRE) && (!rbuf_empty(&uart->txbuf)))
    {
        // Send the next character from transmit buffer
        uart->regs->thr = rbuf_get(&uart->txbuf);
        condition_broadcast(&uart->txbuf_not_full);
    }
    // Disable Interrupt if THR is empty and no data to send.
    else if ((line_status & LSR_THRE))
    {
        uart->regs->ier &= ~IER_THREIE;
    }
}

void rbuf_init(struct ringbuf *rbuf)
{
    rbuf->hpos = 0;
    rbuf->tpos = 0;
}

int rbuf_empty(const struct ringbuf *rbuf)
{
    return (rbuf->hpos == rbuf->tpos);
}

int rbuf_full(const struct ringbuf *rbuf)
{
    return ((uint16_t)(rbuf->tpos - rbuf->hpos) == SERIAL_RBUFSZ);
}

void rbuf_put(struct ringbuf *rbuf, char c)
{
    uint_fast16_t tpos;

    tpos = rbuf->tpos;
    rbuf->data[tpos % SERIAL_RBUFSZ] = c;
    asm volatile("" ::: "memory");
    rbuf->tpos = tpos + 1;
}

char rbuf_get(struct ringbuf *rbuf)
{
    uint_fast16_t hpos;
    char c;

    hpos = rbuf->hpos;
    c = rbuf->data[hpos % SERIAL_RBUFSZ];
    asm volatile("" ::: "memory");
    rbuf->hpos = hpos + 1;
    return c;
}