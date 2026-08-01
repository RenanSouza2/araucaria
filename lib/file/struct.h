#ifndef FILE_STRUCT_H
#define FILE_STRUCT_H

#include <stdio.h>

#include "../../mods/macros/struct.h"
#include "../../mods/macros/uint.h" // IWYU pragma: keep

STRUCT(file)
{
    FILE *fp;
    uint64_t amount;
    uint64_t count;
    uint64_t pos;
};

#endif
