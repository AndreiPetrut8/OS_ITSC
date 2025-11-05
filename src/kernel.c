#include <stdint.h>

void kernel_main(void) {
    volatile uint16_t *vga = (uint16_t*)0xB8000;

    vga[0] = 0x0748; // 'H'
    vga[1] = 0x0769; // 'i'

    for (;;) {
        asm volatile("hlt");
    }
}
