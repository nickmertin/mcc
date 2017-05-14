#include "launchers.h"
#include "string.h"

struct handle_exists_state {
    const char *opt;
    bool exists;
};

static void handle_exists(void *data, void *state) {
    struct handle_exists_state *s = (struct handle_exists_state *) state;
    if (s->exists)
        return;
    if (!strcmp(((struct launcher_opt *) data)->name, s->opt))
        s->exists = true;
}

static void handle_get(void *data, void *state) {
    struct launcher_opt *s = (struct launcher_opt *) state;
    if (s->value)
        return;
    struct launcher_opt *cur = (struct launcher_opt *) data;
    if (!strcmp(cur->name, s->name))
        s->value = cur->value;
}

bool get_launcher_opt_exists(linked_list_t list, const char *opt) {
    struct handle_exists_state state = {.opt = opt, .exists = false};
    linked_list_foreach(&list, (struct delegate_t) {.func = &handle_exists, .state = &state});
    return state.exists;
}

const char *get_launcher_opt(linked_list_t list, const char *opt) {
    struct launcher_opt state = {.name = opt, .value = NULL};
    linked_list_foreach(&list, (struct delegate_t) {.func = &handle_get, .state = &state});
    return state.value;
}
