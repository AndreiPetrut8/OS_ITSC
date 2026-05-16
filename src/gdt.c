#include "gdt.h"

struct gdt_entry gdt[6];
struct gdt_ptr gp;

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;
    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].access      = access;
}

void gdt_init(void) {
    gp.limit = sizeof(gdt) - 1;
    gp.base  = (uint32_t)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);                /* null */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); /* kernel code */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); /* kernel data */
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); /* user code  */
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); /* user data  */
    gdt_set_gate(5, 0, 0, 0, 0);                /* TSS - filled by tss.c */

    gdt_flush();
}
