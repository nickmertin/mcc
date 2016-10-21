#ifndef ISU_LINKED_LIST_H
#define ISU_LINKED_LIST_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "delegate.h"

struct linked_list_node_t {
    struct linked_list_node_t *ptr;
    char data[0];
};

typedef struct linked_list_node_t *linked_list_t;

linked_list_t *linked_list_create();

bool linked_list_insert(linked_list_t *list, size_t i, void *data, size_t size);

bool linked_list_remove(linked_list_t *list, size_t i);

size_t linked_list_size(linked_list_t *list);

void linked_list_reverse(linked_list_t *list);

void linked_list_foreach(linked_list_t *list, struct delegate_t consumer);

void linked_list_destroy(linked_list_t *list);

#endif //ISU_LINKED_LIST_H
