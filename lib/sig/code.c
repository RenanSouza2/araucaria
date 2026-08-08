#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "../../mods/clu/header.h"
#include "../../mods/macros/assert.h" // IWYU pragma: keep

#include "../file/header.h"
#include "../num/header.h"
#include "../num/internal.h"
#include "../num/struct.h"

#define CLU_SIG_IS_SAFE(SIG) CLU_HANDLER_IS_SAFE((SIG).num);



#ifdef DEBUG

#include "../num/debug.h"

sig_num_t sig_num_create_variadic(uint64_t signal, uint64_t n, va_list *args)
{
    num_p num = num_create_variadic(n, args);
    return sig_num_create(signal, num);
}

sig_num_t sig_num_create_immed(uint64_t signal, uint64_t n, ...)
{
    va_list args;
    va_start(args, n);
    return sig_num_create_variadic(signal, n, &args);
}

sig_num_t sig_num_create_rand(uint64_t count)
{
    num_p num = num_create_rand(count);
    uint64_t signal = rand_64() & 1 ? POSITIVE : NEGATIVE;
    return sig_num_create(signal, num);
}



bool sig_num_keep(sig_num_t sig_1, sig_num_t sig_2)
{
    CLU_SIG_IS_SAFE(sig_1);
    CLU_SIG_IS_SAFE(sig_2);

    if(!uint64(sig_1.signal, sig_2.signal))
    {
        printf("\n\tSIG ASSERT ERROR\t| DIFFERENT SIGNAL");
        return false;
    }

    if(!num_keep(sig_1.num, sig_2.num))
    {
        printf("\n\tSIG ASSERT ERROR\t| DIFFERENT NUMBER");
        return false;
    }

    return true;
}

bool sig_num_eq_dbg(sig_num_t sig_1, sig_num_t sig_2)
{
    CLU_SIG_IS_SAFE(sig_1);
    CLU_SIG_IS_SAFE(sig_2);

    if(!sig_num_keep(sig_1, sig_2))
    {
        printf("\n");
        sig_num_display_tag("\tsig_1", sig_1);
        sig_num_display_tag("\tsig_2", sig_2);
        return false;
    }

    sig_num_free(sig_1);
    sig_num_free(sig_2);
    return true;
}

bool sig_num_immed(sig_num_t sig, uint64_t signal, uint64_t n, ...)
{
    CLU_SIG_IS_SAFE(sig);

    va_list args;
    va_start(args, n);
    sig_num_t sig_2 = sig_num_create_variadic(signal, n, &args);

    return sig_num_eq_dbg(sig, sig_2);
}

#endif



void sig_num_display(sig_num_t sig, bool full)
{
    CLU_SIG_IS_SAFE(sig);
    assert(sig.num);

    if(sig.signal == ZERO)
    {
        printf("  0");
        return;
    }

    printf("%c ", sig.signal & POSITIVE ? '+': '-');

    printf("[");
    num_display_opts(sig.num, nullptr, true, full);
    printf("]");
}

void sig_num_display_tag(const char tag[], sig_num_t sig)
{
    CLU_SIG_IS_SAFE(sig);
    assert(sig.num);

    printf("\n%s:\t", tag);
    sig_num_display_full(sig);
}

void sig_num_display_full(sig_num_t sig)
{
    CLU_SIG_IS_SAFE(sig);
    assert(sig.num);

    sig_num_display(sig, true);
}

void sig_num_display_dec(sig_num_t sig)
{
    CLU_SIG_IS_SAFE(sig);
    assert(sig.num);

    if(sig.signal == ZERO)
    {
        printf("  0");
        return;
    }

    printf("%c ", sig.signal & POSITIVE ? '+': '-');
    num_display_dec(sig.num);
}



sig_num_t sig_num_create(uint64_t signal, num_p num)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    if(num_is_zero(num))
    {
        signal = ZERO;
    }
    else
    {
        assert(signal != ZERO)
    }

    return (sig_num_t)
    {
        .signal = signal,
        .num = num
    };
}

void sig_num_free(sig_num_t sig)
{
    CLU_SIG_IS_SAFE(sig);

    num_free(sig.num);
}



sig_num_t sig_num_wrap(int64_t value)
{
    if(value < 0)
    {
        num_p num = num_wrap((uint64_t)(-(int128_t)value));
        return sig_num_create(NEGATIVE, num);
    }

    num_p num = num_wrap((uint64_t)value);
    return sig_num_create(POSITIVE, num);
}

sig_num_t sig_num_wrap_int128(int128_t value)
{
    if(value == INT128_MIN)
    {
        num_p num = num_wrap_uint128(B128(127));
        return sig_num_create(NEGATIVE, num);
    }

    if(value < 0)
    {
        num_p num = num_wrap_uint128((uint128_t)(-value));
        return sig_num_create(NEGATIVE, num);
    }

    num_p num = num_wrap_uint128((uint128_t)value);
    return sig_num_create(POSITIVE, num);
}

sig_num_t sig_num_wrap_num(num_p num)   // TODO TEST
{
    return sig_num_create(POSITIVE, num);
}

sig_num_t sig_num_wrap_str(const char str[])
{
    uint64_t signal = str[0] == '-' ? NEGATIVE : POSITIVE;
    uint64_t offset = str[0] == '-' || str[0] == '+' ? 1 : 0;
    num_p num = num_wrap_str(&str[offset]);
    return sig_num_create(signal, num);
}

sig_num_t sig_num_copy(sig_num_t sig)
{
    CLU_SIG_IS_SAFE(sig);
    assert(sig.num);

    num_p num = num_copy(sig.num);
    return sig_num_create(sig.signal, num);
}

sig_num_t sig_num_realloc_disk(sig_num_t sig)
{
    CLU_SIG_IS_SAFE(sig);
    assert(sig.num);

    num_p num = num_realloc_disk(CLU_ARGS(sig.num));
    return sig_num_create(sig.signal, num);
}

sig_num_t sig_num_head_grow(sig_num_t sig, uint64_t count) // TODO test
{
    CLU_SIG_IS_SAFE(sig);
    assert(sig.num);

    sig.num = num_head_grow(sig.num, count);
    return sig;
}

sig_num_t sig_num_head_trim(sig_num_t sig, uint64_t count) // TODO test
{
    CLU_SIG_IS_SAFE(sig);
    assert(sig.num);

    num_head_trim(sig.num, count);
    return sig_num_create(sig.signal, sig.num);
}



void file_write_sig_num_raw(file_p fp, sig_num_t sig)
{
    file_write_uint64(fp, sig.signal);
    file_write_uint64(fp, sig.num->count);
    for(uint64_t i=0; i<sig.num->count; i++)
    {
        file_write_uint64(fp, sig.num->chunk[i]);
    }
}

void file_write_sig_num(file_p fp, sig_num_t sig)
{
    file_write_start(fp);
    file_write_sig_num_raw(fp, sig);
    file_write_end(fp);
}

void sig_num_save(const char file_path[], sig_num_t sig)
{
    file_t fp = file_write_open(file_path, 1);
    file_write_sig_num(&fp, sig);
    file_write_close(&fp);
}



sig_num_t file_read_sig_num_raw(FILE *fp)
{
    uint64_t signal = file_read_uint64(fp);
    uint64_t count = file_read_uint64(fp);
    num_p num = num_create(CLU_ARGS(count, count));
    for(uint64_t i=0; i<count; i++)
    {
        num->chunk[i] = file_read_uint64(fp);
    }

    return sig_num_create(signal, num);
}

sig_num_t file_read_sig_num(FILE *fp, uint64_t index)
{
    file_read_move_to_index(fp, index);
    return file_read_sig_num_raw(fp);
}

sig_num_t sig_num_load(char file_path[])
{
    FILE *fp = file_read_open(file_path);
    assert(fp);
    sig_num_t sig = file_read_sig_num(fp, 0);
    fclose(fp);

    return sig;
}



bool sig_num_is_zero(sig_num_t sig)
{
    CLU_SIG_IS_SAFE(sig);
    assert(sig.num);

    return sig.signal == ZERO;
}

int64_t sig_num_cmp(sig_num_t sig_1, sig_num_t sig_2)
{
    CLU_SIG_IS_SAFE(sig_1);
    CLU_SIG_IS_SAFE(sig_2);

    if(sig_1.signal & POSITIVE)
    {
        if(sig_2.signal & POSITIVE)
        {
            return num_cmp(sig_1.num, sig_2.num);
        }

        return 1;
    }

    if(sig_2.signal & POSITIVE)
    {
        return -1;
    }

    return -num_cmp(sig_1.num, sig_2.num);
}



sig_num_t sig_num_shl(sig_num_t sig, uint64_t bits)
{
    CLU_SIG_IS_SAFE(sig);
    assert(sig.num);

    sig.num = num_shl(sig.num, bits);
    return sig;
}

sig_num_t sig_num_shr(sig_num_t sig, uint64_t bits)
{
    CLU_SIG_IS_SAFE(sig);
    assert(sig.num);

    num_p num = num_shr(sig.num, bits);
    return sig_num_create(sig.signal, num);
}



sig_num_t sig_num_opposite(sig_num_t sig)
{
    CLU_SIG_IS_SAFE(sig);

    sig.signal = sig.signal >> 1 | (sig.signal & POSITIVE) << 1;
    return sig;
}

sig_num_t sig_num_add(sig_num_t sig_1, sig_num_t sig_2)
{
    CLU_SIG_IS_SAFE(sig_1);
    CLU_SIG_IS_SAFE(sig_2);

    uint64_t signal_res = sig_1.signal & sig_2.signal;
    if(signal_res)
    {
        num_p num_res = num_add(sig_1.num, sig_2.num);
        return sig_num_create(signal_res, num_res);
    }

    if(num_cmp(sig_1.num, sig_2.num) > 0)
    {
        num_p num_res = num_sub(sig_1.num, sig_2.num);
        signal_res = sig_1.signal;
        return sig_num_create(signal_res, num_res);
    }

    num_p num_res = num_sub(sig_2.num, sig_1.num);
    signal_res = sig_2.signal;
    return sig_num_create(signal_res, num_res);
}

sig_num_t sig_num_sub(sig_num_t sig_1, sig_num_t sig_2)
{
    CLU_SIG_IS_SAFE(sig_1);
    CLU_SIG_IS_SAFE(sig_2);

    sig_2 = sig_num_opposite(sig_2);
    return sig_num_add(sig_1, sig_2);
}

static uint64_t sig_signal_mul(uint64_t signal_1, uint64_t signal_2)
{
    return signal_1 & signal_2 ? POSITIVE : NEGATIVE;
}

sig_num_t sig_num_mul(sig_num_t sig_1, sig_num_t sig_2)
{
    CLU_SIG_IS_SAFE(sig_1);
    CLU_SIG_IS_SAFE(sig_2);

    uint64_t signal_res = sig_signal_mul(sig_1.signal, sig_2.signal);
    num_p num_res = num_mul(sig_1.num, sig_2.num);
    return sig_num_create(signal_res, num_res);
}

sig_num_t sig_num_sqr(sig_num_t sig) // TODO test
{
    CLU_SIG_IS_SAFE(sig);

    num_p num = num_sqr(sig.num);
    return sig_num_create(POSITIVE, num);
}

sig_num_t sig_num_div(sig_num_t sig_1, sig_num_t sig_2)
{
    CLU_SIG_IS_SAFE(sig_1);
    CLU_SIG_IS_SAFE(sig_2);

    uint64_t signal_res = sig_signal_mul(sig_1.signal, sig_2.signal);
    num_p num_res = num_div(sig_1.num, sig_2.num);
    return sig_num_create(signal_res, num_res);
}



sig_num_t sig_num_mul_int(sig_num_t sig, int64_t value)
{
    if(value < 0)
    {
        sig = sig_num_opposite(sig);
        value = -value;
    }

    sig.num = num_mul_uint(sig.num, (uint64_t)value);
    return sig;
}
