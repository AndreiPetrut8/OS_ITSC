#include "kernel.h"
#include "heap.h"
#include "idt.h"
#include "uart.h"
#include "ramfs.h"
#include "fs.h"
#include "ata.h"
#include "pipe.h"
#include "vmm.h"
#include "pmm.h"
#include "gdt.h"
#include "tss.h"
#include "vga.h"

int demo_graphic = 0; // 1 = procese pe qemu
static int vga_col = 0;
static int vga_row = 0;

static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

extern uint8_t _binary_bin_u1_bin_start;
extern uint8_t _binary_bin_u1_bin_end;
extern uint8_t _binary_bin_u2_bin_start;
extern uint8_t _binary_bin_u2_bin_end;

uint32_t kernel_idle_esp = 0;
int last_scheduled = -1;

ramfs_entry_t ramfs[] = {
    {"u1", &_binary_bin_u1_bin_start, 0},
    {"u2", &_binary_bin_u2_bin_start, 0}};

void disable_cursor()
{
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void kclear_screen()
{
    for (uint16_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
    {
        VGA_ADDRESS[i] = ' ' | VGA_COLOR;
    }
    vga_col = 0;
    vga_row = 0;
}

void kclear_line()
{
    for (int i = 0; i < VGA_WIDTH; i++)
    {
        VGA_ADDRESS[i + vga_row * VGA_WIDTH] = ' ' | VGA_COLOR;
    }
}

void kprint_newline(void)
{
    vga_col = 0;
    vga_row++;
    if (vga_row >= VGA_HEIGHT)
    {
        vga_row = 0;
    }
    kclear_line();
}

void kprint(const char *str)
{
    for (int i = 0; str[i] != '\0'; i++)
    {
        if (str[i] == '\n')
        {
            kprint_newline();
        }
        else
        {
            const int index = vga_row * VGA_WIDTH + vga_col;
            VGA_ADDRESS[index] = str[i] | VGA_COLOR;
            vga_col++;
            if (vga_col >= VGA_WIDTH)
            {
                kprint_newline();
            }
        }
    }
}

void kprint_int(int num)
{
    char buffer[16];
    int i = 0;
    if (num == 0)
    {
        kprint("0");
        return;
    }

    if (num < 0)
    {
        kprint("-");
        num = -num;
    }

    while (num > 0)
    {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    }

    while (--i >= 0)
    {
        char c[2] = {buffer[i], '\0'};
        kprint(c);
    }
}

void kprint_hex(uint32_t n)
{
    kprint("0x");
    char hex_chars[] = "0123456789ABCDEF";
    for (int i = 28; i >= 0; i -= 4)
    {
        int index = (n >> i) & 0xF;
        char c[2] = {hex_chars[index], '\0'};
        kprint(c);
    }
}

void simple_delay(int loops)
{
    volatile int t = 0;
    for (int i = 0; i < loops * 10000; i++)
    {
        t++;
    }
}

////////////////////////////////////////////////////////////////
// OPREA A FOST AICI
////////////////////////////////////////////////////////////////

void process1()
{
    if (demo_graphic)
        term_print("[PID 1] Petrut\n");
    else
        kprint("[PID 1] Petrut\n");
}
void process2()
{
    if (demo_graphic)
        term_print("[PID 2] Mio\n");
    else
        kprint("[PID 2] Mio\n");
}
void process3()
{
    if (demo_graphic)
        term_print("[PID 3] Oprea\n");
    else
        kprint("[PID 3] Oprea\n");
}

pcb_t proc_table[MAX_PROCESSES];
int nr_procese = 0;
int current = -1;
int slice = 0;
int preemptive_mode = 0;

int pick_next_ready(int from)
{
    for (int i = 0; i < nr_procese; i++)
    {
        int idx = (from + i) % nr_procese;
        if (proc_table[idx].state == PROC_READY || proc_table[idx].state == PROC_RUNNING)
        {
            return idx;
        }
    }
    return -1;
}

void update_waiting()
{
    for (int i = 0; i < nr_procese; i++)
    {
        if (proc_table[i].state == PROC_WAITING)
        {
            if (proc_table[i].wait_ticks > 0)
            {
                proc_table[i].wait_ticks--;
            }
            if (proc_table[i].wait_ticks <= 0)
            {
                proc_table[i].state = PROC_READY;
            }
        }
    }
}

void yield()
{
    if (preemptive_mode)
        return;

    if (current < 0 || current >= nr_procese)
        return;

    kprint("[Yield] Process ");
    kprint_int(current + 1);
    kprint(" yielding\n");

    if (proc_table[current].state == PROC_RUNNING)
    {
        proc_table[current].state = PROC_WAITING;
        proc_table[current].wait_ticks = 3;
    }

    int next = pick_next_ready((current + 1) % nr_procese);
    if (next >= 0)
    {
        current = next;
        proc_table[current].state = PROC_RUNNING;
    }
    slice = 0;
}

void scheduler_tick()
{
    if (nr_procese == 0)
        return;

    update_waiting();

    int has_ready = 0;
    for (int i = 0; i < nr_procese; i++)
    {
        if (proc_table[i].state == PROC_READY || proc_table[i].state == PROC_RUNNING)
        {
            has_ready = 1;
            break;
        }
    }

    if (!has_ready)
    {
        kprint("[Idle] All processes WAITING - CPU idle\n");
        simple_delay(50);
        return;
    }

    if (current < 0 || current >= nr_procese ||
        proc_table[current].state != PROC_RUNNING)
    {
        int next = pick_next_ready(current);
        if (next < 0)
            return;
        current = next;
        proc_table[current].state = PROC_RUNNING;
        slice = 0;
    }

    proc_table[current].entry();
    proc_table[current].remaining--;
    slice++;

    simple_delay(20);

    if (proc_table[current].remaining <= 0)
    {
        kprint("[Done] Process ");
        kprint_int(current + 1);
        kprint(" finished\n");

        proc_table[current].state = PROC_TERMINATED;

        for (int i = current; i < nr_procese - 1; i++)
        {
            proc_table[i] = proc_table[i + 1];
        }
        proc_table[nr_procese - 1].state = PROC_UNUSED;
        nr_procese--;

        if (nr_procese == 0)
        {
            kprint("All processes are finished!\n");
            current = -1;
            return;
        }

        current = current % nr_procese;
        if (proc_table[current].state == PROC_READY)
            proc_table[current].state = PROC_RUNNING;
        slice = 0;
        return;
    }

    if (preemptive_mode && slice >= QUANTUM)
    {
        kprint("[Preemption] Time finished PID ");
        kprint_int(current + 1);
        kprint(" - next process\n");

        proc_table[current].state = PROC_READY;
        int next = pick_next_ready((current + 1) % nr_procese);
        if (next >= 0)
        {
            current = next;
            proc_table[current].state = PROC_RUNNING;
        }
        slice = 0;
    }
}

////////////////////////////////////////////////////////////////
// ATATA DE LA OPREA
////////////////////////////////////////////////////////////////

extern uint8_t _proc1_start;
extern uint8_t _proc1_end;
extern uint8_t _proc2_start;
extern uint8_t _proc2_end;
extern uint8_t _proc3_start;
extern uint8_t _proc3_end;

process_info_t kernel_processes[5] = {
    {(void (*)())0x400000, &_binary_bin_u1_bin_start, &_binary_bin_u1_bin_end},
    {(void (*)())0x500000, &_binary_bin_u2_bin_start, &_binary_bin_u2_bin_end},
    {process1, &_proc1_start, &_proc1_end},
    {process2, &_proc2_start, &_proc2_end},
    {process3, &_proc3_start, &_proc3_end}};

// Test function for heap.c

void syscall_write(const char *message, uint32_t len)
{
    asm volatile("int $0x80" : : "a"(1), "b"(1), "c"(message), "d"(len));
}

void syscall_yield()
{
    asm volatile("int $0x80" : : "a"(2));
}

uint32_t syscall_gettime()
{
    uint32_t ticks;
    asm volatile("int $0x80" : "=a"(ticks) : "a"(3));
    return ticks;
}

////////////////////////////////////////////////////////////////
// AM SCHIMBAT PUTIN SI AICI CA SUNT MAI MULTE STARI ACUM
////////////////////////////////////////////////////////////////

void list_processes()
{
    uart_print("\nPID | State    | Remaining Ticks\n");
    uart_print("--------------------------------\n");

    for (int i = 0; i < nr_procese; i++)
    {
        uart_print(" ");
        char pid_char = i + '0';
        char pid_str[2] = {pid_char, '\0'};
        uart_print(pid_str);
        uart_print("   | ");

        switch (proc_table[i].state)
        {
        case PROC_READY:
            uart_print("READY   ");
            break;
        case PROC_RUNNING:
            uart_print("RUNNING ");
            break;
        case PROC_WAITING:
            uart_print("WAITING ");
            break;
        case PROC_TERMINATED:
            uart_print("DONE    ");
            break;
        default:
            uart_print("UNUSED  ");
            break;
        }

        uart_print(" | ");
        uart_print_int(proc_table[i].remaining);
        uart_print("\n");
    }

    if (nr_procese == 0)
    {
        uart_print("Nu exista procese active.\n");
    }
}

////////////////////////////////////////////////////////////////
// BUN GATA <3
////////////////////////////////////////////////////////////////

void kill_process(int pid)
{
    if (pid < 0 || pid >= nr_procese)
    {
        uart_print("Eroare: PID invalid\n");
        return;
    }

    for (int i = 0; i < MAX_FDS; i++)
    {
        int fd_val = proc_table[pid].fd_table[i];
        if (fd_val != FD_NONE)
        {
            if (FD_IS_PIPE(fd_val))
                pipe_close(FD_PIPE_ID(fd_val), FD_PIPE_END(fd_val));
            proc_table[pid].fd_table[i] = FD_NONE;
        }
    }

    proc_table[pid].remaining = 0;

    uart_print("Procesul ");
    uart_print_int(pid);
    uart_print(" a fost marcat pentru terminare.\n");
}

////////////////////////////////////////////////////////////////
// Comanda WAIT - blochez manual un proces
////////////////////////////////////////////////////////////////

void wait_process(int pid, int ticks)
{
    if (pid < 0 || pid >= nr_procese)
    {
        uart_print("Eroare: PID invalid\n");
        return;
    }

    if (proc_table[pid].state == PROC_TERMINATED ||
        proc_table[pid].state == PROC_UNUSED)
    {
        uart_print("Eroare: procesul nu este activ\n");
        return;
    }

    if (ticks <= 0)
        ticks = 10;

    proc_table[pid].state = PROC_WAITING;
    proc_table[pid].wait_ticks = ticks;

    uart_print("Process ");
    uart_print_int(pid);
    uart_print(" -> WAITING for ");
    uart_print_int(ticks);
    uart_print(" ticks.\n");
}

////////////////////////////////////////////////////////////////
// SFARSIT COMANDA WAIT
////////////////////////////////////////////////////////////////

int load_and_run(int program_index)
{
    if (program_index < 0 || program_index > 1)
        return -1;

    uint32_t load_addr = (program_index == 0) ? 0x400000 : 0x500000;
    uint32_t size = ramfs[program_index].size;

    uint8_t *dest = (uint8_t *)load_addr;
    uint8_t *src = ramfs[program_index].data;

    for (uint32_t i = 0; i < size; i++)
        dest[i] = src[i];

    asm volatile("cli");
    if (nr_procese < MAX_PROCESSES)
    {
        int pid = nr_procese;
        proc_table[pid].entry = (void (*)())load_addr;
        proc_table[pid].remaining = 50;
        proc_table[pid].state = PROC_READY;
        proc_table[pid].wait_ticks = 0;
        proc_table[pid].page_dir = vmm_get_kernel_pd();
        proc_table[pid].is_user = 0;
        fd_init(&proc_table[pid]);
        proc_table[pid].pipeline_tag = 0;
        nr_procese++;

        uart_print("Program ");
        uart_print(ramfs[program_index].name);
        uart_print(" incarcat\n");
    }
    else
    {
        uart_print("Eroare: Prea multe procese!\n");
    }
    asm volatile("sti");
    return 0;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, int n)
{
    while (n && *s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
        n--;
    }
    if (n == 0)
    {
        return 0;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

void fd_init(pcb_t *proc)
{
    proc->fd_table[0] = FD_UART_IN;
    proc->fd_table[1] = FD_UART_OUT;
    proc->fd_table[2] = FD_VGA_OUT;
    for (int i = 3; i < MAX_FDS; i++)
        proc->fd_table[i] = FD_NONE;
}

int fd_write(int fd_val, const char *buf, uint32_t len)
{
    if (fd_val == FD_UART_OUT)
    {
        for (uint32_t i = 0; i < len; i++)
            uart_putc(buf[i]);
        return (int)len;
    }
    if (fd_val == FD_VGA_OUT)
    {
        for (uint32_t i = 0; i < len; i++)
        {
            char c[2] = {buf[i], '\0'};
            kprint(c);
        }
        return (int)len;
    }
    if (FD_IS_PIPE(fd_val))
        return pipe_write(FD_PIPE_ID(fd_val), buf, len);
    return -1;
}

int fd_read(int fd_val, char *buf, uint32_t len)
{
    if (fd_val == FD_UART_IN)
    {
        if (len > 0 && uart_received())
        {
            buf[0] = uart_getc();
            return 1;
        }
        return 0;
    }
    if (FD_IS_PIPE(fd_val))
        return pipe_read(FD_PIPE_ID(fd_val), buf, len);
    return -1;
}

int spawn_pipeline_process(int program_index, int stdin_fd, int stdout_fd, int stderr_fd)
{
    if (program_index < 0 || program_index > 1)
        return -1;
    if (nr_procese >= MAX_PROCESSES)
        return -1;

    uint32_t size = ramfs[program_index].size;
    uint32_t pages_needed = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    uint32_t virt_base = (program_index == 0) ? 0x400000 : 0x500000;

    uint32_t pd = vmm_create_pd();
    if (!pd)
        return -1;

    uint8_t *dest = (uint8_t *)virt_base;
    uint8_t *src = ramfs[program_index].data;
    for (uint32_t i = 0; i < size; i++)
        dest[i] = src[i];

    for (uint32_t p = 0; p < pages_needed; p++)
    {
        uint32_t phys = virt_base + p * PAGE_SIZE;
        vmm_map_page((uint32_t *)pd, virt_base + p * PAGE_SIZE, phys, 0x07);
    }

    uint32_t ustack_virt = 0xB0000000;
    uint32_t ustack_phys = pmm_alloc_frame();
    if (!ustack_phys)
        return -1;
    vmm_map_page((uint32_t *)pd, ustack_virt - PAGE_SIZE, ustack_phys, 0x07);

    uint32_t kstack = (uint32_t)kmalloc(4096);
    uint32_t *frame_top = (uint32_t *)(kstack + 4096);
    uint32_t *frame = frame_top;

    *--frame = 0x23;
    *--frame = ustack_virt;
    *--frame = 0x202;
    *--frame = 0x1B;
    *--frame = virt_base;
    *--frame = 0; /* eax */
    *--frame = 0; /* ecx */
    *--frame = 0; /* edx */
    *--frame = 0; /* ebx */
    *--frame = 0; /* esp dummy */
    *--frame = 0; /* ebp */
    *--frame = 0; /* esi */
    *--frame = 0; /* edi */

    int pid = nr_procese;
    proc_table[pid].entry = (void (*)())virt_base;
    proc_table[pid].remaining = 100;
    proc_table[pid].state = PROC_READY;
    proc_table[pid].wait_ticks = 0;
    proc_table[pid].page_dir = pd;
    proc_table[pid].saved_esp = (uint32_t)frame;
    proc_table[pid].kernel_stack = (uint32_t)(kstack + 4096);
    proc_table[pid].user_stack = ustack_virt;
    proc_table[pid].is_user = 1;
    fd_init(&proc_table[pid]);
    proc_table[pid].fd_table[0] = stdin_fd;
    proc_table[pid].fd_table[1] = stdout_fd;
    proc_table[pid].fd_table[2] = stderr_fd;
    proc_table[pid].pipeline_tag = 1;
    nr_procese++;
    return pid;
}

static char *trim(char *s)
{
    while (*s == ' ')
        s++;
    char *end = s;
    while (*end)
        end++;
    end--;
    while (end > s && *end == ' ')
        *end-- = '\0';
    return s;
}

static int is_pipeline(const char *cmd)
{
    for (int i = 0; cmd[i]; i++)
        if (cmd[i] == '|')
            return 1;
    return 0;
}

void execute_pipeline(char *cmd)
{
    char *stages[4];
    int n = 0;

    stages[n++] = cmd;
    for (int i = 0; cmd[i] && n < 4; i++)
    {
        if (cmd[i] == '|')
        {
            cmd[i] = '\0';
            stages[n++] = &cmd[i + 1];
        }
    }

    int pipe_fds[3][2];
    for (int i = 0; i < n - 1; i++)
    {
        int pid = pipe_alloc();
        if (pid < 0)
        {
            uart_print("Eroare: nu s-a putut crea pipe\n");
            return;
        }
        pipe_fds[i][0] = FD_MAKE_PIPE(pid, 0);
        pipe_fds[i][1] = FD_MAKE_PIPE(pid, 1);
    }

    for (int i = 0; i < n; i++)
    {
        char *prog = trim(stages[i]);
        int stdin_fd = (i == 0) ? FD_UART_IN : pipe_fds[i - 1][0];
        int stdout_fd = (i == n - 1) ? FD_UART_OUT : pipe_fds[i][1];

        if (strcmp(prog, "u1") == 0)
        {
            spawn_pipeline_process(0, stdin_fd, stdout_fd, FD_VGA_OUT);
        }
        else if (strcmp(prog, "u2") == 0)
        {
            spawn_pipeline_process(1, stdin_fd, stdout_fd, FD_VGA_OUT);
        }
        else
        {
            uart_print("Eroare: in pipeline doar u1/u2 sunt suportate: ");
            uart_print(prog);
            uart_print("\n");
            return;
        }
    }

    uart_print("[Pipeline] Astept terminare procese...\n");
    int active;
    do
    {
        active = 0;
        for (int i = 0; i < nr_procese; i++)
        {
            if (proc_table[i].pipeline_tag &&
                proc_table[i].state != PROC_TERMINATED &&
                proc_table[i].state != PROC_UNUSED)
            {
                active = 1;
                break;
            }
        }
        if (active)
            asm volatile("sti; hlt");
    } while (active);

    for (int i = 0; i < nr_procese; i++)
        proc_table[i].pipeline_tag = 0;

    uart_print("[Pipeline] Terminat.\n");

    for (int i = 0; i < n - 1; i++)
    {
        pipe_close(FD_PIPE_ID(pipe_fds[i][0]), FD_PIPE_END(pipe_fds[i][0]));
        pipe_close(FD_PIPE_ID(pipe_fds[i][1]), FD_PIPE_END(pipe_fds[i][1]));
    }
}

void shell_loop()
{
    char cmd_buf[64];

    while (1)
    {
        uart_print("user@simple_os:");
        uart_print("> ");
        uart_readline(cmd_buf, 64);

        if (is_pipeline(cmd_buf) && strncmp(cmd_buf, "exec ", 5) != 0)
        {
            execute_pipeline(cmd_buf);
            continue;
        }

        if (strcmp(cmd_buf, "ps") == 0)
        {
            asm volatile("cli");
            list_processes();
            asm volatile("sti");
        }
        else if (strcmp(cmd_buf, "ls") == 0)
        {
            fs_ls();
        }
        else if (strncmp(cmd_buf, "mkdir ", 6) == 0)
        {
            fs_mkdir(cmd_buf + 6);
        }
        else if (strncmp(cmd_buf, "touch ", 6) == 0)
        {
            fs_create_file(cmd_buf + 6);
        }
        else if (strncmp(cmd_buf, "cd ", 3) == 0)
        {
            fs_cd(cmd_buf + 3);
        }
        else if (strncmp(cmd_buf, "rm ", 3) == 0)
        {
            fs_rm(cmd_buf + 3);
        }
        else if (strncmp(cmd_buf, "kill ", 5) == 0)
        {
            int pid = cmd_buf[5] - '0';
            asm volatile("cli");
            kill_process(pid);
            asm volatile("sti");
        }
        else if (strncmp(cmd_buf, "wait ", 5) == 0)
        {
            int pid = cmd_buf[5] - '0';
            int ticks = 0;
            if (cmd_buf[6] == ' ')
            {
                int i = 7;
                while (cmd_buf[i] >= '0' && cmd_buf[i] <= '9')
                {
                    ticks = ticks * 10 + (cmd_buf[i] - '0');
                    i++;
                }
            }
            asm volatile("cli");
            wait_process(pid, ticks);
            asm volatile("sti");
        }
        else if (strncmp(cmd_buf, "time", 4) == 0)
        {
            uint32_t t = syscall_gettime();
            uart_print("Ticks de la boot: ");
            uart_print_int(t);
            uart_print("\n");
        }
        // else if (strncmp(cmd_buf, "write", 5) == 0)
        // {
        // char *msg = cmd_buf + 6;
        //  uint32_t len = 0;
        //  while (msg[len] != '\0')
        //      len++;
        //   syscall_write(msg, len);
        // }
        else if (strncmp(cmd_buf, "yield", 5) == 0)
        {
            syscall_yield();
        }
        else if (strncmp(cmd_buf, "exec ", 5) == 0)
        {
            char *msg = cmd_buf + 5;
            if (is_pipeline(msg))
            {
                execute_pipeline(msg);
            }
            else if (strcmp(msg, "u1") == 0)
            {
                int loaded = load_and_run(0);
                if (loaded >= 0)
                {
                    uint32_t target = 0x400000;
                    while (1)
                    {
                        int found = 0;
                        for (int i = 0; i < nr_procese; i++)
                        {
                            if ((uint32_t)proc_table[i].entry == target &&
                                proc_table[i].state != PROC_TERMINATED &&
                                proc_table[i].state != PROC_UNUSED)
                            {
                                found = 1;
                                break;
                            }
                        }
                        if (!found)
                            break;
                        asm volatile("sti; hlt");
                    }
                }
            }
            else if (strcmp(msg, "u2") == 0)
            {
                int loaded = load_and_run(1);
                if (loaded >= 0)
                {
                    uint32_t target = 0x500000;
                    while (1)
                    {
                        int found = 0;
                        for (int i = 0; i < nr_procese; i++)
                        {
                            if ((uint32_t)proc_table[i].entry == target &&
                                proc_table[i].state != PROC_TERMINATED &&
                                proc_table[i].state != PROC_UNUSED)
                            {
                                found = 1;
                                break;
                            }
                        }
                        if (!found)
                            break;
                        asm volatile("sti; hlt");
                    }
                }
            }
        }
        else if (strcmp(cmd_buf, "help") == 0)
        {
            uart_print("Comenzi Sistem de Fisiere:\n");
            uart_print("  ls                  - Listeaza fisierele\n");
            uart_print("  mkdir <nume>        - Creaza director\n");
            uart_print("  cd <nume>           - Schimba directorul (.. pt inapoi)\n");
            uart_print("  rm <nume>           - Sterge fisier/director\n");
            uart_print("  cat <nume>          - Afiseaza continutul unui fisier\n");
            uart_print("  write <nume> <text> - Scrie text intr-un fisier\n");
            uart_print("  touch <nume>        - Creeaza un fisier \n");
            uart_print("Comenzi Procese:\n");
            uart_print("  ps                      - Listeaza procesele si starile lor\n");
            uart_print("  kill <pid>              - Termina un proces\n");
            uart_print("  wait <pid> [t]          - Pune un proces in WAITING (t ticks, default 10)\n");
            uart_print("  exec <prog>             - Executa un program (u1, u2)\n");
            uart_print("  exec <prog1> | <prog2>  - Executa doua programe in pipeline (u1, u2)\n");

            uart_print("Alte comenzi: mem, pmem, time, yield, save\n");
        }
        else if (strcmp(cmd_buf, "mem") == 0)
        {
            heap_test();
        }
        else if (strcmp(cmd_buf, "pmem") == 0)
        {
            heap_test_processes();
        }
        else if (strncmp(cmd_buf, "write ", 6) == 0)
        {
            char *args = cmd_buf + 6;
            char filename[32];
            int i = 0;
            while (args[i] != '\0' && args[i] != ' ' && i < 31)
            {
                filename[i] = args[i];
                i++;
            }
            filename[i] = '\0';

            if (args[i] == ' ')
            {
                char *content = args + i + 1;
                uint32_t len = 0;
                while (content[len] != '\0' && content[len] != '\r' && content[len] != '\n')
                {
                    len++;
                }

                if (len > 0)
                {
                    fs_write_file(filename, content, len);
                }
                else
                {
                    uart_print("Eroare: Nu ai introdus text după numele fișierului.\n");
                }
            }
            else
            {
                uart_print("Eroare Sintaxa: write <nume_fisier> <text>\n");
            }
        }
        else if (strncmp(cmd_buf, "cat ", 4) == 0)
        {

            fs_cat(cmd_buf + 4);
        }
        else if (strcmp(cmd_buf, "save") == 0)
        {
            fs_save_to_disk();
        }
        else if (strcmp(cmd_buf, "demo") == 0)
        {
            if (nr_procese > 0)
            {
                uart_print("Procese deja active.\n");
                continue;
            }

            demo_graphic = 1; // pentru procese
            term_enter();
            term_print("=== Geamuri 98 :: Procese ===\n\n");

            proc_table[0].entry = process1;
            proc_table[0].remaining = 10000;
            proc_table[0].state = PROC_READY;
            proc_table[0].wait_ticks = 0;
            proc_table[0].page_dir = vmm_get_kernel_pd();
            proc_table[0].is_user = 0;
            fd_init(&proc_table[0]);
            proc_table[0].pipeline_tag = 0;

            proc_table[1].entry = process2;
            proc_table[1].remaining = 10000;
            proc_table[1].state = PROC_READY;
            proc_table[1].wait_ticks = 0;
            proc_table[1].page_dir = vmm_get_kernel_pd();
            proc_table[1].is_user = 0;
            fd_init(&proc_table[1]);
            proc_table[1].pipeline_tag = 0;

            proc_table[2].entry = process3;
            proc_table[2].remaining = 10000;
            proc_table[2].state = PROC_READY;
            proc_table[2].wait_ticks = 0;
            proc_table[2].page_dir = vmm_get_kernel_pd();
            proc_table[2].is_user = 0;
            fd_init(&proc_table[2]);
            proc_table[2].pipeline_tag = 0;

            nr_procese = 3;
            current = 0;
            slice = 0;
            uart_print("Demo pornit.\n");
        }
        else if (strcmp(cmd_buf, "exit") == 0)
        {
            uart_print("Shutting down...\n");
            outb(0x501, 0x00);
            asm volatile("cli");
            for (;;)
                asm volatile("hlt");
        }
        else
        {
            uart_print("Eroare: Comanda necunoscuta.\n");
        }
    }
}

void kernel_main(void)
{

    extern uint8_t _bss_start[];
    extern uint8_t _bss_end[];
    for (uint8_t *p = _bss_start; p < _bss_end; p++)
        *p = 0;

    // kclear_screen();
    // disable_cursor();
    draw_boot_screen();

    kprint("Kernel is starting...\n");
    uart_print("Booting OS...\n");

    void *heap_base = (void *)0x00200000;
    size_t heap_size = 100 * 1024;
    kheap_init(heap_base, heap_size);

    pmm_init(32 * 1024, 0x00219000);

    uart_init();
    uart_print("UART Initialized...\n");

    gdt_init();
    tss_init(0x00900000);
    tss_install_gdt();

    vmm_init(32 * 1024);
    pipe_init();

    preemptive_mode = 1;

    fs_init();

    nr_procese = 0;
    current = -1;

    init_exceptions();

    init_syscalls();
    init_interrupts();

    ramfs[0].size = (uint32_t)&_binary_bin_u1_bin_end - (uint32_t)&_binary_bin_u1_bin_start;
    ramfs[1].size = (uint32_t)&_binary_bin_u2_bin_end - (uint32_t)&_binary_bin_u2_bin_start;

    draw_desktop();
    shell_loop();

    for (;;)
    {
        asm volatile("hlt");
    }
}

/*
void kernel_main(void) {
    // 1. Inițializare Video
    kclear_screen();
    disable_cursor();
    kprint("--- Kernel Started ---\n");
    simple_delay(10000);
    // 2. Inițializare Heap
    // Conform boot.asm, Kernelul este la 0x100000.
    // Alegem o zonă sigură pentru Heap la 0x200000 (2MB mark) cu mărimea 100KB.
    // Aceasta evită suprascrierea codului kernelului.
    void* heap_base = (void*)0x00200000;
    size_t heap_size = 100 * 1024;
    kclear_screen();
    kprint("Initializing Heap at: 0x200000...\n");
    kheap_init(heap_base, heap_size);

    // Test simplu de alocare (opțional, pentru a verifica heap.c)
    char* test_ptr = (char*)kmalloc(10);
    if (test_ptr) {
        kprint("Heap Allocation Test: OK\n");
        kfree(test_ptr);
    } else {
        kprint("Heap Allocation Test: FAIL\n");
    }

    // 3. Rulare Scheduler - Mod Cooperativ
    kprint("\n=== Cooperative Scheduling ===\n");
    // Resetăm variabilele pentru siguranță (deși sunt inițializate global)
    nr_procese = 3;
    // Re-populăm array-urile deoarece scheduler_tick le modifică distructiv
    processes[0] = process1; remaining[0] = 5;
    processes[1] = process2; remaining[1] = 3;
    processes[2] = process3; remaining[2] = 8;
    current = 0;
    slice = 0;
    preemptive_mode = 0;

    for (int t = 0; t < 10; t++) {
        scheduler_tick();
        if (t == 3) yield();
        simple_delay(10000); // Înlocuiește usleep(200000)
    }

    // 4. Rulare Scheduler - Mod Preemptive
    // Resetăm starea pentru testul 2
    nr_procese = 3;
    processes[0] = process1; remaining[0] = 5;
    processes[1] = process2; remaining[1] = 3;
    processes[2] = process3; remaining[2] = 8;
    current = 0;
    slice = 0;

    preemptive_mode = 1;
    kprint("\n=== Preemptive Scheduling ===\n");

    while (nr_procese > 0) {
        scheduler_tick();
        simple_delay(10000);
    }
    kclear_screen();
    kprint("Kernel Halted.");

    for (;;) {
        asm volatile("hlt");
    }
}

void kernel_main(void) {

  kclear_screen();
  disable_cursor();

  char str[] = "AAAAAAAAAA\0";

  kprint(str);

  for (;;) {
    asm volatile("hlt");
  }
  }*/
