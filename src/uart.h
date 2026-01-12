#ifndef UART_H
#define UART_H

#include <stdint.h>

#define COM1 0x3F8

void uart_init();
void uart_putc(char c);
char uart_getc();
void uart_print_int(int num);
void uart_print_hex(uint32_t n);
void uart_print(const char *s);

int  uart_received();
void uart_readline(char* buffer, int max_len);

#endif
