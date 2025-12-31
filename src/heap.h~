#pragma once
#include <stddef.h>
#include <stdint.h>

void kheap_init(void *heap_base, size_t heap_size);

void *kmalloc(size_t size);
void kfree(void *ptr);

void kheap_dump(void);

size_t kheap_total_size(void);
size_t kheap_free_size(void);
size_t kheap_used_size(void);
size_t kheap_alloc_count(void);
