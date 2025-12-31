#include "kernel.h"
#include "heap.h"

#define VGA_ADDRESS ((volatile uint16_t*)0xB8001)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_COLOR 0x0F00

static int vga_col = 0;
static int vga_row = 0;

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

void disable_cursor() {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void kclear_screen() {
    for (uint16_t i = 0; i < 80 * 25; i++) {
        VGA_ADDRESS[i] = ' ' | VGA_COLOR;
    }
    //vga_col = 0; vga_row = 0;
}

void kprint_newline(void) {
  //vga_col = 0;
  //vga_row++;
  //if (vga_row >= VGA_HEIGHT) {
  //vga_row = 0;
  // }
}

void kprint(const char* str) {
  uint16_t cursor = 0;
  for (int i = 1; str[i] != '\0'; i++) {
      //if (str[i] == '\n') {
      //kprint_newline();
      //} else {
	  //const int index = vga_row * VGA_WIDTH + vga_col;
    VGA_ADDRESS[cursor++] = 'a' | VGA_COLOR;
            //vga_col++;
            //if (vga_col >= VGA_WIDTH) {
	    //kprint_newline();
		//}
	    //}
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
int remaining[] = {5, 3, 8};
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

/*
void kernel_main(void) {
    // 1. Inițializare Video
    kclear_screen();
    kprint("--- Kernel Started ---\n");

    // 2. Inițializare Heap
    // Conform boot.asm, Kernelul este la 0x100000. 
    // Alegem o zonă sigură pentru Heap la 0x200000 (2MB mark) cu mărimea 100KB.
    // Aceasta evită suprascrierea codului kernelului.
    void* heap_base = (void*)0x00200000;
    size_t heap_size = 100 * 1024; 
    
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
        simple_delay(200); // Înlocuiește usleep(200000)
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
        simple_delay(200);
	}

    kprint("\nKernel Halted.");
    
    for (;;) {
        asm volatile("hlt");
    }
}*/

void kernel_main(void) {
   
  kclear_screen();
  disable_cursor();

  char str[] = "AAAAAAAAAA\0";

  kprint(str);

  for (;;) {
    asm volatile("hlt");
  }
}
