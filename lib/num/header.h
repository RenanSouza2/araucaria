#ifndef NUM_H
#define NUM_H

#include "struct.h"
#include "../../mods/macros/struct.h"
#include "../../mods/macros/uint.h"
#include "../../mods/clu/header.h"

PLACEHOLDER(num_config)
PLACEHOLDER(num)

void araucaria_disk_config_set(araucaria_disk_config_p config);
bool araucaria_disk_config_is_set();

void num_display_dec(num_p num);
void num_display(num_p num);
void num_display_tag(const char tag[], num_p num);
void num_display_full(const char tag[], num_p num);

num_p num_realloc_disk(CLU_PARAMS(num_p num));
void num_display_opts(num_p num, const char tag[], bool length, bool full);

num_p num_wrap(uint64_t value);
num_p num_wrap_uint128(uint128_t value);
num_p num_wrap_str(const char str[]);
num_p num_read_dec(const char file_name[]);
uint64_t num_unwrap(num_p num);
num_p num_copy(num_p num);
void num_free(num_p num);

bool num_is_zero(num_p num);
int64_t num_cmp(num_p num_1, num_p num_2);

num_p num_shl(num_p num, uint64_t bits);
num_p num_shr(num_p num, uint64_t bits);
num_p num_add_uint(num_p num, uint64_t value);
num_p num_sub_uint(num_p num, uint64_t value);
num_p num_mul_uint(num_p num, uint64_t value);
num_p num_div_mod_uint(num_p num, uint64_t value);

num_p num_add(num_p num_1, num_p num_2);
num_p num_sub(num_p num_1, num_p num_2);
num_p num_mul(num_p num_1, num_p num_2);
void num_mul_estimate_memory(
    uint64_t count_1,
    uint64_t count_2,
    uint64_t disk_threshold,
    uint64_t *out_max_bytes,
    uint64_t *out_avg_bytes
);
num_p num_pow(num_p num, uint64_t value);
num_p num_sqr(num_p num);
void num_div_mod(
    num_p *out_num_q,
    num_p *out_num_r,
    num_p num_1,
    num_p num_2
);
num_p num_div(num_p num_1, num_p num_2);
num_p num_mod(num_p num_1, num_p num_2);
num_p num_gcd(num_p num_1, num_p num_2);

num_p num_base_to(num_p num, uint64_t base);
num_p num_base_from(num_p num, uint64_t base);

#endif
