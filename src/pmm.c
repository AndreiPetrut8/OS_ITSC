#include "pmm.h"

static uint32_t *bitmap = 0;
static uint32_t total_frames = 0;
static uint32_t used_frames  = 0;

void pmm_init(uint32_t total_mem_kb, uint32_t kernel_end_addr) {
    total_frames = (total_mem_kb * 1024) / PAGE_SIZE;

    uint32_t bitmap_bytes = total_frames / 8;
    if (total_frames % 8) bitmap_bytes++;

    // Place bitmap just above the kernel, page-aligned
    bitmap = (uint32_t *)((kernel_end_addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));

    // Mark everything used initially
    for (uint32_t i = 0; i < bitmap_bytes / 4; i++)
        bitmap[i] = 0xFFFFFFFF;

    // Mark frames after the bitmap as free
    uint32_t first_free = ((uint32_t)bitmap + bitmap_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint32_t f = first_free; f < total_frames; f++) {
        uint32_t idx = f / 32;
        uint32_t off = f % 32;
        bitmap[idx] &= ~(1 << off);
    }
    used_frames = first_free;
}

uint32_t pmm_alloc_frame(void) {
    for (uint32_t f = 0; f < total_frames; f++) {
        uint32_t idx = f / 32;
        uint32_t off = f % 32;
        if (!(bitmap[idx] & (1 << off))) {
            bitmap[idx] |= (1 << off);
            used_frames++;
            return f * PAGE_SIZE;
        }
    }
    return 0; // out of memory
}

void pmm_free_frame(uint32_t phys) {
    uint32_t f = phys / PAGE_SIZE;
    if (f >= total_frames) return;
    uint32_t idx = f / 32;
    uint32_t off = f % 32;
    if (bitmap[idx] & (1 << off)) {
        bitmap[idx] &= ~(1 << off);
        used_frames--;
    }
}

uint32_t pmm_total_frames(void) { return total_frames; }
uint32_t pmm_used_frames(void)  { return used_frames; }
