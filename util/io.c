#include "io.h"
#include <malloc.h>
#include <string.h>
#include "linked_list.h"

struct handle_read_state {
    char *contents;
    size_t last_size;
    size_t count;
};

static void handle_read(void *data, void *state) {
    struct handle_read_state *s = state;
    memcpy(s->contents, data, --s->count || !s->last_size ? 1024 : s->last_size);
    s->contents += 1024;
}

char *read_all(FILE *file) {
    char buffer[1024];
    size_t last_size, count = 0;
    linked_list_t *blocks = linked_list_create();
    do
        if ((last_size = fread(buffer, 1, 1024, file))) {
            linked_list_insert(blocks, 0, buffer, last_size);
            ++count;
        }
    while (last_size == 1024);
    linked_list_reverse(blocks);
    size_t size = 1024 * count;
    if (last_size)
        size -= 1024 - last_size;
    char *contents = malloc(size + 1);
    struct handle_read_state state = { .contents = contents, .last_size = last_size, .count = count };
    linked_list_foreach(blocks, (struct delegate_t) { .func = &handle_read, .state = &state });
    contents[size] = 0;
    linked_list_destroy(blocks);
    return contents;
}
