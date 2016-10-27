#include <stdlib.h>
#include <memory.h>
#include "buffer.h"

buffer_t *buffer_create() {
    buffer_t *buffer = malloc(sizeof(buffer_t));
    *buffer = NULL;
    return buffer;
}

buffer_t *buffer_create_size(size_t size) {
    buffer_t *buffer = malloc(sizeof(buffer_t));
    *buffer = malloc(sizeof(struct buffer_node_t) + size);
    (*buffer)->length = size;
    return buffer;
}

struct buffer_iterator_t buffer_iterate(buffer_t *buffer) {
    return (struct buffer_iterator_t) { .current = (*buffer)->data, .remaining = (*buffer)->length, .next = (*buffer)->next };
}

bool buffer_iterator_next(struct buffer_iterator_t *iterator) {
    if (!--iterator->remaining) {
        if (!iterator->next)
            return false;
        *iterator = (struct buffer_iterator_t) {.current = iterator->next->data, .remaining = iterator->next->length, .next = iterator->next->next};
    } else
        ++iterator->current;
    return true;
}

void buffer_append(buffer_t *buffer, const char *data, size_t length) {
    while (*buffer)
        buffer = &(*buffer)->next;
    *buffer = malloc(sizeof(struct buffer_node_t) + length);
    (*buffer)->next = NULL;
    (*buffer)->length = length;
    memcpy((*buffer)->data, data, length);
}

void buffer_append_one(buffer_t *buffer, char c) {
    buffer_append(buffer, &c, 1);
}

size_t buffer_length(buffer_t *buffer) {
    size_t length = 0;
    while (*buffer) {
        length += (*buffer)->length;
        buffer = &(*buffer)->next;
    }
    return length;
}

bool buffer_empty(buffer_t *buffer) {
    return !(*buffer && (*buffer)->length);
}

void buffer_defrag(buffer_t *buffer) {
    buffer_t *new = buffer_create_size(buffer_length(buffer));
    for (size_t pos = 0; *buffer; buffer = &(*buffer)->next) {
        memcpy((char *)(*new +1) + pos, *buffer + 1, (*buffer)->length);
        pos += (*buffer)->length;
    }
    buffer_t ptr = *buffer;
    while (ptr) {
        buffer_t next = ptr->next;
        free(ptr);
        ptr = next;
    }
    *buffer = *new;
    free(new);
}

void buffer_destroy(buffer_t *buffer) {
    buffer_t ptr = *buffer;
    while (ptr) {
        buffer_t next = ptr->next;
        free(ptr);
        ptr = next;
    }
    free(buffer);
}
