#ifndef ISU_GENERATOR_H
#define ISU_GENERATOR_H

#include <stdio.h>
#include "../code_graph.h"

void generate_code(struct cg_file_graph *graph, FILE *out);

#endif //ISU_GENERATOR_H
