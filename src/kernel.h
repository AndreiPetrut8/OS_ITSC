#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <stddef.h>

void kprint(const char* str);
void kprint_int(int num);
void kprint_hex(uint32_t n);
void kprint_newline(void);
void kclear_screen(void);
void list_processes();
void kill_process(int pid);
void yield();
void scheduler_tick();

void kernel_main(void);

#endif
