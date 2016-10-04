#include <stdlib.h>
#include <memory.h>
#include "dictionary.h"
#include "misc.h"

dictionary_t *dictionary_create() {
    dictionary_t *dictionary = malloc(sizeof(dictionary_t));
    *dictionary = NULL;
    return dictionary;
}

void dictionary_set(dictionary_t *dictionary, const char *key, void *value) {
    while (*dictionary) {
        if (!strcmp((*dictionary)->key, key)) {
            free((*dictionary)->value);
            (*dictionary)->value = strdup(value);
            return;
        }
        dictionary = &(*dictionary)->next;
    }
    struct dictionary_node_t *n = malloc(sizeof(struct dictionary_node_t));
    n->key = strdup(key);
    n->value = strdup(value);
    n->next = NULL;
    (*dictionary)->next = n;
}

void *dictionary_get(dictionary_t *dictionary, const char *key) {
    while (*dictionary) {
        if (!strcmp((*dictionary)->key, key))
            return (*dictionary)->value;
        dictionary = &(*dictionary)->next;
    }
    return NULL;
}

void dictionary_destroy(dictionary_t *dictionary) {
    dictionary_t  c = *dictionary;
    while (c) {
        dictionary_t  n = c->next;
        free(c);
        c = n;
    }
    free(dictionary);
}
