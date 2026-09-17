#ifndef FLT_DEBUG_H
#define FLT_DEBUG_H

#include <stdbool.h>

#include "../../mods/macros/specifiers.h"

#include "header.h" // IWYU pragma: keep

#ifdef DEBUG

int64_t int_rand(int64_t min, int64_t max);

flt_num_t flt_num_create_immed(
    int64_t exponent,
    uint64_t size,
    uint64_t signal,
    uint64_t n,
    ...
);
flt_num_t flt_num_create_rand(int64_t exponent, uint64_t count);

bool flt_num_eq_dbg(flt_num_t flt_1, flt_num_t flt_2);
bool flt_num_immed(
    flt_num_t flt,
    int64_t exponent,
    uint64_t size,
    uint64_t signal,
    uint64_t n,
    ...
);

#endif

STATIC uint64_t int64_get_sign(int64_t i);
STATIC int64_t int64_add(int64_t a, int64_t b);

STATIC flt_num_t flt_num_normalize(flt_num_t flt);
STATIC flt_num_t flt_num_create(int64_t exponent, uint64_t size_cap, sig_num_t sig);

STATIC flt_num_t flt_num_set_exponent(flt_num_t flt, int64_t exponent);

bool flt_num_is_zero(flt_num_t flt);

#endif
