#include <stdio.h>

#include "debug.h"
#include "../../mods/clu/header.h"
#include "../../mods/macros/assert.h" // IWYU pragma: keep

#include "../num/header.h"
#include "../num/internal.h"

#define CLU_MOD_IS_SAFE(MOD)            \
    {                                   \
        CLU_HANDLER_IS_SAFE((MOD).num); \
        CLU_HANDLER_IS_SAFE((MOD).max); \
    }



#ifdef DEBUG

#include "../num/debug.h"

bool mod_num_immed(mod_num_t mod, uint64_t n, ...)
{
    CLU_MOD_IS_SAFE(mod);

    va_list args;
    va_start(args, n);
    num_p num = num_create_variadic(n, &args);
    return num_eq_dbg(mod.num, num);
}

#endif



void mod_num_display(char tag[], mod_num_t mod)
{
    CLU_MOD_IS_SAFE(mod);

    printf("\n%s: (", tag);
    num_display_opts(mod.num, nullptr, false, true);
    printf(") / (");
    num_display_opts(mod.max, nullptr, false, true);
    printf(")");
}



mod_num_t mod_num_create(num_p num, num_p p)
{
    CLU_HANDLER_IS_SAFE(num);

    return (mod_num_t)
    {
        .num = num_mod(num, num_copy(p)),
        .max = p
    };
}

mod_num_t mod_num_wrap(uint64_t value, num_p P)
{
    num_p num = num_wrap(value);
    return mod_num_create(num, P);
}

mod_num_t mod_num_copy(mod_num_t mod) // TODO test
{
    CLU_MOD_IS_SAFE(mod);

    num_p num = num_copy(mod.num);
    return mod_num_create(num, mod.max);
}

void mod_num_free(mod_num_t mod)
{
    CLU_MOD_IS_SAFE(mod);

    num_free(mod.num);
}

static mod_num_t mod_num_adjust(mod_num_t mod) // TODO test
{
    CLU_MOD_IS_SAFE(mod);

    mod.num = num_mod(mod.num, num_copy(mod.max));
    return mod;
}



bool mod_num_cmp(mod_num_t mod_1, mod_num_t mod_2) // TODO test
{
    CLU_MOD_IS_SAFE(mod_1);
    CLU_MOD_IS_SAFE(mod_2);

    return num_cmp(mod_1.num, mod_2.num) == 0;
}



mod_num_t mod_num_add(mod_num_t mod_1, mod_num_t mod_2)
{
    CLU_MOD_IS_SAFE(mod_1);
    CLU_MOD_IS_SAFE(mod_2);

    mod_1.num = num_add(mod_1.num, mod_2.num);
    return mod_num_adjust(mod_1);
}

mod_num_t mod_num_sub(mod_num_t mod_1, mod_num_t mod_2)
{
    CLU_MOD_IS_SAFE(mod_1);
    CLU_MOD_IS_SAFE(mod_2);

    if(num_cmp(mod_1.num, mod_2.num) < 0)
        mod_1.num = num_add(mod_1.num, num_copy(mod_1.max));

    mod_1.num = num_sub(mod_1.num, mod_2.num);
    return mod_1;
}

mod_num_t mod_num_mul(mod_num_t mod_1, mod_num_t mod_2)
{
    CLU_MOD_IS_SAFE(mod_1);
    CLU_MOD_IS_SAFE(mod_2);

    mod_1.num = num_mul(mod_1.num, mod_2.num);
    return mod_num_adjust(mod_1);
}

mod_num_t mod_num_sqr(mod_num_t mod)
{
    CLU_MOD_IS_SAFE(mod);

    mod.num = num_sqr(mod.num);
    return mod_num_adjust(mod);
}

mod_num_t mod_num_pow(mod_num_t mod, uint64_t value) // TODO test
{
    CLU_MOD_IS_SAFE(mod);

    if(num_is_zero(mod.num))
    {
        assert(value);
        return mod;
    }

    mod_num_t mod_res = mod_num_wrap(1, mod.max);
    for(uint64_t mask = 0x8000000000000000; mask; mask >>= 1)
    {
        mod_res = mod_num_sqr(mod_res);
        if(value & mask)
        {
            mod_res = mod_num_mul(mod_res, mod_num_copy(mod));
        }
    }
    mod_num_free(mod);
    return mod_res;
}

mod_num_t mod_num_div(mod_num_t mod_1, mod_num_t mod_2)
{
    assert(!num_is_zero(mod_2.num));

    num_p num_q, num_r;
    num_div_mod(&num_q, &num_r, num_copy(mod_1.num), num_copy(mod_2.num));
    if(num_is_zero(num_r))
    {
        num_free(num_r);

        num_p num = mod_1.max;
        mod_num_free(mod_1);
        mod_num_free(mod_2);
        return mod_num_create(num_q, num);
    }
    num_free(num_q);

    mod_num_t mod_1_next = mod_num_create(num_r, mod_2.num);

    num_p num_2 = num_mod(num_copy(mod_1.max), num_copy(mod_2.num));
    num_2 = num_sub(num_copy(mod_2.num), num_2);

    mod_num_t mod_2_next = mod_num_create(num_2, mod_2.num);

    mod_num_t mod_n = mod_num_div(mod_1_next, mod_2_next);

    num_p num = num_mul(mod_n.num, num_copy(mod_1.max));
    num = num_add(num, mod_1.num);
    num = num_div(num, mod_2.num);

    num_p num_cut = mod_1.max;
    return mod_num_create(num, num_cut);
}
