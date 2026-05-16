#ifndef PMM_H
#define PMM_H

#include <stdint.h>

#define PAGE_SIZE 4096

void pmm_init(uint32_t total_mem_kb, uint32_t kernel_end_addr);
uint32_t pmm_alloc_frame(void);
void pmm_free_frame(uint32_t phys);
uint32_t pmm_total_frames(void);
uint32_t pmm_used_frames(void);

#endif
