#pragma once
#include <stdint.h>

struct idt_entry_t {
  uint16_t offset_low;
  uint16_t selector;
  uint8_t  zero;
  uint8_t  type_attr;
  uint16_t offset_high;
} __attribute__((packed));

typedef struct {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
} registers_t;


extern struct idt_entry_t idt[256];

void init_syscalls();
void init_interrupts();
void pic_remap();
void pit_init(uint32_t frequency);
void idt_set_gate(int num, uint32_t base, uint16_t sel, uint8_t flags);
