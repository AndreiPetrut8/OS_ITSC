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

#define MAX_FDS 16

#define FD_NONE -1
#define FD_UART_IN -2
#define FD_UART_OUT -3
#define FD_VGA_OUT -4

#define FD_IS_PIPE(fd) ((fd) >= 0)
#define FD_PIPE_ID(fd) ((fd) >> 1)
#define FD_PIPE_END(fd) ((fd) & 1)
#define FD_MAKE_PIPE(id, end) (((id) << 1) | (end))

extern uint32_t kernel_idle_esp;

extern void proc1_wrapper(void);
extern void proc2_wrapper(void);
extern void proc3_wrapper(void);

extern int current;

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
    int fd_table[MAX_FDS];
    int pipeline_tag;
    uint32_t page_dir;
    uint32_t saved_esp;
    uint32_t kernel_stack;
    uint32_t user_stack;
    int is_user;
} pcb_t;

void kprint(const char *str);
void kprint_int(int num);
void kprint_hex(uint32_t n);
void kprint_newline(void);
void kclear_screen(void);
void list_processes();
void kill_process(int pid);
void wait_process(int pid, int ticks);
void init_exceptions();
void yield();
void scheduler_tick();
int load_and_run(int program_index);

void kernel_main(void);

void fd_init(pcb_t *proc);
int fd_write(int fd_val, const char *buf, uint32_t len);
int fd_read(int fd_val, char *buf, uint32_t len);

int spawn_pipeline_process(int program_index, int stdin_fd, int stdout_fd, int stderr_fd);
void execute_pipeline(char *cmd);

extern pcb_t proc_table[MAX_PROCESSES];
extern int nr_procese;
extern int current;
extern int slice;
extern int preemptive_mode;

#endif
