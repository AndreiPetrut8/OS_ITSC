#include "uart.h"

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
    outb(COM1 + 3, 0x80); // Enable DLAB (set baud rate divisor)
    outb(COM1 + 0, 0x03); // Divisor low (38400 baud)
    outb(COM1 + 1, 0x00); // Divisor high
    outb(COM1 + 3, 0x03); // 8 bits, no parity, 1 stop bit
    outb(COM1 + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

int uart_tx_empty() {
    return inb(COM1 + 5) & 0x20;
}

void uart_putc(char c) {
    while (uart_tx_empty() == 0);
    outb(COM1, c);
}

int uart_received() {
    return inb(COM1 + 5) & 1;
}

char uart_getc() {
    while (uart_received() == 0);
    return inb(COM1);
}

void uart_print_int(int num) {
    char buffer[16];
    int i = 0;

    if (num == 0) {
        uart_putc('0');
        return;
    }

    while (num > 0) {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    }

    while (--i >= 0) {
        uart_putc(buffer[i]);
    }
}

void uart_print_hex(uint32_t n) {
    uart_print("0x");
    char hex_chars[] = "0123456789ABCDEF";
    for (int i = 28; i >= 0; i -= 4) {
        int index = (n >> i) & 0xF;
        uart_putc(hex_chars[index]);
    }
}

void uart_print(const char *s) {
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}

void uart_readline(char* buffer, int max_len) {
    int i = 0;
    while (i < max_len - 1) {
        char c = uart_getc();
        
        if (c == '\r' || c == '\n') {
            uart_putc('\r');
            uart_putc('\n');
            break;
        } else if (c == 127 || c == 8) {
            if (i > 0) {
                i--;
                uart_print("\b \b");
            }
        } else {
            uart_putc(c);
            buffer[i++] = c;
        }
    }
    buffer[i] = '\0';
}
