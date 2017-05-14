#include <stdlib.h>
#include <memory.h>
#include "misc.h"

struct copy_to_array_internal_state {
    char *array;
    size_t step;
};

struct contains_size_t_internal_state {
    size_t value;
    bool found;
};

static void copy_to_array_internal(void *data, struct copy_to_array_internal_state *state) {
    memcpy(state->array, data, state->step);
    state->array += state->step;
}

static void contains_size_t_internal(size_t *data, struct contains_size_t_internal_state *state) {
    state->found |= *data == state->value;
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
    struct copy_to_array_internal_state state = {.array = array, .step = step};
    linked_list_foreach(list, (struct delegate_t) {.func = (void (*)(void *, void *)) &copy_to_array_internal, .state = &state});
}

bool contains_size_t(linked_list_t *list, size_t value) {
    struct contains_size_t_internal_state state = {.value = value, .found = false};
    linked_list_foreach(list, (struct delegate_t) {.func = (void (*)(void *, void *)) &contains_size_t_internal, .state = &state});
    return state.found;
}
