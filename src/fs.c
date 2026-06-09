#include <stdint.h>
#include "fs.h"
#include "uart.h"
#include "heap.h"
#include "ata.h"

#define MAX_NAME_LEN 32
#define FS_START_SECTOR 100
#define DATA_START_SECTOR 101
#define MAX_DISK_NODES 16

typedef struct
{
    char name[MAX_NAME_LEN];
    int type;
    int is_used;
    int parent_idx;
    uint32_t size;
    uint32_t data_sector;
} disk_entry_t;

static fs_node_t *root = NULL;
static fs_node_t *current_dir = NULL;

int m_strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

char *strcpy(char *dest, const char *src)
{
    char *d = dest;
    while ((*d++ = *src++))
        ;
    return dest;
}

fs_node_t *create_node(const char *name, entry_type_t type)
{
    fs_node_t *node = (fs_node_t *)kmalloc(sizeof(fs_node_t));
    if (!node)
        return NULL;

    int i;
    for (i = 0; i < MAX_NAME_LEN - 1 && name[i] != '\0'; i++)
        node->name[i] = name[i];

    node->name[i] = '\0';
    node->type = type;
    node->data = NULL;
    node->size = 0;
    node->parent = NULL;
    node->children = NULL;
    node->next = NULL;

    return node;
}
void fs_init()
{
    disk_entry_t table[MAX_DISK_NODES];
    ata_read_sector(FS_START_SECTOR, (uint8_t *)table);

    if (table[0].is_used == 1 && m_strcmp(table[0].name, "root") == 0)
    {
        uart_print("FS: Restaurare sistem de fisiere si date...\n");

        fs_node_t *nodes_map[MAX_DISK_NODES];
        for (int i = 0; i < MAX_DISK_NODES; i++)
            nodes_map[i] = NULL;

        for (int i = 0; i < MAX_DISK_NODES; i++)
        {
            if (table[i].is_used)
            {
                nodes_map[i] = create_node(table[i].name, table[i].type);
                if (!nodes_map[i])
                    continue;

                if (table[i].type == FS_FILE && table[i].size > 0)
                {
                    nodes_map[i]->size = table[i].size;
                    nodes_map[i]->data = (uint8_t *)kmalloc(table[i].size);

                    if (nodes_map[i]->data)
                    {

                        uint8_t sector_buffer[512];
                        ata_read_sector(table[i].data_sector, sector_buffer);

                        for (uint32_t j = 0; j < table[i].size; j++)
                        {
                            nodes_map[i]->data[j] = sector_buffer[j];
                        }
                    }
                    else
                    {
                        uart_print("FS Eroare: Memorie insuficienta la restaurare fisier.\n");
                        nodes_map[i]->size = 0;
                    }
                }
            }
        }

        root = nodes_map[0];
        current_dir = root;
        for (int i = 1; i < MAX_DISK_NODES; i++)
        {
            if (table[i].is_used && nodes_map[i] != NULL)
            {
                int p_idx = table[i].parent_idx;

                if (p_idx >= 0 && p_idx < MAX_DISK_NODES && nodes_map[p_idx] != NULL)
                {
                    fs_node_t *parent_node = nodes_map[p_idx];
                    fs_node_t *child_node = nodes_map[i];

                    child_node->parent = parent_node;
                    child_node->next = parent_node->children;
                    parent_node->children = child_node;
                }
            }
        }
    }
    else
    {
        uart_print("Creare sistem nou...\n");
        root = create_node("root", FS_DIRECTORY);
        current_dir = root;
    }
}

static int serialize_node(fs_node_t *node, disk_entry_t *table, int parent_index, int *current_idx)
{
    if (!node || *current_idx >= MAX_DISK_NODES)
    {
        return *current_idx;
    }

    int my_index = *current_idx;
    (*current_idx)++;

    table[my_index].is_used = 1;
    strcpy(table[my_index].name, node->name);
    table[my_index].type = node->type;
    table[my_index].parent_idx = parent_index;
    table[my_index].size = node->size;

    if (node->type == FS_FILE && node->size > 0 && node->data)
    {
        table[my_index].data_sector = DATA_START_SECTOR + my_index;

        uint8_t sector_buffer[512];
        for (int i = 0; i < 512; i++)
            sector_buffer[i] = 0;
        uint32_t bytes_to_copy = node->size > 512 ? 512 : node->size;
        for (uint32_t i = 0; i < bytes_to_copy; i++)
        {
            sector_buffer[i] = node->data[i];
        }

        ata_write_sector(table[my_index].data_sector, sector_buffer);
    }
    else
    {
        table[my_index].data_sector = 0;
    }

    fs_node_t *child = node->children;
    while (child)
    {
        serialize_node(child, table, my_index, current_idx);
        child = child->next;
    }

    return my_index;
}

void fs_save_to_disk()
{
    disk_entry_t table[MAX_DISK_NODES];

    for (int i = 0; i < MAX_DISK_NODES; i++)
    {
        table[i].is_used = 0;
        table[i].parent_idx = -1;
        table[i].size = 0;
        table[i].data_sector = 0;
    }

    int total_nodes = 0;

    serialize_node(root, table, -1, &total_nodes);

    ata_write_sector(FS_START_SECTOR, (uint8_t *)table);
    uart_print("Structura si datele au fost salvate pe disc.\n");
}

int fs_mkdir(const char *name)
{
    fs_node_t *new_dir = create_node(name, FS_DIRECTORY);
    if (!new_dir)
    {
        return -1;
    }
    new_dir->parent = current_dir;
    new_dir->next = current_dir->children;
    current_dir->children = new_dir;
    return 0;
}

int fs_cd(const char *name)
{
    if (m_strcmp(name, "..") == 0)
    {
        if (current_dir->parent)
            current_dir = current_dir->parent;
        return 0;
    }
    fs_node_t *curr = current_dir->children;
    while (curr)
    {
        if (curr->type == FS_DIRECTORY && m_strcmp(name, curr->name) == 0)
        {
            current_dir = curr;
            return 0;
        }
        curr = curr->next;
    }
    uart_print("Director negasit.\n");
    return -1;
}

void fs_ls(void)
{
    fs_node_t *curr = current_dir->children;
    uart_print("Locatie: ");
    uart_print(current_dir->name);
    uart_print("\n");
    while (curr)
    {
        uart_print(curr->type == FS_DIRECTORY ? "[DIR] " : "[FILE] ");
        uart_print(curr->name);
        uart_print("\n");
        curr = curr->next;
    }
}

static void free_node_recursive(fs_node_t *node)
{
    if (!node)
        return;

    fs_node_t *curr_child = node->children;
    while (curr_child)
    {
        fs_node_t *next_child = curr_child->next;
        free_node_recursive(curr_child);
        curr_child = next_child;
    }

    if (node->data)
    {
        kfree(node->data);
    }

    kfree(node);
}

static int is_parent_of(fs_node_t *target, fs_node_t *node)
{
    fs_node_t *curr = node;
    while (curr)
    {
        if (curr == target)
            return 1;
        curr = curr->parent;
    }
    return 0;
}

void fs_rm(const char *name)
{
    if (m_strcmp(name, "root") == 0)
    {
        uart_print("Directorul root nu poate fi sters\n");
        return;
    }

    fs_node_t *current = current_dir->children;
    fs_node_t *prev = NULL;

    while (current)
    {
        if (m_strcmp(name, current->name) == 0)
        {

            if (is_parent_of(current, current_dir))
            {
                uart_print("Eroare: Nu se poate sterge un director aflat in utilizare.\n");
                return;
            }

            if (prev)
            {
                prev->next = current->next;
            }
            else
            {
                current_dir->children = current->next;
            }
            free_node_recursive(current);

            uart_print("Elementul a fost sters cu succes din memorie.\n");
            return;
        }

        prev = current;
        current = current->next;
    }

    uart_print("Fisierul sau directorul nu a fost gasit\n");
}

int fs_create_file(const char *name)
{
    fs_node_t *curr = current_dir->children;
    while (curr)
    {
        if (m_strcmp(name, curr->name) == 0)
        {
            uart_print("Fisierul sau directorul exista deja.\n");
            return -1;
        }
        curr = curr->next;
    }

    fs_node_t *new_file = create_node(name, FS_FILE);
    if (!new_file)
    {
        return -1;
    }

    new_file->parent = current_dir;
    new_file->next = current_dir->children;
    current_dir->children = new_file;

    return 0;
}
int fs_write_file(const char *name, const char *content, uint32_t size)
{

    if (size > 512)
    {
        uart_print("Eroare: Dimensiunea maxima este de 512 octeti.\n");
        return -1;
    }

    fs_node_t *curr = current_dir->children;
    while (curr)
    {
        if (curr->type == FS_FILE && m_strcmp(name, curr->name) == 0)
        {
            if (curr->data)
            {
                kfree(curr->data);
                curr->data = NULL;
                curr->size = 0;
            }

            if (size == 0 || !content)
            {
                return 0;
            }

            curr->data = (uint8_t *)kmalloc(size);
            if (!curr->data)
            {
                uart_print("FS_DEBUG: EROARE! kmalloc a returnat NULL (lipsa memorie heap).\n");
                return -1;
            }

            for (uint32_t i = 0; i < size; i++)
            {
                curr->data[i] = (uint8_t)content[i];
            }
            curr->size = size;

            return 0;
        }
        curr = curr->next;
    }

    uart_print("Eroare: Fisierul nu a fost gasit in directorul curent.\n");
    return -1;
}

void fs_cat(const char *name)
{
    fs_node_t *curr = current_dir->children;

    while (curr)
    {
        if (m_strcmp(name, curr->name) == 0)
        {
            if (curr->type != FS_FILE)
            {
                uart_print("Eroare: Acesta este un director, nu un fisier.\n");
                return;
            }

            if (curr->size == 0 || !curr->data)
            {
                uart_print("(fisierul este gol)\n");
                return;
            }

            for (uint32_t i = 0; i < curr->size; i++)
            {
                uart_putc((char)curr->data[i]);
            }
            uart_print("\n");
            return;
        }
        curr = curr->next;
    }

    uart_print("Eroare: Fisierul nu a fost gasit.\n");
}
