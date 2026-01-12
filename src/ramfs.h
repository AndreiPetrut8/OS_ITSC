#ifndef RAMFS_H
#define RAMFS_H

#include <stdint.h>

typedef struct {
    char* name;
    uint8_t* data;
    uint32_t size;
} ramfs_entry_t;

#endif
