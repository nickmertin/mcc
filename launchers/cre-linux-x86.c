#include <stdio.h>
#include <errno.h>
#include "../compiler/cre/parser.h"
#include "../generator/code_graph.h"
#include "../compiler/cre/grapher.h"
#include "../generator/linux-x86/generator.h"
#include "../util/io.h"
#include "launchers.h"

const char *cli_options[] = { "pipe", "comment", NULL };
const char *cli_short_options = "o:";

int mcc_launch(linked_list_t opts, linked_list_t args) {
    FILE *in, *out;
    if (get_launcher_opt_exists(opts, "pipe")) {
        in = stdin;
        out = stdout;
    } else {
        if (!args)
            return -1;
        if (!(in = fopen(args->data, "r")))
            return errno;
        const char *path = get_launcher_opt(opts, "o");
        if (!path) {
            fclose(in);
            return -1;
        }
        if (!(out = fopen(path, "w"))) {
            fclose(in);
            return errno;
        }
    }
    enable_comments = get_launcher_opt_exists(opts, "comment");
    generate_code(graph(parse(read_all(in))), get_file_name(in), out);
    if (in != stdin)
        fclose(in);
    if (out != stdout)
        fclose(out);
    return 0;
}
