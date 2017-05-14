#ifndef ISU_GRAPHER_H
#define ISU_GRAPHER_H

#include "parser.h"
#include "../../generator/code_graph.h"

struct cg_file_graph *graph(struct cre_parsed_file *file);

#endif //ISU_GRAPHER_H
