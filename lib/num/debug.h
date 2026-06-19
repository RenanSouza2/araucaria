#ifndef NUM_DEBUG_H
#define NUM_DEBUG_H

#include "../../mods/macros/specifiers.h"

#include "header.h"

#ifdef DEBUG

#include <stdarg.h>

uint64_t rand_64();
uint64_t rand_64_range(uint64_t min, uint64_t max);

num_p num_create_variadic(uint64_t n, va_list *args);
num_p num_create_immed(uint64_t n, ...);
num_p num_create_rand(uint64_t count);

bool int64(int64_t i1, int64_t i2);
bool uint64(uint64_t u1, uint64_t u2);
bool uint128_immed(uint128_t u1, uint64_t v2h, uint64_t v2l);

bool num_keep(num_p num_1, num_p num_2);
bool num_eq_dbg(num_p num_1, num_p num_2);
bool num_immed(num_p num, uint64_t n, ...);

#endif // DEBUG

STATIC uint64_t uint_from_char(char c);

STATIC num_p num_expand_to(num_p num, uint64_t size);

STATIC num_p num_normalize(num_p num);

STATIC num_p num_wrap_dec(const char str[]);
STATIC num_p num_wrap_hex(const char str[]);

STATIC num_p num_shl_core(num_p num, uint64_t bits);
STATIC num_p num_shr_core(num_p num, uint64_t bits);

STATIC num_p num_add_uint_offset(num_p num, uint64_t pos, uint64_t value);
STATIC num_p num_sub_uint_offset(num_p num, uint64_t pos, uint64_t value);

STATIC int64_t num_cmp_offset(num_p num_1, uint64_t pos_1, num_p num_2);
STATIC num_p num_sub_offset(num_p num_1, uint64_t pos_1, num_p num_2);

STATIC void num_display_span_full(const char tag[], num_p num, uint64_t n, uint64_t k);
STATIC uint64_t ssm_bit_inv(uint64_t i, uint64_t K);
STATIC void num_ssm_add_mod_immed(
    num_p num_1, uint64_t pos_1,
    num_p num_2, uint64_t pos_2,
    uint64_t n
);
STATIC void num_ssm_sub_mod(
    num_p num_res, uint64_t pos_res,
    num_p num_1, uint64_t pos_1,
    num_p num_2, uint64_t pos_2,
    uint64_t n
);
STATIC void num_ssm_opposite(num_p num, uint64_t pos, uint64_t n);
STATIC void num_ssm_shl(
    num_p num_res, uint64_t pos_res,
    num_p num, uint64_t pos,
    uint64_t n,
    uint64_t bits
);
STATIC void num_ssm_shr(
    num_p num_res, uint64_t pos_res,
    num_p num, uint64_t pos,
    uint64_t n,
    uint64_t bits
);
STATIC void num_ssm_shl_mod(
    num_p num_aux,
    num_p num,
    uint64_t pos,
    uint64_t n,
    uint64_t bits
);
STATIC void num_ssm_shr_mod(
    num_p num_aux,
    num_p num,
    uint64_t pos,
    uint64_t n,
    uint64_t bits
);
STATIC ssm_params_t ssm_get_params(uint64_t count);
STATIC ssm_params_t ssm_get_params_wrap(uint64_t n);
STATIC void num_ssm_fft_fwd(num_p num_aux, num_p num, ssm_params_p p);
STATIC void num_ssm_fft_inv(num_p num_aux, num_p num, ssm_params_p p);

STATIC void num_ssm_pad_wrap(num_p num_fft, num_p num, uint64_t pos, ssm_params_p p);
STATIC void num_ssm_depad_wrap(
    num_p num_aux_1,
    num_p num_aux_2,
    num_p num_res,
    uint64_t pos,
    num_p num_fft,
    ssm_params_p p
);

STATIC num_p num_ssm_pad_no_wrap(num_p num, ssm_params_p p);
STATIC num_p num_ssm_depad_no_wrap(num_p num_fft, ssm_params_p p);
STATIC void num_ssm_mul_wrap(
    num_p num_aux_1,
    num_p num_aux_2,
    num_p num_fft_1,
    num_p num_fft_2,
    num_p num_1,
    num_p num_2,
    uint64_t pos,
    ssm_params_p p
);

// STATIC num_p num_mul_classic(num_p num_1, num_p num_2);
STATIC num_p num_mul_ssm(num_p num_1, num_p num_2);
STATIC num_p num_mul_core(num_p num_1, num_p num_2);

STATIC num_p num_sqr_classic(num_p num);
STATIC num_p num_sqr_ssm(num_p num);

STATIC uint64_t num_div_normalize(num_p *num_1, num_p *num_2);

#endif // NUM_DEBUG_H
