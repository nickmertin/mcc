#ifndef ISU_BUFFER_H
#define ISU_BUFFER_H

#include <stddef.h>
#include <stdbool.h>

struct buffer_node_t {
    size_t length;
    struct buffer_node_t *next;
    char data[0];
};

struct buffer_iterator_t {
    char *current;
    size_t remaining;
    struct buffer_node_t *next;
};

typedef struct buffer_node_t *buffer_t;

buffer_t *buffer_create();

buffer_t *buffer_create_size(size_t size);

struct buffer_iterator_t buffer_iterate(buffer_t *buffer);

bool buffer_iterator_next(struct buffer_iterator_t *iterator);

void buffer_append(buffer_t *buffer, const char *data, size_t length);

void buffer_append_one(buffer_t *buffer, char c);

size_t buffer_length(buffer_t *buffer);

bool buffer_empty(buffer_t *buffer);

void buffer_defrag(buffer_t *buffer);

void buffer_destroy(buffer_t *buffer);

#endif //ISU_BUFFER_H
