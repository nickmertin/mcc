#include <stdio.h>
#include "../compiler/cre/parser.h"
#include "../generator/code_graph.h"
#include "../compiler/cre/grapher.h"
#include "../generator/linux-x86/generator.h"

int main(int argc, const char **argv) {
    char buffer[256];
    printf("Path: ");
    fgets(buffer, 256, stdin);
    buffer[strlen(buffer) - 1] = 0;
    FILE *file = fopen(buffer, "r");
    char contents[10240];
    size_t len = fread(contents, 1, 10240, file);
    contents[len] = 0;
    fclose(file);
    generate_code(graph(parse(contents)), stdout);
}
