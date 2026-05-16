#include "pipe.h"

static pipe_t  pipes[MAX_PIPES];
static uint8_t pipe_used[MAX_PIPES];

void pipe_init(void) {
    for (int i = 0; i < MAX_PIPES; i++)
        pipe_used[i] = 0;
}

int pipe_alloc(void) {
    for (int i = 0; i < MAX_PIPES; i++) {
        if (!pipe_used[i]) {
            pipe_used[i] = 1;
            pipes[i].read_pos   = 0;
            pipes[i].write_pos  = 0;
            pipes[i].count      = 0;
            pipes[i].read_open  = 1;
            pipes[i].write_open = 1;
            return i;
        }
    }
    return -1;
}

void pipe_free(int id) {
    if (id >= 0 && id < MAX_PIPES)
        pipe_used[id] = 0;
}

int pipe_write(int id, const char *data, uint32_t len) {
    if (id < 0 || id >= MAX_PIPES || !pipe_used[id])
        return -1;
    pipe_t *p = &pipes[id];
    if (!p->write_open)
        return -1;

    uint32_t written = 0;
    while (written < len && p->count < PIPE_BUF_SIZE) {
        p->buffer[p->write_pos] = data[written++];
        p->write_pos = (p->write_pos + 1) % PIPE_BUF_SIZE;
        p->count++;
    }
    return (int)written;
}

int pipe_read(int id, char *data, uint32_t len) {
    if (id < 0 || id >= MAX_PIPES || !pipe_used[id])
        return -1;
    pipe_t *p = &pipes[id];
    if (!p->read_open)
        return -1;

    uint32_t n = 0;
    while (n < len && p->count > 0) {
        data[n++] = p->buffer[p->read_pos];
        p->read_pos = (p->read_pos + 1) % PIPE_BUF_SIZE;
        p->count--;
    }

    if (n == 0 && !p->write_open)
        return -1;

    return (int)n;
}

void pipe_close(int id, int end) {
    if (id < 0 || id >= MAX_PIPES || !pipe_used[id])
        return;
    if (end == 0) pipes[id].read_open  = 0;
    else          pipes[id].write_open = 0;

    if (!pipes[id].read_open && !pipes[id].write_open)
        pipe_free(id);
}
