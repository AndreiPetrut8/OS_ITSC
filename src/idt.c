#include "idt.h"
#include "heap.h"
#include "kernel.h"
#include "uart.h"

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

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

volatile uint32_t system_ticks = 0;

void timer_handler() {
    system_ticks++;
    static int in_tick = 0;
    if (in_tick) {
        return;
    }
    in_tick = 1;
    scheduler_tick();
    in_tick = 0;
}

void syscall_handler(registers_t* regs) {
  switch (regs->eax) {

  case 1:
    {
      int fd = regs->ebx;
      char* buffer = (char*)regs->ecx;
      uint32_t len = regs->edx;

      if (fd == 1) {
	uart_print(buffer);
      } 
      else if (fd == 2) {
	char temp[len];
	for (uint32_t i = 0; i < len; i++) {
	  temp[i] = buffer[i];
	}
	kprint(temp);
      }
      regs->eax = len;
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
