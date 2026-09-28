#ifndef FXD_INTERNAL_H
#define FXD_INTERNAL_H

#include <stdbool.h>
#include <stdio.h>

#include "header.h"

void fxd_num_display_dec_core(FILE *fp, fxd_num_t fxd, uint64_t threads, bool bare);

#endif
