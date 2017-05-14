#include <stdio.h>
#include <getopt.h>
#include <malloc.h>
#include <strings.h>
#include <string.h>
#include "launchers/launchers.h"
#include "util/misc.h"

int main(int argc, char **argv) {
    linked_list_t *opts = linked_list_create(), *args = linked_list_create();
    const char **options = cli_options;
    size_t opt_count = 0;
    while (*options) {
        ++options;
        ++opt_count;
    }
    if (opt_count)
    {
        struct option *option_specs = malloc(sizeof(struct option) * (opt_count + 1));
        for (int i = 0; i < opt_count; ++i)
            option_specs[i] = (struct option) { .name = cli_options[i], .has_arg = optional_argument, .flag = NULL, .val = 0 };
        bzero(&option_specs[opt_count], sizeof(struct option));
        int opt, s_opt;
        while ((s_opt = getopt_long_only(argc, argv, cli_short_options, option_specs, &opt)) != -1) {
            struct launcher_opt opt_result;
            if (s_opt) {
                char name[] = { (char) s_opt, 0 };
                opt_result = (struct launcher_opt) { .name = strdup(name), .value = optarg };
            }
            else
                opt_result = (struct launcher_opt) { .name = cli_options[opt], .value = optarg };
            linked_list_insert(opts, 0, &opt_result, sizeof(struct launcher_opt));
        }
    }
    for (int i = optind; i < argc; ++i)
        linked_list_insert(args, 0, argv[i], strlen(argv[i]) + 1);
    linked_list_reverse(args);
    return mcc_launch(*opts, *args);
}
