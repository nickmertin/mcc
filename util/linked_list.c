#include <malloc.h>
#include <memory.h>
#include "linked_list.h"

linked_list_t *linked_list_create() {
    linked_list_t *list = malloc(sizeof(linked_list_t));
    *list = NULL;
    return list;
}

bool linked_list_insert(linked_list_t *list, size_t i, void *data, size_t size) {
    if (!list)
        return false;
    struct linked_list_node_t *node = *list, *last = NULL;
    while (node && i) {
        struct linked_list_node_t *c = node;
        node = node->ptr;
        last = c;
        --i;
    }
    if (i)
        return false;
    struct linked_list_node_t *new = malloc(sizeof(struct linked_list_node_t) + size);
    new->ptr = node;
    memcpy(new->data, data, size);
    if (last)
        last->ptr = new;
    else
        *list = new;
    return true;
}

bool linked_list_remove(linked_list_t *list, size_t i) {
    if (!list)
        return false;
    struct linked_list_node_t *node = *list, *last = NULL;
    while (node && i) {
        struct linked_list_node_t *c = node;
        node = node->ptr;
        last = c;
        --i;
    }
    if (i || !node)
        return false;
    struct linked_list_node_t *next = node->ptr;
    free(node);
    if (last)
        last->ptr = next;
    return true;
}

size_t linked_list_size(linked_list_t *list) {
    if (!list)
        return 0;
    struct linked_list_node_t *node = *list;
    size_t count = 0;
    while (node) {
        node = node->ptr;
        ++count;
    }
    return count;
}

void linked_list_reverse(linked_list_t *list) {
    if (!list)
        return;
    struct linked_list_node_t *last = 0, *current = *list;
    while (current) {
        struct linked_list_node_t *next = current->ptr;
        current->ptr = last;
        last = current;
        current = next;
    }
    *list = last;
}

void linked_list_foreach(linked_list_t *list, struct delegate_t consumer) {
    if (!list)
        return;
    struct linked_list_node_t *current = *list;
    while (current) {
        delegate_invoke(consumer, current->data);
        current = current->ptr;
    }
}

void linked_list_destroy(linked_list_t *list) {
    if (!list)
        return;
    struct linked_list_node_t *node = *list;
    while (node) {
        struct linked_list_node_t *c = node;
        node = node->ptr;
        free(c);
    }

}
