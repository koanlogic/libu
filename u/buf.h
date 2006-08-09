#ifndef _U_LIBU_BUF_H_
#define _U_LIBU_BUF_H_
#include <stdlib.h>
#include <unistd.h>

struct u_buf_s
{
    char *data;
    size_t size, len;
};

typedef struct u_buf_s u_buf_t;

int u_buf_append(u_buf_t *buf, void *data, size_t size);
int u_buf_clear(u_buf_t *buf);
int u_buf_detach(u_buf_t *buf);
int u_buf_set(u_buf_t *buf, void *data, size_t size);
int u_buf_load(u_buf_t *buf, char *fqn);
int u_buf_free(u_buf_t *buf);
int u_buf_create(u_buf_t **pbuf);
int u_buf_reserve(u_buf_t *buf, size_t size);
void* u_buf_ptr(u_buf_t *buf);
size_t u_buf_len(u_buf_t *buf);
size_t u_buf_size(u_buf_t *buf);

#endif
