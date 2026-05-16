#ifndef VMM_H
#define VMM_H

#include <stdint.h>

#define PAGE_SIZE 4096

/* identity-map all RAM, enable paging, create kernel page directory */
void     vmm_init(uint32_t mem_kb);

uint32_t vmm_get_kernel_pd(void);
void     vmm_switch_pd(uint32_t pd_phys);

/* create/destroy a user page directory (kernel mappings are copied) */
uint32_t vmm_create_pd(void);
void     vmm_destroy_pd(uint32_t pd_phys);

void     vmm_map_page(uint32_t *pd, uint32_t virt, uint32_t phys, uint32_t flags);
void     vmm_unmap_page(uint32_t *pd, uint32_t virt);

#endif
