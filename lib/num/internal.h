#ifndef NUM_INTERNAL_H
#define NUM_INTERNAL_H

#include "header.h"
#include "../../mods/clu/header.h"

num_p num_create(CLU_PARAMS(uint64_t size, uint64_t count));
num_p num_create_dirty(CLU_PARAMS(uint64_t size, uint64_t count));
num_p num_realloc_disk(CLU_PARAMS(num_p num));

num_p num_head_grow(num_p num, uint64_t count);
void num_head_trim(num_p num, uint64_t count);
void num_break(num_p *out_num_hi, num_p *out_num_lo, num_p num, uint64_t count);

#endif
