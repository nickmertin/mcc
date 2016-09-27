#ifndef ISU_CONTROLLER_H
#define ISU_CONTROLLER_H

#include <stdint.h>

enum pragma_type_t : uint8_t {
    PRAGMA_NONE,
};

struct pragma_t {
    enum pragma_type_t type;
};

typedef void(*add_pragma_t)(struct pragma_t *);

char *read_file(const char *path, add_pragma_t adder);

#endif //ISU_CONTROLLER_H
