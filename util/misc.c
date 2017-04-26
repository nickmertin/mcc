#include <stdlib.h>
#include <memory.h>
#include "misc.h"

struct copy_to_array_internal_state_t {
    char *array;
    size_t step;
};

static void copy_to_array_internal(void *data, struct copy_to_array_internal_state_t *state) {
    memcpy(state->array, data, state->step);
    state->array += state->step;
}

char *strdup(const char *string) {
    if (!string)
        return NULL;
    char *r = malloc(strlen(string) + 1);
    strcpy(r, string);
    return r;
}

char *strrange(const char *string, size_t start, size_t length) {
    if (!string)
        return NULL;
    char *r = malloc(length + 1);
    memcpy(r, string + start, min(length, strlen(string) - start));
    r[length] = 0;
    return r;
}

void setFlag(void *data, size_t flag, bool value) {
    char *f = (char *) data + flag / 8;
    *f &= ~(1 << (flag % 8));
    if (value)
        *f |= 1 << (flag % 8);
}

bool getFlag(void *data, size_t flag) {
    return (bool) (((char *) data)[flag / 8] & (1 << (flag % 8)));
}

void copy_to_array(linked_list_t *list, void *array, size_t step) {
    struct copy_to_array_internal_state_t state = { .array = array, .step = step };
    linked_list_foreach(list, (struct delegate_t) { .func = (void (*)(void *, void *)) &copy_to_array_internal, .state = &state });
}
