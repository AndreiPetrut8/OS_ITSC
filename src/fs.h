#ifndef FS_H
#define FS_H

#include "heap.h"

#define MAX_NAME_LEN 32

typedef enum { FS_FILE, FS_DIRECTORY } entry_type_t;

typedef struct fs_node {
    char name[MAX_NAME_LEN];
    entry_type_t type;
    uint8_t *data;
    size_t size;
    struct fs_node *parent;
    struct fs_node *children;
    struct fs_node *next;
} fs_node_t;

void fs_init(void);
int fs_mkdir(const char *name);
void fs_rm(const char *name);
int fs_touch(const char *name); // pentru a crea un fisier gol
void fs_ls(void);
int fs_cd(const char *name);
void fs_save_to_disk(void);
void fs_load_from_disk(void);
int fs_create_file(const char *name);
int fs_write_file(const char *name, const char *content, uint32_t size);
void fs_cat(const char *name);

#endif
