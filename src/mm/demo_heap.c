#include <stdio.h>
#include <string.h>
#include "heap.h"

#define DEMO_HEAP_SIZE (64 * 1024)
static uint8_t demo_heap[DEMO_HEAP_SIZE];

int main(void) 
{
    printf("Demo kmalloc/kfree minimal\n");
    kheap_init(demo_heap, DEMO_HEAP_SIZE);
    kheap_dump();

    void *a = kmalloc(64);
    printf("Allocated A (64) -> %p\n", a);
    void *b = kmalloc(1000);
    printf("Allocated B (1000) -> %p\n", b);
    void *c = kmalloc(2000);
    printf("Allocated C (2000) -> %p\n", c);
    kheap_dump();

    printf("Free B\n");
    kfree(b);
    kheap_dump();

    printf("Allocate D (900) — should reuse freed B if best-fit\n");
    void *d = kmalloc(900);
    printf("Allocated D -> %p\n", d);
    kheap_dump();

    printf("Free A, C, D\n");
    kfree(a);
    kfree(c);
    kfree(d);
    kheap_dump();

    void *arr[20];
    for (int i = 0; i < 20; i++) {
        arr[i] = kmalloc(256 + i*8);
        printf("arr[%d] = %p\n", i, arr[i]);
    }
    kheap_dump();

    for (int i = 0; i < 20; i += 2) {
        kfree(arr[i]);
    }
    printf("Freed half of small blocks\n");
    kheap_dump();

    return 0;
}
