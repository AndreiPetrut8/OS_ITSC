#include "vmm.h"
#include "pmm.h"

static uint32_t *kernel_pd = 0;

void vmm_init(uint32_t mem_kb) {
    uint32_t mem_size = mem_kb * 1024;

    kernel_pd = (uint32_t*)pmm_alloc_frame();
    for (int i = 0; i < 1024; i++) kernel_pd[i] = 0;

    /* identity-map every page of physical RAM */
    for (uint32_t addr = 0; addr < mem_size; addr += PAGE_SIZE) {
        vmm_map_page(kernel_pd, addr, addr, 0x03);
    }

    vmm_switch_pd((uint32_t)kernel_pd);

    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;          /* PG bit */
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
}

uint32_t vmm_get_kernel_pd(void) {
    return (uint32_t)kernel_pd;
}

void vmm_switch_pd(uint32_t pd_phys) {
    asm volatile("mov %0, %%cr3" :: "r"(pd_phys));
}

void vmm_map_page(uint32_t *pd, uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x3FF;

    if (!(pd[pd_idx] & 0x01)) {
        uint32_t pt_phys = pmm_alloc_frame();
        if (!pt_phys) return;
        uint32_t *pt = (uint32_t*)pt_phys;
        for (int i = 0; i < 1024; i++) pt[i] = 0;
        pd[pd_idx] = pt_phys | (flags & 0xFFF) | 0x01;
    }

    uint32_t pt_phys = pd[pd_idx] & ~0xFFF;
    uint32_t *pt = (uint32_t*)pt_phys;
    pt[pt_idx] = phys | (flags & 0xFFF) | 0x01;
}

void vmm_unmap_page(uint32_t *pd, uint32_t virt) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x3FF;

    if (!(pd[pd_idx] & 0x01)) return;

    uint32_t pt_phys = pd[pd_idx] & ~0xFFF;
    uint32_t *pt = (uint32_t*)pt_phys;
    pt[pt_idx] = 0;

    int empty = 1;
    for (int i = 0; i < 1024; i++) {
        if (pt[i] & 0x01) { empty = 0; break; }
    }
    if (empty) {
        pmm_free_frame(pt_phys);
        pd[pd_idx] = 0;
    }
}

uint32_t vmm_create_pd(void) {
    uint32_t pd_phys = pmm_alloc_frame();
    if (!pd_phys) return 0;
    uint32_t *pd = (uint32_t*)pd_phys;

    for (int i = 0; i < 1024; i++) pd[i] = 0;

    for (int i = 0; i < 1024; i++) {
        if (kernel_pd[i] & 0x01) {
            uint32_t pt_phys = pmm_alloc_frame();
            if (!pt_phys) {
                for (int j = 0; j < i; j++) {
                    if (pd[j] & 0x01) pmm_free_frame(pd[j] & ~0xFFF);
                }
                pmm_free_frame(pd_phys);
                return 0;
            }
            uint32_t *new_pt = (uint32_t*)pt_phys;
            uint32_t *old_pt = (uint32_t*)(kernel_pd[i] & ~0xFFF);
            for (int j = 0; j < 1024; j++) new_pt[j] = old_pt[j];
            pd[i] = pt_phys | (kernel_pd[i] & 0xFFF);
        }
    }
    return pd_phys;
}

void vmm_destroy_pd(uint32_t pd_phys) {
    uint32_t *pd = (uint32_t*)pd_phys;
    for (int i = 0; i < 1024; i++) {
        if (pd[i] & 0x01) {
            pmm_free_frame(pd[i] & ~0xFFF);
        }
    }
    pmm_free_frame(pd_phys);
}
