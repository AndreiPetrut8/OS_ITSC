#include "idt.h"
#include "heap.h"
#include "kernel.h"
#include "uart.h"
#include "pipe.h"
#include "tss.h"

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

extern pcb_t proc_table[];
extern int   nr_procese;

extern int current;
extern void load_idt(void* base, uint16_t size);
extern void isr_syscall_stub(void);

struct idt_entry_t idt[256];

void pit_init(uint32_t frequency) {

    uint32_t divisor = 1193182 / frequency;

    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

void pic_remap() {

    outb(0x20, 0x11);
    outb(0xA0, 0x11);

    outb(0x21, 0x20);
    outb(0xA1, 0x28);

    outb(0x21, 0x04);
    outb(0xA1, 0x02);

    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    
    outb(0x21, 0xFE);
    outb(0xA1, 0xFF);
}

void idt_set_gate(int num, uint32_t base, uint16_t sel, uint8_t flags) {
  idt[num].offset_low = base & 0xFFFF;
  idt[num].offset_high = (base >> 16) & 0xFFFF;
  idt[num].selector = sel;
  idt[num].zero = 0;
  idt[num].type_attr = flags;
}

void init_interrupts() {
    pic_remap();
    pit_init(100);

    extern void isr_timer_stub();
    idt_set_gate(0x20, (uint32_t)isr_timer_stub, 0x08, 0x8E);

    load_idt(idt, sizeof(idt) - 1);

    asm volatile("sti");
    
    uart_print("Timer configurat la 100Hz. Intreruperi activate.\n");
}

extern void isr_gpf_stub(void);
extern void isr_df_stub(void);

void init_exceptions(void) {
    idt_set_gate(0x08, (uint32_t)isr_df_stub, 0x08, 0x8E);
    idt_set_gate(0x0D, (uint32_t)isr_gpf_stub, 0x08, 0x8E);
}

void gpf_handler_c(uint32_t esp) {
    uart_print("\n!!! GPF at ESP: ");
    uart_print_hex(esp);
    uart_print(" !!!\n");
    for (;;) asm volatile("cli; hlt");
}

void df_handler_c(uint32_t esp) {
    uart_print("\n!!! DOUBLE FAULT !!!\n");
    for (;;) asm volatile("cli; hlt");
}

volatile uint32_t system_ticks = 0;

extern pcb_t proc_table[];
extern int nr_procese;
extern int current;
extern uint32_t kernel_idle_esp;

uint32_t timer_handler_c(uint32_t esp) {
    system_ticks++;
    static int in_tick = 0;
    if (in_tick) return esp;
    in_tick = 1;

    if (current >= 0 && current < nr_procese) {
        proc_table[current].saved_esp = esp;
    } else {
        kernel_idle_esp = esp;
    }

    scheduler_tick();

    in_tick = 0;
    return esp;
}

void syscall_handler(registers_t* regs) {
    switch (regs->eax) {

    case 1:
        {
            int fd   = regs->ebx;
            char *buf = (char*)regs->ecx;
            uint32_t len = regs->edx;

            if (fd >= 0 && fd < MAX_FDS && current >= 0 && current < nr_procese) {
                int fd_val = proc_table[current].fd_table[fd];
                regs->eax = fd_write(fd_val, buf, len);
            } else {
                regs->eax = -1;
            }
        }
        break;

    case 2:
        kprint("[Kernel] SYS_YIELD: Cedare procesor\n");
        yield();
        break;

    case 3:
        regs->eax = system_ticks;
        break;

    case 4:
        regs->eax = (uint32_t)kmalloc(regs->ebx);
        break;

    case 5:
        kprint("[Kernel] SYS_EXIT: Proces terminat\n");
        kill_process(current);
        break;

    case 6:
        {
            int fd   = regs->ebx;
            char *buf = (char*)regs->ecx;
            uint32_t len = regs->edx;

            if (fd >= 0 && fd < MAX_FDS && current >= 0 && current < nr_procese) {
                int fd_val = proc_table[current].fd_table[fd];
                regs->eax = fd_read(fd_val, buf, len);
            } else {
                regs->eax = -1;
            }
        }
        break;

    case 7:
        {
            int *user_pipefd = (int*)regs->ebx;
            int pipe_id = pipe_alloc();
            if (pipe_id < 0) {
                regs->eax = -1;
                break;
            }
            int fd0 = -1, fd1 = -1;
            for (int i = 0; i < MAX_FDS; i++) {
                if (proc_table[current].fd_table[i] == FD_NONE) {
                    if (fd0 < 0) fd0 = i;
                    else { fd1 = i; break; }
                }
            }
            if (fd0 < 0 || fd1 < 0) {
                pipe_free(pipe_id);
                regs->eax = -1;
                break;
            }
            proc_table[current].fd_table[fd0] = FD_MAKE_PIPE(pipe_id, 0);
            proc_table[current].fd_table[fd1] = FD_MAKE_PIPE(pipe_id, 1);
            user_pipefd[0] = fd0;
            user_pipefd[1] = fd1;
            regs->eax = 0;
        }
        break;

    case 8:
        {
            int oldfd = regs->ebx;
            int newfd = regs->ecx;
            if (oldfd >= 0 && oldfd < MAX_FDS && newfd >= 0 && newfd < MAX_FDS &&
                current >= 0 && current < nr_procese) {
                proc_table[current].fd_table[newfd] = proc_table[current].fd_table[oldfd];
                regs->eax = newfd;
            } else {
                regs->eax = -1;
            }
        }
        break;

    case 9:
        {
            int fd = regs->ebx;
            if (fd >= 0 && fd < MAX_FDS && current >= 0 && current < nr_procese) {
                int fd_val = proc_table[current].fd_table[fd];
                if (FD_IS_PIPE(fd_val))
                    pipe_close(FD_PIPE_ID(fd_val), FD_PIPE_END(fd_val));
                proc_table[current].fd_table[fd] = FD_NONE;
            }
            regs->eax = 0;
        }
        break;

    default:
        kprint("[Kernel] Syscall necunoscut\n");
        break;
    }
}

void init_syscalls() {
  uint32_t isr_addr = (uint32_t)&isr_syscall_stub;
  idt_set_gate(0x80, isr_addr, 0x08, 0xEE);

  load_idt(idt, sizeof(idt) - 1);
    
  kprint("IDT Initialized. Syscalls ready on INT 0x80.\n");
}
