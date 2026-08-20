#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include "debug.h"
#include "internal.h"
#include "../../mods/macros/assert.h" // IWYU pragma: keep
#include "../../mods/macros/stdbit.h" // IWYU pragma: keep
#include "../../mods/macros/threads.h"
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
    .disk_threshold_bytes = UINT64_MAX,
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

uint64_t araucaria_disk_config_get_threshold_bytes()
{
    return s_araucaria_disk_config.disk_threshold_bytes;
}



static void num_display_dec_core(num_p num, uint64_t threads)
{
    if(num->count == 0)
    {
        printf("0");
        return;
    }

    constexpr uint64_t base = 1'000'000'000'000'000'000;
    num = num_base_to_threads(num_copy(num), base, threads);
    printf(U64P(), num->chunk[num->count-1]);
    for(uint64_t i=num->count-2; i!=UINT64_MAX; i--)
    {
        printf(U64P(018), num->chunk[i]);
    }

    num_free(num);
}

void num_display_dec(num_p num)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    num_display_dec_core(num, 1);
}

// Same as num_display_dec, but the caller picks how many threads the base
// conversion (num_base_to_threads) may fan out across -- see its comment for how
// the thread budget gets split across the conversion's recursion.
void num_display_dec_threads(num_p num, uint64_t threads)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    num_display_dec_core(num, threads);
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

    if(total_size > s_araucaria_disk_config.disk_threshold_bytes)
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

    if(total_size > s_araucaria_disk_config.disk_threshold_bytes)
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
    // Unregister before unmapping: once munmap runs, the kernel is free to hand this
    // VA range to a concurrent mmap on another thread, whose registration would then
    // race this handle's removal from clu's tracking if the order were reversed.
    CLU_HANDLER_UNREGISTER(num)
    int res = munmap(num, total_size);
    assert(res == 0);
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

#if !defined(NO_ASSEMBLY) && defined(__x86_64__) && defined(__BMI2__) && defined(__ADX__) && defined(__GNUC__) && !defined(__clang__)
    #define NUM_ASM_X86_64
#elif !defined(NO_ASSEMBLY) && defined(__aarch64__)
    #define NUM_ASM_AARCH64
#endif

#ifdef NUM_ASM_X86_64

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

#define BUTTERFLY_STEP(OFF)                                                          \
    "mov %[a], [%[dest_1] + " #OFF "]                   \n\t" /* a = *(dest_1 + OFF) */\
    "mov %[b], [%[dest_2] + " #OFF "]                   \n\t" /* b = *(dest_2 + OFF) */\
    "mov %[nb], %[b]                                    \n\t" /* nb = b              */\
    "not %[nb]                                          \n\t" /* nb = ~b             */\
    "mov %[s], %[a]                                     \n\t" /* s = a               */\
    "adox %[s], %[b]                                    \n\t" /* s = a + b + OF      */\
    "adcx %[a], %[nb]                                   \n\t" /* a = a + ~b + CF     */\
    "mov [%[dest_1] + " #OFF "], %[s]                   \n\t" /* *(dest_1 + OFF) = s */\
    "mov [%[dest_2] + " #OFF "], %[a]                   \n\t" /* *(dest_2 + OFF) = a */\

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

// normalizes coeficient if it is less than 2 modulus
static void num_ssm_normalize(num_p num_fft, uint64_t pos, uint64_t n)
{
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_fft)
    assert(num_fft->chunk[pos + n - 1] <= 2)

    uint64_t * chunk = &num_fft->chunk[pos];

    // The top word is the carry out of a 64 * (n - 1) bit add, so it is 0 or 1 about half
    // the time each and testing it first costs a mispredict. Reducing it unconditionally
    // is a load, a subtract and two stores, and the borrow out of word 0 needs word 0 to
    // have been below the top word, which for a full width coefficient never happens.
    uint64_t value = chunk[n - 1];
    uint64_t word = chunk[0];
    chunk[n - 1] = 0;
    chunk[0] = word - value;
    if(word >= value)
    {
        return;
    }

    for(uint64_t i = 1; i < n; i++)
    {
        uint64_t borrowed = chunk[i]--;
        if(borrowed)
        {
            break;
        }
    }

    if(chunk[n - 1] != UINT64_MAX)
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

// Eight words per pass. The four flag shuffling instructions are per pass, not per word,
// so doubling the block halves them; stp leaves NZCV alone, which is what lets the sum
// stores sit inside the adcs chain and keeps the sum registers down to four.
#define BUTTERFLY_STEP_8                                                                                    \
    "ldp %[a_0], %[a_1], [%[dest_1]]                \n\t" /* (a_0, a_1)  = *dest_1                       */  \
    "ldp %[a_2], %[a_3], [%[dest_1], #16]           \n\t" /* (a_2, a_3)  = *(dest_1 + 16)                */  \
    "ldp %[a_4], %[a_5], [%[dest_1], #32]           \n\t" /* (a_4, a_5)  = *(dest_1 + 32)                */  \
    "ldp %[a_6], %[a_7], [%[dest_1], #48]           \n\t" /* (a_6, a_7)  = *(dest_1 + 48)                */  \
    "ldp %[b_0], %[b_1], [%[dest_2]]                \n\t" /* (b_0, b_1)  = *dest_2                       */  \
    "ldp %[b_2], %[b_3], [%[dest_2], #16]           \n\t" /* (b_2, b_3)  = *(dest_2 + 16)                */  \
    "ldp %[b_4], %[b_5], [%[dest_2], #32]           \n\t" /* (b_4, b_5)  = *(dest_2 + 32)                */  \
    "ldp %[b_6], %[b_7], [%[dest_2], #48]           \n\t" /* (b_6, b_7)  = *(dest_2 + 48)                */  \
    "subs xzr, %[carry], #1                         \n\t" /* CF = carry                                 */  \
    "adcs %[s_0], %[a_0], %[b_0]                    \n\t" /* s_0 = a_0 + b_0 + CF                       */  \
    "adcs %[s_1], %[a_1], %[b_1]                    \n\t" /* s_1 = a_1 + b_1 + CF                       */  \
    "adcs %[s_2], %[a_2], %[b_2]                    \n\t" /* s_2 = a_2 + b_2 + CF                       */  \
    "adcs %[s_3], %[a_3], %[b_3]                    \n\t" /* s_3 = a_3 + b_3 + CF                       */  \
    "stp %[s_0], %[s_1], [%[dest_1]]                \n\t" /* *dest_1        = (s_0, s_1)                */  \
    "stp %[s_2], %[s_3], [%[dest_1], #16]           \n\t" /* *(dest_1 + 16) = (s_2, s_3)                */  \
    "adcs %[s_0], %[a_4], %[b_4]                    \n\t" /* s_0 = a_4 + b_4 + CF                       */  \
    "adcs %[s_1], %[a_5], %[b_5]                    \n\t" /* s_1 = a_5 + b_5 + CF                       */  \
    "adcs %[s_2], %[a_6], %[b_6]                    \n\t" /* s_2 = a_6 + b_6 + CF                       */  \
    "adcs %[s_3], %[a_7], %[b_7]                    \n\t" /* s_3 = a_7 + b_7 + CF                       */  \
    "cset %[carry], cs                              \n\t" /* carry = CF                                 */  \
    "stp %[s_0], %[s_1], [%[dest_1], #32]           \n\t" /* *(dest_1 + 32) = (s_0, s_1)                */  \
    "stp %[s_2], %[s_3], [%[dest_1], #48]           \n\t" /* *(dest_1 + 48) = (s_2, s_3)                */  \
    "cmp xzr, %[borrow]                             \n\t" /* CF = 1 - borrow                            */  \
    "sbcs %[a_0], %[a_0], %[b_0]                    \n\t" /* a_0 -= b_0 + (1 - CF)                      */  \
    "sbcs %[a_1], %[a_1], %[b_1]                    \n\t" /* a_1 -= b_1 + (1 - CF)                      */  \
    "sbcs %[a_2], %[a_2], %[b_2]                    \n\t" /* a_2 -= b_2 + (1 - CF)                      */  \
    "sbcs %[a_3], %[a_3], %[b_3]                    \n\t" /* a_3 -= b_3 + (1 - CF)                      */  \
    "stp %[a_0], %[a_1], [%[dest_2]]                \n\t" /* *dest_2        = (a_0, a_1)                */  \
    "stp %[a_2], %[a_3], [%[dest_2], #16]           \n\t" /* *(dest_2 + 16) = (a_2, a_3)                */  \
    "sbcs %[a_4], %[a_4], %[b_4]                    \n\t" /* a_4 -= b_4 + (1 - CF)                      */  \
    "sbcs %[a_5], %[a_5], %[b_5]                    \n\t" /* a_5 -= b_5 + (1 - CF)                      */  \
    "sbcs %[a_6], %[a_6], %[b_6]                    \n\t" /* a_6 -= b_6 + (1 - CF)                      */  \
    "sbcs %[a_7], %[a_7], %[b_7]                    \n\t" /* a_7 -= b_7 + (1 - CF)                      */  \
    "cset %[borrow], cc                             \n\t" /* borrow = 1 - CF                            */  \
    "stp %[a_4], %[a_5], [%[dest_2], #32]           \n\t" /* *(dest_2 + 32) = (a_4, a_5)                */  \
    "stp %[a_6], %[a_7], [%[dest_2], #48]           \n\t" /* *(dest_2 + 48) = (a_6, a_7)                */

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

    uint128_t carry = 0;
    #pragma GCC unroll 8
    for(uint64_t i = 0; i < n; i++)
    {
        carry += U128(dest[i]) + src_2[i];
        dest[i] = LOW(carry);
        carry = HIGH(carry);
    }

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
    uint64_t rem = n & 7;

    __asm__ __volatile__ (
        ".intel_syntax noprefix                         \n\t"

        "shr %[j], 3                                    \n\t" // j /= 8
        "xor %[pos], %[pos]                             \n\t" // pos = 0 (and inherently clears CF)
        "test %[j], %[j]                                \n\t" // no borrow chain live yet: free to disturb CF here
        "jz sub_tail_setup%=                            \n\t"

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

        "sub_tail_setup%=:                              \n\t"
        "mov rcx, %[rem]                                \n\t"
        "jrcxz sub_tail_skip%=                          \n\t"

        "sub_tail_begin%=:                               \n\t"

        SUB_CLASSIC_STEP(0, src_1, reg_1)

        "lea %[pos], [%[pos] + 8]                       \n\t"
        "dec rcx                                        \n\t" // dec leaves CF alone
        "jnz sub_tail_begin%=                            \n\t"

        "sub_tail_skip%=:                               \n\t"

        ".att_syntax prefix                             \n\t"
        // out
        :   [pos] "+&r" (pos),
            [j] "+&r" (j),
            [reg_1] "=&r" (reg_1),
            [reg_2] "=&r" (reg_2)
        // in
        :   [dest] "r" (dest),
            [src_1] "r" (src_1),
            [src_2] "r" (src_2),
            [rem] "r" (rem)
        // clobber
        :   "rcx",
            "cc",
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
    uint64_t rem = n & 7;

    __asm__ __volatile__ (
        ".intel_syntax noprefix                         \n\t"

        "shr %[j], 3                                    \n\t" // j /= 8
        "xor %[pos], %[pos]                             \n\t" // pos = 0 (and inherently clears CF)
        "test %[j], %[j]                                \n\t" // no borrow chain live yet: free to disturb CF here
        "jz sub_tail_setup%=                            \n\t"

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

        "sub_tail_setup%=:                              \n\t"
        "mov rcx, %[rem]                                \n\t"
        "jrcxz sub_tail_skip%=                          \n\t"

        "sub_tail_begin%=:                               \n\t"

        SUB_CLASSIC_STEP(0, dest, reg_1)

        "lea %[pos], [%[pos] + 8]                       \n\t"
        "dec rcx                                        \n\t" // dec leaves CF alone
        "jnz sub_tail_begin%=                            \n\t"

        "sub_tail_skip%=:                               \n\t"

        ".att_syntax prefix                             \n\t"
        // out
        :   [pos] "+&r" (pos),
            [j] "+&r" (j),
            [reg_1] "=&r" (reg_1),
            [reg_2] "=&r" (reg_2)
        // in
        :   [dest] "r" (dest),
            [src_2] "r" (src_2),
            [rem] "r" (rem)
        // clobber
        :   "rcx",
            "cc",
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

void num_ssm_opposite(num_p num_fft, uint64_t chunk_pos, uint64_t n)
{
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_fft)

    uint64_t * restrict dest = &num_fft->chunk[chunk_pos];

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

static void num_ssm_sub_span_mod(
    num_p num_res, uint64_t pos,
    num_p num_src, uint64_t src_pos,
    uint64_t off,
    uint64_t len,
    uint64_t n
);

// The negacyclic rotate below only needs the few words of the shifted operand that cross
// the 2^(64 * (n - 1)) boundary, and it can shift the coefficient where it already lives
// instead of building both halves in num_aux. These four helpers are what num_ssm_shl and
// num_ssm_shr become once the destination is the source and the span is bounded: no
// memset over the words that are known zero, and no second array to read back.

// num_res[pos_res .. pos_res + len) = (num_fft[pos .. pos + n) >> bits), low words only.
// Requires (bits / 64) + len == n - 1, which is what the callers below always pass.
static void num_ssm_shr_low(
    num_p num_res, uint64_t pos_res,
    num_p num_fft, uint64_t pos,
    uint64_t n,
    uint64_t bits,
    uint64_t len
)
{
    CLU_HANDLER_IS_SAFE(num_res)
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_res && num_fft)

    constexpr uint64_t mask = 0x3f;

    uint64_t count = bits >> chunk_bits_log_2;
    bits &= mask;
    assert(count + len == n - 1)

    uint64_t * restrict dest = &num_res->chunk[pos_res];
    const uint64_t * restrict src = &num_fft->chunk[pos];

    if(bits == 0)
    {
        memcpy(dest, &src[count], len * sizeof(uint64_t));
        return;
    }

    uint64_t inv_bits = chunk_bits - bits;
    #pragma GCC unroll 8
    for(uint64_t i = 0; i < len; i++)
    {
        dest[i] = (src[count + i] >> bits) | (src[count + i + 1] << inv_bits);
    }
}

// num_res[pos_res + off .. pos_res + off + len) = words [off, off + len) of
// (num_fft[pos .. pos + n) << bits), where off == bits / 64.
static void num_ssm_shl_high(
    num_p num_res, uint64_t pos_res,
    num_p num_fft, uint64_t pos,
    uint64_t bits,
    uint64_t len
)
{
    CLU_HANDLER_IS_SAFE(num_res)
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_res && num_fft)

    constexpr uint64_t mask = 0x3f;

    uint64_t count = bits >> chunk_bits_log_2;
    bits &= mask;

    uint64_t * restrict dest = &num_res->chunk[pos_res + count];
    const uint64_t * restrict src = &num_fft->chunk[pos];

    if(bits == 0)
    {
        memcpy(dest, src, len * sizeof(uint64_t));
        return;
    }

    uint64_t inv_bits = chunk_bits - bits;
    #pragma GCC unroll 8
    for(uint64_t i = len - 1; i != 0; i--)
    {
        dest[i] = (src[i] << bits) | (src[i - 1] >> inv_bits);
    }

    dest[0] = src[0] << bits;
}

// num_fft[pos .. pos + n) <<= bits, in place. Descending so the words a step reads are
// always below the word it writes.
static void num_ssm_shl_self(num_p num_fft, uint64_t pos, uint64_t n, uint64_t bits)
{
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_fft)

    constexpr uint64_t mask = 0x3f;

    uint64_t count = bits >> chunk_bits_log_2;
    assert(count < n)
    bits &= mask;

    uint64_t * chunk = &num_fft->chunk[pos];

    if(bits == 0)
    {
        memmove(&chunk[count], chunk, (n - count) * sizeof(uint64_t));
        memset(chunk, 0, count * sizeof(uint64_t));
        return;
    }

    uint64_t inv_bits = chunk_bits - bits;
    #pragma GCC unroll 8
    for(uint64_t i = n - 1; i > count; i--)
    {
        chunk[i] = (chunk[i - count] << bits) | (chunk[i - count - 1] >> inv_bits);
    }

    chunk[count] = chunk[0] << bits;
    memset(chunk, 0, count * sizeof(uint64_t));
}

// num_fft[pos .. pos + n) >>= bits, in place. Ascending, for the mirror reason.
static void num_ssm_shr_self(num_p num_fft, uint64_t pos, uint64_t n, uint64_t bits)
{
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_fft)

    constexpr uint64_t mask = 0x3f;

    uint64_t count = bits >> chunk_bits_log_2;
    assert(count < n)
    bits &= mask;

    uint64_t * chunk = &num_fft->chunk[pos];

    if(bits == 0)
    {
        memmove(chunk, &chunk[count], (n - count) * sizeof(uint64_t));
        memset(&chunk[n - count], 0, count * sizeof(uint64_t));
        return;
    }

    uint64_t inv_bits = chunk_bits - bits;
    uint64_t stop = n - count - 1;
    #pragma GCC unroll 8
    for(uint64_t i = 0; i < stop; i++)
    {
        chunk[i] = (chunk[count + i] >> bits) | (chunk[count + i + 1] << inv_bits);
    }

    chunk[stop] = chunk[n - 1] >> bits;
    memset(&chunk[n - count], 0, count * sizeof(uint64_t));
}

// Words of the shifted coefficient that land at or above 2^(64 * (n - 1)). A normalized
// coefficient is below 2^(64 * (n - 1)) unless it is exactly that power, which is the one
// case where the top word is set and the wrap spans the whole span; the rotates fall back
// to the two-buffer path for it rather than special-casing the count.
static uint64_t ssm_wrap_len(uint64_t bits)
{
    constexpr uint64_t mask = 0x3f;
    uint64_t count = bits >> chunk_bits_log_2;
    return (bits & mask) ? count + 1 : count;
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

    uint64_t len = ssm_wrap_len(bits);
    if(num_fft->chunk[pos + n - 1] || len == 0 || len > n - 1)
    {
        num_ssm_shr(num_aux, 0, num_fft, pos, n, (chunk_bits * n) - chunk_bits - bits);
        num_ssm_shl(num_aux, n, num_fft, pos, n, bits);
        num_aux->chunk[(2 * n) - 1] = 0;
        num_ssm_sub_mod(num_fft, pos, num_aux, n, num_aux, 0, n);
        return;
    }

    num_ssm_shr_low(num_aux, 0, num_fft, pos, n, (chunk_bits * n) - chunk_bits - bits, len);
    num_ssm_shl_self(num_fft, pos, n, bits);
    num_fft->chunk[pos + n - 1] = 0;
    num_ssm_sub_span_mod(num_fft, pos, num_aux, 0, 0, len, n);
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

    uint64_t len = ssm_wrap_len(bits);
    if(num_fft->chunk[pos + n - 1] || len == 0 || len > n - 1)
    {
        num_ssm_shl(num_aux, 0, num_fft, pos, n, (chunk_bits * n) - chunk_bits - bits);
        num_ssm_shr(num_aux, n, num_fft, pos, n, bits);
        num_aux->chunk[n - 1] = 0;
        num_ssm_sub_mod(num_fft, pos, num_aux, n, num_aux, 0, n);
        return;
    }

    uint64_t off = n - 1 - len;
    num_ssm_shl_high(num_aux, 0, num_fft, pos, (chunk_bits * n) - chunk_bits - bits, len);
    num_ssm_shr_self(num_fft, pos, n, bits);
    num_ssm_sub_span_mod(num_fft, pos, num_aux, off, off, len, n);
}

// fft[pos_1] = A + B and fft[pos_2] = A - B, reading each coefficient once. Splitting it
// into num_ssm_sub_mod into num_aux, num_ssm_add_mod_immed and a copy back reads both
// coefficients twice and writes n words of num_aux that exist only to be copied.
//
// The two chains cannot share the flags, so each block reseeds its own: subs against the
// saved carry for adcs, cmp against the saved borrow for sbcs. Both are two instructions
// per four words, which is what buys the second pass.
//
// Shared by every flat, K-indexed loop this file parallelizes (FFT pre/post-loops,
// pointwise's K-loop): only the outermost call of whichever recursion it's used in
// parallelizes; deeper recursive calls are invoked with threads=1, so this never has
// to reason about nested fan-out.
static uint64_t ssm_worker_count(uint64_t threads, uint64_t K)
{
    uint64_t workers = threads < K ? threads : K;
    return workers ? workers : 1;
}

static void ssm_worker_range(
    uint64_t worker,
    uint64_t workers,
    uint64_t K,
    uint64_t * out_start,
    uint64_t * out_end
)
{
    *out_start = (K * worker) / workers;
    *out_end = (K * (worker + 1)) / workers;
}



// A borrowing difference leaves A - B + 2^(64 * n) in the span where A - B + 2^(64 * (n
// - 1)) + 1 is wanted, so the modulus is added back afterwards. That is what
// num_ssm_denormalize does, except the carry out of the top word here is the 2^(64 * n)
// being discarded rather than an overflow to assert on.
static void num_ssm_butterfly(
    num_p num_aux,
    num_p num_fft,
    uint64_t pos_1,
    uint64_t pos_2,
    uint64_t n
)
{
    CLU_HANDLER_IS_SAFE(num_aux)
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_aux && num_fft)

#if defined(NUM_ASM_X86_64) || defined(NUM_ASM_AARCH64)

    (void)num_aux;

    uint64_t * restrict dest_1 = &num_fft->chunk[pos_1];
    uint64_t * restrict dest_2 = &num_fft->chunk[pos_2];

    constexpr uint64_t unroll_log_2 = 3;
    constexpr uint64_t unroll_mask = 7;

    uint64_t j = n >> unroll_log_2;
    uint64_t tail = n & unroll_mask;
    uint64_t carry = 0;
    uint64_t borrow = 0;

#endif

#ifdef NUM_ASM_X86_64

    uint64_t a, b, nb, s;

    __asm__ __volatile__ (
        ".intel_syntax noprefix                         \n\t"

        "xor %k[carry], %k[carry]                       \n\t" // carry = 0, and CF = 0, OF = 0
        "xor %k[borrow], %k[borrow]                     \n\t" // borrow = 0
        "stc                                             \n\t" // CF = 1: the +1 of a - b == a + ~b + 1, fed in once as the LSB's carry-in (stc leaves OF alone)

        "mov rcx, %[j]                                  \n\t" // rcx = j
        "jrcxz loop_pre_skip%=                          \n\t" // short jump to a nearby trampoline
        "jmp loop_begin%=                               \n\t" // near jump, taken whenever j != 0

        "loop_pre_skip%=:                                \n\t"
        "jmp tail_setup%=                               \n\t" // long jump, only when j == 0

        "loop_begin%=:                                  \n\t" // LOOP_BUTTERFLY_BEGIN

        BUTTERFLY_STEP( 0)
        BUTTERFLY_STEP( 8)
        BUTTERFLY_STEP(16)
        BUTTERFLY_STEP(24)
        BUTTERFLY_STEP(32)
        BUTTERFLY_STEP(40)
        BUTTERFLY_STEP(48)
        BUTTERFLY_STEP(56)

        "lea %[dest_1], [%[dest_1] + 64]                \n\t" // dest_1 += 64 (lea leaves CF, OF alone)
        "lea %[dest_2], [%[dest_2] + 64]                \n\t" // dest_2 += 64
        "lea rcx, [rcx - 1]                             \n\t" // rcx-- (lea leaves CF, OF alone)
        "jrcxz loop_exit%=                              \n\t" // short jump to a nearby trampoline
        "jmp loop_begin%=                               \n\t" // long jump back, taken every iteration but the last
        "loop_exit%=:                                   \n\t"
        "jmp tail_setup%=                               \n\t"

        "tail_setup%=:                                  \n\t"
        "mov rcx, %[tail]                                \n\t" // rcx = tail
        "jrcxz tail_skip%=                              \n\t"

        "tail_begin%=:                                  \n\t" // LOOP_BUTTERFLY_TAIL_BEGIN

        BUTTERFLY_STEP(0)

        "lea %[dest_1], [%[dest_1] + 8]                 \n\t" // dest_1 += 8
        "lea %[dest_2], [%[dest_2] + 8]                 \n\t" // dest_2 += 8
        "lea rcx, [rcx - 1]                             \n\t" // rcx--
        "jrcxz tail_skip%=                              \n\t"
        "jmp tail_begin%=                               \n\t"

        "tail_skip%=:                                   \n\t"
        "seto %b[carry]                                 \n\t" // carry = OF
        "setnc %b[borrow]                                \n\t" // borrow = !CF: a + ~b + 1 carries out (CF = 1) exactly when a >= b, i.e. no borrow

        ".att_syntax prefix                             \n\t"
        // out
        :   [dest_1] "+r" (dest_1),
            [dest_2] "+r" (dest_2),
            [a] "=&r" (a),
            [b] "=&r" (b),
            [nb] "=&r" (nb),
            [s] "=&r" (s),
            [carry] "=&r" (carry),
            [borrow] "=&r" (borrow)
        // in
        :   [j] "r" (j),
            [tail] "r" (tail)
        // clobber
        :   "rcx",
            "cc",
            "memory"
    );

#elif defined(NUM_ASM_AARCH64)

    uint64_t a_0, a_1, a_2, a_3, a_4, a_5, a_6, a_7;
    uint64_t b_0, b_1, b_2, b_3, b_4, b_5, b_6, b_7;
    uint64_t s_0, s_1, s_2, s_3;

    __asm__ __volatile__ (
        "cbz %[j], 2f                                   \n\t"

        "1:                                             \n\t" // LOOP_BUTTERFLY_BEGIN

        BUTTERFLY_STEP_8

        "add %[dest_1], %[dest_1], #64                  \n\t" // dest_1 += 64
        "add %[dest_2], %[dest_2], #64                  \n\t" // dest_2 += 64
        "sub %[j], %[j], #1                             \n\t" // j--
        "cbnz %[j], 1b                                  \n\t"

        "2:                                             \n\t"
        "cbz %[tail], 4f                                \n\t"

        "3:                                             \n\t" // LOOP_BUTTERFLY_TAIL_BEGIN

        "ldr %[a_0], [%[dest_1]]                        \n\t" // a_0 = *dest_1
        "ldr %[b_0], [%[dest_2]]                        \n\t" // b_0 = *dest_2
        "subs xzr, %[carry], #1                         \n\t" // CF = carry
        "adcs %[s_0], %[a_0], %[b_0]                    \n\t" // s_0 = a_0 + b_0 + CF
        "cset %[carry], cs                              \n\t" // carry = CF
        "cmp xzr, %[borrow]                             \n\t" // CF = 1 - borrow
        "sbcs %[a_0], %[a_0], %[b_0]                    \n\t" // a_0 -= b_0 + (1 - CF)
        "cset %[borrow], cc                             \n\t" // borrow = 1 - CF
        "str %[s_0], [%[dest_1]], #8                    \n\t" // *dest_1 = s_0, then dest_1 += 8
        "str %[a_0], [%[dest_2]], #8                    \n\t" // *dest_2 = a_0, then dest_2 += 8
        "sub %[tail], %[tail], #1                       \n\t" // tail--
        "cbnz %[tail], 3b                               \n\t"

        "4:                                             \n\t"
        // out
        :   [dest_1] "+r" (dest_1),
            [dest_2] "+r" (dest_2),
            [j] "+&r" (j),
            [tail] "+&r" (tail),
            [carry] "+&r" (carry),
            [borrow] "+&r" (borrow),
            [a_0] "=&r" (a_0),
            [a_1] "=&r" (a_1),
            [a_2] "=&r" (a_2),
            [a_3] "=&r" (a_3),
            [a_4] "=&r" (a_4),
            [a_5] "=&r" (a_5),
            [a_6] "=&r" (a_6),
            [a_7] "=&r" (a_7),
            [b_0] "=&r" (b_0),
            [b_1] "=&r" (b_1),
            [b_2] "=&r" (b_2),
            [b_3] "=&r" (b_3),
            [b_4] "=&r" (b_4),
            [b_5] "=&r" (b_5),
            [b_6] "=&r" (b_6),
            [b_7] "=&r" (b_7),
            [s_0] "=&r" (s_0),
            [s_1] "=&r" (s_1),
            [s_2] "=&r" (s_2),
            [s_3] "=&r" (s_3)
        // in
        :
        // clobber
        :   "cc",
            "memory"
    );

#endif

#if defined(NUM_ASM_X86_64) || defined(NUM_ASM_AARCH64)

    // A - B is negative about half the time, so testing the borrow costs a coin flip the
    // branch predictor cannot win. Adding it unconditionally is two loads and two stores,
    // and leaves only the ripple out of word 0 behind a branch, which needs word 0 to have
    // been all ones and so is never taken in practice.
    uint64_t * chunk = &num_fft->chunk[pos_2];
    uint64_t low = chunk[0] + borrow;
    bool ripple = low < borrow;
    chunk[0] = low;
    chunk[n - 1] += borrow;

    for(uint64_t i = 1; ripple; i++)
    {
        chunk[i]++;
        ripple = chunk[i] == 0;
    }

    num_ssm_normalize(num_fft, pos_1, n);
    num_ssm_normalize(num_fft, pos_2, n);

#else

    num_ssm_sub_mod(num_aux, 0, num_fft, pos_1, num_fft, pos_2, n);
    num_ssm_add_mod_immed(num_fft, pos_1, num_fft, pos_2, n);
    memcpy(&num_fft->chunk[pos_2], num_aux->chunk, n * sizeof(uint64_t));

#endif
}

// Stage s of the forward transform pairs logical index x with x + s whenever
// (x / s) is even, twiddling by ssm_bit_inv(x / 2s, K / 2s) * bits * s; stages run
// from the deepest (s = K/2) up to the top (s = 1). Butterflies within one stage
// touch disjoint pairs, so running them in any order reaches the same result as the
// divide-and-combine recursion this used to be.
//
// Running one stage per pass over the array is memory-bound and cannot be fixed by
// threading: K * n limbs is hundreds of MB at the sizes this is used at, every stage
// reads and writes all of it, so a transform moves log2(K) * 2 * K * n limbs and
// saturates DRAM bandwidth a couple of threads in. Stages are therefore fused r at a
// time. Write the index as x = a + gl*b + (gl << r)*c with a < gl and b < 2^r: the r
// stages with strides [gl, gl << (r-1)] only ever pair indices differing in the b
// field, so the 2^r elements of one (a, c) block are closed under all r of them. A
// block is loaded once, carried through r stages, and written back once, which cuts
// traffic by a factor of r as long as those 2^r elements stay in cache.

// Bytes one fused block may occupy. Wants to be L2: overshooting it costs the whole
// gain, undershooting only fuses fewer stages per pass, and an n large enough that a
// block spills to L3 still beats going to DRAM r times. Deliberately a constant
// rather than a runtime cache query - this has to build and behave the same on
// x86-64 and AArch64.
constexpr uint64_t ssm_fft_block_bytes = 1024 * 1024;

// How many stages one pass can fuse: as many as keep a block inside the budget,
// never more than the stages actually left to run. An n so large that two elements
// don't fit degrades to r = 1, which is one stage per pass - what this did before
// fusing, so no size gets worse.
static uint64_t ssm_fft_fuse_bits(uint64_t n, uint64_t stages_left)
{
    uint64_t fit = ssm_fft_block_bytes / (n * sizeof(uint64_t));
    uint64_t r = fit < 2 ? 1 : (uint64_t)stdc_bit_width(fit) - 1;
    return r < stages_left ? r : stages_left;
}

// The r stages with strides [gl, gl << (r-1)] over the single block (a, c), stages
// descending the same way the unfused loop runs them.
static void ssm_fft_fwd_block(
    num_p num_aux,
    num_p num_fft,
    uint64_t pos,
    uint64_t step,
    uint64_t n,
    uint64_t K,
    uint64_t bits,
    uint64_t gl,
    uint64_t r,
    uint64_t a,
    uint64_t c
)
{
    uint64_t width = U64(1) << r;
    uint64_t base = a + ((gl << r) * c);

    for(uint64_t k = r; k-- > 0;)
    {
        uint64_t stride = gl << k;
        uint64_t bits_local = bits * stride;
        uint64_t half = K / (2 * stride);
        uint64_t reach = U64(1) << k;

        for(uint64_t b_0 = 0; b_0 < width; b_0 += 2 * reach)
        {
            for(uint64_t b = b_0; b < b_0 + reach; b++)
            {
                uint64_t x_1 = base + (gl * b);
                uint64_t pos_1 = (pos + (step * x_1)) * n;
                uint64_t pos_2 = (pos + (step * (x_1 + stride))) * n;

                uint64_t i = (b >> (k + 1)) | (c << ((r - k) - 1));
                uint64_t shift = ssm_bit_inv(i, half) * bits_local;

                num_ssm_shl_mod(num_aux, num_fft, pos_2, n, shift);

                num_ssm_butterfly(num_aux, num_fft, pos_1, pos_2, n);
            }
        }
    }
}

// Where the padded array is built from, when that build is fused into the forward
// transform's first pass (see num_ssm_fft_fwd_rec). Mirrors num_ssm_pad_no_wrap's
// layout decisions, hoisted out of the element loop.
typedef struct
{
    num_p num;
    uint64_t M;
    uint64_t full_chunks;
    uint64_t tail;
    bool extra;
} ssm_fft_fwd_pad_t;

// Build one block's 2^r elements straight from the operand. Standalone, this is
// num_ssm_pad_no_wrap: a zeroing allocation of the whole n*K array plus a scatter
// over it, which the first pass then reads back. Done here the element is assembled
// in cache and written out once, so the array is never read from DRAM at all and
// never separately zeroed.
static void ssm_fft_fwd_pad_block(
    ssm_fft_fwd_pad_t * pad,
    num_p num_fft,
    uint64_t pos,
    uint64_t step,
    uint64_t n,
    uint64_t K,
    uint64_t gl,
    uint64_t r,
    uint64_t a,
    uint64_t c
)
{
    uint64_t width = U64(1) << r;
    uint64_t base = a + ((gl << r) * c);
    const uint64_t * restrict src = pad->num->chunk;

    for(uint64_t b = 0; b < width; b++)
    {
        uint64_t x = base + (gl * b);
        uint64_t * restrict dest = &num_fft->chunk[(pos + (step * x)) * n];

        uint64_t copy = 0;
        if(x < pad->full_chunks)
        {
            copy = pad->M;
        }
        else if(x == pad->full_chunks)
        {
            copy = pad->tail;
        }

        if(copy)
        {
            memcpy(dest, &src[pad->M * x], copy * sizeof(uint64_t));
        }
        memset(&dest[copy], 0, (n - copy) * sizeof(uint64_t));

        if(pad->extra && (x == K - 1))
        {
            dest[pad->M] = src[pad->num->count - 1];
        }
    }
}

// The transform's per-element entry twiddle (element x scaled by 2^(Q*x)), applied to
// one block's 2^r elements. It used to be a standalone pass over the whole K*n array
// before the stages began; run here it rides along with the pass that is already
// pulling the block into cache, which removes that pass's DRAM traffic outright. Every
// element belongs to exactly one block of the first pass, so each is still twiddled
// exactly once, and all of a block's elements are done before any butterfly pairs them.
static void ssm_fft_fwd_preloop_block(
    num_p num_aux,
    num_p num_fft,
    uint64_t pos,
    uint64_t step,
    uint64_t n,
    uint64_t Q,
    uint64_t gl,
    uint64_t r,
    uint64_t a,
    uint64_t c
)
{
    uint64_t width = U64(1) << r;
    uint64_t base = a + ((gl << r) * c);

    for(uint64_t b = 0; b < width; b++)
    {
        uint64_t x = base + (gl * b);
        num_ssm_shl_mod(num_aux, num_fft, (pos + (step * x)) * n, n, Q * x);
    }
}

// Every pass from stride K/2 down to stride_end, run by one worker per residue class
// a == worker (mod workers) with no synchronisation between them at all (see
// num_ssm_fft_fwd_rec's comment for why the classes stay disjoint). One dispatch
// covers the whole range. Also the serial path, as worker 0 of 1 down to stride 1.
typedef struct
{
    num_p num_aux;
    num_p num_fft;
    uint64_t pos;
    uint64_t step;
    uint64_t n;
    uint64_t K;
    uint64_t bits;
    uint64_t Q;
    ssm_fft_fwd_pad_t * pad;
    uint64_t stride_end;
    uint64_t worker;
    uint64_t workers;
} ssm_fft_fwd_split_worker_t;

static void * ssm_fft_fwd_split_worker(void * arg)
{
    ssm_fft_fwd_split_worker_t * w = arg;

    uint64_t stages_left = (uint64_t)stdc_bit_width(w->K / w->stride_end) - 1;
    uint64_t stride_hi = w->K / 2;
    bool first = true;

    while(stages_left)
    {
        uint64_t r = ssm_fft_fuse_bits(w->n, stages_left);
        uint64_t gl = stride_hi >> (r - 1);
        uint64_t blocks = w->K / (gl << r);

        for(uint64_t c = 0; c < blocks; c++)
        {
            for(uint64_t a = w->worker; a < gl; a += w->workers)
            {
                if(first && w->pad)
                {
                    ssm_fft_fwd_pad_block(
                        w->pad, w->num_fft, w->pos, w->step,
                        w->n, w->K, gl, r, a, c
                    );
                }

                if(first)
                {
                    ssm_fft_fwd_preloop_block(
                        w->num_aux, w->num_fft, w->pos, w->step,
                        w->n, w->Q, gl, r, a, c
                    );
                }

                ssm_fft_fwd_block(
                    w->num_aux, w->num_fft, w->pos, w->step,
                    w->n, w->K, w->bits, gl, r, a, c
                );
            }
        }

        first = false;
        stages_left -= r;
        stride_hi = gl / 2;
    }
    return nullptr;
}

// idx numbers the K >> r blocks of one pass as (c, a) flattened c*gl + a.
typedef struct
{
    num_p num_aux;
    num_p num_fft;
    uint64_t pos;
    uint64_t step;
    uint64_t n;
    uint64_t K;
    uint64_t bits;
    uint64_t gl;
    uint64_t r;
    uint64_t idx_start;
    uint64_t idx_end;
} ssm_fft_fwd_pass_worker_t;

static void * ssm_fft_fwd_pass_worker(void * arg)
{
    ssm_fft_fwd_pass_worker_t * w = arg;

    for(uint64_t idx = w->idx_start; idx < w->idx_end; idx++)
    {
        ssm_fft_fwd_block(
            w->num_aux, w->num_fft, w->pos, w->step,
            w->n, w->K, w->bits, w->gl, w->r, idx % w->gl, idx / w->gl
        );
    }
    return nullptr;
}

// A stage with stride s pairs x with x + s, so for a power-of-two split count P every
// stage with s >= P keeps x in its class x == a (mod P): worker w can own the whole
// class a == w (mod P) and run every pass from stride K/2 down to stride P without
// ever touching another worker's element. That is one dispatch for
// log2(K) - log2(P) stages, with workers free to sit on different passes at once.
//
// Only the last log2(P) stages (stride < P) merge across classes. Those still need a
// join between passes, but fusing means ceil(log2(P) / r) of them rather than
// log2(P). Dispatches per transform: log2(K) -> 1 + ceil(log2(P) / r).
//
// The split count is workers rounded down to a power of two (the residue argument
// needs P | s); the tail passes keep the full worker count, since splitting a pass's
// block list doesn't care.
// num_aux->size >= 2 * n
// pad non-null builds the array from the operand inside the first pass instead of
// expecting it already populated (see ssm_fft_fwd_pad_block).
static void num_ssm_fft_fwd_rec(
    num_p num_aux,
    num_p num_fft, uint64_t pos,
    uint64_t step,
    uint64_t n,
    uint64_t K,
    uint64_t bits,
    uint64_t Q,
    ssm_fft_fwd_pad_t * pad,
    uint64_t threads
)
{
    CLU_HANDLER_IS_SAFE(num_aux)
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_aux)
    assert(num_fft)
    assert(num_aux->size >= 2 * n)

    uint64_t workers = ssm_worker_count(threads, K / 2);

    if(workers <= 1)
    {
        ssm_fft_fwd_split_worker_t serial =
        {
            .num_aux = num_aux,
            .num_fft = num_fft,
            .pos = pos,
            .step = step,
            .n = n,
            .K = K,
            .bits = bits,
            .Q = Q,
            .pad = pad,
            .stride_end = 1,
            .worker = 0,
            .workers = 1,
        };
        ssm_fft_fwd_split_worker(&serial);
        return;
    }

    uint64_t split = B(stdc_bit_width(workers) - 1);

    num_p * worker_aux = malloc(workers * sizeof(*worker_aux));
    assert(worker_aux)
    pthread_t * worker_ids = malloc(workers * sizeof(*worker_ids));
    assert(worker_ids)

    for(uint64_t w=0; w<workers; w++)
    {
        worker_aux[w] = num_create_dirty(CLU_ARGS(2 * n, 0));
    }

    ssm_fft_fwd_split_worker_t * split_args = malloc(split * sizeof(*split_args));
    assert(split_args)

    for(uint64_t w=0; w<split; w++)
    {
        split_args[w] = (ssm_fft_fwd_split_worker_t)
        {
            .num_aux = worker_aux[w],
            .num_fft = num_fft,
            .pos = pos,
            .step = step,
            .n = n,
            .K = K,
            .bits = bits,
            .Q = Q,
            .pad = pad,
            .stride_end = split,
            .worker = w,
            .workers = split,
        };

        TREAT(pthread_create(&worker_ids[w], nullptr, ssm_fft_fwd_split_worker, &split_args[w]))
    }
    for(uint64_t w=0; w<split; w++)
    {
        TREAT(pthread_join(worker_ids[w], nullptr))
    }

    free(split_args);

    ssm_fft_fwd_pass_worker_t * worker_args = malloc(workers * sizeof(*worker_args));
    assert(worker_args)

    uint64_t stages_left = (uint64_t)stdc_bit_width(split) - 1;
    uint64_t stride_hi = split / 2;

    while(stages_left)
    {
        uint64_t r = ssm_fft_fuse_bits(n, stages_left);
        uint64_t gl = stride_hi >> (r - 1);
        uint64_t blocks = K >> r;
        uint64_t pass_workers = workers < blocks ? workers : blocks;

        for(uint64_t w=0; w<pass_workers; w++)
        {
            uint64_t idx_start, idx_end;
            ssm_worker_range(w, pass_workers, blocks, &idx_start, &idx_end);

            worker_args[w] = (ssm_fft_fwd_pass_worker_t)
            {
                .num_aux = worker_aux[w],
                .num_fft = num_fft,
                .pos = pos,
                .step = step,
                .n = n,
                .K = K,
                .bits = bits,
                .gl = gl,
                .r = r,
                .idx_start = idx_start,
                .idx_end = idx_end,
            };

            TREAT(pthread_create(&worker_ids[w], nullptr, ssm_fft_fwd_pass_worker, &worker_args[w]))
        }
        for(uint64_t w=0; w<pass_workers; w++)
        {
            TREAT(pthread_join(worker_ids[w], nullptr))
        }

        stages_left -= r;
        stride_hi = gl / 2;
    }

    for(uint64_t w=0; w<workers; w++)
    {
        num_free(worker_aux[w]);
    }
    free(worker_ids);
    free(worker_args);
    free(worker_aux);
}

// num_aux->size >= 2 * n
void num_ssm_fft_fwd(num_p num_aux, num_p num_fft, ssm_params_p p, uint64_t threads)
{
    CLU_HANDLER_IS_SAFE(num_aux)
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_aux)
    assert(num_fft)
    assert(num_aux->size >= 2 * p->n)
    assert(num_fft->size >= p->n * p->K)

    num_ssm_fft_fwd_rec(num_aux, num_fft, 0, 1, p->n, p->K, 2 * p->Q, p->Q, nullptr, threads);
}

// num_ssm_fft_fwd over an array that has not been populated yet: the padding of NUM
// into n-limb blocks happens inside the transform's first pass. NUM is only read.
// num_fft->size >= p->n * p->K, num_aux->size >= 2 * p->n
static void num_ssm_fft_fwd_pad(
    num_p num_aux,
    num_p num_fft,
    num_p num,
    ssm_params_p p,
    uint64_t threads
)
{
    CLU_HANDLER_IS_SAFE(num_aux)
    CLU_HANDLER_IS_SAFE(num_fft)
    CLU_HANDLER_IS_SAFE(num)
    assert(num_aux)
    assert(num_fft)
    assert(num)
    assert(num_aux->size >= 2 * p->n)
    assert(num_fft->size >= p->n * p->K)

    uint64_t full_chunks = num->count / p->M;
    if(full_chunks > p->K)
    {
        full_chunks = p->K;
    }

    ssm_fft_fwd_pad_t pad =
    {
        .num = num,
        .M = p->M,
        .full_chunks = full_chunks,
        .tail = num->count % p->M,
        .extra = (bool)(num->count == (p->M * p->K) + 1),
    };

    num_ssm_fft_fwd_rec(num_aux, num_fft, 0, 1, p->n, p->K, 2 * p->Q, p->Q, &pad, threads);
}

// The inverse transform is the mirror: stage s pairs x with x + s whenever (x / s)
// is even, twiddling by (x mod s) * bits * k / 2s, and stages ascend from s = 1 up
// to s = k/2. The same index decomposition fuses it - x = a + gl*b + (gl << r)*c,
// with the r stages of strides [gl, gl << (r-1)] closed over the b field - except
// that here a block's 2^r elements are contiguous rather than strided, since the
// pass fusing the deepest stages is the one with gl = 1.

// The r stages with strides [gl, gl << (r-1)] over the single block (a, c), stages
// ascending the same way the unfused loop runs them.
static void ssm_fft_inv_block(
    num_p num_aux,
    num_p num,
    uint64_t pos,
    uint64_t n,
    uint64_t k,
    uint64_t bits,
    uint64_t gl,
    uint64_t r,
    uint64_t a,
    uint64_t c
)
{
    uint64_t width = U64(1) << r;
    uint64_t base = a + ((gl << r) * c);

    for(uint64_t kk = 0; kk < r; kk++)
    {
        uint64_t stride = gl << kk;
        uint64_t bits_local = (bits * k) / (2 * stride);
        uint64_t reach = U64(1) << kk;

        for(uint64_t b_0 = 0; b_0 < width; b_0 += 2 * reach)
        {
            for(uint64_t b = b_0; b < b_0 + reach; b++)
            {
                uint64_t x_1 = base + (gl * b);
                uint64_t pos_1 = (pos + x_1) * n;
                uint64_t pos_2 = (pos + x_1 + stride) * n;

                uint64_t i = a + (gl * (b - b_0));

                num_ssm_shr_mod(num_aux, num, pos_2, n, i * bits_local);

                num_ssm_butterfly(num_aux, num, pos_1, pos_2, n);
            }
        }
    }
}

// The transform's per-element exit twiddle (undoing the entry scaling and dividing by
// k), applied to one block's 2^r elements. Mirrors ssm_fft_fwd_preloop_block: it used
// to be a standalone pass over the whole k*n array after the stages finished, and runs
// here inside the last pass, which already holds the block. Blocks of the last pass
// partition the transform, so each element is still twiddled exactly once, after every
// butterfly that touches it. Elements at or past lim take the shift in two steps
// because a single one would exceed what num_ssm_shr_mod accepts.
static void ssm_fft_inv_postloop_block(
    num_p num_aux,
    num_p num,
    uint64_t pos,
    uint64_t n,
    uint64_t Q,
    uint64_t k_,
    uint64_t lim,
    uint64_t gl,
    uint64_t r,
    uint64_t a,
    uint64_t c
)
{
    uint64_t width = U64(1) << r;
    uint64_t base = a + ((gl << r) * c);

    for(uint64_t b = 0; b < width; b++)
    {
        uint64_t x = base + (gl * b);
        uint64_t pos_x = (pos + x) * n;

        if(x < lim)
        {
            num_ssm_shr_mod(num_aux, num, pos_x, n, (Q * x) + k_);
            continue;
        }

        num_ssm_shr_mod(num_aux, num, pos_x, n, Q * x);
        num_ssm_shr_mod(num_aux, num, pos_x, n, k_);
    }
}

// These live further down the file, past the transform they are fused into.
static bool ssm_is_recursive(uint64_t n);

static void num_ssm_mul_mod_span(
    num_p num_aux,
    num_p num_1,
    num_p num_2,
    uint64_t pos,
    uint64_t n
);

// Scratch and operand for the convolution's pointwise multiply when it is fused into
// the inverse transform (see num_ssm_fft_inv_rec). One of these per worker: the
// recursive buffers are reused across every element that worker handles, so nothing
// is allocated inside the element loop. p_next is nullptr when n is small enough for
// the non-recursive base case, which needs no buffers of its own beyond the 2n
// scratch the transform already carries.
typedef struct
{
    num_p num_fft_2;
    ssm_params_p p_next;
    num_p num_aux_1;
    num_p num_fft_1_next;
    num_p num_fft_2_next;
} ssm_fft_inv_pointwise_t;

static ssm_fft_inv_pointwise_t ssm_fft_inv_pointwise_create(
    num_p num_fft_2,
    ssm_params_p p_next,
    uint64_t n
)
{
    if(!num_fft_2)
    {
        return (ssm_fft_inv_pointwise_t){0};
    }

    if(!p_next)
    {
        return (ssm_fft_inv_pointwise_t){ .num_fft_2 = num_fft_2 };
    }

    return (ssm_fft_inv_pointwise_t)
    {
        .num_fft_2 = num_fft_2,
        .p_next = p_next,
        .num_aux_1 = num_create_dirty(CLU_ARGS(n, 0)),
        .num_fft_1_next = num_create_dirty(CLU_ARGS(p_next->n * p_next->K, 0)),
        .num_fft_2_next = num_create_dirty(CLU_ARGS(p_next->n * p_next->K, 0)),
    };
}

static void ssm_fft_inv_pointwise_free(ssm_fft_inv_pointwise_t * pw)
{
    if(!pw->p_next)
    {
        return;
    }

    num_free(pw->num_aux_1);
    num_free(pw->num_fft_1_next);
    num_free(pw->num_fft_2_next);
}

// Multiply one block's 2^r elements of num by the matching elements of num_fft_2,
// in place. Per-element like the twiddles, so it rides along with the pass that is
// already holding the block, which is what removes the standalone pointwise pass:
// that pass wrote every product to DRAM only for the transform's first pass to read
// it straight back.
static void ssm_fft_inv_pointwise_block(
    ssm_fft_inv_pointwise_t * pw,
    num_p num_aux,
    num_p num,
    uint64_t pos,
    uint64_t n,
    uint64_t gl,
    uint64_t r,
    uint64_t a,
    uint64_t c
)
{
    uint64_t width = U64(1) << r;
    uint64_t base = a + ((gl << r) * c);

    for(uint64_t b = 0; b < width; b++)
    {
        uint64_t x = base + (gl * b);
        uint64_t pos_x = (pos + x) * n;

        if(pw->p_next)
        {
            // NOLINTNEXTLINE(readability-suspicious-call-argument)
            num_ssm_mul_wrap(
                pw->num_aux_1,
                num_aux,
                pw->num_fft_1_next,
                pw->num_fft_2_next,
                num,
                pw->num_fft_2,
                pos_x,
                pw->p_next
            );
            continue;
        }

        num_ssm_mul_mod_span(num_aux, num, pw->num_fft_2, pos_x, n);
    }
}

// Every pass whose blocks fit inside one worker's contiguous chunk of k / workers
// elements, run with no synchronisation between workers at all (see
// num_ssm_fft_inv_rec's comment). One dispatch covers the whole range. Also the
// serial path, as worker 0 of 1, whose chunk is the entire transform.
typedef struct
{
    num_p num_aux;
    num_p num;
    uint64_t pos;
    uint64_t n;
    uint64_t k;
    uint64_t bits;
    uint64_t Q;
    uint64_t k_;
    uint64_t lim;
    bool postloop;
    ssm_fft_inv_pointwise_t * pw;
    uint64_t worker;
    uint64_t workers;
} ssm_fft_inv_split_worker_t;

static void * ssm_fft_inv_split_worker(void * arg)
{
    ssm_fft_inv_split_worker_t * w = arg;

    uint64_t chunk = w->k / w->workers;
    uint64_t stages_left = (uint64_t)stdc_bit_width(chunk) - 1;
    uint64_t gl = 1;
    bool first = true;

    while(stages_left)
    {
        uint64_t r = ssm_fft_fuse_bits(w->n, stages_left);
        uint64_t span = gl << r;
        uint64_t c_start = (w->worker * chunk) / span;
        uint64_t c_end = c_start + (chunk / span);

        bool last = w->postloop && (stages_left == r);

        for(uint64_t c = c_start; c < c_end; c++)
        {
            for(uint64_t a = 0; a < gl; a++)
            {
                if(first && w->pw)
                {
                    ssm_fft_inv_pointwise_block(
                        w->pw, w->num_aux, w->num, w->pos, w->n, gl, r, a, c
                    );
                }

                ssm_fft_inv_block(
                    w->num_aux, w->num, w->pos,
                    w->n, w->k, w->bits, gl, r, a, c
                );

                if(last)
                {
                    ssm_fft_inv_postloop_block(
                        w->num_aux, w->num, w->pos,
                        w->n, w->Q, w->k_, w->lim, gl, r, a, c
                    );
                }
            }
        }

        first = false;
        stages_left -= r;
        gl = span;
    }
    return nullptr;
}

// idx numbers the k >> r blocks of one pass as (c, a) flattened c*gl + a.
typedef struct
{
    num_p num_aux;
    num_p num;
    uint64_t pos;
    uint64_t n;
    uint64_t k;
    uint64_t bits;
    uint64_t gl;
    uint64_t r;
    uint64_t Q;
    uint64_t k_;
    uint64_t lim;
    bool postloop;
    uint64_t idx_start;
    uint64_t idx_end;
} ssm_fft_inv_pass_worker_t;

static void * ssm_fft_inv_pass_worker(void * arg)
{
    ssm_fft_inv_pass_worker_t * w = arg;

    for(uint64_t idx = w->idx_start; idx < w->idx_end; idx++)
    {
        uint64_t a = idx % w->gl;
        uint64_t c = idx / w->gl;

        ssm_fft_inv_block(
            w->num_aux, w->num, w->pos,
            w->n, w->k, w->bits, w->gl, w->r, a, c
        );

        if(w->postloop)
        {
            ssm_fft_inv_postloop_block(
                w->num_aux, w->num, w->pos,
                w->n, w->Q, w->k_, w->lim, w->gl, w->r, a, c
            );
        }
    }
    return nullptr;
}

// A stage with stride s keeps every butterfly inside a contiguous aligned run of 2s
// elements, so for a power-of-two split count P every pass whose blocks span at most
// k / P elements stays inside the chunk [w * k/P, (w+1) * k/P) that worker w owns:
// worker w runs all of them - log2(k) - log2(P) stages - under a single dispatch
// with no synchronisation, free to sit on a different pass than its neighbours.
//
// Only the last log2(P) stages span across chunks. Those still need a join between
// passes, but fusing means ceil(log2(P) / r) of them rather than log2(P).
// Dispatches per transform: log2(k) -> 1 + ceil(log2(P) / r).
//
// bits_local, which the unfused loop carried down the stages by halving, is written
// out as (bits * k) / 2s so a worker starting mid-way can compute it.
// num_aux->size >= 2 * n
// num_fft_2 non-null fuses the convolution's pointwise multiply into the first pass
// (see ssm_fft_inv_pointwise_block). Only worth it at the top level, where the arrays
// are hundreds of MB; the recursive inner multiplies work on a few KB that stay in
// cache regardless, so they keep the plain num_ssm_mul_pointwise path.
static void num_ssm_fft_inv_rec(
    num_p num_aux,
    num_p num,
    uint64_t pos,
    uint64_t n,
    uint64_t k,
    uint64_t bits,
    uint64_t Q,
    num_p num_fft_2,
    uint64_t threads
)
{
    CLU_HANDLER_IS_SAFE(num_aux)
    CLU_HANDLER_IS_SAFE(num)
    assert(num_aux)
    assert(num)
    assert(num_aux->size >= 2 * n)

    uint64_t k_ = stdc_trailing_zeros(k);
    uint64_t lim = ((chunk_bits * (n - 1)) - k_) / Q;

    ssm_params_t p_next;
    bool pw_recursive = false;
    if(num_fft_2)
    {
        pw_recursive = ssm_is_recursive(n);
        if(pw_recursive)
        {
            p_next = ssm_get_params_wrap(n);
        }
    }

    uint64_t workers = ssm_worker_count(threads, k / 2);

    if(workers <= 1)
    {
        ssm_fft_inv_pointwise_t pw = ssm_fft_inv_pointwise_create(num_fft_2, pw_recursive ? &p_next : nullptr, n);

        ssm_fft_inv_split_worker_t serial =
        {
            .num_aux = num_aux,
            .num = num,
            .pos = pos,
            .n = n,
            .k = k,
            .bits = bits,
            .Q = Q,
            .k_ = k_,
            .lim = lim,
            .postloop = true,
            .pw = num_fft_2 ? &pw : nullptr,
            .worker = 0,
            .workers = 1,
        };
        ssm_fft_inv_split_worker(&serial);
        ssm_fft_inv_pointwise_free(&pw);
        return;
    }

    uint64_t split = B(stdc_bit_width(workers) - 1);

    num_p * worker_aux = malloc(workers * sizeof(*worker_aux));
    assert(worker_aux)
    pthread_t * worker_ids = malloc(workers * sizeof(*worker_ids));
    assert(worker_ids)

    for(uint64_t w=0; w<workers; w++)
    {
        worker_aux[w] = num_create_dirty(CLU_ARGS(2 * n, 0));
    }

    ssm_fft_inv_split_worker_t * split_args = malloc(split * sizeof(*split_args));
    assert(split_args)
    ssm_fft_inv_pointwise_t * split_pw = malloc(split * sizeof(*split_pw));
    assert(split_pw)

    for(uint64_t w=0; w<split; w++)
    {
        split_pw[w] = ssm_fft_inv_pointwise_create(num_fft_2, pw_recursive ? &p_next : nullptr, n);

        split_args[w] = (ssm_fft_inv_split_worker_t)
        {
            .num_aux = worker_aux[w],
            .num = num,
            .pos = pos,
            .n = n,
            .k = k,
            .bits = bits,
            .Q = Q,
            .k_ = k_,
            .lim = lim,
            .postloop = false,
            .pw = num_fft_2 ? &split_pw[w] : nullptr,
            .worker = w,
            .workers = split,
        };

        TREAT(pthread_create(&worker_ids[w], nullptr, ssm_fft_inv_split_worker, &split_args[w]))
    }
    for(uint64_t w=0; w<split; w++)
    {
        TREAT(pthread_join(worker_ids[w], nullptr))
    }

    for(uint64_t w=0; w<split; w++)
    {
        ssm_fft_inv_pointwise_free(&split_pw[w]);
    }
    free(split_pw);
    free(split_args);

    ssm_fft_inv_pass_worker_t * worker_args = malloc(workers * sizeof(*worker_args));
    assert(worker_args)

    uint64_t stages_left = (uint64_t)stdc_bit_width(split) - 1;
    uint64_t gl = k / split;

    while(stages_left)
    {
        uint64_t r = ssm_fft_fuse_bits(n, stages_left);
        uint64_t blocks = k >> r;
        uint64_t pass_workers = workers < blocks ? workers : blocks;

        for(uint64_t w=0; w<pass_workers; w++)
        {
            uint64_t idx_start, idx_end;
            ssm_worker_range(w, pass_workers, blocks, &idx_start, &idx_end);

            worker_args[w] = (ssm_fft_inv_pass_worker_t)
            {
                .num_aux = worker_aux[w],
                .num = num,
                .pos = pos,
                .n = n,
                .k = k,
                .bits = bits,
                .gl = gl,
                .r = r,
                .Q = Q,
                .k_ = k_,
                .lim = lim,
                .postloop = (stages_left == r),
                .idx_start = idx_start,
                .idx_end = idx_end,
            };

            TREAT(pthread_create(&worker_ids[w], nullptr, ssm_fft_inv_pass_worker, &worker_args[w]))
        }
        for(uint64_t w=0; w<pass_workers; w++)
        {
            TREAT(pthread_join(worker_ids[w], nullptr))
        }

        stages_left -= r;
        gl <<= r;
    }

    for(uint64_t w=0; w<workers; w++)
    {
        num_free(worker_aux[w]);
    }
    free(worker_ids);
    free(worker_args);
    free(worker_aux);
}

// num_aux->size >= 2 * p->n
void num_ssm_fft_inv(num_p num_aux, num_p num_fft, ssm_params_p p, uint64_t threads)
{
    CLU_HANDLER_IS_SAFE(num_aux)
    CLU_HANDLER_IS_SAFE(num_fft)
    assert(num_aux)
    assert(num_fft)
    assert(num_aux->size >= 2 * p->n)

    num_ssm_fft_inv_rec(num_aux, num_fft, 0, p->n, p->K, 2 * p->Q, p->Q, nullptr, threads);
}

// num_ssm_fft_inv with the convolution's pointwise multiply against num_fft_2 folded
// into its first pass. num_fft_2 is only read, and is left untouched.
// num_aux->size >= 2 * p->n
static void num_ssm_fft_inv_pointwise(
    num_p num_aux,
    num_p num_fft,
    num_p num_fft_2,
    ssm_params_p p,
    uint64_t threads
)
{
    CLU_HANDLER_IS_SAFE(num_aux)
    CLU_HANDLER_IS_SAFE(num_fft)
    CLU_HANDLER_IS_SAFE(num_fft_2)
    assert(num_aux)
    assert(num_fft)
    assert(num_fft_2)
    assert(num_aux->size >= 2 * p->n)

    num_ssm_fft_inv_rec(num_aux, num_fft, 0, p->n, p->K, 2 * p->Q, p->Q, num_fft_2, threads);
}

constexpr uint64_t ssm_recursive_threshold = 129;

static bool ssm_is_recursive(uint64_t n)
{
    return (bool)((n > ssm_recursive_threshold) && (((n - 1) & (1 - n)) > 4));
}

static bool ssm_pad_is_needed(uint64_t n, uint64_t moduli)
{
    if(n > ssm_recursive_threshold)
    {
        return true;
    }

    // Below one full block there is no 8 wide body to trim, only tail, so unpadding saves
    // a word or two of width while the padded form still runs a single clean block.
    constexpr uint64_t unroll = 8;
    if(n - 1 < unroll)
    {
        return true;
    }

    // For a leaf it is a straight trade: padding buys 8 - moduli words of extra width in
    // every add, subtract, rotate and in the (n - 1)^2 pointwise multiply, and saves the
    // moduli scalar steps that each kernel's tail loop would otherwise run. Below the
    // midpoint the width is worth more than the tail, above it the tail wins.
    constexpr uint64_t tail_break_even = 4;
    return moduli > tail_break_even;
}

// NOLINTBEGIN(readability-magic-numbers)
static ssm_params_t ssm_finish_params(uint64_t count, uint64_t K, uint64_t M)
{
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

    uint64_t moduli = (n - 1) & 7;
    if(moduli && ssm_pad_is_needed(n, moduli))
    {
        n += 8 - moduli;
        Q = 64 * (n - 1) / K;
    }
    assert(64 * (n - 1) % K == 0);

    assert(64 * (n - 1) > (128 * M) + stdc_bit_width(K) - 1);

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
ssm_params_t ssm_get_params(uint64_t count)
{
    uint64_t M = B(stdc_bit_width(count) / 2);
    uint64_t K = 2 * stdc_bit_ceil((count + M - 1) / M);
    M = (count / K) + 1;

    return ssm_finish_params(count, K, M);
}
// NOLINTEND(readability-magic-numbers)

// NOLINTBEGIN(readability-magic-numbers)
ssm_params_t ssm_get_params_wrap(uint64_t n)
{
    uint64_t K1 = 2 * B(stdc_bit_width(n-1) / 2);
    uint64_t K2 = (n - 1) & (1 - n);
    uint64_t K = K1 < K2 ? K1 : K2;
    uint64_t M = (n - 1) / K;

    assert(n == (M * K) + 1);
    return ssm_finish_params(n, K, M);
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

    uint64_t count = n - 1;
    assert(count >= 1)

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
    uint64_t rem = count & 7;

    __asm__ __volatile__ (
        ".intel_syntax noprefix                         \n\t"

        "mov %[j], %[count]                             \n\t" // j = count
        "shr %[j], 3                                    \n\t" // j /= 8
        "mov rdx, [%[src_1]]                            \n\t" // D = *src_1
        "mov %[carry], 0                                \n\t" // carry = 0
        "xor %[_pos], %[_pos]                           \n\t" // _pos = 0
        "test %[j], %[j]                                \n\t" // no flags carried yet: free to disturb CF/OF here
        "jz row0_tail_setup%=                           \n\t"

        "loop_0_begin%=:                                \n\t" // LOOP_0_BEGIN

        MUL_CLASSIC_STEP_ZERO( 0, high, carry, _pos)
        MUL_CLASSIC_STEP_ZERO( 8, carry, high, _pos)
        MUL_CLASSIC_STEP_ZERO(16, high, carry, _pos)
        MUL_CLASSIC_STEP_ZERO(24, carry, high, _pos)
        MUL_CLASSIC_STEP_ZERO(32, high, carry, _pos)
        MUL_CLASSIC_STEP_ZERO(40, carry, high, _pos)
        MUL_CLASSIC_STEP_ZERO(48, high, carry, _pos)
        MUL_CLASSIC_STEP_ZERO(56, carry, high, _pos)

        "lea %[_pos], [%[_pos] + 64]                    \n\t" // _pos += 64 (lea leaves CF alone)
        "dec %[j]                                       \n\t" // j-- (dec leaves CF alone; no OF chain in row 0)
        "jnz loop_0_begin%=                             \n\t"

        "row0_tail_setup%=:                             \n\t"
        "mov rcx, %[rem]                                \n\t" // rcx = rem
        "jrcxz row0_tail_skip%=                         \n\t" // flag-safe: CF may be live here if the block above ran

        "row0_tail_begin%=:                             \n\t" // LOOP_0_TAIL_BEGIN

        "mulx %[high], %[low], [%[src_2] + %[_pos]]     \n\t" // (high, low) = MUL(D, *(src_2 + _pos))
        "adcx %[low], %[carry]                          \n\t" // low += carry + CF
        "mov [%[dest] + %[_pos]], %[low]                \n\t" // *(dest + _pos) = low
        "mov %[carry], %[high]                          \n\t" // carry = high, ready for the next column
        "lea %[_pos], [%[_pos] + 8]                     \n\t"
        "dec rcx                                        \n\t" // dec leaves CF alone; nothing here reads OF
        "jnz row0_tail_begin%=                          \n\t"

        "row0_tail_skip%=:                              \n\t"
        "adcx %[carry], %[zero]                         \n\t" // carry += CF
        "mov [%[dest] + %[_pos]], %[carry]              \n\t" // *(dest + _pos) = carry

        "lea %[src_1], [%[src_1] + 8]                   \n\t" // src_1 += 8
        "lea %[dest], [%[dest] + 8]                     \n\t" // dest += 8
        "dec %[i]                                       \n\t" // i--
        "jz done%=                                       \n\t" // count == 1: row 0 was the only row

        "loop_1_begin%=:                                \n\t"

        "mov %[j], %[count]                             \n\t" // j = count
        "mov rdx, [%[src_1]]                            \n\t" // D = *src_1
        "shr %[j], 3                                    \n\t" // j /= 8
        "mov %[carry], 0                                \n\t" // carry = 0
        "xor %[_pos], %[_pos]                           \n\t" // _pos = 0
        "test %[j], %[j]                                \n\t" // no flags carried yet for this row: free to disturb CF/OF
        "jz row_tail_setup%=                            \n\t"

        "loop_2_begin%=:                                \n\t"

        MUL_CLASSIC_STEP( 0, high, carry, src_2, _pos)
        MUL_CLASSIC_STEP( 8, carry, high, src_2, _pos)
        MUL_CLASSIC_STEP(16, high, carry, src_2, _pos)
        MUL_CLASSIC_STEP(24, carry, high, src_2, _pos)
        MUL_CLASSIC_STEP(32, high, carry, src_2, _pos)
        MUL_CLASSIC_STEP(40, carry, high, src_2, _pos)
        MUL_CLASSIC_STEP(48, high, carry, src_2, _pos)
        MUL_CLASSIC_STEP(56, carry, high, src_2, _pos)

        "adox %[carry], %[zero]                         \n\t" // carry += OF (folded into a GPR before dec below can touch OF)

        "lea %[_pos], [%[_pos] + 64]                    \n\t" // _pos += 64
        "dec %[j]                                       \n\t" // j--
        "jnz loop_2_begin%=                             \n\t"

        "row_tail_setup%=:                              \n\t"
        "mov rcx, %[rem]                                \n\t" // rcx = rem (pending OF was already folded into carry above, or never raised if j == 0)
        "jrcxz row_tail_skip%=                          \n\t" // flag-safe: CF may be live here if the block above ran

        "row_tail_begin%=:                              \n\t" // LOOP_2_TAIL_BEGIN

        "mulx %[high], %[low], [%[src_2] + %[_pos]]     \n\t" // (high, low) = MUL(D, *(src_2 + _pos))
        "adcx %[low], [%[dest] + %[_pos]]               \n\t" // low += *(dest + _pos) + CF
        "adox %[low], %[carry]                          \n\t" // low += carry + OF
        "mov [%[dest] + %[_pos]], %[low]                \n\t" // *(dest + _pos) = low
        "adox %[high], %[zero]                          \n\t" // high += 0 + OF: fold before dec touches OF
        "mov %[carry], %[high]                          \n\t" // carry = high, ready for the next column
        "lea %[_pos], [%[_pos] + 8]                     \n\t"
        "dec rcx                                        \n\t" // dec leaves CF alone; OF was already folded above
        "jnz row_tail_begin%=                           \n\t"

        "row_tail_skip%=:                               \n\t"
        "adcx %[carry], %[zero]                         \n\t" // carry += CF
        "mov [%[dest] + %[_pos]], %[carry]              \n\t" // *(dest + _pos) = carry

        "lea %[src_1], [%[src_1] + 8]                   \n\t" // src_1 += 8
        "lea %[dest], [%[dest] + 8]                     \n\t" // dest += 8
        "dec %[i]                                       \n\t" // i--
        "jnz loop_1_begin%=                             \n\t"

        "done%=:                                        \n\t"

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
            [count] "r" (count),
            [rem] "r" (rem)
        // clobber
        :   "cc",
            "memory",
            "rdx",
            "rcx"
    );

    dest = &num_aux->chunk[0];

#else

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
    for(; i + 3 < count; i += 4)
    {
        uint64_t a = src_1[i];
        uint64_t b = src_1[i + 1];
        uint64_t c = src_1[i + 2];
        uint64_t d = src_1[i + 3];

        uint64_t c0 = 0;
        uint64_t c1 = 0;
        uint64_t prev_1 = 0; // src_2[j - 1], zero at j == 0
        uint64_t prev_2 = 0; // src_2[j - 2], zero at j <= 1
        uint64_t prev_3 = 0; // src_2[j - 3], zero at j <= 2

        #pragma GCC unroll 8
        for(uint64_t j = 0; j < count; j++)
        {
            uint64_t cur = src_2[j];
            uint128_t p1 = MUL(a, cur);
            uint128_t p2 = MUL(b, prev_1);
            uint128_t p3 = MUL(c, prev_2);
            uint128_t p4 = MUL(d, prev_3);
            prev_3 = prev_2;
            prev_2 = prev_1;
            prev_1 = cur;

            uint128_t sum = U128(dest[i + j]) + LOW(p1) + LOW(p2) + LOW(p3) + LOW(p4) + c0;
            dest[i + j] = LOW(sum);

            uint128_t acc = U128(HIGH(sum)) + HIGH(p1) + HIGH(p2) + HIGH(p3) + HIGH(p4) + c1;
            c0 = LOW(acc);
            c1 = HIGH(acc);
        }

        // the four words past the row block are written for the first time here; prev_1 is
        // src_2[count - 1], prev_2 is src_2[count - 2] and prev_3 is src_2[count - 3]
        uint128_t q1 = MUL(b, prev_1);
        uint128_t q2 = MUL(c, prev_2);
        uint128_t q3 = MUL(d, prev_3);
        uint128_t sum = U128(LOW(q1)) + LOW(q2) + LOW(q3) + c0;
        dest[i + count] = LOW(sum);

        uint128_t acc = U128(HIGH(sum)) + HIGH(q1) + HIGH(q2) + HIGH(q3) + c1;
        c0 = LOW(acc);
        c1 = HIGH(acc);

        uint128_t r1 = MUL(c, prev_1);
        uint128_t r2 = MUL(d, prev_2);
        sum = U128(LOW(r1)) + LOW(r2) + c0;
        dest[i + count + 1] = LOW(sum);

        acc = U128(HIGH(sum)) + HIGH(r1) + HIGH(r2) + c1;
        c0 = LOW(acc);
        c1 = HIGH(acc);

        uint128_t s1 = MUL(d, prev_1);
        sum = U128(LOW(s1)) + c0;
        dest[i + count + 2] = LOW(sum);
        dest[i + count + 3] = LOW(U128(HIGH(sum)) + HIGH(s1) + c1);
    }

    // the 4 row loop can leave up to 3 rows; a single 3 row pass takes them before
    // the one row fallback, which costs a dest load and store per product
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

    uint64_t * restrict dest = &num_res->chunk[pos + off];
    const uint64_t * restrict src = &num_src->chunk[src_pos];

    uint128_t borrow = 0;

#ifdef NUM_ASM_X86_64

    {
        uint64_t reg_1, reg_2;
        uint64_t j = len >> 3;
        uint64_t tail = len & 7;
        uint64_t run_off = 0;
        uint64_t out;
        uint64_t * dest_it = dest;
        const uint64_t * src_it = src;

        __asm__ __volatile__ (
            ".intel_syntax noprefix                         \n\t"

            "xor %k[pos], %k[pos]                           \n\t" // pos = 0, and CF = 0 (no incoming borrow for word 0)
            "xor %k[out], %k[out]                           \n\t" // out = 0 (setc below only ever writes its low byte)
            "test %[j], %[j]                                \n\t"
            "jz tail_setup%=                                \n\t"

            "loop_begin%=:                                  \n\t" // LOOP_SUB_BEGIN

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
            "jnz loop_begin%=                                \n\t"

            "tail_setup%=:                                  \n\t"
            "mov rcx, %[tail]                                \n\t" // rcx = tail
            "jrcxz tail_skip%=                               \n\t"

            "tail_begin%=:                                  \n\t" // LOOP_SUB_TAIL_BEGIN

            SUB_CLASSIC_STEP(0, dest, reg_1)

            "lea %[pos], [%[pos] + 8]                       \n\t"
            "dec rcx                                         \n\t" // dec does not modify CF
            "jnz tail_begin%=                                \n\t"

            "tail_skip%=:                                    \n\t"
            "setc %b[out]                                    \n\t" // out = CF (a genuine borrow occurred)

            ".att_syntax prefix                             \n\t"
            // out
            :   [dest] "+r" (dest_it),
                [src_2] "+r" (src_it),
                [pos] "+&r" (run_off),
                [j] "+&r" (j),
                [out] "=&r" (out),
                [reg_1] "=&r" (reg_1),
                [reg_2] "=&r" (reg_2)
            // in
            :   [tail] "r" (tail)
            // clobber
            :   "rcx",
                "cc",
                "memory"
        );

        borrow = out;
    }

#elif defined(NUM_ASM_AARCH64)

    // Same 8 wide sbcs chain as num_ssm_sub_mod_immed, but over an arbitrary len rather
    // than the 8k + 1 the full width kernels are guaranteed, so the tail loop earns its
    // keep here. The borrow leaves in a register instead of being consumed by a
    // normalize, because the caller still has to walk it up to the top word.
    {
        uint64_t a_0, a_1, a_2, a_3;
        uint64_t b_0, b_1, b_2, b_3;
        constexpr uint64_t unroll_log_2 = 3;
        constexpr uint64_t unroll_mask = 7;

        uint64_t j = len >> unroll_log_2;
        uint64_t tail = len & unroll_mask;
        uint64_t out;
        uint64_t * dest_it = dest;
        const uint64_t * src_it = src;

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
            "cset %[out], cc                                \n\t" // out = borrow (CF clear)
            // out
            :   [dest] "+r" (dest_it),
                [src_2] "+r" (src_it),
                [j] "+&r" (j),
                [tail] "+&r" (tail),
                [out] "=&r" (out),
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

        borrow = out;
    }

#else

    #pragma GCC unroll 8
    for(uint64_t i = 0; i < len; i++)
    {
        uint128_t diff = U128(dest[i]) - src[i] - borrow;
        dest[i] = LOW(diff);
        borrow = HIGH(diff) & 1;
    }

#endif

    dest = &num_res->chunk[pos];

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
    num_ssm_fft_fwd(num_aux, num_fft, p, 1);
}

static void num_ssm_mul_pointwise(
    num_p num_aux_1,
    num_p num_aux_2,
    num_p num_fft_1,
    num_p num_fft_2,
    ssm_params_p p,
    uint64_t threads
);

// KEEPS NUM_1 NUM_2. Always single-threaded: only num_ssm_mul_pointwise's outermost
// call ever fans out across threads; every call to num_ssm_mul_wrap happens one
// recursion level below that, so it has nothing to pass but 1.
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
        p,
        1
    );

    num_ssm_fft_inv(num_aux_2, num_fft_1, p, 1);
    num_ssm_depad_wrap(
        num_aux_1,
        num_aux_2,
        num_1,
        pos,
        num_fft_1,
        p
    );
}



typedef struct
{
    num_p num_fft_1;
    num_p num_fft_2;
    ssm_params_p p;
    num_p num_aux_2;
    uint64_t i_start;
    uint64_t i_end;
} ssm_pointwise_base_worker_t;

static void * ssm_pointwise_base_worker(void * arg)
{
    ssm_pointwise_base_worker_t * w = arg;
    for(uint64_t i = w->i_start; i < w->i_end; i++)
    {
        num_ssm_mul_mod_span(w->num_aux_2, w->num_fft_1, w->num_fft_2, i * w->p->n, w->p->n);
    }
    return nullptr;
}

typedef struct
{
    num_p num_fft_1;
    num_p num_fft_2;
    ssm_params_p p;
    ssm_params_p p_next;
    num_p num_aux_1;
    num_p num_aux_2;
    num_p num_fft_1_next;
    num_p num_fft_2_next;
    uint64_t i_start;
    uint64_t i_end;
} ssm_pointwise_rec_worker_t;

static void * ssm_pointwise_rec_worker(void * arg)
{
    ssm_pointwise_rec_worker_t * w = arg;
    for(uint64_t i = w->i_start; i < w->i_end; i++)
    {
        // NOLINTNEXTLINE(readability-suspicious-call-argument)
        num_ssm_mul_wrap(
            w->num_aux_1,
            w->num_aux_2,
            w->num_fft_1_next,
            w->num_fft_2_next,
            w->num_fft_1,
            w->num_fft_2,
            i * w->p->n,
            w->p_next
        );
    }
    return nullptr;
}

// num_aux_1->size >= p->n
// num_aux_2->size >= 2 * p->n
static void num_ssm_mul_pointwise(
    num_p num_aux_1,
    num_p num_aux_2,
    num_p num_fft_1,
    num_p num_fft_2,
    ssm_params_p p,
    uint64_t threads
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

    uint64_t workers = ssm_worker_count(threads, p->K);

    if(!ssm_is_recursive(p->n))
    {
        if(workers <= 1)
        {
            for(uint64_t i=0; i<p->K; i++)
            {
                num_ssm_mul_mod_span(num_aux_2, num_fft_1, num_fft_2, i * p->n, p->n);
            }
            return;
        }

        ssm_pointwise_base_worker_t * worker_args = malloc(workers * sizeof(*worker_args));
        assert(worker_args)
        pthread_t * worker_ids = malloc(workers * sizeof(*worker_ids));
        assert(worker_ids)

        for(uint64_t w=0; w<workers; w++)
        {
            uint64_t i_start, i_end;
            ssm_worker_range(w, workers, p->K, &i_start, &i_end);
            worker_args[w] = (ssm_pointwise_base_worker_t)
            {
                .num_fft_1 = num_fft_1,
                .num_fft_2 = num_fft_2,
                .p = p,
                .num_aux_2 = num_create_dirty(CLU_ARGS(2 * p->n, 0)),
                .i_start = i_start,
                .i_end = i_end,
            };
            TREAT(pthread_create(&worker_ids[w], nullptr, ssm_pointwise_base_worker, &worker_args[w]))
        }
        for(uint64_t w=0; w<workers; w++)
        {
            TREAT(pthread_join(worker_ids[w], nullptr))
            num_free(worker_args[w].num_aux_2);
        }

        free(worker_ids);
        free(worker_args);
        return;
    }

    ssm_params_t p_next = ssm_get_params_wrap(p->n);

    if(workers <= 1)
    {
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
        return;
    }

    ssm_pointwise_rec_worker_t * worker_args = malloc(workers * sizeof(*worker_args));
    assert(worker_args)
    pthread_t * worker_ids = malloc(workers * sizeof(*worker_ids));
    assert(worker_ids)

    for(uint64_t w=0; w<workers; w++)
    {
        uint64_t i_start, i_end;
        ssm_worker_range(w, workers, p->K, &i_start, &i_end);
        worker_args[w] = (ssm_pointwise_rec_worker_t)
        {
            .num_fft_1 = num_fft_1,
            .num_fft_2 = num_fft_2,
            .p = p,
            .p_next = &p_next,
            .num_aux_1 = num_create_dirty(CLU_ARGS(p->n, 0)),
            .num_aux_2 = num_create_dirty(CLU_ARGS(2 * p->n, 0)),
            .num_fft_1_next = num_create_dirty(CLU_ARGS(p_next.n * p_next.K, 0)),
            .num_fft_2_next = num_create_dirty(CLU_ARGS(p_next.n * p_next.K, 0)),
            .i_start = i_start,
            .i_end = i_end,
        };
        TREAT(pthread_create(&worker_ids[w], nullptr, ssm_pointwise_rec_worker, &worker_args[w]))
    }
    for(uint64_t w=0; w<workers; w++)
    {
        TREAT(pthread_join(worker_ids[w], nullptr))
        num_free(worker_args[w].num_aux_1);
        num_free(worker_args[w].num_aux_2);
        num_free(worker_args[w].num_fft_1_next);
        num_free(worker_args[w].num_fft_2_next);
    }

    free(worker_ids);
    free(worker_args);
}



// Production pads inside the forward transform now (ssm_fft_fwd_pad_block), so this
// standalone form is reached only by its own round-trip test, which is why it needs
// the attribute: STATIC makes it static in release, where nothing calls it.
[[maybe_unused]]
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
    bool free_inputs,
    uint64_t threads
)
{
    CLU_HANDLER_IS_SAFE(num_aux)
    CLU_HANDLER_IS_SAFE(num)
    assert(num_aux)
    assert(num)
    assert(num_aux->size >= 2 * p->n)

    // Allocated dirty: every limb of every element is written by the pad fused into
    // the transform's first pass, so num_create's zeroing of the whole n*K array
    // would be a wasted pass over hundreds of MB.
    num_p num_fft = num_create_dirty(CLU_ARGS(p->n * p->K, 0));

    num_ssm_fft_fwd_pad(num_aux, num_fft, num, p, threads);

    if(free_inputs)
    {
        num_free(num);
    }
    return num_fft;
}

// KEEPS NUM_1 NUM_2
num_p num_mul_ssm(num_p num_1, num_p num_2, bool free_inputs, uint64_t threads)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    ssm_params_t p = ssm_get_params(num_1->count + num_2->count);
    num_p num_aux_2 = num_create_dirty(CLU_ARGS(2 * p.n, 0));
    num_p num_fft_1 = num_ssm_prepare_no_wrap(num_aux_2, num_1, &p, free_inputs, threads);
    num_p num_fft_2 = num_ssm_prepare_no_wrap(num_aux_2, num_2, &p, free_inputs, threads);

    // The pointwise multiply rides inside the inverse transform's first pass rather
    // than running as a pass of its own, so the products are consumed from cache
    // instead of being written to DRAM and read straight back.
    num_ssm_fft_inv_pointwise(num_aux_2, num_fft_1, num_fft_2, &p, threads);
    num_free(num_fft_2);
    num_free(num_aux_2);

    return num_ssm_depad_no_wrap(num_fft_1, &p);
}




static bool mul_is_classic(uint64_t count_1, uint64_t count_2)
{
    constexpr uint64_t threshold = 256;
    return (bool)((count_1 < threshold) || (count_2 < threshold));
}

// Separate from mul_is_classic's "use SSM at all" threshold: an operand can be well
// past that and still be too small for spawning many threads to pay off
// (pthread_create/join plus a per-worker scratch allocation cost real time, worth
// paying for a huge multiply, not for a merely SSM-eligible one). A flat yes/no gate
// on whether to thread at all isn't enough either -- found via num_div_mod_bz_rec's
// recursion, which calls num_mul_core at every level with shrinking operands: capped
// at a single global thread count once past the gate, the many mid-sized calls just
// past it still got the full caller-requested thread count and spawned more workers
// than that size could productively use, regressing above ~4 threads. Scaling the
// *ceiling* to size instead (rather than a bool) fixes that: this is the one place
// that decides "how much threading is this size worth" since it's the one place that
// actually sees the operand size on every call, so every caller (division's deep
// recursion included) gets this for free instead of having to reason about it itself.
//
// mul_min_limbs_per_thread is a starting point, not a derived constant -- tuned from
// one real measurement (division at ~4-9M limb operands), not a closed-form model of
// pthread overhead vs. SSM work. Revisit if it doesn't hold up at other sizes.
constexpr uint64_t mul_min_limbs_per_thread = 16384;

static uint64_t mul_threads_ceiling(uint64_t count_1, uint64_t count_2)
{
    uint64_t count = count_1 < count_2 ? count_1 : count_2;
    uint64_t ceiling = count / mul_min_limbs_per_thread;
    return ceiling ? ceiling : 1;
}

num_p num_mul_core(num_p num_1, num_p num_2, bool free_inputs, uint64_t threads)
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

    uint64_t ceiling = mul_threads_ceiling(num_1->count, num_2->count);
    if(threads > ceiling)
    {
        threads = ceiling;
    }

    return num_mul_ssm(num_1, num_2, free_inputs, threads);
}



// 0 if disk_threshold_bytes would redirect this size to disk, matching
// num_create's per-buffer check.
static double mem_estimate_ram_bytes(uint64_t size, uint64_t disk_threshold_bytes)
{
    uint64_t bytes = sizeof(num_t) + (size * sizeof(uint64_t));
    if(bytes > disk_threshold_bytes)
    {
        return 0.0;
    }

    return (double)bytes;
}

// Butterfly-count proxy for FFT cost; K is always a power of two here.
static double mem_estimate_log2_pow2(uint64_t k)
{
    if(k < 2)
    {
        return 0.0;
    }

    return (double)(stdc_bit_width(k) - 1);
}

typedef struct
{
    double integral;  // bytes * time: for time-weighted averaging
    double duration;  // time: word-operation proxy, not wall clock
} mem_profile_t;

// Mirrors num_ssm_mul_pointwise's allocation sites and recursion, weighting each
// phase's RAM by a word-operation duration proxy. `threads` mirrors the real code's
// single-level cap: only this top call fans out across `workers`; the recursive
// self-call below always passes 1.
static mem_profile_t ssm_pointwise_mem_estimate(
    uint64_t n,
    uint64_t K,
    double live_baseline,
    uint64_t disk_threshold_bytes,
    uint64_t threads
)
{
    uint64_t workers = ssm_worker_count(threads, K);

    if(!ssm_is_recursive(n))
    {
        double extra = (double)(workers - 1) * mem_estimate_ram_bytes(2 * n, disk_threshold_bytes);
        double live = live_baseline + extra;
        double dt = ((double)K * (double)n * (double)n) / (double)workers;
        return (mem_profile_t)
        {
            .integral = live * dt,
            .duration = dt,
        };
    }

    ssm_params_t p_next = ssm_get_params_wrap(n);
    double extra_aux = (double)(workers - 1) * (
        mem_estimate_ram_bytes(n, disk_threshold_bytes) +
        mem_estimate_ram_bytes(2 * n, disk_threshold_bytes)
    );
    double next_bytes = (double)workers * 2.0 * mem_estimate_ram_bytes(p_next.n * p_next.K, disk_threshold_bytes);
    double live = live_baseline + extra_aux + next_bytes;

    mem_profile_t inner = ssm_pointwise_mem_estimate(p_next.n, p_next.K, live, disk_threshold_bytes, 1);

    double fft_inv_dt = (double)p_next.n * (double)p_next.K * mem_estimate_log2_pow2(p_next.K);
    double iter_integral = inner.integral + (live * fft_inv_dt);
    double iter_duration = inner.duration + fft_inv_dt;

    return (mem_profile_t)
    {
        .integral = (iter_integral * (double)K) / (double)workers,
        .duration = (iter_duration * (double)K) / (double)workers,
    };
}

// Time-weighted average RAM (bytes) live during num_mul_threads, mirroring
// num_mul_core's allocation sites. disk_threshold_bytes excludes buffers num_create
// would put on disk; threads should match whatever will be passed to num_mul_threads.
uint64_t num_mul_estimate_memory(
    uint64_t count_1,
    uint64_t count_2,
    uint64_t disk_threshold_bytes,
    uint64_t threads
)
{
    if(count_1 == 0 || count_2 == 0)
    {
        return sizeof(num_t) + sizeof(uint64_t);
    }

    if(mul_is_classic(count_1, count_2))
    {
        double bytes = mem_estimate_ram_bytes(count_1, disk_threshold_bytes)
            + mem_estimate_ram_bytes(count_2, disk_threshold_bytes)
            + mem_estimate_ram_bytes(count_1 + count_2, disk_threshold_bytes);
        return (uint64_t)bytes;
    }

    double live = mem_estimate_ram_bytes(count_1, disk_threshold_bytes)
        + mem_estimate_ram_bytes(count_2, disk_threshold_bytes);
    double integral = 0.0;
    double duration = 0.0;

    ssm_params_t p = ssm_get_params(count_1 + count_2);
    uint64_t n = p.n;
    uint64_t K = p.K;

    live += mem_estimate_ram_bytes(n, disk_threshold_bytes) + mem_estimate_ram_bytes(2 * n, disk_threshold_bytes);

    // Fwd/inv FFT phases are threaded the same single-level way as the pointwise
    // K-loop, but the recursion bottoms out at K=2 per leaf, so K/2 leaves is the most
    // fan-out achievable (vs pointwise's min(threads, K)).
    uint64_t fft_workers = ssm_worker_count(threads, K / 2);

    for(uint64_t side = 0; side < 2; side++)
    {
        uint64_t count = side == 0 ? count_1 : count_2;
        double extra = (double)(fft_workers - 1) * mem_estimate_ram_bytes(2 * n, disk_threshold_bytes);
        live += mem_estimate_ram_bytes(n * K, disk_threshold_bytes);
        live -= mem_estimate_ram_bytes(count, disk_threshold_bytes);

        double dt = ((double)n * (double)K * mem_estimate_log2_pow2(K)) / (double)fft_workers;
        integral += (live + extra) * dt;
        duration += dt;
    }

    mem_profile_t pw = ssm_pointwise_mem_estimate(n, K, live, disk_threshold_bytes, threads);
    integral += pw.integral;
    duration += pw.duration;

    live -= mem_estimate_ram_bytes(n, disk_threshold_bytes);
    live -= mem_estimate_ram_bytes(n * K, disk_threshold_bytes);

    double fft_inv_extra = (double)(fft_workers - 1) * mem_estimate_ram_bytes(2 * n, disk_threshold_bytes);
    double fft_inv_dt = ((double)n * (double)K * mem_estimate_log2_pow2(K)) / (double)fft_workers;
    integral += (live + fft_inv_extra) * fft_inv_dt;
    duration += fft_inv_dt;

    live -= mem_estimate_ram_bytes(2 * n, disk_threshold_bytes);

    uint64_t target_count = (p.M * (p.K - 1)) + p.n;
    live += mem_estimate_ram_bytes(target_count, disk_threshold_bytes);
    live -= mem_estimate_ram_bytes(n * K, disk_threshold_bytes);

    double depad_dt = (double)target_count;
    integral += live * depad_dt;
    duration += depad_dt;

    return duration > 0.0 ? (uint64_t)(integral / duration) : (uint64_t)live;
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
    ssm_params_p p,
    uint64_t threads
);

// KEEPS NUM. Always single-threaded: only num_ssm_sqr_pointwise's outermost call ever
// fans out across threads; every call to num_ssm_sqr_wrap happens one recursion level
// below that, so it has nothing to pass but 1.
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
        p,
        1
    );

    num_ssm_fft_inv(num_aux_2, num_fft, p, 1);
    num_ssm_depad_wrap(
        num_aux_1,
        num_aux_2,
        num,
        pos,
        num_fft,
        p
    );
}

typedef struct
{
    num_p num_fft;
    ssm_params_p p;
    num_p num_aux_2;
    uint64_t i_start;
    uint64_t i_end;
} ssm_sqr_pointwise_base_worker_t;

static void * ssm_sqr_pointwise_base_worker(void * arg)
{
    ssm_sqr_pointwise_base_worker_t * w = arg;
    for(uint64_t i = w->i_start; i < w->i_end; i++)
    {
        num_ssm_sqr_mod_span(w->num_aux_2, w->num_fft, i * w->p->n, w->p->n);
    }
    return nullptr;
}

typedef struct
{
    num_p num_fft;
    ssm_params_p p;
    ssm_params_p p_next;
    num_p num_aux_1;
    num_p num_aux_2;
    num_p num_fft_next;
    uint64_t i_start;
    uint64_t i_end;
} ssm_sqr_pointwise_rec_worker_t;

static void * ssm_sqr_pointwise_rec_worker(void * arg)
{
    ssm_sqr_pointwise_rec_worker_t * w = arg;
    for(uint64_t i = w->i_start; i < w->i_end; i++)
    {
        // NOLINTNEXTLINE(readability-suspicious-call-argument)
        num_ssm_sqr_wrap(
            w->num_aux_1,
            w->num_aux_2,
            w->num_fft_next,
            w->num_fft,
            i * w->p->n,
            w->p_next
        );
    }
    return nullptr;
}

// num_aux_1->size >= p->n
// num_aux_2->size >= 2 * p->n
static void num_ssm_sqr_pointwise(
    num_p num_aux_1,
    num_p num_aux_2,
    num_p num_fft,
    ssm_params_p p,
    uint64_t threads
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

    uint64_t workers = ssm_worker_count(threads, p->K);

    if(!ssm_is_recursive(p->n))
    {
        if(workers <= 1)
        {
            for(uint64_t i=0; i<p->K; i++)
            {
                num_ssm_sqr_mod_span(num_aux_2, num_fft, i * p->n, p->n);
            }
            return;
        }

        ssm_sqr_pointwise_base_worker_t * worker_args = malloc(workers * sizeof(*worker_args));
        assert(worker_args)
        pthread_t * worker_ids = malloc(workers * sizeof(*worker_ids));
        assert(worker_ids)

        for(uint64_t w=0; w<workers; w++)
        {
            uint64_t i_start, i_end;
            ssm_worker_range(w, workers, p->K, &i_start, &i_end);
            worker_args[w] = (ssm_sqr_pointwise_base_worker_t)
            {
                .num_fft = num_fft,
                .p = p,
                .num_aux_2 = num_create_dirty(CLU_ARGS(2 * p->n, 0)),
                .i_start = i_start,
                .i_end = i_end,
            };
            TREAT(pthread_create(&worker_ids[w], nullptr, ssm_sqr_pointwise_base_worker, &worker_args[w]))
        }
        for(uint64_t w=0; w<workers; w++)
        {
            TREAT(pthread_join(worker_ids[w], nullptr))
            num_free(worker_args[w].num_aux_2);
        }

        free(worker_ids);
        free(worker_args);
        return;
    }

    ssm_params_t p_next = ssm_get_params_wrap(p->n);

    if(workers <= 1)
    {
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
        return;
    }

    ssm_sqr_pointwise_rec_worker_t * worker_args = malloc(workers * sizeof(*worker_args));
    assert(worker_args)
    pthread_t * worker_ids = malloc(workers * sizeof(*worker_ids));
    assert(worker_ids)

    for(uint64_t w=0; w<workers; w++)
    {
        uint64_t i_start, i_end;
        ssm_worker_range(w, workers, p->K, &i_start, &i_end);
        worker_args[w] = (ssm_sqr_pointwise_rec_worker_t)
        {
            .num_fft = num_fft,
            .p = p,
            .p_next = &p_next,
            .num_aux_1 = num_create_dirty(CLU_ARGS(p->n, 0)),
            .num_aux_2 = num_create_dirty(CLU_ARGS(2 * p->n, 0)),
            .num_fft_next = num_create_dirty(CLU_ARGS(p_next.n * p_next.K, 0)),
            .i_start = i_start,
            .i_end = i_end,
        };
        TREAT(pthread_create(&worker_ids[w], nullptr, ssm_sqr_pointwise_rec_worker, &worker_args[w]))
    }
    for(uint64_t w=0; w<workers; w++)
    {
        TREAT(pthread_join(worker_ids[w], nullptr))
        num_free(worker_args[w].num_aux_1);
        num_free(worker_args[w].num_aux_2);
        num_free(worker_args[w].num_fft_next);
    }

    free(worker_ids);
    free(worker_args);
}

num_p num_sqr_ssm(num_p num, uint64_t threads)
{
    CLU_HANDLER_IS_SAFE(num)
    assert(num)

    ssm_params_t p = ssm_get_params(2 * num->count);
    num_p num_aux_1 = num_create_dirty(CLU_ARGS(p.n, 0));
    num_p num_aux_2 = num_create_dirty(CLU_ARGS(2 * p.n, 0));
    num_p num_fft = num_ssm_prepare_no_wrap(num_aux_2, num, &p, true, threads);

    num_ssm_sqr_pointwise(num_aux_1, num_aux_2, num_fft, &p, threads);
    num_free(num_aux_1);

    num_ssm_fft_inv(num_aux_2, num_fft, &p, threads);
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
// This recursion never forks into concurrent branches (each step depends on the
// previous one's effect on the shared remainder), so there's no oversubscription risk
// from threading -- `threads` is passed unchanged at every level, all the way down.
// It does call itself many times (depth ~log2(num_2->count)) with the multiply's
// operand shrinking every level; threading every one of those calls uniformly was
// tried and made things *worse* purely from pthread_create/join overhead at the many
// small deep calls. Fixed at the source instead of here: num_mul_core now caps
// `threads` to a size-scaled ceiling (mul_threads_ceiling) for every call, so this
// function doesn't need to reason about its own recursion depth or size at all -- it
// just forwards `threads` everywhere and trusts num_mul_core to only ever spawn as
// many workers as each call's own size can productively use.
static num_p num_div_mod_bz_rec(
    num_p num_aux,
    num_p num_1,
    num_p num_2,
    bz_frame_t f[],
    uint64_t threads
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
            threads
        );
        num_normalize(num_1);

        if(num_is_zero(num_q_tmp))
        {
            num_q[i] = num_q_tmp;
            continue;
        }

        num_p num_aux_2 = num_mul_core(num_q_tmp, &f->num_2_0, false, threads);
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
static num_p num_div_mod_bz(num_p num_1, num_p num_2, uint64_t threads)
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

        num_p num_q_tmp = num_div_mod_bz_rec(num_aux, &num_1_1, num_2, f, threads);
        num_normalize(num_1);
        num_q_tmp = num_expand_to(num_q_tmp, n_2 + num_q->count);
        num_add_offset(num_q_tmp, n_2, num_q);
        num_free(num_q);
        num_q = num_q_tmp;
    }

    num_p num_q_tmp = num_div_mod_bz_rec(num_aux, num_1, num_2, f, threads);
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

    return num_mul_core(num_1, num_2, true, 1);
}

// Same as num_mul, but the caller picks how many threads the SSM pointwise multiply
// (if reached) may fan its K-loop out across. threads is read once per call, so a
// multiplication already in flight is unaffected by what later callers pass.
num_p num_mul_threads(num_p num_1, num_p num_2, uint64_t threads)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    return num_mul_core(num_1, num_2, true, threads);
}

static num_p num_sqr_core(num_p num, uint64_t threads)
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

    uint64_t ceiling = mul_threads_ceiling(num->count, num->count);
    if(threads > ceiling)
    {
        threads = ceiling;
    }

    return num_sqr_ssm(num, threads);
}

num_p num_sqr(num_p num)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    return num_sqr_core(num, 1);
}

// Same as num_sqr, but the caller picks how many threads the SSM pointwise square (if
// reached) may fan its K-loop out across, same as num_mul_threads.
num_p num_sqr_threads(num_p num, uint64_t threads)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    return num_sqr_core(num, threads);
}

static num_p num_pow_core(num_p num, uint64_t value, uint64_t threads)
{
    if(num->count == 0)
    {
        assert(value);
        return num;
    }

    num_p num_res = num_wrap(1);
    for(uint64_t mask = B(63); mask; mask >>= 1)
    {
        num_res = num_sqr_threads(num_res, threads);
        if(value & mask)
        {
            num_res = num_mul_threads(num_res, num_copy(num), threads);
        }
    }
    num_free(num);
    return num_res;
}

num_p num_pow(num_p num, uint64_t value) // TODO TEST
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    return num_pow_core(num, value, 1);
}

// Same as num_pow, but the caller picks how many threads each squaring/multiply
// in the repeated-squaring loop may fan out across, same as num_mul_threads.
num_p num_pow_threads(num_p num, uint64_t value, uint64_t threads) // TODO TEST
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    return num_pow_core(num, value, threads);
}

// out_num_q and out_num_r can be nullptr
static void num_div_mod_core(
    num_p *out_num_q,
    num_p *out_num_r,
    num_p num_1,
    num_p num_2,
    uint64_t threads
)
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
    num_p num_q = num_div_mod_bz(num_1, num_2, threads);
    num_div_mod_finalize(out_num_q, out_num_r, num_q, num_1, num_2, bits);
}

void num_div_mod(num_p *out_num_q, num_p *out_num_r, num_p num_1, num_p num_2)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    num_div_mod_core(out_num_q, out_num_r, num_1, num_2, 1);
}

// Same as num_div_mod, but the caller picks how many threads the one multiply inside
// the Burnikel-Ziegler recursion (code.c num_div_mod_bz_rec) may use. That recursion
// never forks into concurrent branches, so unlike num_mul_threads/num_sqr_threads
// there's no "outermost call only" restriction -- every level can use the full count.
void num_div_mod_threads(
    num_p *out_num_q,
    num_p *out_num_r,
    num_p num_1,
    num_p num_2,
    uint64_t threads
)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    num_div_mod_core(out_num_q, out_num_r, num_1, num_2, threads);
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

num_p num_div_threads(num_p num_1, num_p num_2, uint64_t threads)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    num_p num_q;
    num_div_mod_threads(&num_q, nullptr, num_1, num_2, threads);
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

num_p num_mod_threads(num_p num_1, num_p num_2, uint64_t threads)
{
    CLU_HANDLER_IS_SAFE(num_1)
    CLU_HANDLER_IS_SAFE(num_2)
    assert(num_1)
    assert(num_2)

    num_div_mod_threads(nullptr, &num_1, num_1, num_2, threads);
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



typedef struct
{
    num_p num;
    num_p * num_bases;
    uint64_t i;
    uint64_t threads;
    num_p result;
} num_base_to_worker_t;

static num_p num_base_to_rec(num_p num, num_p num_bases[], uint64_t i, uint64_t threads);

static void * num_base_to_worker(void * arg)
{
    num_base_to_worker_t * w = arg;
    w->result = num_base_to_rec(w->num, w->num_bases, w->i, w->threads);
    return nullptr;
}

// Binary-splitting base conversion: each level divides num by base^(2^i), and the
// resulting hi/lo halves (num_q, num_r) recurse independently of one another --
// unlike num_div_mod_bz_rec's recursion, which stays on one call stack throughout,
// this one can fork num_q's branch onto its own thread while num_r's runs on the
// current one. threads is split in half per fork, on each branch's own share of
// the recursion, and passed unsplit into num_div_mod_threads for the division at
// this level -- safe to hand over in full because num_mul_core (the one place the
// division's internal multiply funnels through) re-derives its own ceiling from
// operand size on every call regardless of what's requested, the same reason
// num_div_mod_bz_rec's per-level calls don't over-thread either (see
// mul_threads_ceiling's comment).
static num_p num_base_to_rec(num_p num, num_p num_bases[], uint64_t i, uint64_t threads)
{
    CLU_HANDLER_IS_SAFE(num);
    assert(num);

    if(i == UINT64_MAX)
    {
        return num;
    }

    if(num_cmp(num, num_bases[i]) < 0)
    {
        return num_base_to_rec(num, num_bases, i - 1, threads);
    }

    num_p num_q, num_r;
    num_div_mod_threads(&num_q, &num_r, num, num_copy(num_bases[i]), threads);

    if(threads <= 1 || i == 0)
    {
        num_q = num_base_to_rec(num_q, num_bases, i - 1, threads);
        num_r = num_base_to_rec(num_r, num_bases, i - 1, threads);
    }
    else
    {
        uint64_t threads_q = threads / 2;
        uint64_t threads_r = threads - threads_q;

        num_base_to_worker_t worker = (num_base_to_worker_t)
        {
            .num = num_q,
            .num_bases = num_bases,
            .i = i - 1,
            .threads = threads_q,
        };
        pthread_t worker_id;
        TREAT(pthread_create(&worker_id, nullptr, num_base_to_worker, &worker))

        num_r = num_base_to_rec(num_r, num_bases, i - 1, threads_r);

        TREAT(pthread_join(worker_id, nullptr))
        num_q = worker.result;
    }

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

    return num_base_to_threads(num, base, 1);
}

// Same as num_base_to, but the caller picks how many threads the conversion's
// binary-splitting recursion may fan out across -- see num_base_to_rec's comment.
num_p num_base_to_threads(num_p num, uint64_t base, uint64_t threads)
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

    uint64_t ceiling = mul_threads_ceiling(num->count, num->count);
    if(threads > ceiling)
    {
        threads = ceiling;
    }

    num_p num_res = num_base_to_rec(num, num_bases, max - 1, threads);

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
