#include "delegate.h"

static void wrapper(void *data, void *state) {
    ((void (*)(void *)) state)(data);
}

struct delegate_t delegate_wrap(void (*func)(void *)) {
    return { .func = &wrapper, .state = func };
}

void delegate_invoke(struct delegate_t delegate, void *data) {
    delegate.func(data, delegate.state);
}
