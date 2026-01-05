#include <stdint.h>

#define COM1 0x3F8


static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void uart_init() {
    outb(COM1 + 1, 0x00); // Disable interrupts
    outb(COM1 + 3, 0x80); // Enable DLAB
    outb(COM1 + 0, 0x03); // Divisor low (38400 baud)
    outb(COM1 + 1, 0x00); // Divisor high
    outb(COM1 + 3, 0x03); // 8 bits, no parity, 1 stop
    outb(COM1 + 2, 0xC7); // Enable FIFO
    outb(COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

int uart_tx_empty() {
    return inb(COM1 + 5) & 0x20;
}

void uart_putc(char c) {
    while (!uart_tx_empty());
    outb(COM1, c);
}

void uart_print(const char *s) {
    while (*s) {
        if (*s == '\n')
            uart_putc('\r'); //urmatorul punct de insertie
        uart_putc(*s++);
    }
}

void kernel_main() {
    uart_init();
    uart_print("Hello there !\n");

    while (1) {
        __asm__ volatile ("hlt");
    }

}
