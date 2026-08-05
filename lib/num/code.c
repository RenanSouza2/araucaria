#include <stdint.h>
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

uint64_t rand_64()
{
    // NOLINTNEXTLINE(readability-magic-numbers)
    return (U64(rand_32()) << 32) | rand_32();
}

uint64_t rand_64_range(uint64_t min, uint64_t max)
{
    assert(min < max);
    uint64_t range = max - min;
    return (rand_64() % range) + min;
}



num_p num_create_variadic(uint64_t n, va_list *args)
{
    num_p num = num_create(CLU_ARGS(n, n));
    for(uint64_t i=0; i<n; i++)
    {
        num->chunk[n-1-i] = va_arg(*args, uint64_t);
    }

    return num;
}

num_p num_create_immed(uint64_t n, ...)
{
    va_list args;
    va_start(args, n);
    return num_create_variadic(n, &args);
}

num_p num_create_rand(uint64_t count)
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



bool int64(int64_t i1, int64_t i2)
{
    if(i1 != i2)
    {
        printf("\n\n\tINT64 ASSERT ERROR\t| (" D64P() ") (" D64P() ")", i1, i2);
        return false;
    }

    return true;
}

bool uint64(uint64_t u1, uint64_t u2)
{
    if(u1 != u2)
    {
        printf("\n\n\tUINT64 ASSERT ERROR\t| (" U64PX ") (" U64PX ")", u1, u2);
        return false;
    }

    return true;
}

bool uint128_immed(uint128_t u1, uint64_t v2h, uint64_t v2l)
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



bool num_keep(num_p num_1, num_p num_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

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

bool num_eq_dbg(num_p num_1, num_p num_2)
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

bool num_immed(num_p num, uint64_t n, ...)
{
    va_list args;
    va_start(args, n);
    num_p num_2 = num_create_variadic(n, &args);
    return num_eq_dbg(num, num_2);
}

#endif



uint64_t uint_from_char(char c)
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



static araucaria_disk_config_t s_araucaria_disk_config = {
    .disk_threshold = UINT64_MAX,
    .is_set = false
};

void araucaria_disk_config_set(araucaria_disk_config_p config)
{
    s_araucaria_disk_config = *config;
    s_araucaria_disk_config.is_set = true;
}

bool araucaria_disk_config_is_set()
{
    return s_araucaria_disk_config.is_set;
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
    assert(s_araucaria_disk_config.is_set);

    constexpr uint64_t path_max = 1024;
    char template_path[path_max];
    snprintf(template_path, sizeof(template_path), "%s/bignum_XXXXXX", s_araucaria_disk_config.disk_path);
    int fd = mkstemp(template_path);
    assert(fd != -1);
    unlink(template_path);

    uint64_t total_size = sizeof(num_t) + (size * sizeof(uint64_t));
    int res = ftruncate(fd, (off_t)total_size);
    assert(res == 0);
    num_p num = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    assert(num != MAP_FAILED);

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

    if(size > s_araucaria_disk_config.disk_threshold)
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

    if(size > s_araucaria_disk_config.disk_threshold)
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

num_p num_realloc_disk(CLU_PARAMS(num_p num))
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    if(num->is_mmap)
    {
        return num;
    }

    num_p num_res = num_create_disk(CLU_ARGS_RELAY(num->count, num->count));
    memcpy(num_res->chunk, num->chunk, num->count * sizeof(uint64_t));
    num_free(num);
    return num_res;
}

num_p num_expand_to(num_p num, uint64_t size)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    if(size <= num->size)
    {
        return num;
    }

    uint64_t size_old = num->size;

    num_p num_new = num_create_dirty(CLU_ARGS(size, num->count));
    memcpy(num_new->chunk, num->chunk, size_old * sizeof(uint64_t));
    memset(&num_new->chunk[size_old], 0, (size - size_old) * sizeof(uint64_t));
    num_free(num);
    return num_new;
}

static void num_clear(num_p num)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    num->count = 0;
    memset(num->chunk, 0, num->size * sizeof(uint64_t));
}

num_p num_normalize(num_p num)
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
        num_clear(num);
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
        .chunk = &num->chunk[pos_init]
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
        return num_wrap(U64(value));
    }

    num_p num = num_create_dirty(CLU_ARGS(2, 2));
    num->chunk[0] = LOW(value);
    num->chunk[1] = HIGH(value);
    return num;
}

num_p num_wrap_dec(const char str[])
{
    constexpr uint64_t dec_base = 10;
    constexpr uint64_t chunk_len = 18;
    constexpr uint64_t chunk_base = 1'000'000'000'000'000'000;


    uint64_t len = strlen(str);
    uint64_t pos = len / chunk_len;
    uint64_t extra = len % chunk_len != 0;
    num_p num = num_create(CLU_ARGS(pos + extra, pos + extra));

    if(extra)
    {
        uint64_t value = uint_from_str(str, len % chunk_len, dec_base);
        num->chunk[pos] = value;
    }

    for(uint64_t i=len % chunk_len; i<len; i+=chunk_len)
    {
        uint64_t value = uint_from_str(&str[i], chunk_len, dec_base);
        pos--;
        num->chunk[pos] = value;
    }

    num_normalize(num);
    return num_base_from(num, chunk_base);
}

num_p num_wrap_hex(const char str[])
{
    uint64_t len = strlen(str);
    assert(len > 1 && str[0] == '0' && str[1] == 'x');

    constexpr uint64_t chars_per_chunk = 16;
    uint64_t pos = (len - 2) / chars_per_chunk;
    uint64_t extra = (len - 2) % chars_per_chunk != 0;
    num_p num = num_create(CLU_ARGS(pos + extra, pos + extra));

    if(extra)
    {
        uint64_t value = uint_from_str(&str[2], (len - 2) % chars_per_chunk, chars_per_chunk);
        num->chunk[pos] = value;
    }

    for(uint64_t i = 2 + ((len - 2) % chars_per_chunk); i < len; i += chars_per_chunk)
    {
        uint64_t value = uint_from_str(&str[i], chars_per_chunk, chars_per_chunk);
        pos--;
        num->chunk[pos] = value;
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
    return U64(res);
}

num_p num_read_dec(const char file_name[])
{
    constexpr uint64_t dec_base = 10;
    constexpr uint64_t chunk_len = 18;
    constexpr uint64_t chunk_base = 1'000'000'000'000'000'000;

    FILE *fp = fopen(file_name, "r");
    assert(fp);

    int res = fseek(fp, 0, SEEK_END);
    assert(!res);
    uint64_t size = get_ftell(fp);
    res = fseek(fp, 0, SEEK_SET);
    assert(!res);

    uint64_t pos = size / chunk_len;
    uint64_t extra = size % chunk_len;

    num_p num = num_create(CLU_ARGS(pos + extra, pos + extra));
    if(extra)
    {
        uint64_t value = uint_read(fp, size % chunk_len, dec_base);
        num->chunk[pos] = value;
    }

    while(get_ftell(fp) < size)
    {
        uint64_t value = uint_read(fp, chunk_len, dec_base);
        pos--;
        num->chunk[pos] = value;
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
    int res = munmap(num, total_size);
    assert(res == 0);
    CLU_HANDLER_UNREGISTER(num)
}



void num_add_uint_offset(num_p num, uint64_t pos, uint64_t value)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    if(value == 0)
    {
        return;
    }

    if(pos == num->count)
    {
        assert(num->size > num->count);
        num->chunk[pos] = value;
        num->count = pos + 1;
        return;
    }

    assert(pos < num->count);

    uint64_t count = num->count;
    uint64_t * restrict chunk = num->chunk;

    uint128_t carry = value;
    for(uint64_t i = pos; i < count && carry; i++)
    {
        uint128_t sum = U128(chunk[i]) + carry;
        chunk[i] = LOW(sum);
        carry = HIGH(sum);
    }

    if(carry)
    {
        assert(num->size > num->count);
        chunk[num->count] = LOW(carry);
        num->count++;
    }
}

void num_sub_uint_offset(num_p num, uint64_t pos, uint64_t value)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    uint128_t borrow = value;

    uint64_t count = num->count;
    uint64_t * restrict chunk = num->chunk;

    for(uint64_t i = pos; i < count && borrow; i++)
    {
        uint128_t diff = U128(chunk[i]) - borrow;
        chunk[i] = LOW(diff);
        borrow = HIGH(diff) & 1;
    }
    assert(borrow == 0);

    num_normalize(num);
}

// BITS shoud be less than 64
void num_shl_core(num_p num, uint64_t bits) // TODO test
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);
    assert(bits < 64); // TODO test

    if(bits == 0)
    {
        return;
    }

    uint64_t count = num->count;
    uint64_t * restrict chunk = num->chunk;

    uint64_t carry = 0;
    for(uint64_t i = 0; i < count; i++)
    {
        uint64_t value = chunk[i];
        chunk[i] = (value << bits) | carry;
        carry = value >> (chunk_bits - bits);
    }

    if(carry)
    {
        assert(num->size > count);
        chunk[count] = carry;
        num->count = count + 1;
    }
}

// BITS shoud be less than 64
void num_shr_core(num_p num, uint64_t bits) // TODO test
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);
    assert(bits < 64); // TODO test

    if(bits == 0)
    {
        return;
    }

    uint64_t count = num->count;
    uint64_t * restrict chunk = num->chunk;
    uint64_t carry = 0;

    for(uint64_t i = count - 1; i != UINT64_MAX; i--)
    {
        uint64_t value = chunk[i];
        chunk[i] = (value >> bits) | carry;
        carry = value << (chunk_bits - bits);
    }

    num_normalize(num);
}


int64_t num_cmp_offset(num_p num_1, uint64_t pos_1, num_p num_2) // TODO TEST
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    uint64_t count_1 = num_1->count;
    uint64_t count_2 = num_2->count;

    if(count_1 > count_2 + pos_1)
    {
        return 1;
    }

    if(count_1 < count_2 + pos_1)
    {
        return -1;
    }

    const uint64_t * restrict chunk_1 = num_1->chunk;
    const uint64_t * restrict chunk_2 = num_2->chunk;

    for(uint64_t i = count_2 - 1; i != UINT64_MAX; i--)
    {
        uint64_t value_1 = chunk_1[pos_1 + i];
        uint64_t value_2 = chunk_2[i];

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
static void num_add_offset(num_p num_1, uint64_t pos_1, num_p num_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    if(num_2->count == 0)
    {
        return;
    }

    uint64_t count_src = num_2->count;
    uint64_t count_max = pos_1 + count_src;
    assert(num_1->size >= count_max);

    if(num_1->count < count_max)
    {
        num_1->count = count_max;
    }

    uint64_t * restrict dest = num_1->chunk;
    const uint64_t * restrict src = num_2->chunk;
    uint128_t carry = 0;

    #pragma GCC unroll 32
    for(uint64_t i = 0; i < count_src; i++)
    {
        carry += U128(src[i]) + dest[pos_1 + i];
        dest[pos_1 + i] = LOW(carry);
        carry = HIGH(carry);
    }

    if(carry)
    {
        num_add_uint_offset(num_1, count_max, LOW(carry));
    }
}

// keeps NUM2
void num_sub_offset(num_p num_1, uint64_t pos_1, num_p num_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    uint64_t count_src = num_2->count;
    assert(num_1->count >= count_src + pos_1);

    uint64_t * restrict dest = num_1->chunk;
    const uint64_t * restrict src = num_2->chunk;
    uint128_t borrow = 0;

    #pragma GCC unroll 32
    for(uint64_t i = 0; i < count_src; i++)
    {
        uint128_t diff = U128(dest[pos_1 + i]) - src[i] - borrow;
        dest[pos_1 + i] = LOW(diff);
        borrow = HIGH(diff) & 1;
    }

    if(borrow)
    {
        num_sub_uint_offset(num_1, pos_1 + count_src, LOW(borrow));
    }

    num_normalize(num_1);
}



// preserves NUM
// num_res->size >= num->count + 1
// r is not zero
static void num_mul_uint_buffer(num_p num_res, num_p num, uint64_t value) // TODO TEST
{
    CLU_HANDLER_IS_SAFE(num_res)
    CLU_HANDLER_IS_SAFE(num)
    assert(num_res)
    assert(num)
    assert(num_res->size >= num->count + 1)

    uint64_t count = num->count;
    uint64_t * restrict dest = num_res->chunk;
    const uint64_t * restrict src = num->chunk;

    num_res->count = count + 1;

    uint128_t carry = 0;
    #pragma GCC unroll 32
    for(uint64_t i = 0; i < count; i++)
    {
        carry += MUL(src[i], value);
        dest[i] = LOW(carry);
        carry = HIGH(carry);
    }
    dest[count] = LOW(carry);

    num_normalize(num_res);
}

// The kernels below are picked by what the toolchain can actually build, not by
// operating system. Keying them off __linux__ / __APPLE__ handed the x86-64 blocks to a
// Linux aarch64 build and the AArch64 blocks to a macOS x86-64 build, and neither of
// those assembles at all; both are ordinary hosts today. What each block really needs:
//
//   x86-64  BMI2 (mulx) and ADX (adcx / adox), which -march=native only defines on
//           Broadwell and later, plus gcc: the templates are .intel_syntax noprefix but
//           operands expand to gcc's AT&T register spelling, which GNU as tolerates and
//           clang's integrated assembler rejects outright.
//   AArch64 nothing past the base ISA, and either compiler.
//
// So every host now lands on a path it can build, and each block is shared by both
// operating systems instead of one each.
#if !defined(NO_ASSEMBLY) && defined(__x86_64__) && defined(__BMI2__) && defined(__ADX__) && defined(__GNUC__) && !defined(__clang__)
    #define NUM_ASM_X86_64
#elif !defined(NO_ASSEMBLY) && defined(__aarch64__)
    #define NUM_ASM_AARCH64
#endif

#ifdef NUM_ASM_X86_64

#define ADD_CLASSIC_STEP(OFF, REG)                                                                    \
    "mov %[" #REG "], [%[src_2] + %[pos] + " #OFF "]    \n\t" /* REG  = *(src_2 + pos + OFF)        */\
    "adcx %[" #REG "], [%[dest] + %[pos] + " #OFF "]    \n\t" /* REG += *(dest + pos + OFF) + CF    */\
    "mov [%[dest] + %[pos] + " #OFF "], %[" #REG "]     \n\t" /* *(dest + pos + OFF) = REG          */\

#define SUB_CLASSIC_STEP(OFF, SRC_1, REG)                                                                 \
    "mov %[" #REG "], [%[" #SRC_1"] + %[pos] + " #OFF "]    \n\t" /* REG  = *(SRC_1 + pos + OFF)        */\
    "sbb %[" #REG "], [%[src_2] + %[pos] + " #OFF "]        \n\t" /* REG -= *(src_2 + pos + OFF) + CF   */\
    "mov [%[dest] + %[pos] + " #OFF "], %[" #REG "]         \n\t" /* *(dest + pos + OFF) = REG          */\

#define MUL_CLASSIC_STEP_ZERO(OFF, HIGH, CARRY, POS)                                                                      \
    "mulx %[" #HIGH "], %[low], [%[src_2] + %[" #POS "] + " #OFF "] \n\t" /* (HIGH, low) = MUL(D, *(src_2 + POS + OFF)) */\
    "adcx %[low], %[" #CARRY "]                                     \n\t" /* low += carry + CF                          */\
    "mov [%[dest] + %[" #POS "] + " #OFF "], %[low]                 \n\t" /* *(dest + POS + OFF) = low                  */\

#define MUL_CLASSIC_STEP(OFF, HIGH, CARRY, SRC, POS)                                                                          \
    "mulx %[" #HIGH "], %[low], [%[" #SRC "] + %[" #POS "] + " #OFF "]  \n\t" /* (HIGH, low) = MUL(D, *(src + POS + OFF))   */\
    "adcx %[low], [%[dest] + %[" #POS "] + " #OFF "]                    \n\t" /* low += *(dest + POS + OFF) + CF            */\
    "adox %[low], %[" #CARRY "]                                         \n\t" /* low += carry + OF                          */\
    "mov [%[dest] + %[" #POS "] + " #OFF "], %[low]                     \n\t" /* *(dest + POS + OFF) = low                  */\

#endif

// KEEPS NUM_1 NUM_2
num_p num_mul_classic(num_p num_1, num_p num_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1 && num_2)

    uint64_t count_1 = num_1->count;
    uint64_t count_2 = num_2->count;

    if (count_1 == 0 || count_2 == 0)
    {
        return num_create(CLU_ARGS(0, 0));
    }

    uint64_t target_count = count_1 + count_2;
    num_p num_res = num_create_dirty(CLU_ARGS(target_count, target_count));
    uint64_t * restrict dest = num_res->chunk;
    const uint64_t * restrict src_1 = num_1->chunk;
    const uint64_t * restrict src_2 = num_2->chunk;

#ifdef NUM_ASM_X86_64

    uint64_t high, low;
    uint64_t carry, pos;
    uint64_t j;
    uint64_t i=count_1;
    uint64_t zero = 0;

    __asm__ __volatile__ (
        ".intel_syntax noprefix                         \n\t"

        "mov %[j], %[count_2]                           \n\t" // j = count_2
        "shr %[j], 5                                    \n\t" // j /= 32
        "mov rdx, [%[src_1]]                            \n\t" // D = *src_1
        "mov %[carry], 0                                \n\t" // carry = 0
        "xor %[pos], %[pos]                             \n\t" // pos = 0
        "test %[j], %[j]                                \n\t"
        "jz loop_0_skip%=                               \n\t"

        "loop_0_begin%=:                                \n\t" // LOOP_0_BEGIN

        MUL_CLASSIC_STEP_ZERO(  0, high, carry, pos)
        MUL_CLASSIC_STEP_ZERO(  8, carry, high, pos)
        MUL_CLASSIC_STEP_ZERO( 16, high, carry, pos)
        MUL_CLASSIC_STEP_ZERO( 24, carry, high, pos)
        MUL_CLASSIC_STEP_ZERO( 32, high, carry, pos)
        MUL_CLASSIC_STEP_ZERO( 40, carry, high, pos)
        MUL_CLASSIC_STEP_ZERO( 48, high, carry, pos)
        MUL_CLASSIC_STEP_ZERO( 56, carry, high, pos)
        MUL_CLASSIC_STEP_ZERO( 64, high, carry, pos)
        MUL_CLASSIC_STEP_ZERO( 72, carry, high, pos)
        MUL_CLASSIC_STEP_ZERO( 80, high, carry, pos)
        MUL_CLASSIC_STEP_ZERO( 88, carry, high, pos)
        MUL_CLASSIC_STEP_ZERO( 96, high, carry, pos)
        MUL_CLASSIC_STEP_ZERO(104, carry, high, pos)
        MUL_CLASSIC_STEP_ZERO(112, high, carry, pos)
        MUL_CLASSIC_STEP_ZERO(120, carry, high, pos)
        MUL_CLASSIC_STEP_ZERO(128, high, carry, pos)
        MUL_CLASSIC_STEP_ZERO(136, carry, high, pos)
        MUL_CLASSIC_STEP_ZERO(144, high, carry, pos)
        MUL_CLASSIC_STEP_ZERO(152, carry, high, pos)
        MUL_CLASSIC_STEP_ZERO(160, high, carry, pos)
        MUL_CLASSIC_STEP_ZERO(168, carry, high, pos)
        MUL_CLASSIC_STEP_ZERO(176, high, carry, pos)
        MUL_CLASSIC_STEP_ZERO(184, carry, high, pos)
        MUL_CLASSIC_STEP_ZERO(192, high, carry, pos)
        MUL_CLASSIC_STEP_ZERO(200, carry, high, pos)
        MUL_CLASSIC_STEP_ZERO(208, high, carry, pos)
        MUL_CLASSIC_STEP_ZERO(216, carry, high, pos)
        MUL_CLASSIC_STEP_ZERO(224, high, carry, pos)
        MUL_CLASSIC_STEP_ZERO(232, carry, high, pos)
        MUL_CLASSIC_STEP_ZERO(240, high, carry, pos)
        MUL_CLASSIC_STEP_ZERO(248, carry, high, pos)

        "lea %[pos], [%[pos] + 256]                     \n\t" // pos += 256
        "dec %[j]                                       \n\t" // j--
        "jnz loop_0_begin%=                             \n\t"

        "adcx %[carry], %[zero]                         \n\t" // carry += CF
        "mov [%[dest] + %[pos]], %[carry]               \n\t" // *(dest + pos) = carry

        "lea %[src_1], [%[src_1] + 8]                   \n\t" // src_1 += 8
        "lea %[dest], [%[dest] + 8]                     \n\t" // dest += 8
        "dec %[i]                                       \n\t" // i--
        "jz loop_1_skip%=                               \n\t" // <--- ADDED: Skip loop 1 if i == 0

        "loop_1_begin%=:                                \n\t"

        "mov %[j], %[count_2]                           \n\t" // j = count_2
        "mov rdx, [%[src_1]]                            \n\t" // D = *src_1
        "shr %[j], 5                                    \n\t" // j /= 32
        "mov %[carry], 0                                \n\t" // carry = 0
        "xor %[pos], %[pos]                             \n\t" // pos = 0
        "test %[j], %[j]                                \n\t"
        "jz loop_2_end%=                                \n\t"

        "loop_2_begin%=:                                \n\t"

        MUL_CLASSIC_STEP(  0, high, carry, src_2, pos)
        MUL_CLASSIC_STEP(  8, carry, high, src_2, pos)
        MUL_CLASSIC_STEP( 16, high, carry, src_2, pos)
        MUL_CLASSIC_STEP( 24, carry, high, src_2, pos)
        MUL_CLASSIC_STEP( 32, high, carry, src_2, pos)
        MUL_CLASSIC_STEP( 40, carry, high, src_2, pos)
        MUL_CLASSIC_STEP( 48, high, carry, src_2, pos)
        MUL_CLASSIC_STEP( 56, carry, high, src_2, pos)
        MUL_CLASSIC_STEP( 64, high, carry, src_2, pos)
        MUL_CLASSIC_STEP( 72, carry, high, src_2, pos)
        MUL_CLASSIC_STEP( 80, high, carry, src_2, pos)
        MUL_CLASSIC_STEP( 88, carry, high, src_2, pos)
        MUL_CLASSIC_STEP( 96, high, carry, src_2, pos)
        MUL_CLASSIC_STEP(104, carry, high, src_2, pos)
        MUL_CLASSIC_STEP(112, high, carry, src_2, pos)
        MUL_CLASSIC_STEP(120, carry, high, src_2, pos)
        MUL_CLASSIC_STEP(128, high, carry, src_2, pos)
        MUL_CLASSIC_STEP(136, carry, high, src_2, pos)
        MUL_CLASSIC_STEP(144, high, carry, src_2, pos)
        MUL_CLASSIC_STEP(152, carry, high, src_2, pos)
        MUL_CLASSIC_STEP(160, high, carry, src_2, pos)
        MUL_CLASSIC_STEP(168, carry, high, src_2, pos)
        MUL_CLASSIC_STEP(176, high, carry, src_2, pos)
        MUL_CLASSIC_STEP(184, carry, high, src_2, pos)
        MUL_CLASSIC_STEP(192, high, carry, src_2, pos)
        MUL_CLASSIC_STEP(200, carry, high, src_2, pos)
        MUL_CLASSIC_STEP(208, high, carry, src_2, pos)
        MUL_CLASSIC_STEP(216, carry, high, src_2, pos)
        MUL_CLASSIC_STEP(224, high, carry, src_2, pos)
        MUL_CLASSIC_STEP(232, carry, high, src_2, pos)
        MUL_CLASSIC_STEP(240, high, carry, src_2, pos)
        MUL_CLASSIC_STEP(248, carry, high, src_2, pos)

        "adox %[carry], %[zero]                         \n\t" // carry += OF

        "lea %[pos], [%[pos] + 256]                     \n\t" // pos += 256
        "dec %[j]                                       \n\t" // j--
        "jnz loop_2_begin%=                             \n\t"

        "loop_2_end%=:                                  \n\t"

        "adcx %[carry], %[zero]                         \n\t" // carry += CF
        "mov [%[dest] + %[pos]], %[carry]               \n\t" // *(dest + pos) = carry

        "lea %[src_1], [%[src_1] + 8]                   \n\t" // src_1 += 8
        "lea %[dest], [%[dest] + 8]                     \n\t" // dest += 8
        "dec %[i]                                       \n\t" // i--
        "jnz loop_1_begin%=                             \n\t"

        "loop_1_skip%=:                                 \n\t" // <--- ADDED: Target label for the skip

        "mov %[j], %[count_1]                           \n\t" // j == count_1
        "shl %[j], 3                                    \n\t" // j *= 8
        "sub %[src_1], %[j]                             \n\t" // src_1 -= j
        "sub %[dest], %[j]                              \n\t" // dest -= j

        "mov %[i], %[count_2]                           \n\t" // i = count_2
        "and %[i], ~31                                  \n\t" // i = i - i % 32
        "lea %[src_2], [%[src_2] + 8 * %[i]]            \n\t" // src_2 += 8 * i
        "lea %[dest], [%[dest] + 8 * %[i]]              \n\t" // dest += 8 * i
        "mov %[j], %[count_2]                           \n\t" // j = count_2
        "sub %[j], %[i]                                 \n\t" // j -= i
        "test %[j], %[j]                                \n\t"
        "jz loop_tail_skip%=                            \n\t"

        "jmp loop_tail%=                                \n\t"

        "loop_0_skip%=:                                 \n\t"

        "mov %[i], %[count_1]                           \n\t" // i = count_1

        "loop_clear_begin%=:                            \n\t"
        "mov [%[dest] + 8 * %[i] - 8], %[zero]          \n\t" // *(dest + 8 * i - 8) = 0
        "dec %[i]                                       \n\t" // j--
        "jnz loop_clear_begin%=                         \n\t"

        "mov %[j], %[count_2]                           \n\t" // j = count_2

        "loop_tail%=:                                   \n\t"

        "loop_tail_1_begin%=:                           \n\t"
        "mov %[i], %[count_1]                           \n\t" // i = count_1
        "shr %[i], 5                                    \n\t" // i /= 32
        "mov %[carry], 0                                \n\t" // carry = 0
        "xor %[pos], %[pos]                             \n\t" // pos = 0;
        "mov rdx, [%[src_2]]                            \n\t" // D = *src_2
        "test %[i], %[i]                                \n\t"
        "jz loop_tail_2_tail_prepare%=                  \n\t"

        "loop_tail_2_begin%=:                           \n\t"

        MUL_CLASSIC_STEP(  0, high, carry, src_1, pos)
        MUL_CLASSIC_STEP(  8, carry, high, src_1, pos)
        MUL_CLASSIC_STEP( 16, high, carry, src_1, pos)
        MUL_CLASSIC_STEP( 24, carry, high, src_1, pos)
        MUL_CLASSIC_STEP( 32, high, carry, src_1, pos)
        MUL_CLASSIC_STEP( 40, carry, high, src_1, pos)
        MUL_CLASSIC_STEP( 48, high, carry, src_1, pos)
        MUL_CLASSIC_STEP( 56, carry, high, src_1, pos)
        MUL_CLASSIC_STEP( 64, high, carry, src_1, pos)
        MUL_CLASSIC_STEP( 72, carry, high, src_1, pos)
        MUL_CLASSIC_STEP( 80, high, carry, src_1, pos)
        MUL_CLASSIC_STEP( 88, carry, high, src_1, pos)
        MUL_CLASSIC_STEP( 96, high, carry, src_1, pos)
        MUL_CLASSIC_STEP(104, carry, high, src_1, pos)
        MUL_CLASSIC_STEP(112, high, carry, src_1, pos)
        MUL_CLASSIC_STEP(120, carry, high, src_1, pos)
        MUL_CLASSIC_STEP(128, high, carry, src_1, pos)
        MUL_CLASSIC_STEP(136, carry, high, src_1, pos)
        MUL_CLASSIC_STEP(144, high, carry, src_1, pos)
        MUL_CLASSIC_STEP(152, carry, high, src_1, pos)
        MUL_CLASSIC_STEP(160, high, carry, src_1, pos)
        MUL_CLASSIC_STEP(168, carry, high, src_1, pos)
        MUL_CLASSIC_STEP(176, high, carry, src_1, pos)
        MUL_CLASSIC_STEP(184, carry, high, src_1, pos)
        MUL_CLASSIC_STEP(192, high, carry, src_1, pos)
        MUL_CLASSIC_STEP(200, carry, high, src_1, pos)
        MUL_CLASSIC_STEP(208, high, carry, src_1, pos)
        MUL_CLASSIC_STEP(216, carry, high, src_1, pos)
        MUL_CLASSIC_STEP(224, high, carry, src_1, pos)
        MUL_CLASSIC_STEP(232, carry, high, src_1, pos)
        MUL_CLASSIC_STEP(240, high, carry, src_1, pos)
        MUL_CLASSIC_STEP(248, carry, high, src_1, pos)

        "adox %[carry], %[zero]                         \n\t" // carry += OF

        "lea %[pos], [%[pos] + 256]                     \n\t" // pos += 256
        "dec %[i]                                       \n\t" // i--"
        "jnz loop_tail_2_begin%=                        \n\t"

        "adcx %[carry], %[zero]                         \n\t" // carry += CF

        "loop_tail_2_tail_prepare%=:                    \n\t"
        "adcx %[carry], %[zero]                         \n\t" // carry += CF
        "mov %[i], %[count_1]                           \n\t" // i = count_1
        "and %[i], 31                                   \n\t" // i = i % 32
        "test %[i], %[i]                                \n\t"
        "jz loop_tail_2_skip%=                          \n\t"

        "loop_tail_2_tail_begin%=:                      \n\t"

        MUL_CLASSIC_STEP(0, high, carry, src_1, pos)
        "mov %[carry], %[high]                          \n\t" // carry = high
        "adox %[carry], %[zero]                         \n\t" // carry += OF

        "lea %[pos], [%[pos] + 8]                       \n\t" // pos += 8
        "dec %[i]                                       \n\t" // i--
        "jnz loop_tail_2_tail_begin%=                   \n\t"

        "loop_tail_2_skip%=:                            \n\t"

        "adcx %[carry], %[zero]                         \n\t" // carry += CF
        "mov [%[dest] + %[pos]], %[carry]               \n\t" // *(dest + pos) = carry

        "lea %[src_2], [%[src_2] + 8]                   \n\t" // src_2 += 8
        "lea %[dest], [%[dest] + 8]                     \n\t" // dest += 8
        "dec %[j]                                       \n\t"
        "jnz loop_tail_1_begin%=                        \n\t"

        "loop_tail_skip%=:                              \n\t"

        ".att_syntax prefix                             \n\t"
        // out
        :   [high] "=&r" (high),
            [low] "=&r" (low),
            [carry] "=&r" (carry),
            [pos] "=&r" (pos),
            [j] "=&r" (j),
            [i] "+&r" (i),
            [src_1] "+&r" (src_1),
            [src_2] "+&r" (src_2),
            [dest] "+&r" (dest)
        // in
        :   [zero] "r" (zero),
            [count_1] "r" (count_1),
            [count_2] "r" (count_2)
        // clobber
        :   "cc",
            "memory",
            "rdx"
    );

#else

    uint128_t carry = 0;
    uint64_t v1 = src_1[0];
    uint64_t loop_2_max = count_2 & ~U64(31);
    #pragma GCC unroll 32
    for(uint64_t j = 0; j < loop_2_max; j++)
    {
        carry += MUL(v1, src_2[j]);
        dest[j] = LOW(carry);
        carry = HIGH(carry);
    }
    dest[loop_2_max] = LOW(carry);

    for(uint64_t i = 1; i < count_1; i++)
    {
        v1 = src_1[i];
        carry = 0;
        #pragma GCC unroll 32
        for(uint64_t j = 0; j < loop_2_max; j++)
        {
            carry += dest[i + j] + MUL(v1, src_2[j]);
            dest[i + j] = LOW(carry);
            carry = HIGH(carry);
        }
        dest[i + loop_2_max] = LOW(carry);
    }

    for(uint64_t j=loop_2_max; j<count_2; j++)
    {
        uint64_t v2 = src_2[j];
        carry = 0;
        #pragma GCC unroll 32
        for(uint64_t i=0; i<count_1; i++)
        {
            carry += dest[i + j] + MUL(v2, src_1[i]);
            dest[i + j] = LOW(carry);
            carry = HIGH(carry);
        }
        dest[count_1 + j] = LOW(carry);
    }

#endif

    return num_normalize(num_res);
}

static void num_sqr_classic_buffer(num_p num_res, num_p num)
{
    CLU_HANDLER_IS_SAFE(num_res)
    CLU_HANDLER_IS_SAFE(num)
    assert(num_res)
    assert(num)
    assert(num_res->size >= 2 * num->count)

    uint64_t count = num->count;
    uint64_t * restrict dest = num_res->chunk;
    const uint64_t * restrict src = num->chunk;

    memset(dest, 0, num_res->size * sizeof(uint64_t));
    num_res->count = 2 * count;

    if(count == 0)
    {
        return;
    }

#ifdef NUM_ASM_X86_64

    uint64_t high, low, carry, pos, j;
    uint64_t zero = 0;

    // --- PHASE 1: CROSS PRODUCTS ---

    if (count > 1)
    {
        uint64_t value = src[0];
        uint64_t inner_count = count - 1;
        uint64_t * restrict d = &dest[1];
        const uint64_t * restrict s = &src[1];

        j = inner_count >> 3;
        uint64_t tail = inner_count & 7;

        __asm__ __volatile__ (
            ".intel_syntax noprefix                         \n\t"
            "mov rdx, %[value]                              \n\t"
            "mov %[carry], 0                                \n\t"
            "xor %[pos], %[pos]                             \n\t"
            "test %[j], %[j]                                \n\t"
            "jz loop_cp0_tail_prepare%=                     \n\t"

            "loop_cp0_begin%=:                              \n\t"
            MUL_CLASSIC_STEP_ZERO(  0, high, carry, pos)
            MUL_CLASSIC_STEP_ZERO(  8, carry, high, pos)
            MUL_CLASSIC_STEP_ZERO( 16, high, carry, pos)
            MUL_CLASSIC_STEP_ZERO( 24, carry, high, pos)
            MUL_CLASSIC_STEP_ZERO( 32, high, carry, pos)
            MUL_CLASSIC_STEP_ZERO( 40, carry, high, pos)
            MUL_CLASSIC_STEP_ZERO( 48, high, carry, pos)
            MUL_CLASSIC_STEP_ZERO( 56, carry, high, pos)

            "lea %[pos], [%[pos] + 64]                      \n\t"
            "dec %[j]                                       \n\t"
            "jnz loop_cp0_begin%=                           \n\t"

            "loop_cp0_tail_prepare%=:                       \n\t"
            "dec %[tail]                                    \n\t"
            "js loop_cp0_end%=                              \n\t"

            "loop_cp0_tail_begin%=:                         \n\t"
            MUL_CLASSIC_STEP_ZERO(0, high, carry, pos)
            "mov %[carry], %[high]                          \n\t"
            "lea %[pos], [%[pos] + 8]                       \n\t"
            "dec %[tail]                                    \n\t"
            "jns loop_cp0_tail_begin%=                      \n\t"

            "loop_cp0_end%=:                                \n\t"
            "adcx %[carry], %[zero]                         \n\t"
            "mov [%[dest] + %[pos]], %[carry]               \n\t"

            ".att_syntax prefix                             \n\t"
            // out
            :   [high] "=&r" (high),
                [low] "=&r" (low),
                [carry] "=&r" (carry),
                [pos] "=&r" (pos),
                [j] "+&r" (j),
                [tail] "+&r" (tail)
            // in (Mapping [src_2] to 's' as macro demands)
            :   [value] "r" (value),
                [src_2] "r" (s),
                [dest] "r" (d),
                [zero] "r" (zero)
            // clobber
            :   "cc",
                "memory",
                "rdx"
        );
    }

    // 2. Standard Cross Products for i=1 to count-1
    for(uint64_t i = 1; i < count; i++)
    {
        uint64_t value = src[i];
        uint64_t inner_count = count - i - 1;

        if (inner_count == 0)
        {
            continue;
        }

        uint64_t * restrict d = &dest[(2 * i) + 1];
        const uint64_t * restrict s = &src[i + 1];

        j = inner_count >> 3;           // Changed from 5 to 3
        uint64_t tail = inner_count & 7; // Changed from 31 to 7

        __asm__ __volatile__ (
            ".intel_syntax noprefix                         \n\t"
            "mov rdx, %[value]                              \n\t"
            "mov %[carry], 0                                \n\t"
            "xor %[pos], %[pos]                             \n\t"
            "test %[j], %[j]                                \n\t"
            "jz loop_cp_tail_prepare%=                      \n\t"

            "loop_cp_begin%=:                               \n\t"
            MUL_CLASSIC_STEP(  0, high, carry, s, pos)
            MUL_CLASSIC_STEP(  8, carry, high, s, pos)
            MUL_CLASSIC_STEP( 16, high, carry, s, pos)
            MUL_CLASSIC_STEP( 24, carry, high, s, pos)
            MUL_CLASSIC_STEP( 32, high, carry, s, pos)
            MUL_CLASSIC_STEP( 40, carry, high, s, pos)
            MUL_CLASSIC_STEP( 48, high, carry, s, pos)
            MUL_CLASSIC_STEP( 56, carry, high, s, pos)

            "adox %[carry], %[zero]                         \n\t"

            "lea %[pos], [%[pos] + 64]                      \n\t"
            "dec %[j]                                       \n\t"
            "jnz loop_cp_begin%=                            \n\t"

            "loop_cp_tail_prepare%=:                        \n\t"
            "adcx %[carry], %[zero]                         \n\t"
            "test %[tail], %[tail]                          \n\t"
            "jz loop_cp_end%=                               \n\t"

            "loop_cp_tail_begin%=:                          \n\t"
            MUL_CLASSIC_STEP(0, high, carry, s, pos)
            "mov %[carry], %[high]                          \n\t"
            "adox %[carry], %[zero]                         \n\t"
            "lea %[pos], [%[pos] + 8]                       \n\t"
            "dec %[tail]                                    \n\t"
            "jnz loop_cp_tail_begin%=                       \n\t"

            "loop_cp_end%=:                                 \n\t"
            "adcx %[carry], %[zero]                         \n\t"
            "mov [%[dest] + %[pos]], %[carry]               \n\t"

            ".att_syntax prefix                             \n\t"
            // out
            :   [high] "=&r" (high),
                [low] "=&r" (low),
                [carry] "=&r" (carry),
                [pos] "=&r" (pos),
                [j] "+&r" (j),
                [tail] "+&r" (tail)
            // in
            :   [value] "r" (value),
                [s] "r" (s),
                [dest] "r" (d),
                [zero] "r" (zero)
            // clobber
            :   "cc",
                "memory",
                "rdx"
        );
    }


    // --- PHASE 2: DOUBLE DESTINATION ARRAY ---
    uint64_t pos_src, pos_dest;
    uint64_t _a;
    j = count >> 3;           // Changed from 5 to 3
    uint64_t tail = count & 7; // Changed from 31 to 7

#define COMBINED_STEP(OFF_SRC, OFF_DEST)                                    \
    "mov rdx, [%[src] + %[pos_src] + " #OFF_SRC "]                  \n\t"   \
    "mulx %[high], %[low], rdx                                      \n\t"   \
    "mov %[_a], [%[dest] + %[pos_dest] + " #OFF_DEST "]             \n\t"   \
    "adox %[_a], %[_a]                                              \n\t"   \
    "adcx %[low], %[_a]                                             \n\t"   \
    "mov [%[dest] + %[pos_dest] + " #OFF_DEST "], %[low]            \n\t"   \
    "mov %[_a], [%[dest] + %[pos_dest] + " #OFF_DEST " + 8]         \n\t"   \
    "adox %[_a], %[_a]                                              \n\t"   \
    "adcx %[high], %[_a]                                            \n\t"   \
    "mov [%[dest] + %[pos_dest] + " #OFF_DEST " + 8], %[high]       \n\t"

    __asm__ __volatile__ (
        ".intel_syntax noprefix                                     \n\t"
        "mov rcx, %[j]                                              \n\t"
        "xor %[pos_src], %[pos_src]                                 \n\t"
        "xor %[pos_dest], %[pos_dest]                               \n\t" // Clears both CF and OF
        "test rcx, rcx                                              \n\t"
        "jz loop_comb_tail_prepare%=                                \n\t"

        "loop_comb_begin%=:                                         \n\t"
        COMBINED_STEP(  0,   0)
        COMBINED_STEP(  8,  16)
        COMBINED_STEP( 16,  32)
        COMBINED_STEP( 24,  48)
        COMBINED_STEP( 32,  64)
        COMBINED_STEP( 40,  80)
        COMBINED_STEP( 48,  96)
        COMBINED_STEP( 56, 112)

        "lea %[pos_src], [%[pos_src] + 64]                          \n\t" // Changed from 256 to 64
        "lea %[pos_dest], [%[pos_dest] + 128]                       \n\t" // Changed from 512 to 128

        // Loop control avoiding flag corruption (preserves OF and CF)
        "lea rcx, [rcx - 1]                                         \n\t"
        "jrcxz loop_comb_tail_prepare%=                             \n\t"
        "jmp loop_comb_begin%=                                      \n\t"

        "loop_comb_tail_prepare%=:                                  \n\t"
        "mov rcx, %[tail]                                           \n\t" // mov preserves all flags
        "jrcxz loop_comb_end%=                                      \n\t"

        "loop_comb_tail_begin%=:                                    \n\t"
        COMBINED_STEP(0, 0)
        "lea %[pos_src], [%[pos_src] + 8]                           \n\t"
        "lea %[pos_dest], [%[pos_dest] + 16]                        \n\t"
        "lea rcx, [rcx - 1]                                         \n\t"
        "jrcxz loop_comb_end%=                                      \n\t"
        "jmp loop_comb_tail_begin%=                                 \n\t"

        "loop_comb_end%=:                                           \n\t"
        "mov %[j], rcx                                              \n\t" // Update states to match destruction
        "mov %[tail], rcx                                           \n\t"
        ".att_syntax prefix                                         \n\t"

        :   [pos_src] "=&r" (pos_src),
            [pos_dest] "=&r" (pos_dest),
            [j] "+&r" (j),
            [tail] "+&r" (tail),
            [high] "=&r" (high),
            [low] "=&r" (low),
            [_a] "=&r" (_a)
        :   [src] "r" (src),
            [dest] "r" (dest)
        :   "cc",
            "memory",
            "rdx",
            "rcx"
    );
#undef COMBINED_STEP

#else

    for(uint64_t i=0; i<count; i++)
    {
        uint64_t value = src[i];

        uint128_t carry = 0;
        #pragma GCC unroll 32
        for(uint64_t j=i + 1; j<count; j++)
        {
            carry += dest[i+j] + MUL(value, src[j]);
            dest[i+j] = LOW(carry);
            carry = HIGH(carry);
        }
        dest[i+count] = LOW(carry);
    }

    uint128_t carry = 0;
    #pragma GCC unroll 32
    for(uint64_t i=0; i<count; i++)
    {
        uint64_t value = src[i];
        uint128_t u = MUL(value, value);

        carry += (2 * U128(dest[2 * i])) + LOW(u);
        dest[2 * i] = LOW(carry);
        carry = HIGH(carry);

        carry += 2 * U128(dest[(2 * i) + 1]) + HIGH(u);
        dest[(2 * i) + 1] = LOW(carry);
        carry = HIGH(carry);
    }

#endif

    num_normalize(num_res);
}

num_p num_sqr_classic(num_p num)
{
    num_p num_res = num_create(CLU_ARGS(2 * num->count, 0));
    num_sqr_classic_buffer(num_res, num);
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
void num_display_span_full(
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

uint64_t ssm_bit_inv(uint64_t i, uint64_t K)
{
    uint64_t res = 0;
    for(; K > 1; K>>=1)
    {
        res = (res << 1) | (i & 1);
        i >>= 1;
    }
    return res;
}

static bool num_is_span_zero(num_p num_fft, uint64_t pos, uint64_t count)
{
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_fft)

    const uint64_t * restrict src = num_fft->chunk;

    for(uint64_t i = 0; i < count; i++)
    {
        if(src[i + pos])
        {
            return false;
        }
    }

    return true;
}

static int64_t num_ssm_cmp_uint_offset(
    num_p num_fft, uint64_t pos,
    uint64_t value,
    uint64_t n
)
{
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_fft)

    if(!num_is_span_zero(num_fft, pos + 1, n - 1))
    {
        return 1;
    }

    uint64_t value_num = num_fft->chunk[pos];
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

static void num_ssm_add_uint(num_p num_fft, uint64_t pos, uint64_t n, uint64_t value)
{
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_fft)

    uint64_t * restrict dest = num_fft->chunk;

    uint128_t carry = value;
    for(uint64_t i = 0; i < n && carry; i++)
    {
        uint128_t sum = U128(dest[pos + i]) + carry;
        dest[pos + i] = LOW(sum);
        carry = HIGH(sum);
    }
    assert(!carry);
}

static void num_ssm_sub_uint(num_p num_fft, uint64_t pos, uint64_t n, uint64_t value)
{
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_fft)

    uint64_t * restrict dest = num_fft->chunk;

    uint128_t borrow = value;
    for(uint64_t i = 0; i < n && borrow; i++)
    {
        uint128_t diff = U128(dest[pos + i]) - borrow;
        dest[pos + i] = LOW(diff);
        borrow = HIGH(diff) & 1;
    }
}

// normalizes coeficient if it is less than 2 modulus
static void num_ssm_normalize(num_p num_fft, uint64_t pos, uint64_t n)
{
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_fft)
    assert(num_fft->chunk[pos + n - 1] <= 2)

    uint64_t value = num_fft->chunk[pos + n - 1];
    if(value == 0)
    {
        return;
    }

    num_fft->chunk[pos + n - 1] = 0;
    num_ssm_sub_uint(num_fft, pos, n, value);
    if(num_fft->chunk[pos + n - 1] != UINT64_MAX)
    {
        return;
    }

    if(value != 1)
    {
        num_fft->chunk[pos + n - 1] = 0;
        num_ssm_add_uint(num_fft, pos, n, 1);
        return;
    }

    memset(&num_fft->chunk[pos], 0, (n - 1) * sizeof(uint64_t));
    num_fft->chunk[pos + n - 1] = 1;
}

static void num_ssm_denormalize(num_p num_fft, uint64_t pos, uint64_t n)
{
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_fft)

    num_ssm_add_uint(num_fft, pos, n, 1);
    num_fft->chunk[pos + n - 1] += 1;
}

#ifdef NUM_ASM_AARCH64

// One limb per iteration costs a load, a load, an adcs, a store and the loop overhead,
// so the carry chain sits idle behind five other instructions. These mirror the x86-64
// blocks below: eight limbs per pass, paired loads and stores, so the same work takes
// roughly a third of the instructions. ldp / stp / add / sub leave the carry flag alone,
// which is what lets the adcs chain span the whole unrolled body.

#define ADD_MOD_STEP_4(OFF_0, OFF_1)                                                                      \
    "ldp %[a_0], %[a_1], [%[dest], #" #OFF_0 "]     \n\t" /* (a_0, a_1)  = *(dest + OFF_0)             */  \
    "ldp %[a_2], %[a_3], [%[dest], #" #OFF_1 "]     \n\t" /* (a_2, a_3)  = *(dest + OFF_1)             */  \
    "ldp %[b_0], %[b_1], [%[src_2], #" #OFF_0 "]    \n\t" /* (b_0, b_1)  = *(src_2 + OFF_0)            */  \
    "ldp %[b_2], %[b_3], [%[src_2], #" #OFF_1 "]    \n\t" /* (b_2, b_3)  = *(src_2 + OFF_1)            */  \
    "adcs %[a_0], %[a_0], %[b_0]                    \n\t" /* a_0 += b_0 + CF                           */  \
    "adcs %[a_1], %[a_1], %[b_1]                    \n\t" /* a_1 += b_1 + CF                           */  \
    "adcs %[a_2], %[a_2], %[b_2]                    \n\t" /* a_2 += b_2 + CF                           */  \
    "adcs %[a_3], %[a_3], %[b_3]                    \n\t" /* a_3 += b_3 + CF                           */  \
    "stp %[a_0], %[a_1], [%[dest], #" #OFF_0 "]     \n\t" /* *(dest + OFF_0) = (a_0, a_1)              */  \
    "stp %[a_2], %[a_3], [%[dest], #" #OFF_1 "]     \n\t" /* *(dest + OFF_1) = (a_2, a_3)              */

#define SUB_MOD_STEP_4(SRC_1, OFF_0, OFF_1)                                                                    \
    "ldp %[a_0], %[a_1], [%[" #SRC_1 "], #" #OFF_0 "]   \n\t" /* (a_0, a_1)  = *(SRC_1 + OFF_0)            */  \
    "ldp %[a_2], %[a_3], [%[" #SRC_1 "], #" #OFF_1 "]   \n\t" /* (a_2, a_3)  = *(SRC_1 + OFF_1)            */  \
    "ldp %[b_0], %[b_1], [%[src_2], #" #OFF_0 "]        \n\t" /* (b_0, b_1)  = *(src_2 + OFF_0)            */  \
    "ldp %[b_2], %[b_3], [%[src_2], #" #OFF_1 "]        \n\t" /* (b_2, b_3)  = *(src_2 + OFF_1)            */  \
    "sbcs %[a_0], %[a_0], %[b_0]                        \n\t" /* a_0 -= b_0 + (1 - CF)                     */  \
    "sbcs %[a_1], %[a_1], %[b_1]                        \n\t" /* a_1 -= b_1 + (1 - CF)                     */  \
    "sbcs %[a_2], %[a_2], %[b_2]                        \n\t" /* a_2 -= b_2 + (1 - CF)                     */  \
    "sbcs %[a_3], %[a_3], %[b_3]                        \n\t" /* a_3 -= b_3 + (1 - CF)                     */  \
    "stp %[a_0], %[a_1], [%[dest], #" #OFF_0 "]         \n\t" /* *(dest + OFF_0) = (a_0, a_1)              */  \
    "stp %[a_2], %[a_3], [%[dest], #" #OFF_1 "]         \n\t" /* *(dest + OFF_1) = (a_2, a_3)              */

#define OPPOSITE_STEP_4(OFF_0, OFF_1)                                                                     \
    "ldp %[a_0], %[a_1], [%[dest], #" #OFF_0 "]     \n\t" /* (a_0, a_1)  = *(dest + OFF_0)             */  \
    "ldp %[a_2], %[a_3], [%[dest], #" #OFF_1 "]     \n\t" /* (a_2, a_3)  = *(dest + OFF_1)             */  \
    "sbcs %[a_0], xzr, %[a_0]                       \n\t" /* a_0 = 0 - a_0 - (1 - CF)                  */  \
    "sbcs %[a_1], xzr, %[a_1]                       \n\t" /* a_1 = 0 - a_1 - (1 - CF)                  */  \
    "sbcs %[a_2], xzr, %[a_2]                       \n\t" /* a_2 = 0 - a_2 - (1 - CF)                  */  \
    "sbcs %[a_3], xzr, %[a_3]                       \n\t" /* a_3 = 0 - a_3 - (1 - CF)                  */  \
    "stp %[a_0], %[a_1], [%[dest], #" #OFF_0 "]     \n\t" /* *(dest + OFF_0) = (a_0, a_1)              */  \
    "stp %[a_2], %[a_3], [%[dest], #" #OFF_1 "]     \n\t" /* *(dest + OFF_1) = (a_2, a_3)              */

#endif

void num_ssm_add_mod_immed(
    num_p num_fft_1, uint64_t pos_1,
    num_p num_fft_2, uint64_t pos_2,
    uint64_t n
)
{
    CLU_HANDLER_IS_SAFE(num_fft_1)
    CLU_HANDLER_IS_SAFE(num_fft_2)
    assert(num_fft_1 && num_fft_2)

    uint64_t * restrict dest = &num_fft_1->chunk[pos_1];
    const uint64_t * restrict src_2 = &num_fft_2->chunk[pos_2];

#ifdef NUM_ASM_X86_64

    uint64_t reg_1, reg_2;
    uint64_t j = n;
    uint64_t pos = 0;

    __asm__ __volatile__ (
        ".intel_syntax noprefix                         \n\t"

        "shr %[j], 3                                    \n\t" // j /= 8
        "xor %[pos], %[pos]                             \n\t" // pos = 0 (and inherently clears CF)
        "test %[j], %[j]                                \n\t" // (test also leaves CF clear)
        "jz loop_add_tail%=                             \n\t"

        // The AArch64 blocks guard their loops with cbz; the unrolled bodies here are
        // do-while, so without the test above an n under 8 wrapped j to UINT64_MAX and
        // ran 2^64 passes over the array. Every ssm caller passes n = 8k + 1 with k >= 1,
        // so the branch is never taken in practice, but it costs one fused compare and
        // makes the kernels total on their own. Jumping to the trailing step is also the
        // right answer for n == 1: that single step is then the whole operation.

        "loop_add_begin%=:                              \n\t" // LOOP_ADD_BEGIN

        ADD_CLASSIC_STEP( 0, reg_1)
        ADD_CLASSIC_STEP( 8, reg_2)
        ADD_CLASSIC_STEP(16, reg_1)
        ADD_CLASSIC_STEP(24, reg_2)
        ADD_CLASSIC_STEP(32, reg_1)
        ADD_CLASSIC_STEP(40, reg_2)
        ADD_CLASSIC_STEP(48, reg_1)
        ADD_CLASSIC_STEP(56, reg_2)

        "lea %[pos], [%[pos] + 64]                      \n\t" // pos += 64 (lea does not modify CF)
        "dec %[j]                                       \n\t" // j-- (dec does not modify CF)
        "jnz loop_add_begin%=                           \n\t"

        "loop_add_tail%=:                               \n\t"

        ADD_CLASSIC_STEP(0, reg_1)

        ".att_syntax prefix                             \n\t"
        // out
        :   [pos] "+&r" (pos),
            [j] "+&r" (j),
            [reg_1] "=&r" (reg_1),
            [reg_2] "=&r" (reg_2)
        // in
        :   [dest] "r" (dest),
            [src_2] "r" (src_2)
        // clobber
        :   "cc",
            "memory"
    );

#elif defined(NUM_ASM_AARCH64)

    uint64_t a_0, a_1, a_2, a_3;
    uint64_t b_0, b_1, b_2, b_3;
    constexpr uint64_t unroll_log_2 = 3;
    constexpr uint64_t unroll_mask = 7;

    uint64_t j = n >> unroll_log_2;
    uint64_t tail = n & unroll_mask;

    __asm__ __volatile__ (
        "adds xzr, xzr, xzr                             \n\t" // CF = 0
        "cbz %[j], 2f                                   \n\t"

        "1:                                             \n\t" // LOOP_ADD_BEGIN

        ADD_MOD_STEP_4( 0, 16)
        ADD_MOD_STEP_4(32, 48)

        "add %[dest], %[dest], #64                      \n\t" // dest += 64 (add does not modify CF)
        "add %[src_2], %[src_2], #64                    \n\t" // src_2 += 64
        "sub %[j], %[j], #1                             \n\t" // j-- (sub does not modify CF)
        "cbnz %[j], 1b                                  \n\t"

        "2:                                             \n\t"
        "cbz %[tail], 4f                                \n\t"

        "3:                                             \n\t" // LOOP_ADD_TAIL_BEGIN

        "ldr %[a_0], [%[dest]]                          \n\t" // a_0 = *dest
        "ldr %[b_0], [%[src_2]], #8                     \n\t" // b_0 = *src_2, then src_2 += 8
        "adcs %[a_0], %[a_0], %[b_0]                    \n\t" // a_0 += b_0 + CF
        "str %[a_0], [%[dest]], #8                      \n\t" // *dest = a_0, then dest += 8
        "sub %[tail], %[tail], #1                       \n\t" // tail--
        "cbnz %[tail], 3b                               \n\t"

        "4:                                             \n\t"
        // out
        :   [dest] "+r" (dest),
            [src_2] "+r" (src_2),
            [j] "+&r" (j),
            [tail] "+&r" (tail),
            [a_0] "=&r" (a_0),
            [a_1] "=&r" (a_1),
            [a_2] "=&r" (a_2),
            [a_3] "=&r" (a_3),
            [b_0] "=&r" (b_0),
            [b_1] "=&r" (b_1),
            [b_2] "=&r" (b_2),
            [b_3] "=&r" (b_3)
        // in
        :
        // clobber
        :   "cc",
            "memory"
    );

#else

    uint128_t carry = 0;
    #pragma GCC unroll 8
    for(uint64_t i = 0; i < n; i++)
    {
        carry += U128(dest[i]) + src_2[i];
        dest[i] = LOW(carry);
        carry = HIGH(carry);
    }

#endif

    num_ssm_normalize(num_fft_1, pos_1, n);
}

void num_ssm_sub_mod(
    num_p num_fft_res, uint64_t pos_res,
    num_p num_fft_1, uint64_t pos_1,
    num_p num_fft_2, uint64_t pos_2,
    uint64_t n
)
{
    CLU_HANDLER_IS_SAFE(num_fft_res)
    CLU_HANDLER_IS_SAFE(num_fft_1)
    CLU_HANDLER_IS_SAFE(num_fft_2)
    assert(num_fft_res && num_fft_1 && num_fft_2)

    num_ssm_denormalize(num_fft_1, pos_1, n);

    uint64_t * restrict dest = &num_fft_res->chunk[pos_res];
    const uint64_t * restrict src_1 = &num_fft_1->chunk[pos_1];
    const uint64_t * restrict src_2 = &num_fft_2->chunk[pos_2];

#ifdef NUM_ASM_X86_64

    uint64_t reg_1, reg_2;
    uint64_t j = n;
    uint64_t pos = 0;

    __asm__ __volatile__ (
        ".intel_syntax noprefix                         \n\t"

        "shr %[j], 3                                    \n\t" // j /= 8
        "xor %[pos], %[pos]                             \n\t" // pos = 0 (and inherently clears CF)
        "test %[j], %[j]                                \n\t" // guard the do-while, see num_ssm_add_mod_immed
        "jz loop_sub_tail%=                             \n\t"

        "loop_sub_begin%=:                              \n\t" // LOOP_SUB_BEGIN

        SUB_CLASSIC_STEP( 0, src_1, reg_1)
        SUB_CLASSIC_STEP( 8, src_1, reg_2)
        SUB_CLASSIC_STEP(16, src_1, reg_1)
        SUB_CLASSIC_STEP(24, src_1, reg_2)
        SUB_CLASSIC_STEP(32, src_1, reg_1)
        SUB_CLASSIC_STEP(40, src_1, reg_2)
        SUB_CLASSIC_STEP(48, src_1, reg_1)
        SUB_CLASSIC_STEP(56, src_1, reg_2)

        "lea %[pos], [%[pos] + 64]                      \n\t" // pos += 64 (lea does not modify CF)
        "dec %[j]                                       \n\t" // j-- (dec does not modify CF)
        "jnz loop_sub_begin%=                           \n\t"

        "loop_sub_tail%=:                               \n\t"

        SUB_CLASSIC_STEP(0, src_1, reg_1)

        ".att_syntax prefix                             \n\t"
        // out
        :   [pos] "+&r" (pos),
            [j] "+&r" (j),
            [reg_1] "=&r" (reg_1),
            [reg_2] "=&r" (reg_2)
        // in
        :   [dest] "r" (dest),
            [src_1] "r" (src_1),
            [src_2] "r" (src_2)
        // clobber
        :   "cc",
            "memory"
    );

#elif defined(NUM_ASM_AARCH64)

    uint64_t a_0, a_1, a_2, a_3;
    uint64_t b_0, b_1, b_2, b_3;
    constexpr uint64_t unroll_log_2 = 3;
    constexpr uint64_t unroll_mask = 7;

    uint64_t j = n >> unroll_log_2;
    uint64_t tail = n & unroll_mask;

    __asm__ __volatile__ (
        "cmp xzr, xzr                                   \n\t" // CF = 1 (means NO borrow)
        "cbz %[j], 2f                                   \n\t"

        "1:                                             \n\t" // LOOP_SUB_BEGIN

        SUB_MOD_STEP_4(src_1,  0, 16)
        SUB_MOD_STEP_4(src_1, 32, 48)

        "add %[dest], %[dest], #64                      \n\t" // dest += 64 (add does not modify CF)
        "add %[src_1], %[src_1], #64                    \n\t" // src_1 += 64
        "add %[src_2], %[src_2], #64                    \n\t" // src_2 += 64
        "sub %[j], %[j], #1                             \n\t" // j-- (sub does not modify CF)
        "cbnz %[j], 1b                                  \n\t"

        "2:                                             \n\t"
        "cbz %[tail], 4f                                \n\t"

        "3:                                             \n\t" // LOOP_SUB_TAIL_BEGIN

        "ldr %[a_0], [%[src_1]], #8                     \n\t" // a_0 = *src_1, then src_1 += 8
        "ldr %[b_0], [%[src_2]], #8                     \n\t" // b_0 = *src_2, then src_2 += 8
        "sbcs %[a_0], %[a_0], %[b_0]                    \n\t" // a_0 -= b_0 + (1 - CF)
        "str %[a_0], [%[dest]], #8                      \n\t" // *dest = a_0, then dest += 8
        "sub %[tail], %[tail], #1                       \n\t" // tail--
        "cbnz %[tail], 3b                               \n\t"

        "4:                                             \n\t"
        // out
        :   [dest] "+r" (dest),
            [src_1] "+r" (src_1),
            [src_2] "+r" (src_2),
            [j] "+&r" (j),
            [tail] "+&r" (tail),
            [a_0] "=&r" (a_0),
            [a_1] "=&r" (a_1),
            [a_2] "=&r" (a_2),
            [a_3] "=&r" (a_3),
            [b_0] "=&r" (b_0),
            [b_1] "=&r" (b_1),
            [b_2] "=&r" (b_2),
            [b_3] "=&r" (b_3)
        // in
        :
        // clobber
        :   "cc",
            "memory"
    );

#else

    uint128_t borrow = 0;
    #pragma GCC unroll 8
    for(uint64_t i = 0; i < n; i++)
    {
        uint128_t diff = U128(src_1[i]) - src_2[i] - borrow;
        dest[i] = LOW(diff);
        borrow = HIGH(diff) & 1;
    }

#endif

    num_ssm_normalize(num_fft_1, pos_1, n);
    num_ssm_normalize(num_fft_res, pos_res, n);
}

static void num_ssm_sub_mod_immed(
    num_p num_fft_1, uint64_t pos_1,
    num_p num_fft_2, uint64_t pos_2,
    uint64_t n
)
{
    CLU_HANDLER_IS_SAFE(num_fft_1) CLU_HANDLER_IS_SAFE(num_fft_2)
    assert(num_fft_1 && num_fft_2)

    num_ssm_denormalize(num_fft_1, pos_1, n);

    uint64_t * restrict dest = &num_fft_1->chunk[pos_1];
    const uint64_t * restrict src_2 = &num_fft_2->chunk[pos_2];

#ifdef NUM_ASM_X86_64

    uint64_t reg_1, reg_2;
    uint64_t j = n;
    uint64_t pos = 0;

    __asm__ __volatile__ (
        ".intel_syntax noprefix                         \n\t"

        "shr %[j], 3                                    \n\t" // j /= 8
        "xor %[pos], %[pos]                             \n\t" // pos = 0 (and inherently clears CF)
        "test %[j], %[j]                                \n\t" // guard the do-while, see num_ssm_add_mod_immed
        "jz loop_sub_tail%=                             \n\t"

        "loop_sub_begin%=:                              \n\t" // LOOP_SUB_BEGIN

        SUB_CLASSIC_STEP( 0, dest, reg_1)
        SUB_CLASSIC_STEP( 8, dest, reg_2)
        SUB_CLASSIC_STEP(16, dest, reg_1)
        SUB_CLASSIC_STEP(24, dest, reg_2)
        SUB_CLASSIC_STEP(32, dest, reg_1)
        SUB_CLASSIC_STEP(40, dest, reg_2)
        SUB_CLASSIC_STEP(48, dest, reg_1)
        SUB_CLASSIC_STEP(56, dest, reg_2)

        "lea %[pos], [%[pos] + 64]                      \n\t" // pos += 64 (lea does not modify CF)
        "dec %[j]                                       \n\t" // j-- (dec does not modify CF)
        "jnz loop_sub_begin%=                           \n\t"

        "loop_sub_tail%=:                               \n\t"

        SUB_CLASSIC_STEP(0, dest, reg_1)

        ".att_syntax prefix                             \n\t"
        // out
        :   [pos] "+&r" (pos),
            [j] "+&r" (j),
            [reg_1] "=&r" (reg_1),
            [reg_2] "=&r" (reg_2)
        // in
        :   [dest] "r" (dest),
            [src_2] "r" (src_2)
        // clobber
        :   "cc",
            "memory"
    );

#elif defined(NUM_ASM_AARCH64)

    uint64_t a_0, a_1, a_2, a_3;
    uint64_t b_0, b_1, b_2, b_3;
    constexpr uint64_t unroll_log_2 = 3;
    constexpr uint64_t unroll_mask = 7;

    uint64_t j = n >> unroll_log_2;
    uint64_t tail = n & unroll_mask;

    __asm__ __volatile__ (
        "cmp xzr, xzr                                   \n\t" // CF = 1 (means NO borrow)
        "cbz %[j], 2f                                   \n\t"

        "1:                                             \n\t" // LOOP_SUB_BEGIN

        SUB_MOD_STEP_4(dest,  0, 16)
        SUB_MOD_STEP_4(dest, 32, 48)

        "add %[dest], %[dest], #64                      \n\t" // dest += 64 (add does not modify CF)
        "add %[src_2], %[src_2], #64                    \n\t" // src_2 += 64
        "sub %[j], %[j], #1                             \n\t" // j-- (sub does not modify CF)
        "cbnz %[j], 1b                                  \n\t"

        "2:                                             \n\t"
        "cbz %[tail], 4f                                \n\t"

        "3:                                             \n\t" // LOOP_SUB_TAIL_BEGIN

        "ldr %[a_0], [%[dest]]                          \n\t" // a_0 = *dest
        "ldr %[b_0], [%[src_2]], #8                     \n\t" // b_0 = *src_2, then src_2 += 8
        "sbcs %[a_0], %[a_0], %[b_0]                    \n\t" // a_0 -= b_0 + (1 - CF)
        "str %[a_0], [%[dest]], #8                      \n\t" // *dest = a_0, then dest += 8
        "sub %[tail], %[tail], #1                       \n\t" // tail--
        "cbnz %[tail], 3b                               \n\t"

        "4:                                             \n\t"
        // out
        :   [dest] "+r" (dest),
            [src_2] "+r" (src_2),
            [j] "+&r" (j),
            [tail] "+&r" (tail),
            [a_0] "=&r" (a_0),
            [a_1] "=&r" (a_1),
            [a_2] "=&r" (a_2),
            [a_3] "=&r" (a_3),
            [b_0] "=&r" (b_0),
            [b_1] "=&r" (b_1),
            [b_2] "=&r" (b_2),
            [b_3] "=&r" (b_3)
        // in
        :
        // clobber
        :   "cc",
            "memory"
    );

#else

    uint128_t borrow = 0;
    #pragma GCC unroll 8
    for(uint64_t i = 0; i < n; i++)
    {
        uint128_t diff = U128(dest[i]) - src_2[i] - borrow;
        dest[i] = LOW(diff);
        borrow = HIGH(diff) & 1;
    }

#endif

    num_ssm_normalize(num_fft_1, pos_1, n);
}

#ifdef NUM_ASM_X86_64

#define OPPOSITE_STEP(OFF, REG)                                                                     \
    "sbb %[" #REG "], [%[dest] + %[pos] + " #OFF "]     \n\t" /* REG -= *(dest + pos + OFF) + CF */ \
    "mov [%[dest] + %[pos] + " #OFF "], %[" #REG "]     \n\t" /* *(dest + pos + OFF) = REG       */ \
    "mov %[" #REG "], 0                                 \n\t" /* REG = 0 (preserves CF)          */

#endif

void num_ssm_opposite(num_p num_fft, uint64_t chunk_pos, uint64_t n)
{
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_fft)

    uint64_t * restrict dest = &num_fft->chunk[chunk_pos];

#ifdef NUM_ASM_X86_64

    uint64_t reg_1, reg_2;
    uint64_t j = n;
    uint64_t pos = 0;

    __asm__ __volatile__ (
        ".intel_syntax noprefix                         \n\t"

        "mov %[reg_1], 1                                \n\t" // Init for the 1st limb (1 - dest[0])
        "mov %[reg_2], 0                                \n\t" // Init for the 2nd limb (0 - dest[1])

        "shr %[j], 3                                    \n\t" // j /= 8
        "xor %[pos], %[pos]                             \n\t" // pos = 0 (and inherently clears CF)
        "test %[j], %[j]                                \n\t" // guard the do-while, see num_ssm_add_mod_immed
        "jz loop_opp_tail%=                             \n\t"

        "loop_opp_begin%=:                              \n\t" // LOOP_OPP_BEGIN

        OPPOSITE_STEP( 0, reg_1)
        OPPOSITE_STEP( 8, reg_2)
        OPPOSITE_STEP(16, reg_1)
        OPPOSITE_STEP(24, reg_2)
        OPPOSITE_STEP(32, reg_1)
        OPPOSITE_STEP(40, reg_2)
        OPPOSITE_STEP(48, reg_1)
        OPPOSITE_STEP(56, reg_2)

        "lea %[pos], [%[pos] + 64]                      \n\t" // pos += 64 (lea does not modify CF)
        "dec %[j]                                       \n\t" // j-- (dec does not modify CF)
        "jnz loop_opp_begin%=                           \n\t"

        "loop_opp_tail%=:                               \n\t"

        OPPOSITE_STEP(0, reg_1)

        ".att_syntax prefix                             \n\t"
        // out
        :   [pos] "+&r" (pos),
            [j] "+&r" (j),
            [reg_1] "=&r" (reg_1),
            [reg_2] "=&r" (reg_2)
        // in
        :   [dest] "r" (dest)
        // clobber
        :   "cc",
            "memory"
    );

#elif defined(NUM_ASM_AARCH64)

    uint64_t count = n;
    uint64_t a_0, a_1, a_2, a_3;
    uint64_t j, tail;

    __asm__ __volatile__ (
        "cbz %[count], 4f                               \n\t"

        "mov %[a_0], #1                                 \n\t" // first limb is 1 - dest[0]
        "cmp xzr, xzr                                   \n\t" // CF = 1 (means NO borrow)
        "ldr %[a_1], [%[dest]]                          \n\t" // a_1 = *dest
        "sbcs %[a_0], %[a_0], %[a_1]                    \n\t" // a_0 = 1 - a_1 - (1 - CF)
        "str %[a_0], [%[dest]], #8                      \n\t" // *dest = a_0, then dest += 8

        // the remaining limbs are 0 - dest[i]; n is 8k + 1 in every ssm caller, so this
        // split leaves the unrolled body an exact fit and the tail loop unused
        "sub %[count], %[count], #1                     \n\t" // count-- (sub does not modify CF)
        "lsr %[j], %[count], #3                         \n\t" // j = count / 8 (lsr does not modify CF)
        "and %[tail], %[count], #7                      \n\t" // tail = count % 8 (and does not modify CF)
        "cbz %[j], 2f                                   \n\t"

        "1:                                             \n\t" // LOOP_OPP_BEGIN

        OPPOSITE_STEP_4( 0, 16)
        OPPOSITE_STEP_4(32, 48)

        "add %[dest], %[dest], #64                      \n\t" // dest += 64 (add does not modify CF)
        "sub %[j], %[j], #1                             \n\t" // j--
        "cbnz %[j], 1b                                  \n\t"

        "2:                                             \n\t"
        "cbz %[tail], 4f                                \n\t"

        "3:                                             \n\t" // LOOP_OPP_TAIL_BEGIN

        "ldr %[a_0], [%[dest]]                          \n\t" // a_0 = *dest
        "sbcs %[a_0], xzr, %[a_0]                       \n\t" // a_0 = 0 - a_0 - (1 - CF)
        "str %[a_0], [%[dest]], #8                      \n\t" // *dest = a_0, then dest += 8
        "sub %[tail], %[tail], #1                       \n\t" // tail--
        "cbnz %[tail], 3b                               \n\t"

        "4:                                             \n\t"
        // out
        :   [dest] "+r" (dest),
            [count] "+&r" (count),
            [j] "=&r" (j),
            [tail] "=&r" (tail),
            [a_0] "=&r" (a_0),
            [a_1] "=&r" (a_1),
            [a_2] "=&r" (a_2),
            [a_3] "=&r" (a_3)
        // in
        :
        // clobber
        :   "cc",
            "memory"
    );

#else

    uint128_t borrow = U128(1) - dest[0];
    dest[0] = LOW(borrow);
    borrow = HIGH(borrow) & 1;

    #pragma GCC unroll 8
    for(uint64_t i = 1; i < n; i++)
    {
        uint128_t diff = U128(0) - dest[i] - borrow;
        dest[i] = LOW(diff);
        borrow = HIGH(diff) & 1;
    }

#endif

    num_fft->chunk[chunk_pos + n - 1]++;
    num_ssm_normalize(num_fft, chunk_pos, n);
}

void num_ssm_shl(
    num_p num_fft_res, uint64_t pos_res,
    num_p num_fft, uint64_t pos,
    uint64_t n,
    uint64_t bits
)
{
    CLU_HANDLER_IS_SAFE(num_fft_res)
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_fft_res)
    assert(num_fft)
    assert(num_fft_res->size >= pos_res + n)

    constexpr uint64_t mask = 0x3f;

    uint64_t count = bits >> chunk_bits_log_2;
    assert(count < n);
    bits &= mask;

    uint64_t * restrict dest = &num_fft_res->chunk[pos_res];
    const uint64_t * restrict src = &num_fft->chunk[pos];

    if(bits == 0)
    {
        memcpy(&dest[count], src, (n - count) * sizeof(uint64_t));
        memset(dest, 0, count * sizeof(uint64_t));
        return;
    }

    uint64_t inv_bits = chunk_bits - bits;
    #pragma GCC unroll 8
    for(uint64_t i = count + 1; i < n; i++)
    {
        dest[i] = (src[i - count] << bits) | (src[i - count - 1] >> inv_bits);
    }

    dest[count] = src[0] << bits;
    memset(dest, 0, count * sizeof(uint64_t));
}

void num_ssm_shr(
    num_p num_fft_res, uint64_t pos_res,
    num_p num_fft, uint64_t pos,
    uint64_t n,
    uint64_t bits
)
{
    CLU_HANDLER_IS_SAFE(num_fft_res)
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_fft_res)
    assert(num_fft)
    assert(num_fft_res->size >= pos_res + n)

    constexpr uint64_t mask = 0x3f;

    uint64_t count = bits >> chunk_bits_log_2;
    bits &= mask;

    uint64_t * restrict dest = &num_fft_res->chunk[pos_res];
    const uint64_t * restrict src = &num_fft->chunk[pos];

    if(bits == 0)
    {
        memcpy(dest, &src[count], (n - count) * sizeof(uint64_t));
        memset(&dest[n - count], 0, count * sizeof(uint64_t));
        return;
    }

    uint64_t inv_bits = chunk_bits - bits;
    uint64_t stop = n - count - 1;
    #pragma GCC unroll 8
    for(uint64_t i = 0; i < stop; i++)
    {
        dest[i] = (src[count + i] >> bits) | (src[count + i + 1] << inv_bits);
    }

    dest[n - count - 1] = src[n - 1] >> bits;
    memset(&dest[n - count], 0, count * sizeof(uint64_t));
}

// num_aux->size >= 2 * n
void num_ssm_shl_mod(
    num_p num_aux,
    num_p num_fft, uint64_t pos,
    uint64_t n,
    uint64_t bits
)
{
    CLU_HANDLER_IS_SAFE(num_aux)
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_aux)
    assert(num_fft)
    assert(num_aux->size >= 2 * n)
    assert(num_fft->size >= pos + n);

    if(bits == 0)
    {
        return;
    }

    num_ssm_shr(num_aux, 0, num_fft, pos, n, (chunk_bits * n) - chunk_bits - bits);
    num_ssm_shl(num_aux, n, num_fft, pos, n, bits);
    num_aux->chunk[(2 * n) - 1] = 0;
    num_ssm_sub_mod(num_fft, pos, num_aux, n, num_aux, 0, n);
}

// num_aux->size >= 2 * p->n
void num_ssm_shr_mod(
    num_p num_aux,
    num_p num_fft, uint64_t pos,
    uint64_t n,
    uint64_t bits
)
{
    CLU_HANDLER_IS_SAFE(num_aux)
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_aux)
    assert(num_fft)
    assert(bits <= 64 * (n - 1))
    assert(num_aux->size >= 2 * n)

    if(bits == 0)
    {
        return;
    }

    num_ssm_shl(num_aux, 0, num_fft, pos, n, (chunk_bits * n) - chunk_bits - bits);
    num_ssm_shr(num_aux, n, num_fft, pos, n, bits);
    num_aux->chunk[n - 1] = 0;
    num_ssm_sub_mod(num_fft, pos, num_aux, n, num_aux, 0, n);
}

// num_aux->size >= 2 * n
static void num_ssm_fft_fwd_rec(
    num_p num_aux,
    num_p num_fft, uint64_t pos,
    uint64_t step,
    uint64_t n,
    uint64_t K,
    uint64_t bits
)
{
    CLU_HANDLER_IS_SAFE(num_aux)
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_aux)
    assert(num_fft)
    assert(num_aux->size >= 2 * n)

    if(K > 2)
    {
        num_ssm_fft_fwd_rec(num_aux, num_fft, pos     , 2*step, n, K/2, 2*bits);
        num_ssm_fft_fwd_rec(num_aux, num_fft, pos+step, 2*step, n, K/2, 2*bits);
    }

    for(uint64_t i=0; i<K/2; i++)
    {
        uint64_t pos_1 = (pos + (step * (2 * i))) * n;
        uint64_t pos_2 = (pos + (step * ((2 * i) + 1))) * n;

        uint64_t shift = ssm_bit_inv(i, K / 2) * bits;
        num_ssm_shl_mod(num_aux, num_fft, pos_2, n, shift);

        num_ssm_sub_mod(num_aux, 0, num_fft, pos_1, num_fft, pos_2, n);
        num_ssm_add_mod_immed(num_fft, pos_1, num_fft, pos_2, n);
        memcpy(&num_fft->chunk[pos_2], num_aux->chunk, n * sizeof(uint64_t));
    }
}

// num_aux->size >= 2 * n
void num_ssm_fft_fwd(num_p num_aux, num_p num_fft, ssm_params_p p)
{
    CLU_HANDLER_IS_SAFE(num_aux)
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_aux)
    assert(num_fft)
    assert(num_aux->size >= 2 * p->n)
    assert(num_fft->size >= p->n * p->K)

    for(uint64_t i=0; i<p->K; i++)
    {
        num_ssm_shl_mod(num_aux, num_fft, p->n * i, p->n, p->Q * i);
    }

    num_ssm_fft_fwd_rec(num_aux, num_fft, 0, 1, p->n, p->K, 2 * p->Q);
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
        num_ssm_fft_inv_rec(num_aux, num, pos      , n, k/2, 2*bits);
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
void num_ssm_fft_inv(num_p num_aux, num_p num_fft, ssm_params_p p)
{
    CLU_HANDLER_IS_SAFE(num_aux)
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_aux)
    assert(num_fft)
    assert(num_aux->size >= 2 * p->n)

    num_ssm_fft_inv_rec(num_aux, num_fft, 0, p->n, p->K, 2 * p->Q);

    uint64_t k_ = stdc_trailing_zeros(p->K);
    uint64_t lim = ((chunk_bits * (p->n - 1)) - k_) / p->Q;
    for(uint64_t i=0; i<lim; i++)
    {
        num_ssm_shr_mod(num_aux, num_fft, p->n * i, p->n, (p->Q * i) + k_);
    }
    for(uint64_t i=lim; i<p->K; i++)
    {
        num_ssm_shr_mod(num_aux, num_fft, p->n * i, p->n, p->Q * i);
        num_ssm_shr_mod(num_aux, num_fft, p->n * i, p->n, k_);
    }
}

static bool ssm_is_recursive(uint64_t n)
{
    constexpr uint64_t threshold = 129;
    return (bool)((n > threshold) && (((n - 1) & (1 - n)) > 4));
}

// NOLINTBEGIN(readability-magic-numbers)
ssm_params_t ssm_get_params(uint64_t count)
{
    uint64_t M = B(stdc_bit_width(count) / 2);
    uint64_t K = 2 * stdc_bit_ceil((count + M - 1) / M);
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

    uint64_t moduli = (n - 1) & 7;
    if(moduli)
    {
        n += 8 - moduli;
        Q = 64 * (n - 1) / K;
    }
    assert(64 * (n - 1) % K == 0);

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
ssm_params_t ssm_get_params_wrap(uint64_t n)
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
    assert(64 * (_n - 1) % K == 0);

    uint64_t moduli = (_n - 1) & 7;
    if(moduli)
    {
        _n += 8 - moduli;
        Q = 64 * (_n - 1) / K;
    }
    assert(64 * (_n - 1) % K == 0);
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
    uint64_t * restrict src_1 = &num_1->chunk[pos];
    const uint64_t * restrict src_2 = &num_2->chunk[pos];

    // Both bodies below are 8-wide do-while loops over count words with no remainder
    // pass and no zero guard, so they are only correct for a count that is a non-zero
    // multiple of 8. Every ssm_get_params / ssm_get_params_wrap result satisfies that,
    // and num_mul_classic pays for the general case with a tail path and two skip
    // branches; state the narrower contract here instead of silently relying on it.
    constexpr uint64_t unroll_mask = 7;
    constexpr uint64_t unroll = 8;

    uint64_t count = n - 1;
    assert(count >= unroll && (count & unroll_mask) == 0)

    if(src_1[count] == 1)
    {
        memcpy(src_1, src_2, n * sizeof(uint64_t));
        num_ssm_opposite(num_1, pos, n);
        return;
    }

    if(src_2[count] == 1)
    {
        num_ssm_opposite(num_1, pos, n);
        return;
    }

#ifdef NUM_ASM_X86_64

    uint64_t high, low;
    uint64_t carry, _pos;
    uint64_t j;
    uint64_t i=count;
    uint64_t zero = 0;

    __asm__ __volatile__ (
        ".intel_syntax noprefix                         \n\t"

        "mov %[j], %[count]                             \n\t" // j = count
        "shr %[j], 3                                    \n\t" // j /= 8
        "mov rdx, [%[src_1]]                            \n\t" // D = *src_1
        "mov %[carry], 0                                \n\t" // carry = 0
        "xor %[_pos], %[_pos]                           \n\t" // _pos = 0

        "loop_0_begin%=:                                \n\t" // LOOP_0_BEGIN

        MUL_CLASSIC_STEP_ZERO( 0, high, carry, _pos)
        MUL_CLASSIC_STEP_ZERO( 8, carry, high, _pos)
        MUL_CLASSIC_STEP_ZERO(16, high, carry, _pos)
        MUL_CLASSIC_STEP_ZERO(24, carry, high, _pos)
        MUL_CLASSIC_STEP_ZERO(32, high, carry, _pos)
        MUL_CLASSIC_STEP_ZERO(40, carry, high, _pos)
        MUL_CLASSIC_STEP_ZERO(48, high, carry, _pos)
        MUL_CLASSIC_STEP_ZERO(56, carry, high, _pos)

        "lea %[_pos], [%[_pos] + 64]                    \n\t" // _pos += 64
        "dec %[j]                                       \n\t" // j--
        "jnz loop_0_begin%=                             \n\t"

        "adcx %[carry], %[zero]                         \n\t" // carry += CF
        "mov [%[dest] + %[_pos]], %[carry]              \n\t" // *(dest + _pos) = carry

        "lea %[src_1], [%[src_1] + 8]                   \n\t" // src_1 += 8
        "lea %[dest], [%[dest] + 8]                     \n\t" // dest += 8
        "dec %[i]                                       \n\t" // i--

        "loop_1_begin%=:                                \n\t"

        "mov %[j], %[count]                             \n\t" // j = count
        "mov rdx, [%[src_1]]                            \n\t" // D = *src_1
        "shr %[j], 3                                    \n\t" // j /= 8
        "mov %[carry], 0                                \n\t" // carry = 0
        "xor %[_pos], %[_pos]                           \n\t" // _pos = 0

        "loop_2_begin%=:                                \n\t"

        MUL_CLASSIC_STEP( 0, high, carry, src_2, _pos)
        MUL_CLASSIC_STEP( 8, carry, high, src_2, _pos)
        MUL_CLASSIC_STEP(16, high, carry, src_2, _pos)
        MUL_CLASSIC_STEP(24, carry, high, src_2, _pos)
        MUL_CLASSIC_STEP(32, high, carry, src_2, _pos)
        MUL_CLASSIC_STEP(40, carry, high, src_2, _pos)
        MUL_CLASSIC_STEP(48, high, carry, src_2, _pos)
        MUL_CLASSIC_STEP(56, carry, high, src_2, _pos)

        "adox %[carry], %[zero]                         \n\t" // carry += OF

        "lea %[_pos], [%[_pos] + 64]                    \n\t" // _pos += 64
        "dec %[j]                                       \n\t" // j--
        "jnz loop_2_begin%=                             \n\t"

        "adcx %[carry], %[zero]                         \n\t" // carry += CF
        "mov [%[dest] + %[_pos]], %[carry]              \n\t" // *(dest + _pos) = carry

        "lea %[src_1], [%[src_1] + 8]                   \n\t" // src_1 += 8
        "lea %[dest], [%[dest] + 8]                     \n\t" // dest += 8
        "dec %[i]                                       \n\t" // i--
        "jnz loop_1_begin%=                             \n\t"

        ".att_syntax prefix                             \n\t"
        // out
        :   [high] "=&r" (high),
            [low] "=&r" (low),
            [carry] "=&r" (carry),
            [_pos] "=&r" (_pos),
            [j] "=&r" (j),
            [i] "+&r" (i),
            [src_1] "+&r" (src_1),
            [src_2] "+&r" (src_2),
            [dest] "+&r" (dest)
        // in
        :   [zero] "r" (zero),
            [count] "r" (count)
        // clobber
        :   "cc",
            "memory",
            "rdx"
    );

    dest = &num_aux->chunk[0];

#else

    __builtin_assume((count & unroll_mask) == 0);

    uint128_t carry = 0;
    uint64_t v1 = src_1[0];
    #pragma GCC unroll 8
    for(uint64_t j = 0; j < count; j++)
    {
        carry += MUL(v1, src_2[j]);
        dest[j] = LOW(carry);
        carry = HIGH(carry);
    }
    dest[count] = LOW(carry);

    // Two rows of src_1 per pass: one dest load/store per two multiplies instead of
    // two. The two products plus dest plus carry exceed 128 bits, so the carry is kept
    // as two words (c0 full width, c1 a single bit) rather than a uint128_t.
    uint64_t i = 1;
    for(; i + 2 < count; i += 3)
    {
        uint64_t a = src_1[i];
        uint64_t b = src_1[i + 1];
        uint64_t c = src_1[i + 2];

        uint64_t c0 = 0;
        uint64_t c1 = 0;
        uint64_t prev_1 = 0; // src_2[j - 1], zero at j == 0
        uint64_t prev_2 = 0; // src_2[j - 2], zero at j <= 1

        #pragma GCC unroll 8
        for(uint64_t j = 0; j < count; j++)
        {
            uint64_t cur = src_2[j];
            uint128_t p1 = MUL(a, cur);
            uint128_t p2 = MUL(b, prev_1);
            uint128_t p3 = MUL(c, prev_2);
            prev_2 = prev_1;
            prev_1 = cur;

            uint128_t sum = U128(dest[i + j]) + LOW(p1) + LOW(p2) + LOW(p3) + c0;
            dest[i + j] = LOW(sum);

            uint128_t acc = U128(HIGH(sum)) + HIGH(p1) + HIGH(p2) + HIGH(p3) + c1;
            c0 = LOW(acc);
            c1 = HIGH(acc);
        }

        // the three words past the row block are written for the first time here, so
        // they are assigned; prev_1 is src_2[count - 1] and prev_2 is src_2[count - 2]
        uint128_t p2 = MUL(b, prev_1);
        uint128_t p3 = MUL(c, prev_2);
        uint128_t sum = U128(LOW(p2)) + LOW(p3) + c0;
        dest[i + count] = LOW(sum);

        uint128_t acc = U128(HIGH(sum)) + HIGH(p2) + HIGH(p3) + c1;
        c0 = LOW(acc);
        c1 = HIGH(acc);

        uint128_t p4 = MUL(c, prev_1);
        sum = U128(LOW(p4)) + c0;
        dest[i + count + 1] = LOW(sum);
        dest[i + count + 2] = LOW(U128(HIGH(sum)) + HIGH(p4) + c1);
    }

    for(; i < count; i++)
    {
        v1 = src_1[i];
        carry = 0;
        #pragma GCC unroll 8
        for(uint64_t j = 0; j < count; j++)
        {
            carry += dest[i + j] + MUL(v1, src_2[j]);
            dest[i + j] = LOW(carry);
            carry = HIGH(carry);
        }
        dest[i + count] = LOW(carry);
    }

#endif

    memmove(&dest[n], &dest[n-1], n * sizeof(uint64_t));
    dest[   n -1] = 0;
    dest[(2*n)-1] = 0;
    num_ssm_sub_mod(num_1, pos, num_aux, 0, num_aux, n, n);
}

// time_assembly_benchmark | time mul: 18.136 | original c
// time_assembly_benchmark | time mul: 13.173 | unrolled c
// time_assembly_benchmark | time mul: 12.394 | unrolled assembly 32 | only mul
// time_assembly_benchmark | time mul: 11.376 | unrolled assembly 8  | only mul
// time_assembly_benchmark | time mul: 10.837 | ssm add
// time_assembly_benchmark | time mul: 9.975 | better shift
// time_assembly_benchmark | time mul: 9.900 | sub immed


void num_ssm_pad_wrap(num_p num_fft, num_p num, uint64_t pos, ssm_params_p p)
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

// num_res[pos .. pos + n - 1] += num_src[src_pos .. src_pos + len - 1] << (64 * off)
// modulo 2^(64 * (n - 1)) + 1. Requires off + len <= n - 1, so nothing crosses the
// negacyclic boundary. Same arithmetic as num_ssm_add_mod_immed against a zero-padded
// n word operand, but only touches the span plus however far the carry actually travels.
static void num_ssm_add_span_mod(
    num_p num_res, uint64_t pos,
    num_p num_src, uint64_t src_pos,
    uint64_t off,
    uint64_t len,
    uint64_t n
)
{
    CLU_HANDLER_IS_SAFE(num_res)
    CLU_HANDLER_IS_SAFE(num_src)
    assert(num_res && num_src)
    assert(off + len <= n - 1)

    uint64_t * restrict dest = &num_res->chunk[pos];
    const uint64_t * restrict src = &num_src->chunk[src_pos];

    uint128_t carry = 0;
    #pragma GCC unroll 8
    for(uint64_t i = 0; i < len; i++)
    {
        carry += U128(dest[off + i]) + src[i];
        dest[off + i] = LOW(carry);
        carry = HIGH(carry);
    }

    for(uint64_t i = off + len; carry && i < n; i++)
    {
        carry += dest[i];
        dest[i] = LOW(carry);
        carry = HIGH(carry);
    }
    assert(!carry)

    num_ssm_normalize(num_res, pos, n);
}

// num_res[pos .. pos + n - 1] -= num_src[src_pos .. src_pos + len - 1] << (64 * off)
// modulo 2^(64 * (n - 1)) + 1. Counterpart of num_ssm_add_span_mod.
static void num_ssm_sub_span_mod(
    num_p num_res, uint64_t pos,
    num_p num_src, uint64_t src_pos,
    uint64_t off,
    uint64_t len,
    uint64_t n
)
{
    CLU_HANDLER_IS_SAFE(num_res)
    CLU_HANDLER_IS_SAFE(num_src)
    assert(num_res && num_src)
    assert(off + len <= n - 1)

    num_ssm_denormalize(num_res, pos, n);

    uint64_t * restrict dest = &num_res->chunk[pos];
    const uint64_t * restrict src = &num_src->chunk[src_pos];

    uint128_t borrow = 0;
    #pragma GCC unroll 8
    for(uint64_t i = 0; i < len; i++)
    {
        uint128_t diff = U128(dest[off + i]) - src[i] - borrow;
        dest[off + i] = LOW(diff);
        borrow = HIGH(diff) & 1;
    }

    for(uint64_t i = off + len; borrow && i < n; i++)
    {
        uint128_t diff = U128(dest[i]) - borrow;
        dest[i] = LOW(diff);
        borrow = HIGH(diff) & 1;
    }
    assert(!borrow)

    num_ssm_normalize(num_res, pos, n);
}

// num_aux_1->size >= n
// num_aux_2->size >= 2 * n
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void num_ssm_depad_wrap(
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

        // Common case: the whole coefficient sits below the negacyclic boundary, so it
        // goes straight into the accumulator at its word offset. Only the last few
        // coefficients (p->n > 2 * p->M, so at least the last one) need the wrap path.
        if(dest_pos + src_len <= wrap_boundary)
        {
            if(is_add)
            {
                num_ssm_add_span_mod(num_res, pos, num_fft, src_pos, dest_pos, src_len, n);
            }
            else
            {
                num_ssm_sub_span_mod(num_res, pos, num_fft, src_pos, dest_pos, src_len, n);
            }
            continue;
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
void num_ssm_mul_wrap(
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



num_p num_ssm_pad_no_wrap(num_p num, ssm_params_p p)
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

num_p num_ssm_depad_no_wrap(num_p num, ssm_params_p p)
{
    CLU_HANDLER_IS_SAFE(num)
    assert(num)

    uint64_t target_count = (p->M * (p->K - 1)) + p->n;
    num_p num_res = num_create(CLU_ARGS(target_count, 0));

    for(uint64_t i = 0; i < p->K; i++)
    {
        num_t block;
        num_span(&block, num, p->n * i, p->n * (i + 1));
        num_add_offset(num_res, p->M * i, &block);
    }

    num_free(num);
    return num_normalize(num_res);
}

// num_aux->size >= 2 * n
static num_p num_ssm_prepare_no_wrap(
    num_p num_aux,
    num_p num,
    ssm_params_p p,
    bool free_inputs
)
{
    CLU_HANDLER_IS_SAFE(num_aux)
    CLU_HANDLER_IS_SAFE(num)
    assert(num_aux)
    assert(num)
    assert(num_aux->size >= 2 * p->n)

    num_p num_fft = num_ssm_pad_no_wrap(num, p);
    if(free_inputs)
    {
        num_free(num);
    }

    num_ssm_fft_fwd(num_aux, num_fft, p);
    return num_fft;
}

// KEEPS NUM_1 NUM_2
num_p num_mul_ssm(num_p num_1, num_p num_2, bool free_inputs)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    ssm_params_t p = ssm_get_params(num_1->count + num_2->count);
    num_p num_aux_1 = num_create_dirty(CLU_ARGS(p.n, 0));
    num_p num_aux_2 = num_create_dirty(CLU_ARGS(2 * p.n, 0));
    num_p num_fft_1 = num_ssm_prepare_no_wrap(num_aux_2, num_1, &p, free_inputs);
    num_p num_fft_2 = num_ssm_prepare_no_wrap(num_aux_2, num_2, &p, free_inputs);

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

num_p num_mul_core(num_p num_1, num_p num_2, bool free_inputs)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    if(num_1->count == 0 || num_2->count == 0)
    {
        if(free_inputs)
        {
            num_free(num_1);
            num_free(num_2);
        }
        return num_wrap(0);
    }

    if(mul_is_classic(num_1->count, num_2->count))
    {
        num_p num_res =  num_mul_classic(num_1, num_2);
        if(free_inputs)
        {
            num_free(num_1);
            num_free(num_2);
        }
        return num_res;
    }

    return num_mul_ssm(num_1, num_2, free_inputs);
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

    num_sqr_classic_buffer(num_aux, &num_aux_piece);

    memmove(&num_aux->chunk[n], &num_aux->chunk[n-1], n * sizeof(uint64_t));
    num_aux->chunk[n-1] = 0;
    num_ssm_sub_mod(num, pos, num_aux, 0, num_aux, n, n);
}

static void num_ssm_sqr_pointwise(
    num_p num_aux_1,
    num_p num_aux_2,
    num_p num_fft,
    ssm_params_p p
);

// KEEPS NUM
static void num_ssm_sqr_wrap(
    num_p num_aux_1,
    num_p num_aux_2,
    num_p num_fft,
    num_p num,
    uint64_t pos,
    ssm_params_p p
)
{
    CLU_HANDLER_IS_SAFE(num)
    assert(num)

    num_ssm_prepare_wrap(num_aux_2, num_fft, num, pos, p);

    num_ssm_sqr_pointwise(
        num_aux_1,
        num_aux_2,
        num_fft,
        p
    );

    num_ssm_fft_inv(num_aux_2, num_fft, p);
    num_ssm_depad_wrap(
        num_aux_1,
        num_aux_2,
        num,
        pos,
        num_fft,
        p
    );
}

// num_aux_1->size >= p->n
// num_aux_2->size >= 2 * p->n
static void num_ssm_sqr_pointwise(
    num_p num_aux_1,
    num_p num_aux_2,
    num_p num_fft,
    ssm_params_p p
)
{
    CLU_HANDLER_IS_SAFE(num_aux_1)
    CLU_HANDLER_IS_SAFE(num_aux_2)
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_aux_1)
    assert(num_aux_2)
    assert(num_fft)
    assert(num_aux_1->size >= p->n)
    assert(num_aux_2->size >= 2 * p->n)

    if(!ssm_is_recursive(p->n))
    {
        for(uint64_t i=0; i<p->K; i++)
        {
            num_ssm_sqr_mod_span(num_aux_2, num_fft, i * p->n, p->n);
        }
        return;
    }

    ssm_params_t p_next = ssm_get_params_wrap(p->n);
    num_p num_fft_next = num_create_dirty(CLU_ARGS(p_next.n * p_next.K, 0));
    for(uint64_t i=0; i<p->K; i++)
    {
        // NOLINTNEXTLINE(readability-suspicious-call-argument)
        num_ssm_sqr_wrap(
            num_aux_1,
            num_aux_2,
            num_fft_next,
            num_fft,
            i * p->n,
            &p_next
        );
    }
    num_free(num_fft_next);
}

num_p num_sqr_ssm(num_p num)
{
    CLU_HANDLER_IS_SAFE(num)
    assert(num)

    ssm_params_t p = ssm_get_params(2 * num->count);
    num_p num_aux_1 = num_create_dirty(CLU_ARGS(p.n, 0));
    num_p num_aux_2 = num_create_dirty(CLU_ARGS(2 * p.n, 0));
    num_p num_fft = num_ssm_prepare_no_wrap(num_aux_2, num, &p, true);

    num_ssm_sqr_pointwise(num_aux_1, num_aux_2, num_fft, &p);
    num_free(num_aux_1);

    num_ssm_fft_inv(num_aux_2, num_fft, &p);
    num_free(num_aux_2);

    return num_ssm_depad_no_wrap(num_fft, &p);
}

// Returns quotient
// NUM becomes remainder
num_p num_div_mod_uint(num_p num, uint64_t value)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);
    assert(value);

    uint64_t initial_count = num->count;
    num_p num_q = num_create_dirty(CLU_ARGS(initial_count, initial_count));

    uint64_t * restrict dest = num_q->chunk;
    uint64_t * src = num->chunk; // Cached to avoid double indirection

    for(uint64_t i = initial_count - 1; i != UINT64_MAX; i--)
    {
        uint64_t current_count = num->count; // Read once per iteration
        if((current_count < i) || ((current_count - 1 == i) && (src[i] < value)))
        {
            dest[i] = 0;
            continue;
        }

        if(current_count - 1 == i)
        {
            uint64_t r = src[i] / value;
            dest[i] = r;
            num_sub_uint_offset(num, i, r * value);
            continue;
        }

        uint128_t value_1 = U128HL(src[i + 1], src[i]);
        uint64_t r = U64(value_1 / value);
        value_1 = MUL(r, value);

        num_sub_uint_offset(num, i + 1, HIGH(value_1));
        num_sub_uint_offset(num, i  , LOW(value_1));
        dest[i] = r;
    }

    num_normalize(num_q);
    return num_q;
}

// Input expected to be normalized
// Returns quotient
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

    uint64_t count_2 = num_2->count;
    uint64_t count = num_1->count - count_2 + 1;
    num_p num_q = num_create_dirty(CLU_ARGS(count, count));

    uint64_t * restrict dest = num_q->chunk;
    uint64_t * src_1 = num_1->chunk;
    uint64_t value_2 = num_2->chunk[count_2 - 1];

    for(uint64_t i = count - 1; i != UINT64_MAX; i--)
    {
        if(num_cmp_offset(num_1, i, num_2) < 0)
        {
            dest[i] = 0;
            continue;
        }

        uint64_t current_count_1 = num_1->count;
        if(current_count_1 == count_2 + i)
        {
            dest[i] = 1;
            num_sub_offset(num_1, i, num_2);
            continue;
        }

        uint64_t r;
        if(src_1[current_count_1 - 1] == value_2)
        {
            r = UINT64_MAX;
        }
        else
        {
            uint128_t value_1 = U128HL(src_1[current_count_1 - 1], src_1[current_count_1 - 2]);
            r = U64(value_1 / value_2);
        }

        num_mul_uint_buffer(num_aux, num_2, r);
        while(num_cmp_offset(num_1, i, num_aux) < 0)
        {
            r--;
            num_sub_offset(num_aux, 0, num_2);
        }

        dest[i] = r;
        num_sub_offset(num_1, i, num_aux);
    }

    num_normalize(num_q);
    return num_q;
}

// Input expected to be normalized
// Returns quotient
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
// Returns quotient
// NUM_1 becomes remainder
// Keeps NUM_2
static num_p num_div_mod_bz_rec(
    num_p num_aux,
    num_p num_1,
    num_p num_2,
    bz_frame_t f[]
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
            &f[1]
        );
        num_normalize(num_1);

        if(num_is_zero(num_q_tmp))
        {
            num_q[i] = num_q_tmp;
            continue;
        }

        num_p num_aux_2 = num_mul_core(num_q_tmp, &f->num_2_0, false);
        while(num_cmp_offset(num_1, k * i, num_aux_2) < 0)
        {
            num_q_tmp = num_sub_uint(num_q_tmp, 1);
            num_add_offset(num_1, k * i, num_2);
        }
        num_sub_offset(num_1, k * i, num_aux_2);
        num_free(num_aux_2);

        num_q[i] = num_q_tmp;
    }
    num_q[0] = num_expand_to(num_q[0], k + num_q[1]->count);
    num_add_offset(num_q[0], k, num_q[1]);

    num_free(num_q[1]);
    return num_q[0];
}

// Input expected to be normalized
// Returns quotient
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

    uint64_t n_1 = num_1->count;
    uint64_t n_2 = num_2->count;
    for(; n_1 > 2 * n_2; n_1 -= n_2)
    {
        num_t num_1_1;
        num_span(&num_1_1, num_1, n_1 - (2 * n_2), num_1->count);

        num_p num_q_tmp = num_div_mod_bz_rec(num_aux, &num_1_1, num_2, f);
        num_normalize(num_1);
        num_q_tmp = num_expand_to(num_q_tmp, n_2 + num_q->count);
        num_add_offset(num_q_tmp, n_2, num_q);
        num_free(num_q);
        num_q = num_q_tmp;
    }

    num_p num_q_tmp = num_div_mod_bz_rec(num_aux, num_1, num_2, f);
    num_q_tmp = num_expand_to(num_q_tmp, n_1 - n_2 + num_q->count);
    num_add_offset(num_q_tmp, n_1 - n_2, num_q);

    num_free(num_aux);
    num_free(num_q);
    return num_q_tmp;
}

// Forces the most significant chunk of the dividend to be >= 2^63
uint64_t num_div_normalize(num_p *num_1, num_p *num_2) // TODO TEST
{
    CLU_HANDLER_IS_SAFE(*num_1);
    CLU_HANDLER_IS_SAFE(*num_2);
    assert(*num_1);
    assert(*num_2);

    uint64_t bits = chunk_bits - stdc_bit_width((*num_2)->chunk[(*num_2)->count-1]);
    (*num_1) = num_expand_to((*num_1), (*num_1)->count + 1);
    num_shl_core(*num_1, bits);
    num_shl_core(*num_2, bits);
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
        num_shr_core(num_1, bits);
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
    uint64_t count = bits >> chunk_bits_log_2;
    num = num_expand_to(num, num->count + count + 1);
    num_shl_core(num, bits & mask);
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
    num_shr_core(num, bits & mask);
    return num;
}

num_p num_add_uint(num_p num, uint64_t value)
{
    CLU_HANDLER_IS_SAFE(num)
    assert(num)

    num = num_expand_to(num, num->count + 1);
    num_add_uint_offset(num, 0, value);
    return num;
}

num_p num_sub_uint(num_p num, uint64_t value)
{
    CLU_HANDLER_IS_SAFE(num)
    assert(num)

    num_sub_uint_offset(num, 0, value);
    return num;
}

num_p num_mul_uint(num_p num, uint64_t value)
{
    CLU_HANDLER_IS_SAFE(num)
    assert(num)

    if(value == 0)
    {
        num_clear(num);
        return num;
    }

    num_p num_res = num_create_dirty(CLU_ARGS(num->count + 1, 0));
    num_mul_uint_buffer(num_res, num, value);
    num_free(num);

    return num_res;
}

num_p num_add(num_p num_1, num_p num_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    uint64_t count = num_1->count > num_2->count ? num_1->count : num_2->count;
    num_1 = num_expand_to(num_1, count + 1);
    num_add_offset(num_1, 0, num_2);

    num_free(num_2);
    return num_1;
}

num_p num_sub(num_p num_1, num_p num_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    num_sub_offset(num_1, 0, num_2);

    num_free(num_2);
    return num_1;
}

num_p num_mul(num_p num_1, num_p num_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    return num_mul_core(num_1, num_2, true);
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
    num_r = num_expand_to(num_r, B(i) + num_q->count);
    num_add_offset(num_r, B(i), num_q);
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
