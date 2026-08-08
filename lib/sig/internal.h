#ifndef SIG_INTERNAL_H
#define SIG_INTERNAL_H

// Cross-lib production surface: functions sibling lib/* implementations
// (fxd, flt) are allowed to call, but that are not part of sig's public
// API (header.h). Only lib/*/code.c may include another lib's
// internal.h - main.c and mods/ should never need it.

#include "header.h"

sig_num_t sig_num_head_grow(sig_num_t sig, uint64_t count);
sig_num_t sig_num_head_trim(sig_num_t sig, uint64_t count);

#endif
