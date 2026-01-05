#include "kernel.h"
#include "heap.h"
#include "idt.h"
#include "uart.h"

#define VGA_ADDRESS ((volatile uint16_t*)0xB8000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_COLOR 0x0F00

static int vga_col = 0;
static int vga_row = 0;

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

extern uint8_t _binary_bin_u1_bin_start;
extern uint8_t _binary_bin_u1_bin_end;
extern uint8_t _binary_bin_u2_bin_start;
extern uint8_t _binary_bin_u2_bin_end;

typedef struct {
    char* name;
    uint8_t* data;
    uint32_t size;
} ramfs_entry_t;

ramfs_entry_t ramfs[] = {
    {"u1", &_binary_bin_u1_bin_start, 0},
    {"u2", &_binary_bin_u2_bin_start, 0}
};

void disable_cursor() {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void kclear_screen() {
    for (uint16_t i = 0; i < VGA_WIDTH*VGA_HEIGHT; i++) {
        VGA_ADDRESS[i] = ' ' | VGA_COLOR;
    }
    vga_col = 0; vga_row = 0;
}

void kclear_line(){
  for(int i = 0; i < VGA_WIDTH; i++){
    VGA_ADDRESS[i+vga_row*VGA_WIDTH] = ' ' | VGA_COLOR;
  }
}

void kprint_newline(void) {
  vga_col = 0;
  vga_row++;
  if (vga_row >= VGA_HEIGHT) {
    vga_row = 0;
  }
  kclear_line();
}

void kprint(const char* str) {
  for (int i = 0; str[i] != '\0'; i++) {
    if (str[i] == '\n') {
      kprint_newline();
    } else {
      const int index = vga_row * VGA_WIDTH + vga_col;
      VGA_ADDRESS[index] = str[i] | VGA_COLOR;
      vga_col++;
      if (vga_col >= VGA_WIDTH) {
	kprint_newline();
      }
    }
  }
}

void kprint_int(int num) {
    char buffer[16];
    int i = 0;
    if (num == 0) {
        kprint("0");
        return;
    }
    
    if (num < 0) {
        kprint("-");
        num = -num;
    }

    while (num > 0) {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    }

    while (--i >= 0) {
        char c[2] = {buffer[i], '\0'};
        kprint(c);
    }
}

void kprint_hex(uint32_t n) {
    kprint("0x");
    char hex_chars[] = "0123456789ABCDEF";
    for (int i = 28; i >= 0; i -= 4) {
        int index = (n >> i) & 0xF;
        char c[2] = {hex_chars[index], '\0'};
        kprint(c);
    }
}

void simple_delay(int loops) {
    volatile int t = 0;
    for(int i = 0; i < loops * 10000; i++) {
        t++; 
    }
}

#define QUANTUM 4

void process1() { kprint("[PID 1] Hello\n"); }
void process2() { kprint("[PID 2] Buna ziua\n"); }
void process3() { kprint("[PID 3] Nihau\n"); }

int nr_procese = 3;
void (*processes[])() = {process1, process2, process3};
int remaining[] = {10000, 12000, 15000};
int current = 0;
int slice = 0;
int preemptive_mode = 0;

void yield() {
    if (preemptive_mode) return;
    
    kprint("[Yield] Process ");
    kprint_int(current + 1);
    kprint(" yielding\n");

    current = (current + 1) % nr_procese;
    slice = 0;
}

void scheduler_tick() {
    if (nr_procese == 0) return;

    processes[current]();
    remaining[current]--;
    slice++;

    if (remaining[current] <= 0) {
        kprint("[Done] Process ");
        kprint_int(current + 1);
        kprint(" finished\n");

        for (int i = current; i < nr_procese - 1; i++) {
            processes[i] = processes[i + 1];
            remaining[i] = remaining[i + 1];
        }
        nr_procese--;
        
        if (nr_procese == 0) {
            kprint("All processes are finished!\n");
            return;
        }
        
        current = current % nr_procese; 
        slice = 0;
        return;
    }

    if (preemptive_mode && slice >= QUANTUM) {
        kprint("[Preemption] Time finished PID ");
        kprint_int(current + 1);
        kprint(" - next process\n");
        
        current = (current + 1) % nr_procese;
        slice = 0;
    }
}

void syscall_write(const char* message, uint32_t len) {
  asm volatile("int $0x80" : : "a"(1), "b"(message), "c"(len));
}

void syscall_yield() {
    asm volatile("int $0x80" : : "a"(2));
}

uint32_t syscall_gettime() {
    uint32_t ticks;
    asm volatile("int $0x80" : "=a"(ticks) : "a"(3));
    return ticks;
}

void list_processes() {
    uart_print("\nPID | State   | Remaining Ticks\n");
    uart_print("-------------------------------\n");

    for (int i = 0; i < nr_procese; i++) {
        uart_print(" ");
        char pid_char = i + '0';
        char pid_str[2] = {pid_char, '\0'};
        uart_print(pid_str);
        
        uart_print("   | RUNNING | ");
        
        uart_print_int(remaining[i]);
        uart_print("\n");
    }
    
    if (nr_procese == 0) {
        uart_print("Nu exista procese active.\n");
    }
}

void kill_process(int pid) {
    if (pid < 0 || pid >= nr_procese) {
        uart_print("Eroare: PID invalid\n");
        return;
    }

    remaining[pid] = 0;
    
    uart_print("Procesul ");
    uart_print_int(pid);
    uart_print(" a fost marcat pentru terminare.\n");
}

void load_and_run(int program_index) {
    if (program_index < 0 || program_index > 1) return;

    uint32_t load_addr = (program_index == 0) ? 0x400000 : 0x500000;
    uint32_t size = ramfs[program_index].size;
      
    uint8_t* dest = (uint8_t*)load_addr;
    uint8_t* src = ramfs[program_index].data;
    
    for(uint32_t i = 0; i < size; i++) {
        dest[i] = src[i];
    }

    asm volatile("cli");
    if (nr_procese < 10) {
        processes[nr_procese] = (void (*)())load_addr;
        remaining[nr_procese] = 5000;
        nr_procese++;
        
        uart_print("Program ");
        uart_print(ramfs[program_index].name);
        uart_print(" incarcat\n");
    } else {
        uart_print("Eroare: Prea multe procese!\n");
    }
    asm volatile("sti");
}


int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++; s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char *s1, const char *s2, int n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

void shell_loop() {
    char cmd_buf[64];
    
    while(1) {
        uart_print("user@simple_os> ");
        uart_readline(cmd_buf, 64);

        if (strcmp(cmd_buf, "ps") == 0) {
	  asm volatile("cli");
	  list_processes();
	  asm volatile("sti");
        } 
        else if (strncmp(cmd_buf, "kill ", 5) == 0) {
	  int pid = cmd_buf[5] - '0';
	  asm volatile("cli");
	  kill_process(pid);
	  asm volatile("sti");
        }
	else if (strncmp(cmd_buf, "time", 4) == 0) {
	  uint32_t t = syscall_gettime();
	  uart_print("Ticks de la boot: ");
	  uart_print_int(t);
	  uart_print("\n");
	}
	else if (strncmp(cmd_buf, "write", 5) == 0) {
	  char* msg = cmd_buf + 6;
	  uint32_t len = 0;
	  while(msg[len] != '\0')len++;
	  syscall_write(msg, len);
	}
	else if (strncmp(cmd_buf, "exec", 4) == 0) {
	  char* msg = cmd_buf + 5;
	  if(strcmp(msg, "u1") == 0){
	      load_and_run(0);
	    }
	  else if(strcmp(msg, "u2") == 0){
	      load_and_run(1);
	    }
	}
        else if (strcmp(cmd_buf, "help") == 0) {
            uart_print("Comenzi: help, ps, kill <pid>, exec <prog>\n");
        }
        else {
            uart_print("Eroare: Comanda necunoscuta.\n");
        }
    }
}

void kernel_main(void) {
    kclear_screen();
    disable_cursor();
    kprint("Kernel is starting...\n");

    void* heap_base = (void*)0x00200000;
    size_t heap_size = 100 * 1024; 
    kheap_init(heap_base, heap_size);

    uart_init();
    uart_print("UART Initialized...\n");
    
    preemptive_mode = 1;
    
    init_interrupts();
    init_syscalls();

    kprint("Interrupts and Timer are now active.\n");

    ramfs[0].size = (uint32_t)&_binary_bin_u1_bin_end - (uint32_t)&_binary_bin_u1_bin_start;
    ramfs[1].size = (uint32_t)&_binary_bin_u2_bin_end - (uint32_t)&_binary_bin_u2_bin_start;
    
    shell_loop(); 

    for (;;) {
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
