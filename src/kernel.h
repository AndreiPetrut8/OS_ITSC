#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <stddef.h>

#define VGA_ADDRESS ((volatile uint16_t *)0xB8000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_COLOR 0x0F00

#define QUANTUM 4
#define MAX_PROCESSES 10

typedef enum
{
    PROC_UNUSED = 0,
    PROC_READY = 1,
    PROC_RUNNING = 2,
    PROC_WAITING = 3,
    PROC_TERMINATED = 4
} proc_state_t;

typedef struct
{
    void (*entry)();
    int remaining;
    proc_state_t state;
    int wait_ticks;
} pcb_t;


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
