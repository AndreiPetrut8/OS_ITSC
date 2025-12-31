#include "heap.h"
#include "kernel.h"


#define ALIGN_UP(x, a) (((x) + ((a)-1)) & ~((a)-1))
#define ALIGN 8

typedef struct block_header {
    size_t size;                
    struct block_header *next;  
    int free;                   
} block_header_t;

static uint8_t *heap_start = NULL;
static uint8_t *heap_end = NULL;
static uint8_t *heap_bump = NULL; 
static block_header_t *free_list = NULL;

static size_t alloc_count = 0;

static inline block_header_t *payload_to_header(void *p) 
{
    if (!p) return NULL;
    return (block_header_t*)((uint8_t*)p - sizeof(block_header_t));
}

static inline void *header_to_payload(block_header_t *h) 
{
    return (void*)((uint8_t*)h + sizeof(block_header_t));
}

void kheap_init(void *base, size_t size) 
{
    heap_start = (uint8_t*)ALIGN_UP((uintptr_t)base, ALIGN);
    heap_end = (uint8_t*)base + size;
    heap_bump = heap_start;
    free_list = NULL;
    alloc_count = 0;
}

static block_header_t *find_free_block(size_t asize, block_header_t **prev_ptr) 
{
    block_header_t *prev = NULL;
    block_header_t *cur = free_list;
    while (cur) 
    {
        if (cur->size >= asize) 
        {
            if (prev_ptr) 
            {
                *prev_ptr = prev;
            }
            return cur;
        }
        prev = cur;
        cur = cur->next;
    }
    return NULL;
}

static void split_block(block_header_t *b, size_t asize) 
{
    size_t remaining = b->size - asize;
    if (remaining >= (sizeof(block_header_t) + ALIGN)) 
    {
        uint8_t *new_hdr_addr = (uint8_t*)header_to_payload(b) + asize;
        block_header_t *newh = (block_header_t*)new_hdr_addr;
        newh->size = remaining - sizeof(block_header_t);
        newh->free = 1;
        newh->next = b->next;
        b->size = asize;
        b->next = NULL;
        if (free_list == b) 
        {
            free_list = newh;
        } 
        else 
        {
            block_header_t *p = free_list;
            while (p && p->next != b) 
            {
                p = p->next;
            }
            if (p) 
            {
                p->next = newh;
            }
        }
    } 
    else 
    {
        if (free_list == b)
        {
            free_list = b->next;
        }
        else
        {
            block_header_t *p = free_list;
            while (p && p->next != b)
            {
                p = p->next;
            }

            if (p)
            {
                p->next = b->next;
            }
        }

        b->next = NULL;
        b->free = 0; 
    }
}

static void insert_and_coalesce(block_header_t *b) 
{
    if (!free_list) 
    {
        b->next = NULL;
        free_list = b;
        return;
    }

    block_header_t *prev = NULL;
    block_header_t *cur = free_list;
    while (cur && cur < b) 
    {
        prev = cur;
        cur = cur->next;
    }

    b->next = cur;
    if (prev) prev->next = b;
    else free_list = b;

    if (b->next) 
    {
        uint8_t *b_end = (uint8_t*)header_to_payload(b) + b->size;
        uint8_t *next_hdr = (uint8_t*)b->next;
        if (b_end == next_hdr) 
        {
            b->size = b->size + sizeof(block_header_t) + b->next->size;
            b->next = b->next->next;
        }
    }

    if (prev) 
    {
        uint8_t *prev_end = (uint8_t*)header_to_payload(prev) + prev->size;
        uint8_t *b_hdr = (uint8_t*)b;
        if (prev_end == b_hdr) 
        {
            prev->size = prev->size + sizeof(block_header_t) + b->size;
            prev->next = b->next;
        }
    }
}

void *kmalloc(size_t size) 
{
    if (size == 0) 
    {
        return NULL;
    }
    size = ALIGN_UP(size, ALIGN);

    block_header_t *prev = NULL;
    block_header_t *found = find_free_block(size, &prev);
    if (found) 
    {
        if (prev) 
        {
            prev->next = found->next;
        }
        else 
        {
            free_list = found->next;
        }
        found->free = 0;
        found->next = NULL;
        split_block(found, size);
        alloc_count++;
        return header_to_payload(found);
    }

    size_t need = sizeof(block_header_t) + size;
    uint8_t *next = (uint8_t*)ALIGN_UP((uintptr_t)heap_bump, ALIGN);
    if (next + need > heap_end) 
    {
        return NULL;
        
    }
    block_header_t *h = (block_header_t*)next;
    h->size = size;
    h->free = 0;
    h->next = NULL;
    heap_bump = next + need;
    alloc_count++;
    return header_to_payload(h);
}


void kfree(void *ptr) 
{
    if (!ptr) 
    {
        return;
    }
    block_header_t *h = payload_to_header(ptr);
    uint8_t *h_addr = (uint8_t*)h;
    if (h_addr < heap_start || h_addr >= heap_end) 
    {
        return;
    }

    h->free = 1;
    insert_and_coalesce(h);
    if (alloc_count > 0) alloc_count--;
}

size_t kheap_total_size(void) 
{
    return (size_t)(heap_end - heap_start);
}

size_t kheap_used_size(void) 
{
    size_t total = kheap_total_size();
    size_t free_sum = 0;
    block_header_t *cur = free_list;
    while (cur) 
    {
        free_sum += cur->size + sizeof(block_header_t);
        cur = cur->next;
    }
    size_t bump_used = (size_t)(heap_bump - heap_start);
    if (bump_used > free_sum) 
    {
        return bump_used - free_sum;
    }
    return 0;
}

size_t kheap_free_size(void) 
{
    return kheap_total_size() - kheap_used_size();
}
    
size_t kheap_alloc_count(void) 
{
    return alloc_count;
}

void kheap_dump(void) 
{
    kprint("KHEAP DUMP:\n");
    kprint("  heap start: ");kprint_hex((uint32_t)(uintptr_t)heap_start);kprint("\n");
    kprint("  heap end  : ");kprint_hex((uint32_t)(uintptr_t)heap_end);kprint("\n");
    kprint("  bump ptr  : ");kprint_hex((uint32_t)(uintptr_t)heap_bump);kprint("\n");
    kprint("  total     : ");kprint_int(kheap_total_size());kprint("bytes\n");
    kprint("  used      : ");kprint_int(kheap_used_size());kprint("bytes\n");
    kprint("  free      : ");kprint_int(kheap_free_size());kprint("bytes\n");
    kprint("  allocs    : ");kprint_int(kheap_alloc_count());kprint("\n");

    kprint("  Free list blocks:\n");
    block_header_t *cur = free_list;
    while (cur) 
    {
      kprint("    block @ ");kprint_hex((uint32_t)(uintptr_t)cur);
      kprint(": size= "); kprint_int(cur->size);
      kprint("\n");
        cur = cur->next;
    }
    kprint("\n");
}
