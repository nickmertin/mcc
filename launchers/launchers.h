#ifndef ISU_LAUNCHERS_H
#define ISU_LAUNCHERS_H

#include <stdbool.h>
#include "../util/linked_list.h"

struct launcher_opt {
    const char *name;
    const char *value;
};

extern const char *cli_options[];
extern const char *cli_short_options;

int mcc_launch(linked_list_t opts, linked_list_t args);

bool get_launcher_opt_exists(linked_list_t list, const char *opt);
const char *get_launcher_opt(linked_list_t list, const char *opt);

#endif //ISU_LAUNCHERS_H
