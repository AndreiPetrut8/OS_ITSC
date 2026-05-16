#ifndef TSS_H
#define TSS_H

#include <stdint.h>

void tss_init(uint32_t kernel_stack_top);
void tss_set_esp0(uint32_t stack);
void tss_install_gdt(void);

#endif
