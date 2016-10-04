#ifndef ISU_DICTIONARY_H
#define ISU_DICTIONARY_H

#include <stddef.h>

struct dictionary_node_t {
    char *key;
    void *value;
    struct dictionary_node_t *next;
};

typedef struct dictionary_node_t *dictionary_t;

dictionary_t *dictionary_create();

void dictionary_set(dictionary_t *dictionary, const char *key, void *value);

void *dictionary_get(dictionary_t *dictionary, const char *key);

void dictionary_destroy(dictionary_t *dictionary);

#endif //ISU_DICTIONARY_H
