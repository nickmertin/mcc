#ifndef ISU_GENERATOR_H
#define ISU_GENERATOR_H

#include <stdio.h>
#include "../code_graph.h"

extern bool enable_comments;

void generate_code(struct cg_file_graph *graph, const char *filename, FILE *out);

#endif //ISU_GENERATOR_H
