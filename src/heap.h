#pragma once
#include <stddef.h>
#include <stdint.h>
#include "ramfs.h"

typedef struct {
    void (*entry)();
    uint8_t *start;
    uint8_t *end;
} process_info_t;

void kheap_init(void *heap_base, size_t heap_size);

void *kmalloc(size_t size);
void kfree(void *ptr);

void kheap_dump(void);

size_t kheap_total_size(void);
size_t kheap_free_size(void);
size_t kheap_used_size(void);
size_t kheap_alloc_count(void);

// Test function

void heap_test(void);
void heap_test_processes(void);