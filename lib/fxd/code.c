#include <stdio.h>

#include "debug.h"
#include "../../mods/clu/header.h"

#include "../../mods/macros/assert.h" // IWYU pragma: keep
#include "../../mods/macros/uint.h"

#include "../file/header.h"
#include "../num/header.h"
#include "../num/internal.h"
#include "../num/struct.h"
#include "../sig/header.h"
#include "../sig/internal.h"

#define CLU_FXD_IS_SAFE(FXD) CLU_HANDLER_IS_SAFE((FXD).sig.num)



#ifdef DEBUG

#include "../num/debug.h"
#include "../sig/debug.h"



static fxd_num_t fxd_num_create_variadic(
    uint64_t pos,
    uint64_t signal,
    uint64_t n,
    va_list *args
)
{
    sig_num_t sig = sig_num_create_variadic(signal, n, args);

    assert(pos > 0);
    return (fxd_num_t)
    {
        .pos = pos,
        .sig = sig
    };
}

fxd_num_t fxd_num_create_immed(
    uint64_t pos,
    uint64_t signal,
    uint64_t n,
    ...
)
{
    va_list args;
    va_start(args, n);
    return fxd_num_create_variadic(pos, signal, n, &args);
}



static bool fxd_num_keep(fxd_num_t fxd_1, fxd_num_t fxd_2)
{
    CLU_FXD_IS_SAFE(fxd_1);
    CLU_FXD_IS_SAFE(fxd_2);
    assert(fxd_1.pos == fxd_2.pos);

    if(!uint64(fxd_1.pos, fxd_2.pos))
    {
        printf("\n\tFXD NUM ASSERT ERROR\t| MISMATCH POS");
        return false;
    }

    if(!sig_num_keep(fxd_1.sig, fxd_2.sig))
    {
        printf("\n\tFXD NUM ASSERT ERROR\t| MISMATCH SIG NUM");
        return false;
    }

    return true;
}

bool fxd_num_eq_dbg(fxd_num_t fxd_1, fxd_num_t fxd_2)
{
    CLU_FXD_IS_SAFE(fxd_1);
    CLU_FXD_IS_SAFE(fxd_2);

    if(!fxd_num_keep(fxd_1, fxd_2))
    {
        printf("\n");
        printf("\n");
        fxd_num_display(fxd_1);
        printf("\n");
        fxd_num_display(fxd_2);
        return false;
    }

    fxd_num_free(fxd_1);
    fxd_num_free(fxd_2);
    return true;
}

bool fxd_num_immed(
    fxd_num_t fxd,
    uint64_t pos,
    uint64_t signal,
    uint64_t n,
    ...
)
{
    CLU_FXD_IS_SAFE(fxd);

    va_list args;
    va_start(args, n);
    fxd_num_t fxd_2 = fxd_num_create_variadic(pos, signal, n, &args);
    return fxd_num_eq_dbg(fxd, fxd_2);
}

#endif


static void fxd_num_display_dec_core(fxd_num_t fxd, uint64_t threads)
{
    printf("%c ", fxd.sig.signal == NEGATIVE ? '-' : '+');

    num_p num_hi, num_lo;
    num_break(&num_hi, &num_lo, num_copy(fxd.sig.num), fxd.pos);

    num_display_dec_threads(num_hi, threads);
    num_free(num_hi);

    printf(".");

    if(num_lo->count == 0)
    {
        printf("0");
        num_free(num_lo);
        return;
    }

    uint64_t t = 0;
    num_p num_u = num_wrap(1);
    constexpr uint64_t FXD_DEC_BASE = 1'000'000'000'000'000'000ULL;
    for(uint64_t pos = fxd.pos; num_u->count < pos + 1; t++)
    {
        num_u = num_mul_uint(num_u, FXD_DEC_BASE);

        if(num_u->count > 2)
        {
            num_head_trim(num_u, 1);
            pos--;
        }
    }
    num_free(num_u);

    num_p num = num_pow_threads(num_wrap(FXD_DEC_BASE), t, threads);
    num_lo = num_mul_threads(num_lo, num, threads);
    num_break(&num_lo, &num, num_lo, fxd.pos);
    num_free(num);

    num_lo = num_base_to_threads(num_lo, FXD_DEC_BASE, threads);
    for(uint64_t i=t-1; i!=num_lo->count-1; i--)
    {
        printf("" U64P(018) "", (uint64_t)0);
    }

    for(uint64_t i=num_lo->count-1; i!=UINT64_MAX; i--)
    {
        printf("" U64P(018) "", num_lo->chunk[i]);
    }

    num_free(num_lo);
}

void fxd_num_display_dec(fxd_num_t fxd)
{
    CLU_FXD_IS_SAFE(fxd)

    fxd_num_display_dec_core(fxd, 1);
}

// Same as fxd_num_display_dec, but the caller picks how many threads the
// integer-part display and fractional-part conversion may each fan out across.
void fxd_num_display_dec_threads(fxd_num_t fxd, uint64_t threads)
{
    CLU_FXD_IS_SAFE(fxd)

    fxd_num_display_dec_core(fxd, threads);
}

void fxd_num_display(fxd_num_t fxd)
{
    CLU_FXD_IS_SAFE(fxd);

    printf("" U64P() "\t", fxd.pos);
    sig_num_display(fxd.sig, false);
}

void fxd_num_display_tag(const char tag[], fxd_num_t fxd)
{
    CLU_FXD_IS_SAFE(fxd);

    printf("\n%s:\t", tag);
    fxd_num_display_full(fxd);
}

void fxd_num_display_full(fxd_num_t fxd)
{
    CLU_FXD_IS_SAFE(fxd);

    sig_num_display_full(fxd.sig);
}



fxd_num_t fxd_num_create(sig_num_t sig, uint64_t pos) // TODO test
{
    CLU_HANDLER_IS_SAFE(sig.num);

    return (fxd_num_t)
    {
        .sig = sig,
        .pos = pos
    };
}

fxd_num_t fxd_num_wrap(int64_t value, uint64_t pos) // TODO test
{
    sig_num_t sig = sig_num_wrap(value);
    sig.num = num_head_grow(sig.num, pos);
    return fxd_num_create(sig, pos);
}

fxd_num_t fxd_num_wrap_sig(sig_num_t sig, uint64_t pos) // TODO test
{
    CLU_HANDLER_IS_SAFE(sig.num);

    sig = sig_num_head_grow(sig, pos);
    return fxd_num_create(sig, pos);
}

fxd_num_t fxd_num_copy(fxd_num_t fxd) // TODO test
{
    CLU_FXD_IS_SAFE(fxd);

    sig_num_t sig = sig_num_copy(fxd.sig);
    return fxd_num_create(sig, fxd.pos);
}

void fxd_num_free(fxd_num_t fxd)
{
    CLU_FXD_IS_SAFE(fxd);

    sig_num_free(fxd.sig);
}



fxd_num_t fxd_num_reposition(fxd_num_t fxd, uint64_t pos) // TODO test
{
    CLU_FXD_IS_SAFE(fxd);

    if(sig_num_is_zero(fxd.sig))
    {
        return (fxd_num_t)
        {
            .sig = fxd.sig,
            .pos = pos
        };
    }

    if(pos > fxd.pos)
    {
        return (fxd_num_t)
        {
            .sig = sig_num_head_grow(fxd.sig, pos - fxd.pos),
            .pos = pos
        };
    }

    return (fxd_num_t)
    {
        .sig = sig_num_head_trim(fxd.sig, fxd.pos - pos),
        .pos = pos
    };
}



fxd_num_t fxd_num_base_to(fxd_num_t fxd, uint64_t base) // TODO test
{
    CLU_FXD_IS_SAFE(fxd);

    num_p num_hi, num_lo;
    num_break(&num_hi, &num_lo, fxd.sig.num, fxd.pos);

    num_hi = num_base_to(num_hi, base);

    uint64_t pos = fxd.pos;
    fxd.pos = 0;
    num_p num_u = num_wrap(1);
    while(true)
    {
        num_lo = num_mul(num_lo, num_wrap(base));
        num_u = num_mul(num_u, num_wrap(base));

        if(num_u->count > pos)
        {
            break;
        }

        num_p num_aux;
        num_break(&num_aux, &num_lo, num_lo, pos);
        num_hi = num_head_grow(num_hi, 1);
        num_hi->chunk[0] = num_unwrap(num_aux);
        fxd.pos++;
    }

    fxd.sig.num = num_hi;
    return fxd;
}



int64_t fxd_num_cmp(fxd_num_t fxd_1, fxd_num_t fxd_2) // TODO test
{
    CLU_FXD_IS_SAFE(fxd_1);
    CLU_FXD_IS_SAFE(fxd_2);
    assert(fxd_1.pos == fxd_2.pos);

    return sig_num_cmp(fxd_1.sig, fxd_2.sig);
}

bool fxd_num_is_zero(fxd_num_t fxd)
{
    CLU_FXD_IS_SAFE(fxd);

    return sig_num_is_zero(fxd.sig);
}



fxd_num_t fxd_num_shl(fxd_num_t fxd, uint64_t value) // TODO test
{
    CLU_FXD_IS_SAFE(fxd);

    fxd.sig = sig_num_shl(fxd.sig, value);
    return fxd;
}

fxd_num_t fxd_num_shr(fxd_num_t fxd, uint64_t value) // TODO test
{
    CLU_FXD_IS_SAFE(fxd);

    fxd.sig = sig_num_shr(fxd.sig, value);
    return fxd;
}



fxd_num_t fxd_num_add(fxd_num_t fxd_1, fxd_num_t fxd_2) // TODO test
{
    CLU_FXD_IS_SAFE(fxd_1);
    CLU_FXD_IS_SAFE(fxd_2);
    assert(fxd_1.pos == fxd_2.pos);

    fxd_1.sig = sig_num_add(fxd_1.sig, fxd_2.sig);
    return fxd_1;
}

fxd_num_t fxd_num_sub(fxd_num_t fxd_1, fxd_num_t fxd_2) // TODO test
{
    CLU_FXD_IS_SAFE(fxd_1);
    CLU_FXD_IS_SAFE(fxd_2);
    assert(fxd_1.pos == fxd_2.pos);

    fxd_1.sig = sig_num_sub(fxd_1.sig, fxd_2.sig);
    return fxd_1;
}

fxd_num_t fxd_num_mul(fxd_num_t fxd_1, fxd_num_t fxd_2) // TODO test
{
    return fxd_num_mul_threads(fxd_1, fxd_2, 1);
}

fxd_num_t fxd_num_mul_threads(fxd_num_t fxd_1, fxd_num_t fxd_2, uint64_t threads)
{
    CLU_FXD_IS_SAFE(fxd_1);
    CLU_FXD_IS_SAFE(fxd_2);
    assert(fxd_1.pos == fxd_2.pos);

    fxd_1.sig = sig_num_mul_threads(fxd_1.sig, fxd_2.sig, threads);
    fxd_1.sig = sig_num_head_trim(fxd_1.sig, fxd_1.pos);
    return fxd_1;
}

fxd_num_t fxd_num_sqr(fxd_num_t fxd) // TODO test
{
    CLU_FXD_IS_SAFE(fxd);

    fxd.sig = sig_num_sqr(fxd.sig);
    fxd.sig = sig_num_head_trim(fxd.sig, fxd.pos);
    return fxd;
}

fxd_num_t fxd_num_div(fxd_num_t fxd_1, fxd_num_t fxd_2) // TODO test
{
    return fxd_num_div_threads(fxd_1, fxd_2, 1);
}

fxd_num_t fxd_num_div_threads(fxd_num_t fxd_1, fxd_num_t fxd_2, uint64_t threads)
{
    CLU_FXD_IS_SAFE(fxd_1);
    CLU_FXD_IS_SAFE(fxd_2);
    assert(fxd_1.pos == fxd_2.pos);

    sig_num_t sig = sig_num_head_grow(fxd_1.sig, fxd_1.pos);
    fxd_1.sig = sig_num_div_threads(sig, fxd_2.sig, threads);
    return fxd_1;
}



fxd_num_t fxd_num_mul_sig(fxd_num_t fxd, sig_num_t sig) // TODO test
{
    return fxd_num_mul_sig_threads(fxd, sig, 1);
}

fxd_num_t fxd_num_mul_sig_threads(fxd_num_t fxd, sig_num_t sig, uint64_t threads)
{
    CLU_FXD_IS_SAFE(fxd);
    CLU_HANDLER_IS_SAFE(sig.num);

    fxd.sig = sig_num_mul_threads(fxd.sig, sig, threads);
    return fxd;
}

fxd_num_t fxd_num_div_sig(fxd_num_t fxd, sig_num_t sig) // TODO test
{
    return fxd_num_div_sig_threads(fxd, sig, 1);
}

fxd_num_t fxd_num_div_sig_threads(fxd_num_t fxd, sig_num_t sig, uint64_t threads)
{
    CLU_FXD_IS_SAFE(fxd);
    CLU_HANDLER_IS_SAFE(sig.num);

    fxd.sig = sig_num_div_threads(fxd.sig, sig, threads);
    return fxd;
}



void file_write_fxd_num_raw(file_p fp, fxd_num_t fxd)
{
    file_write_uint64(fp, fxd.pos);
    file_write_sig_num_raw(fp, fxd.sig);
}

void file_write_fxd_num(file_p fp, fxd_num_t fxd)
{
    file_write_start(fp);
    file_write_fxd_num_raw(fp, fxd);
    file_write_end(fp);
}

void fxd_num_save(const char file_path[], fxd_num_t fxd)
{
    file_t fp = file_write_open(file_path, 1);
    file_write_fxd_num(&fp, fxd);
    file_write_close(&fp);
}

fxd_num_t file_read_fxd_num_raw(FILE *fp)
{
    uint64_t pos = file_read_uint64(fp);
    sig_num_t sig = file_read_sig_num_raw(fp);
    return fxd_num_create(sig, pos);
}

fxd_num_t file_read_fxd_num(FILE *fp, uint64_t index)
{
    file_read_move_to_index(fp, index);
    return file_read_fxd_num_raw(fp);
}

fxd_num_t fxd_num_load(const char file_path[])
{
    FILE *fp = file_read_open(file_path);
    assert(fp);
    fxd_num_t fxd = file_read_fxd_num(fp, 0);
    fclose(fp);

    return fxd;
}
