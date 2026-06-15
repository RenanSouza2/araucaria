#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include "debug.h"
#include "../../mods/macros/assert.h" // IWYU pragma: keep
#include "../../mods/macros/stdbit.h" // IWYU pragma: keep
#include "../../mods/macros/uint.h"
#include "../../mods/clu/header.h"
#include "struct.h"



#ifdef DEBUG

static uint16_t rand_16()
{
    // NOLINTNEXTLINE(cert-msc30-cpp, cert-msc30-c, cert-msc50-cpp)
    return (uint16_t)rand();
}

static uint32_t rand_32()
{
    // NOLINTNEXTLINE(readability-magic-numbers)
    return ((uint32_t)rand_16() << 16) | rand_16();
}

STATIC uint64_t rand_64()
{
    // NOLINTNEXTLINE(readability-magic-numbers)
    return (U64(rand_32()) << 32) | rand_32();
}

STATIC uint64_t rand_64_range(uint64_t min, uint64_t max)
{
    assert(min < max);
    uint64_t range = max - min;
    return (rand_64() % range) + min;
}



STATIC num_p num_create_variadic(uint64_t n, va_list *args)
{
    num_p num = num_create(CLU_ARGS(n, n));
    for(uint64_t i=0; i<n; i++)
    {
        num->chunk[n-1-i] = va_arg(*args, uint64_t);
    }

    return num;
}

STATIC num_p num_create_immed(uint64_t n, ...)
{
    va_list args;
    va_start(args, n);
    return num_create_variadic(n, &args);
}

STATIC num_p num_create_rand(uint64_t count)
{
    num_p num = num_create(CLU_ARGS(count, count));
    for(uint64_t i=0; i<count; i++)
    {
        num->chunk[i] = rand_64();
    }

    if(count)
    {
        while(num->chunk[count - 1] == 0)
        {
            num->chunk[count - 1] = rand_64();
        }
    }

    return num;
}



STATIC bool int64(int64_t i1, int64_t i2)
{
    if(i1 != i2)
    {
        printf("\n\n\tINT64 ASSERT ERROR\t| (" D64P() ") (" D64P() ")", i1, i2);
        return false;
    }

    return true;
}

STATIC bool uint64(uint64_t u1, uint64_t u2)
{
    if(u1 != u2)
    {
        printf("\n\n\tUINT64 ASSERT ERROR\t| (" U64PX ") (" U64PX ")", u1, u2);
        return false;
    }

    return true;
}

STATIC bool uint128_immed(uint128_t u1, uint64_t v2h, uint64_t v2l)
{
    if(!uint64(HIGH(u1), v2h))
    {
        printf("\n\tUINT128 ASSERT ERROR\t| HIGH");
        return false;
    }

    if(!uint64(LOW(u1), v2l))
    {
        printf("\n\tUINT128 ASSERT ERROR\t| LOW");
        return false;
    }

    return true;
}



STATIC bool num_keep(num_p num_1, num_p num_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    if(num_1->cannot_expand)
    {
        printf("\n\n\tNUMBER ASSERT ERROR\t| NUM CANNOT EXPAND");
        return false;
    }

    if(num_1->count > num_1->size)
    {
        printf("\n\n\tNUMBER ASSERT ERROR\t| COUNT BIGGER THAN SIZE | " U64P() " " U64P() "", num_1->count, num_1->size);
        return false;
    }

    if(num_1->chunk == nullptr)
    {
        printf("\n\n\tNUMBER ASSERT ERROR\t| CHUNK IS nullptr");
        return false;
    }

    if(!uint64(num_1->count, num_2->count))
    {
        printf("\n\tNUMBER ASSERT ERROR\t| DIFFERENCE IN LENGTH");
        return false;
    }

    for(uint64_t i=0; i<num_1->count; i++)
    {
        if(!uint64(num_1->chunk[i], num_2->chunk[i]))
        {
            printf("\n\tNUMBER ASSERT ERROR\t| DIFFERENCE IN VALUE | " U64P() "", i);
            return false;
        }
    }

    return true;
}

STATIC bool num_eq_dbg(num_p num_1, num_p num_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    if(!num_keep(num_1, num_2))
    {
        printf("\n");
        num_display_full("\tnum_1", num_1);
        num_display_full("\tnum_2", num_2);
        return false;
    }

    num_free(num_1);
    num_free(num_2);
    return true;
}

STATIC bool num_immed(num_p num, uint64_t n, ...)
{
    va_list args;
    va_start(args, n);
    num_p num_2 = num_create_variadic(n, &args);
    return num_eq_dbg(num, num_2);
}

#endif



STATIC uint64_t uint_from_char(char c)
{
    constexpr uint64_t map[256] = {
        ['0'] = 0, ['1'] = 1, ['2'] = 2, ['3'] = 3, ['4'] = 4,
        ['5'] = 5, ['6'] = 6, ['7'] = 7, ['8'] = 8, ['9'] = 9,
        ['a'] = 0xa, ['b'] = 0xb, ['c'] = 0xc,
        ['d'] = 0xd, ['e'] = 0xe, ['f'] = 0xf,
        ['A'] = 0xa, ['B'] = 0xb, ['C'] = 0xc,
        ['D'] = 0xd, ['E'] = 0xe, ['F'] = 0xf,
    };
    uint64_t res = map[(uint64_t)(unsigned char)c];
    assert(c == '0' || res != 0)
    return res;
}

static uint64_t uint_from_str(const char str[], uint64_t size, uint64_t base) // TODO test
{
    uint64_t value = 0;
    for(uint64_t i=0; i<size; i++)
    {
        uint64_t aux = uint_from_char(str[i]);
        assert(aux < base);
        value = (value * base) + aux;
    }

    return value;
}

static uint64_t uint_read(FILE *fp, uint64_t size, uint64_t base)
{
    constexpr uint64_t size_max = 18;
    assert(size <= size_max);

    char str[size_max];
    for(uint64_t i=0; i<size; i++)
    {
        str[i] = (char)fgetc(fp);
    }

    return uint_from_str(str, size, base);
}



static num_config_t s_num_config = {
    .disk_threshold = UINT64_MAX
};

void num_config_set(num_config_p config)
{
    s_num_config = *config;
}



void num_display_dec(num_p num)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    if(num->count == 0)
    {
        printf("0");
        return;
    }

    constexpr uint64_t base = 1'000'000'000'000'000'000;
    num = num_base_to(num_copy(num), base);
    printf(U64P(), num->chunk[num->count-1]);
    for(uint64_t i=num->count-2; i!=UINT64_MAX; i--)
    {
        printf(U64P(018), num->chunk[i]);
    }

    num_free(num);
}

void num_display_opts(num_p num, const char tag[], bool length, bool full)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    if(tag)
    {
        printf("\n%s\t: ", tag);
    }

    if(length)
    {
        if(num->count == 0)
        {
            printf("(    0) | ");
        }
        else
        {
            printf("(" U64P(5) ") | ", num->count);
        }
    }

    if(num->count == 0)
    {
        printf("0");
        return;
    }

    uint64_t max;
    if (full || num->count <= 4)
    {
        max = num->count;
    }
    else
    {
        max = 4;
    }

    for(uint64_t i=0; i<max; i++)
    {
        printf("" U64PX " ", num->chunk[num->count-1-i]);
    }

    if(!full && num->count > 4)
    {
        printf("...");
    }
}

void num_display(num_p num)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    num_display_opts(num, nullptr, true, false);
}

void num_display_tag(const char tag[], num_p num)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    num_display_opts(num, tag, true, false);
}

void num_display_full(const char tag[], num_p num)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    num_display_opts(num, tag, true, true);
}




static num_p num_create_disk(CLU_PARAMS(uint64_t size, uint64_t count))
{
    constexpr uint64_t path_max = 1024;
    char template_path[path_max];
    snprintf(template_path, sizeof(template_path), "%s/bignum_XXXXXX", s_num_config.disk_path);
    int fd = mkstemp(template_path);
    assert(fd != -1);
    unlink(template_path);

    uint64_t total_size = sizeof(num_t) + (size * sizeof(uint64_t));
    assert(ftruncate(fd, (off_t)total_size) == 0);
    num_p num = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    *num = (num_t)
    {
        .size = size,
        .count = count,
        .is_mmap = true,
        .chunk = (chunk_p)&num[1]
    };
    CLU_HANDLER_REGISTER_TAG(num, CLU_STACK_TAG);
    return num;
}

num_p num_create(CLU_PARAMS(uint64_t size, uint64_t count))
{
    assert(size >= count)
    size = size ? size : 1;
    uint64_t total_size = sizeof(num_t) + (size * sizeof(uint64_t));

    if(size > s_num_config.disk_threshold)
    {
        return num_create_disk(CLU_ARGS_RELAY(size, count));
    }

    num_p num = calloc_tag(1, total_size, CLU_STACK_TAG);
    assert(num);

    *num = (num_t)
    {
        .size = size,
        .count = count,
        .chunk = (chunk_p)&num[1]
    };
    return num;
}

num_p num_create_dirty(CLU_PARAMS(uint64_t size, uint64_t count))
{
    assert(size >= count);
    size = size ? size : 1;
    uint64_t total_size = sizeof(num_t) + (size * sizeof(uint64_t));

    if(size > s_num_config.disk_threshold)
    {
        return num_create_disk(CLU_ARGS_RELAY(size, count));
    }

    num_p num = malloc_tag(total_size, CLU_STACK_TAG);
    assert(num);

    *num = (num_t)
    {
        .size = size,
        .count = count,
        .chunk = (chunk_p)&num[1]
    };
    return num;
}

STATIC num_p num_expand_to(num_p num, uint64_t target)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    if(target <= num->size)
    {
        return num;
    }

    assert(!num->cannot_expand);

    uint64_t size_new = 2 * target;
    uint64_t size_old = num->size;

    num_p num_new = num_create_dirty(CLU_ARGS(size_new, num->count));
    memcpy(num_new->chunk, num->chunk, size_old * sizeof(uint64_t));
    memset(&num_new->chunk[size_old], 0, (size_new - size_old) * sizeof(uint64_t));
    num_free(num);
    return num_new;
}

static void num_set_count(num_p num, uint64_t count)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    num->count = count;
    memset(&num->chunk[count], 0, (num->size - count) * sizeof(uint64_t));
}

STATIC num_p num_chunk_set(num_p num, uint64_t pos, uint64_t value)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    if(pos >= num->count && value == 0)
    {
        return num;
    }

    if(pos >= num->size)
    {
        num = num_expand_to(num, pos + 1);
    }

    num->chunk[pos] = value;
    if(pos >= num->count)
    {
        num->count = pos + 1;
    }

    return num;
}

STATIC num_p num_normalize(num_p num)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    while (num->count > 0 && num->chunk[num->count-1] == 0)
    {
        num->count--;
    }

    return num;
}

num_p num_head_grow(num_p num, uint64_t count) // TODO test
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    if(num->count == 0)
    {
        return num;
    }

    if(count == 0)
    {
        return num;
    }

    uint64_t count_res = num->count + count;
    num = num_expand_to(num, count_res);
    memmove(&num->chunk[count], num->chunk, num->count * sizeof(uint64_t));

    memset(num->chunk, 0, count * sizeof(uint64_t));
    num->count = count_res;
    return num;
}

void num_head_trim(num_p num, uint64_t count) // TODO test
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    if(num->count == 0 || count == 0)
    {
        return;
    }

    if(count >= num->count)
    {
        num_set_count(num, 0);
        return;
    }

    uint64_t count_res = num->count - count;
    memmove(num->chunk, &num->chunk[count], count_res * sizeof(uint64_t));

    memset(&num->chunk[num->count - count], 0, count * sizeof(uint64_t));
    num->count = count_res;
}

void num_break(num_p *out_num_hi, num_p *out_num_lo, num_p num, uint64_t count)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    if(count == 0)
    {
        *out_num_hi = num;
        *out_num_lo = num_create(CLU_ARGS(0, 0));
        return;
    }

    if(num->count <= count)
    {
        *out_num_hi = num_create(CLU_ARGS(0, 0));
        *out_num_lo = num;
        return;
    }

    uint64_t size = num->count - count;
    num_p num_hi = num_create_dirty(CLU_ARGS(size, size));
    memcpy(num_hi->chunk, &num->chunk[count], size * sizeof(uint64_t));

    memset(&num->chunk[count], 0, (num->size - count) * sizeof(uint64_t));
    num->count = count;
    num_normalize(num);

    *out_num_hi = num_hi;
    *out_num_lo = num;
}

// NUM_RES should be static memory
static void num_span(num_p num_res, num_p num, uint64_t pos_init, uint64_t pos_max) // TODO TEST
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num_res);
    assert(num);

    assert(pos_max <= num->size);
    uint64_t size = pos_init > pos_max ? 0 : pos_max - pos_init;

    CLU_HANDLER_REGISTER_STATIC(num_res);
    *num_res = (num_t)
    {
        .size = size,
        .count = size,
        .chunk = &num->chunk[pos_init],
        .cannot_expand = true
    };
    num_normalize(num_res);
}



num_p num_wrap(uint64_t value)
{
    if(value == 0)
    {
        return num_create(CLU_ARGS(0, 0));
    }

    num_p num = num_create(CLU_ARGS(1, 1));
    num->chunk[0] = value;
    return num;
}

num_p num_wrap_uint128(uint128_t value)
{
    if(value <= UINT64_MAX)
    {
        return num_wrap((uint64_t)value);
    }

    num_p num = num_create(CLU_ARGS(2, 2));
    num->chunk[0] = LOW(value);
    num->chunk[1] = HIGH(value);
    return num;
}

STATIC num_p num_wrap_dec(const char str[])
{
    constexpr uint64_t dec_base = 10;
    constexpr uint64_t chunk_len = 18;
    constexpr uint64_t chunk_base = 1'000'000'000'000'000'000;


    uint64_t len = strlen(str);
    uint64_t pos = len / chunk_len;
    uint64_t extra = len % chunk_len != 0;
    num_p num = num_create(CLU_ARGS(pos + extra, 0));

    if(extra)
    {
        uint64_t value = uint_from_str(str, len % chunk_len, dec_base);
        num = num_chunk_set(num, pos, value);
    }

    for(uint64_t i=len % chunk_len; i<len; i+=chunk_len)
    {
        uint64_t value = uint_from_str(&str[i], chunk_len, dec_base);
        pos--;
        num = num_chunk_set(num, pos, value);
    }

    num_normalize(num);
    return num_base_from(num, chunk_base);
}

STATIC num_p num_wrap_hex(const char str[])
{
    uint64_t len = strlen(str);
    assert(len > 1 && str[0] == '0' && str[1] == 'x');

    constexpr uint64_t chars_per_chunk = 16;
    uint64_t pos = (len - 2) / chars_per_chunk;
    uint64_t extra = (len - 2) % chars_per_chunk != 0;
    num_p num = num_create(CLU_ARGS(pos + extra, 0));

    if(extra)
    {
        uint64_t value = uint_from_str(&str[2], (len - 2) % chars_per_chunk, chars_per_chunk);
        num = num_chunk_set(num, pos, value);
    }

    for(uint64_t i = 2 + ((len - 2) % chars_per_chunk); i < len; i += chars_per_chunk)
    {
        uint64_t value = uint_from_str(&str[i], chars_per_chunk, chars_per_chunk);
        pos--;
        num = num_chunk_set(num, pos, value);
    }

    num_normalize(num);
    return num;
}

num_p num_wrap_str(const char str[])
{
    return str[0] == '0' && str[1] == 'x' ?
        num_wrap_hex(str) : num_wrap_dec(str);
}

static uint64_t get_ftell(FILE* fp)
{
    int64_t res = ftell(fp);
    assert(res >= 0);
    return (uint64_t)res;
}

num_p num_read_dec(const char file_name[])
{
    constexpr uint64_t dec_base = 10;
    constexpr uint64_t chunk_len = 18;
    constexpr uint64_t chunk_base = 1'000'000'000'000'000'000;

    FILE *fp = fopen(file_name, "r");
    assert(fp);

    assert(!fseek(fp, 0, SEEK_END));
    uint64_t size = get_ftell(fp);
    assert(!fseek(fp, 0, SEEK_SET));

    uint64_t pos = size / chunk_len;
    uint64_t extra = size % chunk_len;

    num_p num = num_create(CLU_ARGS(pos + extra, 0));
    if(extra)
    {
        uint64_t value = uint_read(fp, size % chunk_len, dec_base);
        num = num_chunk_set(num, pos, value);
    }

    while(get_ftell(fp) < size)
    {
        uint64_t value = uint_read(fp, chunk_len, dec_base);
        pos--;
        num = num_chunk_set(num, pos, value);
    }
    fclose(fp);

    num_normalize(num);
    return num_base_from(num, chunk_base);
}

uint64_t num_unwrap(num_p num)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    assert(num->count < 2);
    uint64_t value = num->count ? num->chunk[0] : 0;
    num_free(num);

    return value;
}

num_p num_copy(num_p num) // TODO TEST
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    num_p num_res = num_create_dirty(CLU_ARGS(num->count, num->count));
    memcpy(num_res->chunk, num->chunk, num->count * sizeof(uint64_t));

    return num_res;
}

void num_free(num_p num)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    if(!num->is_mmap)
    {
        free(num);
        return;
    }

    uint64_t total_size = sizeof(num_t) + (num->size * sizeof(uint64_t));
    assert(munmap(num, total_size) == 0);
    CLU_HANDLER_UNREGISTER(num)
}



STATIC num_p num_add_uint_offset(num_p num, uint64_t pos, uint64_t value)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    uint64_t carry = value;
    for(uint64_t i=pos; i<num->count && carry; i++)
    {
        carry = (uint64_t)__builtin_add_overflow(num->chunk[i], carry, &num->chunk[i]);
    }

    if(carry)
    {
        pos = pos > num->count ? pos : num->count;
        num = num_chunk_set(num, pos , LOW(carry));
    }

    return num;
}

STATIC num_p num_sub_uint_offset(num_p num, uint64_t pos, uint64_t value)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    uint64_t borrow = value;
    for(uint64_t i = pos; i < num->count && borrow; i++)
    {
        borrow = (uint64_t)__builtin_sub_overflow(num->chunk[i], borrow, &num->chunk[i]);
    }
    assert(borrow == 0);

    num_normalize(num);
    return num;
}

// keeps NUM
static num_p num_add_mul_uint_offset(
    num_p num_res, uint64_t pos_res,
    num_p num, uint64_t pos,
    uint64_t value
)
{
    CLU_HANDLER_IS_SAFE(num_res)
    CLU_HANDLER_IS_SAFE(num)
    assert(num_res)
    assert(num)

    if(value == 0 || pos >= num->count)
    {
        return num_res;
    }

    uint64_t iter_count = num->count - pos;
    uint64_t target_count = pos_res + iter_count;

    if(num_res->size <= target_count)
    {
        num_res = num_expand_to(num_res, target_count + 1);
    }

    if(num_res->count < target_count)
    {
        num_res->count = target_count;
    }

    uint64_t carry = 0;

    uint64_t * restrict dest = num_res->chunk;
    const uint64_t * restrict src = num->chunk;

    for(uint64_t i = 0; i < iter_count; i++)
    {
        uint64_t dest_idx = pos_res + i;

        uint128_t u = MUL(src[pos + i], value);
        u += dest[dest_idx];
        u += carry;

        dest[dest_idx] = LOW(u);
        carry = HIGH(u);
    }

    if(carry)
    {
        num_res = num_add_uint_offset(num_res, target_count, carry);
    }

    return num_normalize(num_res);
}

// BITS shoud be less than 64
STATIC num_p num_shl_core(num_p num, uint64_t bits) // TODO test
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);
    assert(bits < 64); // TODO test

    if(bits == 0)
    {
        return num;
    }

    uint64_t carry = 0;
    for(uint64_t i=0; i<num->count; i++)
    {
        uint64_t value = num->chunk[i];
        num->chunk[i] = (value << bits) | carry;
        carry = value >> (chunk_bits - bits);
    }

    if(carry)
    {
        num = num_chunk_set(num, num->count, carry);
    }

    return num;
}

// BITS shoud be less than 64
STATIC num_p num_shr_core(num_p num, uint64_t bits) // TODO test
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);
    assert(bits < 64); // TODO test

    if(bits == 0)
    {
        return num;
    }

    uint64_t carry = 0;
    for(uint64_t i=num->count-1; i!=UINT64_MAX; i--)
    {
        uint64_t value = num->chunk[i];
        num->chunk[i] = (value >> bits) | carry;
        carry = value << (chunk_bits - bits);
    }
    num_normalize(num);

    return num;
}



STATIC int64_t num_cmp_offset(num_p num_1, uint64_t pos_1, num_p num_2) // TODO TEST
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    if(num_1->count > num_2->count + pos_1)
    {
        return 1;
    }

    if(num_1->count < num_2->count + pos_1)
    {
        return -1;
    }

    for(uint64_t i = num_2->count-1; i != UINT64_MAX; i--)
    {
        uint64_t value_1 = num_1->chunk[pos_1 + i];
        uint64_t value_2 = num_2->chunk[i];

        if(value_1 > value_2)
        {
            return 1;
        }

        if(value_1 < value_2)
        {
            return -1;
        }
    }

    return 0;
}

// keeps NUM_2
// TODO TEST
static num_p num_add_offset(num_p num_1, uint64_t pos_1, num_p num_2, uint64_t pos_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    if(num_2->count == 0)
    {
        return num_1;
    }

    uint64_t delta = pos_1 - pos_2;
    uint64_t count_max = delta + num_2->count;

    num_1 = num_expand_to(num_1, count_max);

    uint64_t carry = 0;
    for(uint64_t i=pos_2; i<num_2->count; i++)
    {
        uint64_t sum;
        uint64_t c1 = (uint64_t)__builtin_add_overflow(num_1->chunk[delta + i], num_2->chunk[i], &sum);
        uint64_t c2 = (uint64_t)__builtin_add_overflow(sum, carry, &num_1->chunk[delta + i]);
        carry = c1 | c2; // Combine overflow states
    }

    if(carry)
    {
        num_1 = num_add_uint_offset(num_1, count_max, carry);
    }
    else
    {
        if(num_1->count < count_max)
        {
            num_1->count = count_max;
        }
    }

    return num_1;
}

// keeps NUM2
// TODO: improve efficiency
STATIC num_p num_sub_offset(num_p num_1, uint64_t pos_1, num_p num_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    uint64_t borrow = 0;
    uint64_t i = 0;
    for(; i < num_2->count; i++)
    {
        uint64_t diff;
        uint64_t b1 = (uint64_t)__builtin_sub_overflow(num_1->chunk[pos_1 + i], num_2->chunk[i], &diff);
        uint64_t b2 = (uint64_t)__builtin_sub_overflow(diff, borrow, &num_1->chunk[pos_1 + i]);
        borrow = b1 | b2;
    }

    // 2. Propagate any leftover borrow upwards
    for(uint64_t j = pos_1 + i; j < num_1->count && borrow; j++)
    {
        borrow = (uint64_t)__builtin_sub_overflow(num_1->chunk[j], borrow, &num_1->chunk[j]);
    }
    assert(borrow == 0);

    return num_normalize(num_1);
}



// preserves NUM
// num_res->size >= num->count + 1
static num_p num_mul_uint_buffer(num_p num_res, num_p num, uint64_t value) // TODO TEST
{
    CLU_HANDLER_IS_SAFE(num_res)
    CLU_HANDLER_IS_SAFE(num)
    assert(num_res)
    assert(num)
    assert(num_res->size >= num->count + 1)

    if(value == 0 || num->count == 0)
    {
        num_set_count(num_res, 0);
        return num_res;
    }

    num_res->count = num->count + 1;
    num_res->chunk[0] = 0;
    for(uint64_t i=0; i<num->count; i++)
    {
        uint128_t u = MUL(num->chunk[i], value);
        num_res->chunk[i+1] = HIGH(u);
        num_res = num_add_uint_offset(num_res, i, LOW(u));
    }
    return num_normalize(num_res);
}

static num_p num_sqr_classic_buffer(num_p num_res, num_p num)
{
    CLU_HANDLER_IS_SAFE(num_res)
    CLU_HANDLER_IS_SAFE(num)
    assert(num_res)
    assert(num)
    assert(num_res->size >= 2 * num->count)

    num_set_count(num_res, 0);
    for(uint64_t i=0; i<num->count; i++)
    {
        uint64_t value = num->chunk[i];

        uint128_t u = MUL(value, value);
        num_res = num_add_uint_offset(num_res, (2 * i), LOW(u));
        num_res = num_add_uint_offset(num_res, (2 * i) + 1, HIGH(u));

        num_res = num_add_mul_uint_offset(num_res, (2 * i) + 1, num, i + 1, value << 1);

        constexpr uint64_t msb_set = 0x8000000000000000;
        if(value >= msb_set)
        {
            num_res = num_add_offset(num_res, (2 * i) + 2, num, i + 1);
        }
    }
    return num_res;
}

// KEEPS NUM_1 NUM_2
STATIC num_p num_mul_classic(num_p num_1, num_p num_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1 && num_2)

    uint64_t n1 = num_1->count;
    uint64_t n2 = num_2->count;

    if (n1 == 0 || n2 == 0)
    {
        return num_create(CLU_ARGS(0, 0));
    }

    uint64_t target_count = n1 + n2;

    num_p num_res = num_create(CLU_ARGS(target_count, target_count));
    uint64_t * restrict dest = num_res->chunk;
    const uint64_t * restrict src1 = num_1->chunk;
    const uint64_t * restrict src2 = num_2->chunk;

    for(uint64_t i = 0; i < n2; i++)
    {
        uint64_t v2 = src2[i];
        if (v2 == 0)
        {
            continue;
        }

        uint64_t carry = 0;
        for(uint64_t j = 0; j < n1; j++)
        {
            uint64_t dest_idx = i + j;

            uint128_t u = MUL(src1[j], v2);
            uint64_t p_low = LOW(u);
            uint64_t p_high = HIGH(u);

            uint64_t sum;
            uint64_t c1 = (uint64_t)__builtin_add_overflow(p_low, dest[dest_idx], &sum);
            uint64_t c2 = (uint64_t)__builtin_add_overflow(sum, carry, &dest[dest_idx]);

            // Carry is mathematically guaranteed to fit cleanly in 64 bits
            carry = p_high + c1 + c2;
        }
        dest[i + n1] = carry;
    }

    return num_normalize(num_res);
}

STATIC num_p num_sqr_classic(num_p num)
{
    num_p num_res = num_create(CLU_ARGS(2 * num->count, 0));
    num_res->cannot_expand = true;
    num_res = num_sqr_classic_buffer(num_res, num);
    num_res->cannot_expand = false;
    num_free(num);
    return num_res;
}



static void num_display_span(num_p num_fft, uint64_t pos, uint64_t count)
{
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_fft)
    assert(num_fft->size >= pos + count);

    for(uint64_t i=count-1; i!=UINT64_MAX; i--)
    {
        printf("" U64PX " ", num_fft->chunk[pos + i]);
    }
}

[[maybe_unused]]
STATIC void num_display_span_full(
    const char tag[],
    num_p num_fft,
    uint64_t n,
    uint64_t k
)
{
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_fft)

    printf("\n");
    printf("\n%s", tag);
    for(uint64_t i=0; i<k; i++)
    {
        printf("\nc[" U64P() "]\t:", i);
        num_display_span(num_fft, i * n, n);
    }
}

STATIC uint64_t ssm_bit_inv(uint64_t i, uint64_t K)
{
    uint64_t res = 0;
    for(; K > 1; K>>=1)
    {
        res = (res << 1) | (i & 1);
        i >>= 1;
    }
    return res;
}

static bool num_is_span_zero(num_p num, uint64_t pos, uint64_t count)
{
    CLU_HANDLER_IS_SAFE(num)
    assert(num)

    for(uint64_t i=0; i<count; i++)
    {
        if(num->chunk[i + pos])
        {
            return false;
        }
    }

    return true;
}

static int64_t num_ssm_cmp_uint_offset(
    num_p num,
    uint64_t pos,
    uint64_t value,
    uint64_t n
)
{
    CLU_HANDLER_IS_SAFE(num)
    assert(num)

    if(!num_is_span_zero(num, pos + 1, n - 1))
    {
        return 1;
    }

    uint64_t value_num = num->chunk[pos];
    if(value_num > value)
    {
        return 1;
    }

    if(value_num < value)
    {
        return -1;
    }

    return 0;
}

static void num_ssm_add_uint(num_p num, uint64_t pos, uint64_t n, uint64_t value)
{
    CLU_HANDLER_IS_SAFE(num)
    assert(num)

    uint64_t carry = value;
    for(uint64_t i = 0; i < n && carry; i++)
    {
        carry = (uint64_t)__builtin_add_overflow(num->chunk[pos + i], carry, &num->chunk[pos + i]);
    }
}

static void num_ssm_sub_uint(num_p num, uint64_t pos, uint64_t n, uint64_t value)
{
    CLU_HANDLER_IS_SAFE(num)
    assert(num)

    uint64_t borrow = value;
    for(uint64_t i = 0; i < n && borrow; i++)
    {
        borrow = (uint64_t)__builtin_sub_overflow(num->chunk[pos + i], borrow, &num->chunk[pos + i]);
    }
}

// normalizes coeficient if it is less than 2 modulus
static void num_ssm_normalize(num_p num, uint64_t pos, uint64_t n)
{
    CLU_HANDLER_IS_SAFE(num)
    assert(num)
    assert(num->chunk[pos + n - 1] <= 2)

    uint64_t value = num->chunk[pos + n - 1];
    num->chunk[pos + n - 1] = 0;
    num_ssm_sub_uint(num, pos, n, value);

    if(num->chunk[pos + n - 1] != UINT64_MAX)
    {
        return;
    }

    if(value != 1)
    {
        num->chunk[pos + n - 1] = 0;
        num_ssm_add_uint(num, pos, n, 1);
        return;
    }

    memset(&num->chunk[pos], 0, (n - 1) * sizeof(uint64_t));
    num->chunk[pos + n - 1] = 1;
}

static void num_ssm_denormalize(num_p num, uint64_t pos, uint64_t n)
{
    CLU_HANDLER_IS_SAFE(num)
    assert(num)

    num_ssm_add_uint(num, pos, n, 1);
    num->chunk[pos + n - 1] += 1;
}

STATIC void num_ssm_add_mod_immed(
    num_p num_1, uint64_t pos_1,
    num_p num_2, uint64_t pos_2,
    uint64_t n
)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1 && num_2)

    uint64_t * restrict dest = &num_1->chunk[pos_1];
    const uint64_t * restrict src2 = &num_2->chunk[pos_2];

#ifdef __linux__

    uint64_t count = n;
    __asm__ volatile (
        "test %[count], %[count]\n\t"
        "jz 2f\n\t"
        "clc\n"
        "1:\n\t"
        "mov (%[src2]), %%rax\n\t"
        "adc %%rax, (%[dest])\n\t"        // *dest = *dest + rax + CF

        "lea 8(%[src2]), %[src2]\n\t"
        "lea 8(%[dest]), %[dest]\n\t"
        "dec %[count]\n\t"
        "jnz 1b\n\t"
        "2:\n"
        : [dest] "+r" (dest), [src2] "+r" (src2), [count] "+r" (count)
        :
        : "rax", "cc", "memory"
    );

#elifdef __APPLE__

    uint64_t count = n;
    uint64_t tmp1, tmp2; // Need two temporaries now

    __asm__ volatile (
        "cbz %[count], 2f\n\t"               // If count == 0, jump to label 2
        "adds xzr, xzr, xzr\n\t"             // Clear carry flag (C=0)
        "1:\n\t"
        "ldr %[tmp1], [%[src2]], #8\n\t"     // tmp1 = *src2, then src2 += 8
        "ldr %[tmp2], [%[dest]]\n\t"         // tmp2 = *dest (NO post-increment yet)

        "adcs %[tmp2], %[tmp2], %[tmp1]\n\t" // tmp2 = tmp2 + tmp1 + CF, update CF

        "str %[tmp2], [%[dest]], #8\n\t"     // *dest = tmp2, then dest += 8

        "sub %[count], %[count], #1\n\t"     // count-- (leaves CF untouched)
        "cbnz %[count], 1b\n\t"              // Loop if count != 0
        "2:\n"
        : [dest] "+r" (dest), [src2] "+r" (src2), [count] "+r" (count),
          [tmp1] "=&r" (tmp1), [tmp2] "=&r" (tmp2)
        :
        : "cc", "memory"
    );

#else

    uint64_t carry = 0;
    for(uint64_t i = 0; i < n; i++)
    {
        uint64_t c1 = (uint64_t)__builtin_add_overflow(dest[i], src2[i], &dest[i]);
        uint64_t c2 = (uint64_t)__builtin_add_overflow(dest[i], carry, &dest[i]);
        carry = c1 | c2;
    }

#endif

    num_ssm_normalize(num_1, pos_1, n);
}

STATIC void num_ssm_sub_mod(
    num_p num_res, uint64_t pos_res,
    num_p num_1, uint64_t pos_1,
    num_p num_2, uint64_t pos_2,
    uint64_t n
)
{
    CLU_HANDLER_IS_SAFE(num_res)
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_res && num_1 && num_2)

    num_ssm_denormalize(num_1, pos_1, n);

    uint64_t * restrict dest = &num_res->chunk[pos_res];
    const uint64_t * restrict src1 = &num_1->chunk[pos_1];
    const uint64_t * restrict src2 = &num_2->chunk[pos_2];

#ifdef __linux__

    uint64_t count = n;
    __asm__ volatile (
        "test %[count], %[count]\n\t"
        "jz 2f\n\t"
        "clc\n"                           // Clear borrow flag
        "1:\n\t"
        "mov (%[src1]), %%rax\n\t"
        "sbb (%[src2]), %%rax\n\t"        // rax = *src1 - *src2 - CF
        "mov %%rax, (%[dest])\n\t"

        "lea 8(%[src1]), %[src1]\n\t"
        "lea 8(%[src2]), %[src2]\n\t"
        "lea 8(%[dest]), %[dest]\n\t"
        "dec %[count]\n\t"
        "jnz 1b\n\t"
        "2:\n"
        : [dest] "+r" (dest), [src1] "+r" (src1), [src2] "+r" (src2), [count] "+r" (count)
        :
        : "rax", "cc", "memory"
    );

#elifdef __APPLE__

    uint64_t count = n;
    uint64_t tmp1, tmp2;

    __asm__ volatile (
        "cbz %[count], 2f\n\t"               // If count == 0, jump to label 2

        "cmp xzr, xzr\n\t"                   // SET the carry flag (C=1 means NO borrow)

        "1:\n\t"
        "ldr %[tmp1], [%[src1]], #8\n\t"     // tmp1 = *src1, then src1 += 8
        "ldr %[tmp2], [%[src2]], #8\n\t"     // tmp2 = *src2, then src2 += 8

        "sbcs %[tmp1], %[tmp1], %[tmp2]\n\t" // tmp1 = tmp1 - tmp2 - (1 - C), update C

        "str %[tmp1], [%[dest]], #8\n\t"     // *dest = tmp1, then dest += 8

        "sub %[count], %[count], #1\n\t"     // count-- (leaves flags untouched)
        "cbnz %[count], 1b\n\t"              // Loop if count != 0
        "2:\n"
        : [dest] "+r" (dest), [src1] "+r" (src1), [src2] "+r" (src2),
          [count] "+r" (count), [tmp1] "=&r" (tmp1), [tmp2] "=&r" (tmp2)
        :
        : "cc", "memory"
    );

#else

    uint64_t borrow = 0;
    for(uint64_t i=0; i<n; i++)
    {
        uint64_t diff;
        uint64_t b1 = (uint64_t)__builtin_sub_overflow(src1[i], src2[i], &diff);
        uint64_t b2 = (uint64_t)__builtin_sub_overflow(diff, borrow, &dest[i]);
        borrow = b1 | b2;
    }

#endif

    num_ssm_normalize(num_1, pos_1, n);
    num_ssm_normalize(num_res, pos_res, n);
}

static void num_ssm_sub_mod_immed(
    num_p num_1, uint64_t pos_1,
    num_p num_2, uint64_t pos_2,
    uint64_t n
)
{
    CLU_HANDLER_IS_SAFE(num_1) CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1 && num_2)

    num_ssm_denormalize(num_1, pos_1, n);

    uint64_t * restrict dest = &num_1->chunk[pos_1];
    const uint64_t * restrict src2 = &num_2->chunk[pos_2];

#ifdef __linux__

    uint64_t count = n;
    __asm__ volatile (
        "test %[count], %[count]\n\t"
        "jz 2f\n\t"
        "clc\n"
        "1:\n\t"
        "mov (%[src2]), %%rax\n\t"
        "sbb %%rax, (%[dest])\n\t"        // *dest = *dest - rax - CF

        "lea 8(%[src2]), %[src2]\n\t"
        "lea 8(%[dest]), %[dest]\n\t"
        "dec %[count]\n\t"
        "jnz 1b\n\t"
        "2:\n"
        : [dest] "+r" (dest), [src2] "+r" (src2), [count] "+r" (count)
        :
        : "rax", "cc", "memory"
    );

#elifdef __APPLE__

    uint64_t count = n;
    uint64_t tmp1, tmp2; // Two temporaries required for the in-place math

    __asm__ volatile (
        "cbz %[count], 2f\n\t"               // If count == 0, jump to label 2

        "cmp xzr, xzr\n\t"                   // SET the carry flag (C=1 means NO borrow)

        "1:\n\t"
        "ldr %[tmp1], [%[src2]], #8\n\t"     // tmp1 = *src2, then src2 += 8
        "ldr %[tmp2], [%[dest]]\n\t"         // tmp2 = *dest (NO post-increment yet)

        "sbcs %[tmp2], %[tmp2], %[tmp1]\n\t" // tmp2 = tmp2 - tmp1 - (1 - C), update C

        "str %[tmp2], [%[dest]], #8\n\t"     // *dest = tmp2, then dest += 8

        "sub %[count], %[count], #1\n\t"     // count-- (leaves flags untouched)
        "cbnz %[count], 1b\n\t"              // Loop if count != 0
        "2:\n"
        : [dest] "+r" (dest), [src2] "+r" (src2), [count] "+r" (count),
          [tmp1] "=&r" (tmp1), [tmp2] "=&r" (tmp2)
        :
        : "cc", "memory"
    );

#else

    uint64_t borrow = 0;
    for(uint64_t i=0; i<n; i++)
    {
        uint64_t b1 = (uint64_t)__builtin_sub_overflow(dest[i], src2[i], &dest[i]);
        uint64_t b2 = (uint64_t)__builtin_sub_overflow(dest[i], borrow, &dest[i]);
        borrow = b1 | b2;
    }

#endif

    num_ssm_normalize(num_1, pos_1, n);
}

STATIC void num_ssm_opposite(num_p num, uint64_t pos, uint64_t n)
{
    CLU_HANDLER_IS_SAFE(num)
    assert(num)

    uint64_t borrow = (uint64_t)__builtin_sub_overflow(1, num->chunk[pos], &num->chunk[pos]);
    for(uint64_t i = 1; i < n; i++)
    {
        uint64_t diff;
        uint64_t b1 = (uint64_t)__builtin_sub_overflow(0, num->chunk[pos + i], &diff);
        uint64_t b2 = (uint64_t)__builtin_sub_overflow(diff, borrow, &num->chunk[pos + i]);
        borrow = b1 | b2;
    }
    num->chunk[pos + n - 1]++;
    num_ssm_normalize(num, pos, n);
}

STATIC void num_ssm_shl(
    num_p num_res, uint64_t pos_res,
    num_p num, uint64_t pos,
    uint64_t n,
    uint64_t bits
)
{
    CLU_HANDLER_IS_SAFE(num_res)
    CLU_HANDLER_IS_SAFE(num)
    assert(num_res)
    assert(num)
    assert(num_res->size >= pos_res + n)

    constexpr uint64_t mask = 0x3f;

    uint64_t count = bits >> chunk_bits_log_2;
    bits &= mask;

    uint64_t * restrict dest = &num_res->chunk[pos_res];
    const uint64_t * restrict src = &num->chunk[pos];

    if(bits == 0)
    {
        memcpy(&dest[count], src, (n - count) * sizeof(uint64_t));
        memset(dest, 0, count * sizeof(uint64_t));
        return;
    }

    uint64_t inv_bits = chunk_bits - bits;
    for(uint64_t i = count + 1; i < n; i++)
    {
        dest[i] = (src[i - count] << bits) | (src[i - count - 1] >> inv_bits);
    }

    dest[count] = src[0] << bits;
    memset(dest, 0, count * sizeof(uint64_t));
}

STATIC void num_ssm_shr(
    num_p num_res, uint64_t pos_res,
    num_p num, uint64_t pos,
    uint64_t n,
    uint64_t bits
)
{
    CLU_HANDLER_IS_SAFE(num_res)
    CLU_HANDLER_IS_SAFE(num)
    assert(num_res)
    assert(num)
    assert(num_res->size >= pos_res + n)

    constexpr uint64_t mask = 0x3f;

    uint64_t count = bits >> chunk_bits_log_2;
    bits &= mask;

    uint64_t * restrict dest = &num_res->chunk[pos_res];
    const uint64_t * restrict src = &num->chunk[pos];

    if(bits == 0)
    {
        memcpy(dest, &src[count], (n - count) * sizeof(uint64_t));
        memset(&dest[n - count], 0, count * sizeof(uint64_t));
        return;
    }

    uint64_t inv_bits = chunk_bits - bits;
    uint64_t stop = n - count - 1;
    for(uint64_t i = 0; i < stop; i++)
    {
        dest[i] = (src[count + i] >> bits) | (src[count + i + 1] << inv_bits);
    }

    dest[n - count - 1] = src[n - 1] >> bits;
    memset(&dest[n - count], 0, count * sizeof(uint64_t));
}

// num_aux->size >= 2 * n
STATIC void num_ssm_shl_mod(
    num_p num_aux,
    num_p num,
    uint64_t pos,
    uint64_t n,
    uint64_t bits
)
{
    CLU_HANDLER_IS_SAFE(num_aux)
    CLU_HANDLER_IS_SAFE(num)
    assert(num_aux)
    assert(num)
    assert(num_aux->size >= 2 * n)
    assert(num->size >= pos + n);

    if(bits == 0 || num_is_span_zero(num, pos, n))
    {
        return;
    }

    num_ssm_shr(num_aux, 0, num, pos, n, (chunk_bits * n) - chunk_bits - bits);
    num_ssm_shl(num_aux, n, num, pos, n, bits);
    num_aux->chunk[(2 * n) - 1] = 0;
    num_ssm_sub_mod(num, pos, num_aux, n, num_aux, 0, n);
}

// num_aux->size >= 2 * p->n
STATIC void num_ssm_shr_mod(
    num_p num_aux,
    num_p num,
    uint64_t pos,
    uint64_t n,
    uint64_t bits
)
{
    CLU_HANDLER_IS_SAFE(num_aux)
    CLU_HANDLER_IS_SAFE(num)
    assert(num_aux)
    assert(num)
    assert(bits <= 64 * (n - 1))
    assert(num_aux->size >= 2 * n)

    if(bits == 0 || num_is_span_zero(num, pos, n))
    {
        return;
    }

    num_ssm_shl(num_aux, 0, num, pos, n, (chunk_bits * n) - chunk_bits - bits);
    num_ssm_shr(num_aux, n, num, pos, n, bits);
    num_aux->chunk[n - 1] = 0;
    num_ssm_sub_mod(num, pos, num_aux, n, num_aux, 0, n);
}

// num_aux->size >= 2 * n
static void num_ssm_fft_fwd_rec(
    num_p num_aux,
    num_p num,
    uint64_t pos,
    uint64_t step,
    uint64_t n,
    uint64_t K,
    uint64_t bits
)
{
    CLU_HANDLER_IS_SAFE(num_aux)
    CLU_HANDLER_IS_SAFE(num)
    assert(num_aux)
    assert(num)
    assert(num_aux->size >= 2 * n)

    if(K > 2)
    {
        num_ssm_fft_fwd_rec(num_aux, num, pos     , 2*step, n, K/2, 2*bits);
        num_ssm_fft_fwd_rec(num_aux, num, pos+step, 2*step, n, K/2, 2*bits);
    }

    for(uint64_t i=0; i<K/2; i++)
    {
        uint64_t pos_1 = (pos + (step * (2 * i))) * n;
        uint64_t pos_2 = (pos + (step * ((2 * i) + 1))) * n;

        uint64_t shift = ssm_bit_inv(i, K / 2) * bits;
        num_ssm_shl_mod(num_aux, num, pos_2, n, shift);

        num_ssm_sub_mod(num_aux, 0, num, pos_1, num, pos_2, n);
        num_ssm_add_mod_immed(num, pos_1, num, pos_2, n);
        memcpy(&num->chunk[pos_2], num_aux->chunk, n * sizeof(uint64_t));
    }
}

// num_aux->size >= 2 * n
STATIC void num_ssm_fft_fwd(num_p num_aux, num_p num, ssm_params_p p)
{
    CLU_HANDLER_IS_SAFE(num_aux)
    CLU_HANDLER_IS_SAFE(num)
    assert(num_aux)
    assert(num)
    assert(num_aux->size >= 2 * p->n)
    assert(num->size >= p->n * p->K)

    for(uint64_t i=0; i<p->K; i++)
    {
        num_ssm_shl_mod(num_aux, num, p->n * i, p->n, p->Q * i);
    }

    num_ssm_fft_fwd_rec(num_aux, num, 0, 1, p->n, p->K, 2 * p->Q);
}

// num_aux->size >= 2 * n
static void num_ssm_fft_inv_rec(
    num_p num_aux,
    num_p num,
    uint64_t pos,
    uint64_t n,
    uint64_t k,
    uint64_t bits
)
{
    CLU_HANDLER_IS_SAFE(num_aux)
    CLU_HANDLER_IS_SAFE(num)
    assert(num_aux)
    assert(num)
    assert(num_aux->size >= 2 * n)

    if(k > 2)
    {
        num_ssm_fft_inv_rec(num_aux, num, pos     , n, k/2, 2*bits);
        num_ssm_fft_inv_rec(num_aux, num, pos+(k/2), n, k/2, 2*bits);
    }

    for(uint64_t i=0; i<k/2; i++)
    {
        uint64_t pos_1 = (pos + i) * n;
        uint64_t pos_2 = (pos + i + (k/2)) * n;

        num_ssm_shr_mod(num_aux, num, pos_2, n, i * bits);

        num_ssm_sub_mod(num_aux, 0, num, pos_1, num, pos_2, n);
        num_ssm_add_mod_immed(num, pos_1, num, pos_2, n);
        memcpy(&num->chunk[pos_2], num_aux->chunk, n * sizeof(uint64_t));
    }
}

// num_aux->size >= 2 * p->n
STATIC void num_ssm_fft_inv(num_p num_aux, num_p num, ssm_params_p p)
{
    CLU_HANDLER_IS_SAFE(num_aux)
    CLU_HANDLER_IS_SAFE(num)
    assert(num_aux)
    assert(num)
    assert(num_aux->size >= 2 * p->n)

    num_ssm_fft_inv_rec(num_aux, num, 0, p->n, p->K, 2 * p->Q);

    uint64_t k_ = stdc_trailing_zeros(p->K);
    for(uint64_t i=0; i<p->K; i++)
    {
        num_ssm_shr_mod(num_aux, num, p->n * i, p->n, p->Q * i);
        num_ssm_shr_mod(num_aux, num, p->n * i, p->n, k_);
    }
}

#define TRESHOLD 129

static bool ssm_is_recursive(uint64_t n)
{
    return (bool)((n > TRESHOLD) && (((n - 1) & (1 - n)) > 4));
}

// NOLINTBEGIN(readability-magic-numbers)
STATIC ssm_params_t ssm_get_params(uint64_t count)
{
    uint64_t M = B(stdc_bit_width(count) / 2);
    uint64_t K = 4 * stdc_bit_ceil((count + M - 1) / M);
    M = (count / K) + 1;

    uint64_t Q;
    uint64_t n;
    if(K < 64)
    {
        uint64_t P = (2 * M) + 1;
        Q = 64 * P / K;
        n = P + 1;
    }
    else
    {
        Q = (128 * M / K) + 1;
        n = (K * Q / 64) + 1;
    }
    assert(64 * (n - 1) % K == 0);
    assert(n > 2 * M);

    if(n > TRESHOLD)
    {
        uint64_t moduli = (n - 1) & 7;
        if(moduli)
        {
            n += 8 - moduli;

            assert(64 * (n - 1) % K == 0);
            Q = 64 * (n - 1) / K;
        }
    }
    assert(64 * (n - 1) % K == 0);
    assert(n > 2 * M);

    return (ssm_params_t)
    {
        .count = count,
        .M = M,
        .K = K,
        .Q = Q,
        .n = n
    };
}
// NOLINTEND(readability-magic-numbers)

// NOLINTBEGIN(readability-magic-numbers)
STATIC ssm_params_t ssm_get_params_wrap(uint64_t n)
{
    uint64_t K1 = 2 * B(stdc_bit_width(n-1) / 2);
    uint64_t K2 = (n - 1) & (1 - n);
    uint64_t K = K1 < K2 ? K1 : K2;
    uint64_t M = (n - 1) / K;

    uint64_t Q;
    uint64_t _n;
    if(K < 64)
    {
        uint64_t P = (2 * M) + 1;
        Q = 64 * P / K;
        _n = P + 1;
    }
    else
    {
        Q = (128 * M / K) + 1;
        _n = (K * Q / 64) + 1;
    }

    if(_n > TRESHOLD)
    {
        uint64_t moduli = (_n - 1) & 7;
        if(moduli)
        {
            _n += 8 - moduli;

            assert(64 * (_n - 1) % K == 0);
            Q = 64 * (_n - 1) / K;
        }
    }

    assert(n == (M * K) + 1);

    return (ssm_params_t)
    {
        .count = n,
        .M = M,
        .K = K,
        .Q = Q,
        .n = _n
    };
}
// NOLINTEND(readability-magic-numbers)

#ifdef __linux__

static uint64_t num_ssm_add_mul_uint(
    uint64_t *dest, // NOLINT(readability-non-const-parameter)
    const uint64_t *src,
    uint64_t n,
    uint64_t v2
)
{
    uint64_t carry = 0;
    __asm__ volatile (
        "test %[n], %[n]\n\t"          // Check if n == 0
        "jz 2f\n"                      // If zero, jump forward to label 2
        "1:\n\t"                       // Loop start
        "mov (%[src]), %%rax\n\t"      // rax = *src
        "mul %[v2]\n\t"                // rdx:rax = rax * v2

        "add %[carry], %%rax\n\t"      // rax += carry
        "adc $0, %%rdx\n\t"            // rdx += overflow from previous addition

        "add %%rax, (%[dest])\n\t"     // *dest += rax
        "adc $0, %%rdx\n\t"            // rdx += overflow from destination addition

        "mov %%rdx, %[carry]\n\t"      // carry = rdx

        "add $8, %[src]\n\t"           // src pointer++ (8 bytes for uint64_t)
        "add $8, %[dest]\n\t"          // dest pointer++
        "dec %[n]\n\t"                 // n--
        "jnz 1b\n"                     // If n != 0, jump backward to label 1
        "2:\n\t"                       // End

        // --- Output Operands ---
        // "+&r" means read/write, and early-clobber (modified before inputs are consumed)
        : [carry] "+&r" (carry),
          [src] "+&r" (src),
          [dest] "+&r" (dest),
          [n] "+&r" (n)

        // --- Input Operands ---
        // "r" means put this in any available general-purpose register
        : [v2] "r" (v2)

        // --- Clobbers ---
        // Tell the compiler we destroy rax, rdx, CPU flags (cc), and memory
        : "rax", "rdx", "cc", "memory"
    );

    return carry;
}

#elifdef __APPLE__

static uint64_t num_ssm_add_mul_uint(
    uint64_t *dest, // NOLINT(readability-non-const-parameter)
    const uint64_t *src,
    uint64_t n,
    uint64_t v2
)
{
    uint64_t carry = 0;
    uint64_t low, high, src_val, dest_val;

    __asm__ volatile (
        "cbz %[n], 2f\n\t"                   // If n == 0, jump to label 2
        "1:\n\t"
        "ldr %[src_val], [%[src]], #8\n\t"   // src_val = *src, then src += 8

        // 128-bit multiplication (ARM64 separates low and high halves)
        "mul %[low], %[src_val], %[v2]\n\t"  // low = (src_val * v2) [bottom 64 bits]
        "umulh %[high], %[src_val], %[v2]\n\t"// high = (src_val * v2) [top 64 bits]

        // Add previous carry
        "adds %[low], %[low], %[carry]\n\t"  // low += carry (updates CF)
        "adc %[high], %[high], xzr\n\t"      // high += CF (using zero register xzr)

        // Add to destination
        "ldr %[dest_val], [%[dest]]\n\t"     // dest_val = *dest (no post-increment yet)
        "adds %[low], %[low], %[dest_val]\n\t"// low += dest_val (updates CF)
        "str %[low], [%[dest]], #8\n\t"      // *dest = low, then dest += 8

        // Calculate new carry
        "adc %[carry], %[high], xzr\n\t"     // carry = high + CF

        // Loop mechanics
        "sub %[n], %[n], #1\n\t"             // n--
        "cbnz %[n], 1b\n\t"                  // Loop if n != 0
        "2:\n\t"

        // --- Output Operands ---
        : [carry] "+&r" (carry),
          [src] "+&r" (src),
          [dest] "+&r" (dest),
          [n] "+&r" (n),
          [low] "=&r" (low),
          [high] "=&r" (high),
          [src_val] "=&r" (src_val),
          [dest_val] "=&r" (dest_val)
        : [v2] "r" (v2)
        : "cc", "memory"
    );

    return carry;
}

#endif

// num_aux->size >= 2 * n
static void num_ssm_mul_mod_span(
    num_p num_aux,
    num_p num_1,
    num_p num_2,
    uint64_t pos,
    uint64_t n
)
{
    CLU_HANDLER_IS_SAFE(num_aux)
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_aux && num_1 && num_2)
    assert(num_aux->size >= 2 * n)

    uint64_t * restrict dest = num_aux->chunk;
    const uint64_t * restrict src1 = &num_1->chunk[pos];
    const uint64_t * restrict src2 = &num_2->chunk[pos];

    memset(dest, 0, 2 * n * sizeof(uint64_t));
    for(uint64_t i = 0; i < n; i++)
    {
        uint64_t v2 = src2[i];
#if defined(__linux__) || defined(__APPLE__)

        dest[i + n] = num_ssm_add_mul_uint(&dest[i], src1, n, v2);

#else

        uint64_t carry = 0;
        for(uint64_t j = 0; j < n; j++)
        {
            uint64_t dest_idx = i + j;
            uint128_t u = MUL(src1[j], v2);

            uint64_t sum;
            uint64_t c1 = (uint64_t)__builtin_add_overflow(LOW(u), dest[dest_idx], &sum);
            uint64_t c2 = (uint64_t)__builtin_add_overflow(sum, carry, &dest[dest_idx]);

            carry = HIGH(u) + c1 + c2;
        }
        dest[i + n] = carry;

#endif
    }

    memmove(&dest[n], &dest[n-1], n * sizeof(uint64_t));
    dest[n-1] = 0;

    // NOLINTNEXTLINE(readability-suspicious-call-argument)
    num_ssm_sub_mod(num_1, pos, num_aux, 0, num_aux, n, n);
}



STATIC void num_ssm_pad_wrap(num_p num_fft, num_p num, uint64_t pos, ssm_params_p p)
{
    CLU_HANDLER_IS_SAFE(num_fft)
    CLU_HANDLER_IS_SAFE(num)
    assert(num_fft)
    assert(num)
    assert(num->size >= pos + p->count)
    assert(num_fft->size >= p->n * p->K)

    uint64_t * restrict dest = num_fft->chunk;
    const uint64_t * restrict src = &num->chunk[pos];

    memset(dest, 0, p->n * p->K * sizeof(uint64_t));
    for(uint64_t i=0; i < p->K; i++)
    {
        memcpy(&dest[p->n * i], &src[p->M * i], p->M * sizeof(uint64_t));
    }

    dest[(p->n * (p->K - 1)) + p->M] = src[p->count - 1];
}

// num_aux_1->size >= n
// num_aux_2->size >= 2 * n
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
STATIC void num_ssm_depad_wrap(
    num_p num_aux_1,
    num_p num_aux_2,
    num_p num_res,
    uint64_t pos,
    num_p num_fft,
    ssm_params_p p
)
{
    CLU_HANDLER_IS_SAFE(num_aux_1)
    CLU_HANDLER_IS_SAFE(num_aux_2)
    CLU_HANDLER_IS_SAFE(num_res)
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_res->size >= pos + p->count)
    assert(num_aux_1 && num_aux_2 && num_res && num_fft)

    uint64_t n = p->count;
    assert(num_aux_1->size >= p->count && num_aux_2->size >= 2 * n)

    uint64_t wrap_boundary = n - 1;

    memset(&num_res->chunk[pos], 0, n * sizeof(uint64_t));
    for(uint64_t i=0; i<p->K; i++)
    {
        uint64_t dest_pos = i * p->M;
        uint64_t src_pos = i * p->n;
        uint64_t src_len = p->n;

        bool is_add = num_ssm_cmp_uint_offset(num_fft, src_pos + (2 * p->M), i + 1, p->n - (2 * p->M)) < 0;

        if(!is_add)
        {
            num_ssm_opposite(num_fft, src_pos, src_len);
        }

        uint64_t non_wrap_len = (wrap_boundary > dest_pos) ? (wrap_boundary - dest_pos) : 0;
        if (non_wrap_len > src_len)
        {
            non_wrap_len = src_len;
        }
        uint64_t wrap_len = src_len - non_wrap_len;
        uint64_t tail_1 = dest_pos + non_wrap_len;

        if (dest_pos > 0)
        {
            memset(&num_aux_1->chunk[0], 0, dest_pos * sizeof(uint64_t));
        }
        if (non_wrap_len > 0)
        {
            memcpy(&num_aux_1->chunk[dest_pos], &num_fft->chunk[src_pos], non_wrap_len * sizeof(uint64_t));
        }
        if (n > tail_1)
        {
            memset(&num_aux_1->chunk[tail_1], 0, (n - tail_1) * sizeof(uint64_t));
        }

        if (wrap_len > 0)
        {
            memcpy(&num_aux_2->chunk[0], &num_fft->chunk[src_pos + non_wrap_len], wrap_len * sizeof(uint64_t));
            if (n > wrap_len)
            {
                memset(&num_aux_2->chunk[wrap_len], 0, (n - wrap_len) * sizeof(uint64_t));
            }
            num_ssm_sub_mod_immed(num_aux_1, 0, num_aux_2, 0, n);
        }

        // 3. Recombination
        if(is_add)
        {
            num_ssm_add_mod_immed(num_res, pos, num_aux_1, 0, n);
        }
        else
        {
            num_ssm_sub_mod_immed(num_res, pos, num_aux_1, 0, n);
        }
    }
}

// num_aux->size >= 2 * n
static void num_ssm_prepare_wrap(
    num_p num_aux,
    num_p num_fft,
    num_p num,
    uint64_t pos,
    ssm_params_p p
)
{
    CLU_HANDLER_IS_SAFE(num_aux)
    CLU_HANDLER_IS_SAFE(num_fft)
    CLU_HANDLER_IS_SAFE(num)
    assert(num_aux)
    assert(num_fft)
    assert(num)
    assert(num_aux->size >= 2 * p->n)

    num_ssm_pad_wrap(num_fft, num, pos, p);
    num_ssm_fft_fwd(num_aux, num_fft, p);
}

static void num_ssm_mul_pointwise(
    num_p num_aux_1,
    num_p num_aux_2,
    num_p num_fft_1,
    num_p num_fft_2,
    ssm_params_p p
);

// KEEPS NUM_1 NUM_2
STATIC void num_ssm_mul_wrap(
    num_p num_aux_1,
    num_p num_aux_2,
    num_p num_fft_1,
    num_p num_fft_2,
    num_p num_1,
    num_p num_2,
    uint64_t pos,
    ssm_params_p p
)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    num_ssm_prepare_wrap(num_aux_2, num_fft_1, num_1, pos, p);
    num_ssm_prepare_wrap(num_aux_2, num_fft_2, num_2, pos, p);

    num_ssm_mul_pointwise(
        num_aux_1,
        num_aux_2,
        num_fft_1,
        num_fft_2,
        p
    );

    num_ssm_fft_inv(num_aux_2, num_fft_1, p);
    num_ssm_depad_wrap(
        num_aux_1,
        num_aux_2,
        num_1,
        pos,
        num_fft_1,
        p
    );
}



// num_aux_1->size >= p->n
// num_aux_2->size >= 2 * p->n
static void num_ssm_mul_pointwise(
    num_p num_aux_1,
    num_p num_aux_2,
    num_p num_fft_1,
    num_p num_fft_2,
    ssm_params_p p
)
{
    CLU_HANDLER_IS_SAFE(num_aux_1)
    CLU_HANDLER_IS_SAFE(num_aux_2)
    CLU_HANDLER_IS_SAFE(num_fft_1)
    CLU_HANDLER_IS_SAFE(num_fft_2)
    assert(num_aux_1)
    assert(num_aux_2)
    assert(num_fft_1)
    assert(num_fft_2)
    assert(num_aux_1->size >= p->n)
    assert(num_aux_2->size >= 2 * p->n)

    if(!ssm_is_recursive(p->n))
    {
        for(uint64_t i=0; i<p->K; i++)
        {
            num_ssm_mul_mod_span(num_aux_2, num_fft_1, num_fft_2, i * p->n, p->n);
        }
        return;
    }

    ssm_params_t p_next = ssm_get_params_wrap(p->n);
    num_p num_fft_1_next = num_create_dirty(CLU_ARGS(p_next.n * p_next.K, 0));
    num_p num_fft_2_next = num_create_dirty(CLU_ARGS(p_next.n * p_next.K, 0));
    for(uint64_t i=0; i<p->K; i++)
    {
        // NOLINTNEXTLINE(readability-suspicious-call-argument)
        num_ssm_mul_wrap(
            num_aux_1,
            num_aux_2,
            num_fft_1_next,
            num_fft_2_next,
            num_fft_1,
            num_fft_2,
            i * p->n,
            &p_next
        );
    }
    num_free(num_fft_1_next);
    num_free(num_fft_2_next);
}



STATIC num_p num_ssm_pad_no_wrap(num_p num, ssm_params_p p)
{
    CLU_HANDLER_IS_SAFE(num)
    assert(num)

    num_p num_fft = num_create(CLU_ARGS(p->n * p->K, 0));
    uint64_t * restrict dest = num_fft->chunk;
    const uint64_t * restrict src = num->chunk;

    uint64_t full_chunks = num->count / p->M;
    if(full_chunks > p->K)
    {
        full_chunks = p->K;
    }

    for(uint64_t i=0; i < full_chunks; i++)
    {
        memcpy(&dest[p->n * i], &src[p->M * i], p->M * sizeof(uint64_t));
    }

    if(num->count == (p->M * p->K) + 1)
    {
        dest[(p->n * (p->K - 1)) + p->M] = src[num->count - 1];
        return num_fft;
    }

    uint64_t tail = num->count % p->M;
    if(tail)
    {
        memcpy(&dest[p->n * full_chunks], &src[p->M * full_chunks], tail * sizeof(uint64_t));
    }

    return num_fft;
}

STATIC num_p num_ssm_depad_no_wrap(num_p num, ssm_params_p p)
{
    CLU_HANDLER_IS_SAFE(num)
    assert(num)

    uint64_t target_count = (p->M * (p->K - 1)) + p->n;
    num_p num_res = num_create(CLU_ARGS(target_count, 0));

    for(uint64_t i = 0; i < p->K; i++)
    {
        num_t block;
        num_span(&block, num, p->n * i, p->n * (i + 1));
        num_res = num_add_offset(num_res, p->M * i, &block, 0);
    }

    num_free(num);
    return num_res;
}

// num_aux->size >= 2 * n
static num_p num_ssm_prepare_no_wrap(num_p num_aux, num_p num, ssm_params_p p)
{
    CLU_HANDLER_IS_SAFE(num_aux)
    CLU_HANDLER_IS_SAFE(num)
    assert(num_aux)
    assert(num)
    assert(num_aux->size >= 2 * p->n)

    num_p num_fft = num_ssm_pad_no_wrap(num, p);
    num_ssm_fft_fwd(num_aux, num_fft, p);
    return num_fft;
}

// KEEPS NUM_1 NUM_2
STATIC num_p num_mul_ssm(num_p num_1, num_p num_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    ssm_params_t p = ssm_get_params(num_1->count + num_2->count);
    num_p num_aux_1 = num_create_dirty(CLU_ARGS(p.n, 0));
    num_p num_aux_2 = num_create_dirty(CLU_ARGS(2 * p.n, 0));
    num_p num_fft_1 = num_ssm_prepare_no_wrap(num_aux_2, num_1, &p);
    num_p num_fft_2 = num_ssm_prepare_no_wrap(num_aux_2, num_2, &p);

    num_ssm_mul_pointwise(
        num_aux_1,
        num_aux_2,
        num_fft_1,
        num_fft_2,
        &p
    );
    num_free(num_aux_1);
    num_free(num_fft_2);

    num_ssm_fft_inv(num_aux_2, num_fft_1, &p);
    num_free(num_aux_2);

    return num_ssm_depad_no_wrap(num_fft_1, &p);
}




static bool mul_is_classic(uint64_t count_1, uint64_t count_2)
{
    constexpr uint64_t threshold = 256;
    return (bool)((count_1 < threshold) || (count_2 < threshold));
}

// KEEPS NUM_1 NUM_2
STATIC num_p num_mul_core(num_p num_1, num_p num_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    if(num_1->count == 0 || num_2->count == 0)
    {
        return num_wrap(0);
    }

    if(mul_is_classic(num_1->count, num_2->count))
    {
        return num_mul_classic(num_1, num_2);
    }

    return num_mul_ssm(num_1, num_2);
}



static void num_ssm_sqr_mod_span(num_p num_aux, num_p num, uint64_t pos, uint64_t n)
{
    CLU_HANDLER_IS_SAFE(num_aux)
    CLU_HANDLER_IS_SAFE(num)
    assert(num_aux)
    assert(num)
    assert(num_aux->size >= 2 * n)

    num_t num_aux_piece;
    num_span(&num_aux_piece, num, pos, pos + n);

    // TODO: USE ASSEMBLY
    num_sqr_classic_buffer(num_aux, &num_aux_piece);

    memmove(&num_aux->chunk[n], &num_aux->chunk[n-1], n * sizeof(uint64_t));
    num_aux->chunk[n-1] = 0;
    num_ssm_sub_mod(num, pos, num_aux, 0, num_aux, n, n);
}

STATIC num_p num_sqr_ssm(num_p num)
{
    CLU_HANDLER_IS_SAFE(num)
    assert(num)

    uint64_t count = 2 * num->count;
    ssm_params_t p = ssm_get_params(count);
    num_p num_aux_2 = num_create_dirty(CLU_ARGS(2 * p.n, 0));
    num_p num_fft = num_ssm_prepare_no_wrap(num_aux_2, num, &p);
    num_free(num);

    // num_ssm_sqr_pointwise();
    for(uint64_t i=0; i<p.K; i++)
    {
        // TODO: recursive fft
        num_ssm_sqr_mod_span(num_aux_2, num_fft, i * p.n, p.n);
    }

    num_ssm_fft_inv(num_aux_2, num_fft, &p);
    num_free(num_aux_2);

    return num_ssm_depad_no_wrap(num_fft, &p);
}



// Returns quocient
// NUM becomes remainder
num_p num_div_mod_uint(num_p num, uint64_t value)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);
    assert(value);

    num_p num_q = num_create_dirty(CLU_ARGS(num->count, num->count));
    for(uint64_t i = num->count - 1; i != UINT64_MAX; i--)
    {
        if((num->count < i) || ((num->count - 1 == i) && (num->chunk[i] < value)))
        {
            num_q->chunk[i] = 0;
            continue;
        }

        if(num->count - 1 == i)
        {
            uint64_t r = num->chunk[i] / value;
            num_q->chunk[i] = r;
            num = num_sub_uint_offset(num, i, r * value);
            continue;
        }

        uint128_t value_1 = U128HL(num->chunk[i + 1], num->chunk[i]);
        uint64_t r = (uint64_t)(value_1 / value);
        value_1 = MUL(r, value);
        num = num_sub_uint_offset(num, i+1, HIGH(value_1));
        num = num_sub_uint_offset(num, i  , LOW(value_1));
        num_q->chunk[i] = r;
    }
    num_normalize(num_q);
    return num_q;
}

// Input expected to be normalized
// Returns quocient
// NUM_1 becomes remainder
// Keeps NUM_2
// num_aux->size >= num_2+1
static num_p num_div_mod_classic(num_p num_aux, num_p num_1, num_p num_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)
    assert(num_aux->size >= num_2->count+1);
    assert(num_1->count >= num_2->count);

    uint64_t count = num_1->count - num_2->count + 1;
    num_p num_q = num_create_dirty(CLU_ARGS(count, count));
    uint64_t value_2 = num_2->chunk[num_2->count-1];
    for(uint64_t i = count - 1; i != UINT64_MAX; i--)
    {
        if(num_cmp_offset(num_1, i, num_2) < 0)
        {
            num_q->chunk[i] = 0;
            continue;
        }

        if(num_1->count == num_2->count + i)
        {
            num_q->chunk[i] = 1;
            num_1 = num_sub_offset(num_1, i, num_2);
            continue;
        }

        if(num_1->chunk[num_1->count-1] == value_2)
        {
            num_q->chunk[i] = UINT64_MAX;
            num_aux = num_mul_uint_buffer(num_aux, num_2, UINT64_MAX);
            num_1 = num_sub_offset(num_1, i, num_aux);
            continue;
        }

        uint128_t value_1 = U128HL(num_1->chunk[num_1->count-1], num_1->chunk[num_1->count-2]);
        uint64_t r = (uint64_t)(value_1 / value_2);
        num_aux = num_mul_uint_buffer(num_aux, num_2, r);
        while(num_cmp_offset(num_1, i, num_aux) < 0)
        {
            r--;
            num_aux = num_sub_offset(num_aux, 0, num_2);
        }

        num_q->chunk[i] = r;
        num_1 = num_sub_offset(num_1, i, num_aux);
    }

    num_normalize(num_q);
    return num_q;
}

// Input expected to be normalized
// Returns quocient
// NUM_1 becomes remainder
// Keeps NUM_2
static num_p num_div_mod_fallback(num_p num_aux, num_p num_1, num_p num_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)
    assert(num_2->count)

    if(num_cmp(num_1, num_2) < 0)
    {
        return num_create(CLU_ARGS(0, 0));
    }

    if(num_2->count == 1)
    {
        return num_div_mod_uint(num_1, num_2->chunk[0]);
    }

    return num_div_mod_classic(num_aux, num_1, num_2);
}

STRUCT(bz_frame)
{
    bool memoized;
    num_t num_2_1, num_2_0;
};

// Input expected to be normalized
// Returns quocient
// NUM_1 becomes remainder
// Keeps NUM_2
static num_p num_div_mod_bz_rec(
    num_p num_aux,
    num_p num_1,
    num_p num_2,
    bz_frame_t f[],
    bool memoize
)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    if(num_1->count < num_2->count + 2 || num_2->count == 1)
    // if(num_1->count < num_2->count + 128 || num_2->count == 1)
    {
        return num_div_mod_fallback(num_aux, num_1, num_2);
    }

    uint64_t k = num_2->count / 2;
    if(!f->memoized)
    {
        f->memoized = true;

        num_span(&f->num_2_0, num_2, 0, k);
        num_span(&f->num_2_1, num_2, k, num_2->count);
    }

    num_p num_q[2];
    for(uint64_t i=1; i!=UINT64_MAX; i--)
    {
        num_t num_1_1;
        num_span(&num_1_1, num_1, k * (i + 1), num_1->count);
        num_p num_q_tmp = num_div_mod_bz_rec(
            num_aux,
            &num_1_1,
            &f->num_2_1,
            &f[1],
            (bool)(memoize || i)
        );
        num_normalize(num_1);

        if(num_is_zero(num_q_tmp))
        {
            num_q[i] = num_q_tmp;
            continue;
        }

        num_p num_aux_2 = num_mul_core(num_q_tmp, &f->num_2_0);
        while(num_cmp_offset(num_1, k * i, num_aux_2) < 0)
        {
            num_q_tmp = num_sub_uint(num_q_tmp, 1);
            num_1 = num_add_offset(num_1, k * i, num_2, 0);
        }
        num_1 = num_sub_offset(num_1, k * i, num_aux_2);
        num_free(num_aux_2);

        num_q[i] = num_q_tmp;
    }

    num_p num_res = num_add_offset(num_q[0], k, num_q[1], 0);
    num_free(num_q[1]);

    return num_res;
}

// Input expected to be normalized
// Returns quocient
// NUM_1 becomes remainder
// Keeps NUM_2
static num_p num_div_mod_bz(num_p num_1, num_p num_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    bz_frame_t f[chunk_bits];
    memset(f, 0, sizeof(f));

    uint64_t count = stdc_bit_ceil(8 * num_2->count);
    num_p num_aux = num_create(CLU_ARGS(count, 0));
    num_p num_q = num_create(CLU_ARGS(num_1->count - num_2->count + 1, 0));
    num_aux->cannot_expand = true;

    uint64_t n_1 = num_1->count;
    uint64_t n_2 = num_2->count;
    for(; n_1 > 2 * n_2; n_1 -= n_2)
    {
        num_t num_1_1;
        num_span(&num_1_1, num_1, n_1 - (2 * n_2), num_1->count);

        num_p num_q_tmp = num_div_mod_bz_rec(num_aux, &num_1_1, num_2, f, true);
        num_normalize(num_1);
        num_p num_tmp = num_add_offset(num_q_tmp, n_2, num_q, 0);
        num_free(num_q);
        num_q = num_tmp;
    }

    num_p num_q_tmp = num_div_mod_bz_rec(num_aux, num_1, num_2, f, false);
    num_q_tmp = num_add_offset(num_q_tmp, n_1 - n_2, num_q, 0);
    num_free(num_aux);
    num_free(num_q);

    return num_q_tmp;
}

// Forces the biggest chunk of the divident to be > 2^63
uint64_t num_div_normalize(num_p *num_1, num_p *num_2) // TODO TEST
{
    CLU_HANDLER_IS_SAFE(*num_1);
    CLU_HANDLER_IS_SAFE(*num_2);
    assert(*num_1);
    assert(*num_2);

    uint64_t bits = chunk_bits - stdc_bit_width((*num_2)->chunk[(*num_2)->count-1]);
    *num_1 = num_shl_core((*num_1), bits);
    *num_2 = num_shl_core((*num_2), bits);
    return bits;
}

// out_num_q and out_num_r can be nullptr
static void num_div_mod_finalize(
    num_p *out_num_q,
    num_p *out_num_r,
    num_p num_q,
    num_p num_1,
    num_p num_2,
    uint64_t bits
)
{
    num_free(num_2);

    if(out_num_q)
    {
        *out_num_q = num_q;
    }
    else
    {
        num_free(num_q);
    }

    if(out_num_r)
    {
        num_1 = num_shr_core(num_1, bits);
        *out_num_r = num_1;
    }
    else
    {
        num_free(num_1);
    }
}



bool num_is_zero(num_p num)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    return num->count == 0;
}

int64_t num_cmp(num_p num_1, num_p num_2) // TODO TEST
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)

    return num_cmp_offset(num_1, 0, num_2);
}



num_p num_shl(num_p num, uint64_t bits) // TODO TEST
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    if(num->count == 0)
    {
        return num;
    }

    constexpr uint64_t mask = 0x3f;
    num = num_shl_core(num, bits & mask);
    return num_head_grow(num, bits >> chunk_bits_log_2);
}

num_p num_shr(num_p num, uint64_t bits) // TODO TEST
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    if(num->count == 0)
    {
        return num;
    }

    num_head_trim(num, bits >> chunk_bits_log_2);
    constexpr uint64_t mask = 0x3f;
    return num_shr_core(num, bits & mask);
}

num_p num_add_uint(num_p num, uint64_t value)
{
    CLU_HANDLER_IS_SAFE(num)
    assert(num)

    return num_add_uint_offset(num, 0, value);
}

num_p num_sub_uint(num_p num, uint64_t value)
{
    CLU_HANDLER_IS_SAFE(num)
    assert(num)

    return num_sub_uint_offset(num, 0, value);
}

num_p num_mul_uint(num_p num, uint64_t value)
{
    CLU_HANDLER_IS_SAFE(num)
    assert(num)

    num_p num_res = num_create(CLU_ARGS(num->count + 1, 0));
    num_res->cannot_expand = true;
    num_res = num_add_mul_uint_offset(num_res, 0, num, 0, value);
    num_free(num);
    num_res->cannot_expand = false;
    return num_res;
}

num_p num_add(num_p num_1, num_p num_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    num_1 = num_add_offset(num_1, 0, num_2, 0);
    num_free(num_2);
    return num_1;
}

num_p num_sub(num_p num_1, num_p num_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    num_1 = num_sub_offset(num_1, 0, num_2);
    num_free(num_2);
    return num_1;
}

num_p num_mul(num_p num_1, num_p num_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    num_p num_res = num_mul_core(num_1, num_2);
    num_free(num_1);
    num_free(num_2);
    return num_res;
}

num_p num_sqr(num_p num)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    if(num->count == 0)
    {
        return num;
    }

    constexpr uint64_t threshold = 128;
    if(num->count < threshold)
    {
        return num_sqr_classic(num);
    }

    return num_sqr_ssm(num);
}

num_p num_pow(num_p num, uint64_t value) // TODO TEST
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    if(num->count == 0)
    {
        assert(value);
        return num;
    }

    num_p num_res = num_wrap(1);
    for(uint64_t mask = B(63); mask; mask >>= 1)
    {
        num_res = num_sqr(num_res);
        if(value & mask)
        {
            num_res = num_mul(num_res, num_copy(num));
        }
    }
    num_free(num);
    return num_res;
}

// out_num_q and out_num_r can be nullptr
void num_div_mod(num_p *out_num_q, num_p *out_num_r, num_p num_1, num_p num_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    assert(num_2->count);

    if(num_cmp(num_1, num_2) < 0)
    {
        num_p num_q = num_create(CLU_ARGS(0, 0));
        num_div_mod_finalize(out_num_q, out_num_r, num_q, num_1, num_2, 0);
        return;
    }

    if(num_2->count == 1)
    {
        num_p num_q = num_div_mod_uint(num_1, num_2->chunk[0]);
        num_div_mod_finalize(out_num_q, out_num_r, num_q, num_1, num_2, 0);
        return;
    }

    uint64_t bits = num_div_normalize(&num_1, &num_2);
    num_p num_q = num_div_mod_bz(num_1, num_2);
    num_div_mod_finalize(out_num_q, out_num_r, num_q, num_1, num_2, bits);
}

num_p num_div(num_p num_1, num_p num_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    num_p num_q;
    num_div_mod(&num_q, nullptr, num_1, num_2);
    return num_q;
}

num_p num_mod(num_p num_1, num_p num_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    num_div_mod(nullptr, &num_1, num_1, num_2);
    return num_1;
}

num_p num_gcd(num_p num_1, num_p num_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    while(num_2->count != 0)
    {
        num_p num_aux = num_mod(num_1, num_copy(num_2));
        num_1 = num_2;
        num_2 = num_aux;
    }
    num_free(num_2);
    return(num_1);
}



static num_p num_base_to_rec(num_p num, num_p num_bases[], uint64_t i)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    if(i == UINT64_MAX)
    {
        return num;
    }

    if(num_cmp(num, num_bases[i]) < 0)
    {
        return num_base_to_rec(num, num_bases, i - 1);
    }

    num_p num_q, num_r;
    num_div_mod(&num_q, &num_r, num, num_copy(num_bases[i]));
    num_q = num_base_to_rec(num_q, num_bases, i - 1);
    num_r = num_base_to_rec(num_r, num_bases, i - 1);
    num_r = num_add_offset(num_r, B(i), num_q, 0);
    num_free(num_q);
    return num_r;
}

num_p num_base_to(num_p num, uint64_t base)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);
    assert(base > 1);

    constexpr uint64_t len = 100;
    num_p num_base = num_wrap(base);
    num_p num_bases[len];

    uint64_t max;
    for(max=0; num_cmp(num_base, num) <= 0; max++)
    {
        num_bases[max] = num_copy(num_base);
        num_base = num_sqr(num_base);
    }
    num_free(num_base);


    if(max == 0)
    {
        return num;
    }

    num_p num_res = num_base_to_rec(num, num_bases, max - 1);

    for(uint64_t i=0; i<max; i++)
    {
        num_free(num_bases[i]);
    }

    return num_res;
}

num_p num_base_from(num_p num, uint64_t base)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);
    assert(base > 1);

    num_p num_res = num_create(CLU_ARGS(num->count, 0));
    for(uint64_t i=num->count-1; i!=UINT64_MAX; i--)
    {
        assert(num->chunk[i] < base);
        num_res = num_mul_uint(num_res, base);
        num_res = num_add_uint(num_res, num->chunk[i]);
    }
    num_free(num);
    return num_res;
}
