#ifndef SIG_DEBUG_H
#define SIG_DEBUG_H

#include "../../mods/macros/specifiers.h"

#include "header.h" // IWYU pragma: keep

#ifdef DEBUG

#include <stdarg.h>

STATIC sig_num_t sig_num_create_variadic(uint64_t signal, uint64_t n, va_list *args);
STATIC sig_num_t sig_num_create_immed(uint64_t signal, uint64_t n, ...);
STATIC sig_num_t sig_num_create_rand(uint64_t count);

STATIC bool sig_num_keep(sig_num_t sig_1, sig_num_t sig_2);
STATIC bool sig_num_eq_dbg(sig_num_t sig_1, sig_num_t sig_2);
STATIC bool sig_num_immed(sig_num_t sig, uint64_t signal, uint64_t n, ...);

#endif

#endif
