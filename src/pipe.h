#ifndef PIPE_H
#define PIPE_H

#include <stdint.h>

#define MAX_PIPES     16
#define PIPE_BUF_SIZE 4096

typedef struct {
    uint8_t  buffer[PIPE_BUF_SIZE];
    uint32_t read_pos;
    uint32_t write_pos;
    uint32_t count;
    int      read_open;
    int      write_open;
} pipe_t;

void pipe_init(void);
int  pipe_alloc(void);
void pipe_free(int id);
int  pipe_write(int id, const char *data, uint32_t len);
int  pipe_read(int id, char *data, uint32_t len);
void pipe_close(int id, int end);   // end: 0=read, 1=write

#endif
