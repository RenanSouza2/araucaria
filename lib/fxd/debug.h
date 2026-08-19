#ifndef FXD_DEBUG_H
#define FXD_DEBUG_H

#include "../../mods/macros/specifiers.h"

#include "header.h" // IWYU pragma: keep

#ifdef DEBUG

fxd_num_t fxd_num_create_immed(
    uint64_t pos,
    uint64_t signal,
    uint64_t n,
    ...
);

// Exposed like sig_num_eq_dbg and flt_num_eq_dbg, which the sibling suites already
// use directly: fxd's test needs to compare two computed values against each other,
// not just against a literal, which is what fxd_num_immed covers.
bool fxd_num_eq_dbg(fxd_num_t fxd_1, fxd_num_t fxd_2);
bool fxd_num_immed(
    fxd_num_t fxd,
    uint64_t pos,
    uint64_t signal,
    uint64_t n,
    ...
);

#endif

STATIC fxd_num_t fxd_num_create(sig_num_t sig, uint64_t pos);

#endif
