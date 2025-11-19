#include <stdint.h>
#include "kernel.h"

void kernel_main(void) {
    const uint16_t WIDTH = 80;
    const uint16_t HEIGHT = 25;
    volatile uint16_t *vga = (uint16_t*)0xB8001;
    uint16_t color = 0x000F;

   
    for(uint16_t i = 0; i < WIDTH * HEIGHT; i++){
      vga[i] ^= vga[i];
    }

    const uint16_t msg[] = {'H', 'e', 'l', 'l', 'o',',', 'V', 'G', 'A', '!'};

    for(uint16_t i = 0; i < 13; i++){
      vga[i] = (msg[i] | (color << 8));
    }
   

    for (;;) {
        asm volatile("hlt");
    }
}
