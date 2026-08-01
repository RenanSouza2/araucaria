#ifndef SIG_STRUCT_H
#define SIG_STRUCT_H

#include "../../mods/macros/struct.h"

#include "../num/struct.h"

#define POSITIVE 1
#define NEGATIVE 2
#define ZERO 3

STRUCT(sig_num)
{
    uint64_t signal;
    num_p num;
};

#endif
