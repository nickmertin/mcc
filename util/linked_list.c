#include <malloc.h>
#include <memory.h>
#include "linked_list.h"
#include "misc.h"

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
        node = (struct linked_list_node_t *) ((uintptr_t) node->ptr ^ (uintptr_t) last);
        last = c;
        --i;
    }
    if (i)
        return false;
    struct linked_list_node_t *new = malloc(sizeof(struct linked_list_node_t) + size);
    new->ptr = (struct linked_list_node_t *) ((uintptr_t) last ^ (uintptr_t) node);
    memcpy(new->data, data, size);
    if (last)
        last->ptr = (struct linked_list_node_t *) ((uintptr_t) last->ptr ^ (uintptr_t) node ^ (uintptr_t) new);
    if (node)
        node->ptr = (struct linked_list_node_t *) ((uintptr_t) node->ptr ^ (uintptr_t) last ^ (uintptr_t) new);
    return true;
}

bool linked_list_remove(linked_list_t *list, size_t i) {
    if (!list)
        return false;
    struct linked_list_node_t *node = *list, *last = NULL;
    while (node && i) {
        struct linked_list_node_t *c = node;
        node = (struct linked_list_node_t *) ((uintptr_t) node->ptr ^ (uintptr_t) last);
        last = c;
        --i;
    }
    if (i || !node)
        return false;
    free(node);
    struct linked_list_node_t *next = (struct linked_list_node_t *) ((uintptr_t) node->ptr ^ (uintptr_t) last);
    if (last)
        last->ptr = (struct linked_list_node_t *) ((uintptr_t) last->ptr ^ (uintptr_t) node ^ (uintptr_t) next);
    if (next)
        next->ptr = (struct linked_list_node_t *) ((uintptr_t) next->ptr ^ (uintptr_t) node ^ (uintptr_t) last);
    return true;
}

size_t linked_list_size(linked_list_t *list) {
    if (!list)
        return 0;
    struct linked_list_node_t *node = *list, *last = NULL;
    size_t count = 0;
    while (node) {
        struct linked_list_node_t *c = node;
        node = (struct linked_list_node_t *) ((uintptr_t) node->ptr ^ (uintptr_t) last);
        last = c;
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
    (*list)->ptr = last;
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
    struct linked_list_node_t *node = *list, *last = NULL;
    while (node) {
        struct linked_list_node_t *c = node;
        node = (struct linked_list_node_t *) ((uintptr_t) node->ptr ^ (uintptr_t) last);
        last = c;
        free(c);
    }

}
