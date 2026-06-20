#include "../debug.h"
#include "../../../testrc.h"
#include "../../../mods/macros/test.h"



static char buffer[256];

#ifndef NAME
// Fallback definition to keep the IDE/linter happy
// when it parses behavior.c in isolation.
#define NAME "UNDEFINED_TAG"
#endif

[[maybe_unused]]
static char* format_buffer(const char* func)
{
    snprintf(buffer, 256, "%s %s", func, NAME);
    return buffer;
}

#undef FUNC_TAG
#define FUNC_TAG format_buffer(__func__)



static void test_uint_from_char(bool show)
{
    TEST_FN_OPEN

    #define TEST_UINT_FROM_CHAR(TAG, CHAR, UINT)    \
    {                                               \
        TEST_CASE_OPEN(TAG)                         \
        {                                           \
            uint64_t res = uint_from_char(CHAR);    \
            assert(uint64(res, UINT));              \
        }                                           \
        TEST_CASE_CLOSE                             \
    }

    TEST_UINT_FROM_CHAR(1, '0',  0)
    TEST_UINT_FROM_CHAR(2, '1',  1)
    TEST_UINT_FROM_CHAR(3, '9',  9)
    TEST_UINT_FROM_CHAR(4, 'a', 10)
    TEST_UINT_FROM_CHAR(5, 'b', 11)
    TEST_UINT_FROM_CHAR(6, 'f', 15)
    TEST_UINT_FROM_CHAR(7, 'A', 10)
    TEST_UINT_FROM_CHAR(8, 'B', 11)
    TEST_UINT_FROM_CHAR(9, 'F', 15)

    #undef TEST_UINT_FROM_CHAR

    TEST_CASE_OPEN(10)
    {
        TEST_REVERT_OPEN
        {
            uint_from_char('g');
        }
        TEST_REVERT_CLOSE
    }
    TEST_CASE_CLOSE

    TEST_FN_CLOSE
}

static void test_uint128(bool show)
{
    TEST_FN_OPEN

    TEST_CASE_OPEN(1)
    {
        uint128_t u = 1;
        assert(uint128_immed(u, 0, 1));
    }
    TEST_CASE_CLOSE

    TEST_CASE_OPEN(2)
    {
        uint128_t u = UINT64_MAX;
        assert(uint128_immed(u, 0, UINT64_MAX));
    }
    TEST_CASE_CLOSE

    TEST_CASE_OPEN(3)
    {
        uint128_t u = U128HL(1, 0);
        assert(uint128_immed(u, 1, 0));
    }
    TEST_CASE_CLOSE

    TEST_CASE_OPEN(4)
    {
        uint128_t u = U128HL(1, 2);
        assert(uint128_immed(u, 1, 2));
    }
    TEST_CASE_CLOSE

    TEST_CASE_OPEN(5)
    {
        uint128_t u = U128HL(UINT64_MAX, UINT64_MAX);
        assert(uint128_immed(u, UINT64_MAX, UINT64_MAX));
    }
    TEST_CASE_CLOSE

    TEST_CASE_OPEN(6)
    {
        uint128_t u = UINT64_MAX;
        u += 1;
        assert(uint128_immed(u, 1, 0));
    }
    TEST_CASE_CLOSE

    TEST_CASE_OPEN(7)
    {
        uint128_t u = U128HL(1, 0);
        u -= 1;
        assert(uint128_immed(u, 0, UINT64_MAX));
    }
    TEST_CASE_CLOSE

    TEST_CASE_OPEN(8)
    {
        uint128_t u = UINT64_MAX;
        u *= UINT64_MAX;
        assert(uint128_immed(u, UINT64_MAX - 1, 1));
    }
    TEST_CASE_CLOSE

    TEST_CASE_OPEN(9)
    {
        uint128_t u = U128HL(1, 0);
        u <<= 1;
        assert(uint128_immed(u, 2, 0));
    }
    TEST_CASE_CLOSE

    TEST_CASE_OPEN(10)
    {
        uint128_t u = U128HL(1, 0);
        u >>= 1;
        assert(uint128_immed(u, 0, 0x8000000000000000));
    }
    TEST_CASE_CLOSE

    TEST_CASE_OPEN(11)
    {
        uint128_t u = U128HL(1, 0);
        u /= 1;
        assert(uint128_immed(u, 1, 0));
    }
    TEST_CASE_CLOSE

    TEST_CASE_OPEN(12)
    {
        uint128_t u = U128HL(1, 0);
        u /= 2;
        assert(uint128_immed(u, 0, 0x8000000000000000));
    }
    TEST_CASE_CLOSE

    TEST_CASE_OPEN(13)
    {
        uint128_t u = U128HL(UINT64_MAX, UINT64_MAX);
        u /= 1;
        assert(uint128_immed(u, UINT64_MAX, UINT64_MAX));
    }
    TEST_CASE_CLOSE

    TEST_CASE_OPEN(14)
    {
        uint128_t u = U128HL(UINT64_MAX / 2, UINT64_MAX);
        u /= 2;
        assert(uint128_immed(u, UINT64_MAX >> 2, UINT64_MAX));
    }
    TEST_CASE_CLOSE

    TEST_CASE_OPEN(15)
    {
        uint128_t u = U128HL(UINT64_MAX, UINT64_MAX - 1);
        u /= 2;
        assert(uint128_immed(u, UINT64_MAX >> 1, UINT64_MAX));
    }
    TEST_CASE_CLOSE

    TEST_FN_CLOSE
}



static void test_num_create(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_CREATE(TAG, SIZE, COUNT, res_size)     \
    {                                                       \
        TEST_CASE_OPEN(TAG)                                 \
        {                                                   \
            num_p num = num_create(CLU_ARGS(SIZE, COUNT));  \
            assert(uint64(num->size, res_size));            \
            assert(uint64(num->count, COUNT));              \
            assert(num->chunk != nullptr);                  \
            num_free(num);                                  \
        }                                                   \
        TEST_CASE_CLOSE                                     \
    }

    TEST_NUM_CREATE(1, 0, 0, 1)
    TEST_NUM_CREATE(2, 1, 1, 1)
    TEST_NUM_CREATE(3, 2, 2, 2)
    TEST_NUM_CREATE(4, 3, 2, 3)

    #undef TEST_NUM_CREATE

    TEST_CASE_OPEN(5)
    {
        TEST_REVERT_OPEN
        {
            num_create(CLU_ARGS(0, 1));
        }
        TEST_REVERT_CLOSE
    }
    TEST_CASE_CLOSE

    TEST_FN_CLOSE
}

static void test_num_expand_to(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_EXPAND_TO(TAG, NUM, SIZE)          \
    {                                                   \
        TEST_CASE_OPEN(TAG)                             \
        {                                               \
            num_p num = num_create_immed(ARG_OPEN NUM); \
            num = num_expand_to(num, SIZE);             \
            assert(uint64(num->size, SIZE));            \
            assert(num_immed(num, ARG_OPEN NUM));       \
        }                                               \
        TEST_CASE_CLOSE                                 \
    }

    TEST_NUM_EXPAND_TO(1, (0), 2)
    TEST_NUM_EXPAND_TO(2, (2, 1, 2), 2)
    TEST_NUM_EXPAND_TO(2, (2, 1, 2), 3)
    TEST_NUM_EXPAND_TO(3, (2, 1, 2), 10)

    #undef TEST_NUM_EXPAND_TO

    TEST_FN_CLOSE
}

static void test_num_normalize(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_NORMALIZE(TAG, NUM_BEF, NUM_AFT)       \
    {                                                       \
        TEST_CASE_OPEN(TAG)                                 \
        {                                                   \
            num_p num = num_create_immed(ARG_OPEN NUM_BEF); \
            num_normalize(num);                             \
            assert(num_immed(num, ARG_OPEN NUM_AFT));       \
        }                                                   \
        TEST_CASE_CLOSE                                     \
    }

    TEST_NUM_NORMALIZE(1,
        (0),
        (0)
    )
    TEST_NUM_NORMALIZE(2,
        (1, 1),
        (1, 1)
    )
    TEST_NUM_NORMALIZE(3,
        (1, 0),
        (0)
    )
    TEST_NUM_NORMALIZE(4,
        (2, 0, 1),
        (1, 1)
    )
    TEST_NUM_NORMALIZE(5,
        (2, 0, 0),
        (0)
    )
    TEST_NUM_NORMALIZE(6,
        (3, 0, 0, 1),
        (1, 1)
    )

    #undef TEST_NUM_NORMALIZE

    TEST_FN_CLOSE
}

static void test_num_break(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_BREAK(TAG, NUM, COUNT, NUM_HI, NUM_LO)             \
    {                                                                   \
        TEST_CASE_OPEN(TAG)                                             \
        {                                                               \
            num_p num_hi, num_lo, num = num_create_immed(ARG_OPEN NUM); \
            num_break(&num_hi, &num_lo, num, COUNT);                    \
            assert(num_immed(num_hi, ARG_OPEN NUM_HI));                 \
            assert(num_immed(num_lo, ARG_OPEN NUM_LO));                 \
        }                                                               \
        TEST_CASE_CLOSE                                                 \
    }

    TEST_NUM_BREAK(1,
        (0),
        0,
        (0),
        (0)
    )
    TEST_NUM_BREAK(2,
        (0),
        1,
        (0),
        (0)
    )
    TEST_NUM_BREAK(3,
        (1, 1),
        0,
        (1, 1),
        (0)
    )
    TEST_NUM_BREAK(4,
        (1, 1),
        1,
        (0),
        (1, 1)
    )
    TEST_NUM_BREAK(5,
        (1, 1),
        2,
        (0),
        (1, 1)
    )
    TEST_NUM_BREAK(6,
        (2, 1, 2),
        0,
        (2, 1, 2),
        (0)
    )
    TEST_NUM_BREAK(7,
        (2, 1, 2),
        1,
        (1, 1),
        (1, 2)
    )
    TEST_NUM_BREAK(8,
        (2, 1, 2),
        2,
        (0),
        (2, 1, 2)
    )
    TEST_NUM_BREAK(9,
        (2, 1, 2),
        3,
        (0),
        (2, 1, 2)
    )
    TEST_NUM_BREAK(10,
        (3, 1, 0, 2),
        2,
        (1, 1),
        (1, 2)
    )
    TEST_NUM_BREAK(11,
        (3, 1, 0, 0),
        2,
        (1, 1),
        (0)
    )

    #undef TEST_NUM_BREAK

    TEST_FN_CLOSE
}



static void test_num_wrap(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_WRAP(TAG, NUM, ...)            \
    {                                               \
        TEST_CASE_OPEN(TAG)                         \
        {                                           \
            num_p num = num_wrap(NUM);              \
            assert(num_immed(num, __VA_ARGS__));    \
        }                                           \
        TEST_CASE_CLOSE                             \
    }

    TEST_NUM_WRAP(1, 0, 0)
    TEST_NUM_WRAP(2, 2, 1, 2)
    TEST_NUM_WRAP(3, UINT64_MAX, 1, UINT64_MAX)

    #undef TEST_NUM_WRAP

    TEST_FN_CLOSE
}

static void test_num_wrap_dec(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_WRAP_DEC(TAG, STR, ...)        \
    {                                               \
        TEST_CASE_OPEN(TAG)                         \
        {                                           \
            num_p num = num_wrap_dec(STR);          \
            assert(num_immed(num, __VA_ARGS__));    \
        }                                           \
        TEST_CASE_CLOSE                             \
    }

    TEST_NUM_WRAP_DEC(1, "0", 0)
    TEST_NUM_WRAP_DEC(2, "1", 1, 1)
    TEST_NUM_WRAP_DEC(3, "9", 1, 9)
    TEST_NUM_WRAP_DEC(4, "10", 1, 10)
    TEST_NUM_WRAP_DEC(5, "18446744073709551615", 1, UINT64_MAX)
    TEST_NUM_WRAP_DEC(6, "18446744073709551616", 2, 1, 0)
    TEST_NUM_WRAP_DEC(7, "0000", 0)
    TEST_NUM_WRAP_DEC(8, "00001", 1, 1)
    TEST_NUM_WRAP_DEC(9, "", 0)

    #undef TEST_NUM_WRAP_DEC

    TEST_FN_CLOSE
}

static void test_num_wrap_hex(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_WRAP_HEX(TAG, STR, ...)        \
    {                                               \
        TEST_CASE_OPEN(TAG)                         \
        {                                           \
            num_p num = num_wrap_hex(STR);          \
            assert(num_immed(num, __VA_ARGS__));    \
        }                                           \
        TEST_CASE_CLOSE                             \
    }

    TEST_NUM_WRAP_HEX( 1, "0x0", 0)
    TEST_NUM_WRAP_HEX( 2, "0x1", 1, 1)
    TEST_NUM_WRAP_HEX( 3, "0x9", 1, 9)
    TEST_NUM_WRAP_HEX( 4, "0xa", 1, 10)
    TEST_NUM_WRAP_HEX( 5, "0xA", 1, 10)
    TEST_NUM_WRAP_HEX( 6, "0x10", 1, 16)
    TEST_NUM_WRAP_HEX( 7, "0xffffffffffffffff", 1, UINT64_MAX)
    TEST_NUM_WRAP_HEX( 8, "0x10000000000000000", 2, 1, 0)
    TEST_NUM_WRAP_HEX( 9, "0x0000", 0)
    TEST_NUM_WRAP_HEX(10, "0x00001", 1, 1)
    TEST_NUM_WRAP_HEX(11, "0x", 0)

    #undef TEST_NUM_WRAP_HEX

    #define TEST_NUM_WRAP_HEX(TAG, STR) \
    {                                   \
        TEST_CASE_OPEN(TAG)             \
        {                               \
            TEST_REVERT_OPEN            \
            {                           \
                num_wrap_hex(STR);      \
            }                           \
            TEST_REVERT_CLOSE           \
        }                               \
        TEST_CASE_CLOSE                 \
    }

    TEST_NUM_WRAP_HEX(12, "")
    TEST_NUM_WRAP_HEX(13, "0")
    TEST_NUM_WRAP_HEX(14, "ab")
    TEST_NUM_WRAP_HEX(14, "0b")

    #undef TEST_NUM_WRAP_HEX

    TEST_FN_CLOSE
}

static void test_num_wrap_str(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_WRAP_STR(TAG, STR, ...)        \
    {                                               \
        TEST_CASE_OPEN(TAG)                         \
        {                                           \
            num_p num = num_wrap_str(STR);          \
            assert(num_immed(num, __VA_ARGS__));    \
        }                                           \
        TEST_CASE_CLOSE                             \
    }

    TEST_NUM_WRAP_STR( 1, "0", 0)
    TEST_NUM_WRAP_STR( 2, "1", 1, 1)
    TEST_NUM_WRAP_STR( 3, "9", 1, 9)
    TEST_NUM_WRAP_STR( 4, "10", 1, 10)
    TEST_NUM_WRAP_STR( 5, "18446744073709551615", 1, UINT64_MAX)
    TEST_NUM_WRAP_STR( 6, "18446744073709551616", 2, 1, 0)
    TEST_NUM_WRAP_STR( 7, "0000", 0)
    TEST_NUM_WRAP_STR( 8, "00001", 1, 1)
    TEST_NUM_WRAP_STR( 9, "0x0", 0)
    TEST_NUM_WRAP_STR(10, "0x1", 1, 1)
    TEST_NUM_WRAP_STR(11, "0x9", 1, 9)
    TEST_NUM_WRAP_STR(12, "0xa", 1, 10)
    TEST_NUM_WRAP_STR(13, "0xA", 1, 10)
    TEST_NUM_WRAP_STR(14, "0x10", 1, 16)
    TEST_NUM_WRAP_STR(15, "0xffffffffffffffff", 1, UINT64_MAX)
    TEST_NUM_WRAP_STR(16, "0x10000000000000000", 2, 1, 0)
    TEST_NUM_WRAP_STR(17, "0x0000", 0)
    TEST_NUM_WRAP_STR(18, "0x00001", 1, 1)

    #undef TEST_NUM_WRAP_STR

    TEST_FN_CLOSE
}

static void test_num_read_dec(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_READ_DEC(TAG, ...)                             \
    {                                                               \
        TEST_CASE_OPEN(TAG)                                         \
        {                                                           \
            num_p num = num_read_dec("numbers/num_" #TAG ".txt");   \
            assert(num_immed(num, __VA_ARGS__));                    \
        }                                                           \
        TEST_CASE_CLOSE                                             \
    }

    TEST_NUM_READ_DEC(1, 0)
    TEST_NUM_READ_DEC(2, 1, 1)
    TEST_NUM_READ_DEC(3, 1, 10)
    TEST_NUM_READ_DEC(4, 1, 12345678901234567)
    TEST_NUM_READ_DEC(5, 1, 123456789012345678)
    TEST_NUM_READ_DEC(6, 1, 1234567890123456789)
    TEST_NUM_READ_DEC(7, 2, 6, 0xb14e9f812f366c35)

    #undef TEST_NUM_READ_DEC

    TEST_CASE_OPEN(8)
    {
        TEST_REVERT_OPEN
        {
            num_read_dec("numbers/num_8.txt");
        }
        TEST_REVERT_CLOSE
    }
    TEST_CASE_CLOSE

    TEST_FN_CLOSE
}

static void test_num_unwrap(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_UNWRAP(TAG, RES, NUM)              \
    {                                                   \
        TEST_CASE_OPEN(TAG)                             \
        {                                               \
            num_p num = num_create_immed(ARG_OPEN NUM); \
            uint64_t res = num_unwrap(num);             \
            assert(res == (RES));                       \
        }                                               \
        TEST_CASE_CLOSE                                 \
    }

    TEST_NUM_UNWRAP(1, 0, (0))
    TEST_NUM_UNWRAP(2, 1, (1, 1))
    TEST_NUM_UNWRAP(3, UINT64_MAX, (1, UINT64_MAX))

    #undef TEST_NUM_UNWRAP

    TEST_CASE_OPEN(4)
    {
        num_p num = num_create_immed(2, 3, 4);
        TEST_REVERT_OPEN
        {
            num_unwrap(num);
        }
        TEST_REVERT_CLOSE
    }
    TEST_CASE_CLOSE

    TEST_FN_CLOSE
}

static void test_num_copy(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_COPY(TAG, NUM)                     \
    {                                                   \
        TEST_CASE_OPEN(TAG)                             \
        {                                               \
            num_p num = num_create_immed(ARG_OPEN NUM); \
            num_p num_res = num_copy(num);              \
            assert(num_eq_dbg(num_res, num));           \
        }                                               \
        TEST_CASE_CLOSE                                 \
    }

    TEST_NUM_COPY(1, (0))
    TEST_NUM_COPY(2, (1, 1))
    TEST_NUM_COPY(3, (2, 1, 2))

    #undef TEST_NUM_COPY

    TEST_FN_CLOSE
}



static void test_num_cmp_offset(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_CMP(TAG, NUM_1, POS, RELATION, NUM_2)      \
    {                                                           \
        TEST_CASE_OPEN(TAG)                                     \
        {                                                       \
            num_p num_1 = num_create_immed(ARG_OPEN NUM_1);     \
            num_p num_2 = num_create_immed(ARG_OPEN NUM_2);     \
            int64_t cmp = num_cmp_offset(num_1, POS, num_2);    \
            assert(cmp RELATION 0);                             \
            num_free(num_1);                                    \
            num_free(num_2);                                    \
        }                                                       \
        TEST_CASE_CLOSE                                         \
    }

    TEST_NUM_CMP(1, (0), 0, ==, (0))
    TEST_NUM_CMP(2, (1, 1), 0, >, (0))
    TEST_NUM_CMP(3, (0), 0, <, (1, 1))
    TEST_NUM_CMP(4, (1, 1), 0, <, (1, 2))
    TEST_NUM_CMP(5, (1, 2), 0, ==, (1, 2))
    TEST_NUM_CMP(6, (1, 3), 0, >, (1, 2))
    TEST_NUM_CMP(7, (2, 2, 0), 0, >, (2, 1, 0))
    TEST_NUM_CMP(8, (2, 2, 0), 0, ==, (2, 2, 0))
    TEST_NUM_CMP(9, (2, 2, 0), 0, <, (2, 3, 0))
    TEST_NUM_CMP(10,
        (2, 0x8000000000000000, 0), 0,
        <,
        (2, 0x8000000000000000, UINT64_MAX >> 1)
    )
    TEST_NUM_CMP(11,
        (3, 0x8000000000000000, 0, 0), 0,
        <,
        (3, 0x8000000000000000, UINT64_MAX >> 1, 0x8000000000000000)
    )

    #undef TEST_NUM_CMP

    TEST_FN_CLOSE
}

static void test_num_add_uint_offset(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_ADD_UINT_OFFSET(TAG, NUM, OFFSET, VALUE, RES)  \
    {                                                               \
        TEST_CASE_OPEN(TAG)                                         \
        {                                                           \
            num_p num = num_create_immed(ARG_OPEN NUM);             \
            num = num_expand_to(num, num->count + 1);               \
            num_add_uint_offset(num, OFFSET, VALUE);                \
            assert(num_immed(num, ARG_OPEN RES));                   \
        }                                                           \
        TEST_CASE_CLOSE                                             \
    }

    TEST_NUM_ADD_UINT_OFFSET(1, (0), 0, 0, (0))
    TEST_NUM_ADD_UINT_OFFSET(2, (0), 0, 1, (1, 1))
    TEST_NUM_ADD_UINT_OFFSET(3, (1, 1), 0, 2, (1, 3))
    TEST_NUM_ADD_UINT_OFFSET(4, (1, UINT64_MAX), 0, 3, (2, 1, 2))
    TEST_NUM_ADD_UINT_OFFSET(5, (1, 1), 1, 2, (2, 2, 1))
    TEST_NUM_ADD_UINT_OFFSET(6, (1, UINT64_MAX), 1, 3, (2, 3, UINT64_MAX))

    #undef TEST_NUM_ADD_UINT_OFFSET

    #define TEST_NUM_ADD_UINT_OFFSET(TAG, NUM, OFFSET, VALUE)   \
    {                                                           \
        TEST_CASE_OPEN(TAG)                                     \
        {                                                       \
            num_p num = num_create_immed(ARG_OPEN NUM);         \
            TEST_REVERT_OPEN                                    \
            {                                                   \
                num_add_uint_offset(num, OFFSET, VALUE);        \
            }                                                   \
            TEST_REVERT_CLOSE                                   \
        }                                                       \
        TEST_CASE_CLOSE                                         \
    }

    TEST_NUM_ADD_UINT_OFFSET(7, (1, UINT64_MAX), 0, 3)
    TEST_NUM_ADD_UINT_OFFSET(8, (0), 1, 0)
    TEST_NUM_ADD_UINT_OFFSET(9, (0), 1, 1)

    #undef TEST_NUM_ADD_UINT_OFFSET

    TEST_FN_CLOSE
}

static void test_num_sub_uint_offset(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_SUB_UINT_OFFSET(TAG, NUM, POS, VALUE, RES) \
    {                                                           \
        TEST_CASE_OPEN(TAG)                                     \
        {                                                       \
            num_p num = num_create_immed(ARG_OPEN NUM);         \
            num_sub_uint_offset(num, POS, VALUE);               \
            assert(num_immed(num, ARG_OPEN RES));               \
        }                                                       \
        TEST_CASE_CLOSE                                         \
    }

    TEST_NUM_SUB_UINT_OFFSET(1, (0), 0, 0, (0))
    TEST_NUM_SUB_UINT_OFFSET(2, (1, 1), 0, 0, (1, 1))
    TEST_NUM_SUB_UINT_OFFSET(3, (1, 1), 0, 1, (0))
    TEST_NUM_SUB_UINT_OFFSET(4, (1, 2), 0, 1, (1, 1))
    TEST_NUM_SUB_UINT_OFFSET(5, (2, 1, 0), 0, 1, (1, UINT64_MAX))
    TEST_NUM_SUB_UINT_OFFSET(6,
        (5, 1, 0, 0, 0, 0), 0,
        0x8000000000000000,
        (4, UINT64_MAX, UINT64_MAX, UINT64_MAX, 0x8000000000000000)
    )
    TEST_NUM_SUB_UINT_OFFSET(7, (2, 2, 3), 0, 1, (2, 2, 2))
    TEST_NUM_SUB_UINT_OFFSET(8, (2, 2, 3), 1, 1, (2, 1, 3))
    TEST_NUM_SUB_UINT_OFFSET(9, (2, 1, 3), 1, 1, (1, 3))
    TEST_NUM_SUB_UINT_OFFSET(10, (2, 1, 0), 1, 1, (0))
    TEST_NUM_SUB_UINT_OFFSET(11, (3, 1, 0, 1), 2, 1, (1, 1))
    TEST_NUM_SUB_UINT_OFFSET(12, (3, 1, 0, 1), 1, 1, (2, UINT64_MAX, 1))
    TEST_NUM_SUB_UINT_OFFSET(13, (2, 1, 0), 1, 1, (0))
    TEST_NUM_SUB_UINT_OFFSET(14, (0), 0, 0, (0))

    #undef TEST_NUM_SUB_UINT_OFFSET

    #define TEST_NUM_SUB_UINT_OFFSET(TAG, NUM, POS, VALUE)  \
    {                                                       \
        TEST_CASE_OPEN(TAG)                                 \
        {                                                   \
            num_p num = num_create_immed(ARG_OPEN NUM);     \
            TEST_REVERT_OPEN                                \
            {                                               \
                num_sub_uint_offset(num, POS, VALUE);       \
            }                                               \
            TEST_REVERT_CLOSE                               \
        }                                                   \
        TEST_CASE_CLOSE                                     \
    }

    TEST_NUM_SUB_UINT_OFFSET(15, (0), 0, 1)
    TEST_NUM_SUB_UINT_OFFSET(16, (1, 2), 0, 3)
    TEST_NUM_SUB_UINT_OFFSET(17, (1, 2), 1, 1)

    #undef TEST_NUM_SUB_UINT_OFFSET

    TEST_FN_CLOSE
}

static void test_num_sub_offset(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_SUB_OFFSET(TAG, NUM_1, POS, NUM_2, RES)    \
    {                                                           \
        TEST_CASE_OPEN(TAG)                                     \
        {                                                       \
            num_p num_1 = num_create_immed(ARG_OPEN NUM_1);     \
            num_p num_2 = num_create_immed(ARG_OPEN NUM_2);     \
            num_sub_offset(num_1, POS, num_2);                  \
            assert(num_immed(num_1, ARG_OPEN RES));             \
            num_free(num_2);                                    \
        }                                                       \
        TEST_CASE_CLOSE                                         \
    }

    TEST_NUM_SUB_OFFSET(1, (0), 0, (0), (0))
    TEST_NUM_SUB_OFFSET(2, (0), 0, (0), (0))
    TEST_NUM_SUB_OFFSET(3, (2, 1, 2), 1, (1, 1), (1, 2))
    TEST_NUM_SUB_OFFSET(4, (2, 1, 0), 1, (1, 1), (0))
    TEST_NUM_SUB_OFFSET(5, (3, 1, 2, 3), 1, (2, 1, 2), (1, 3))
    TEST_NUM_SUB_OFFSET(6, (3, 1, 2, 3), 1, (2, 1, 1), (2, 1, 3))

    #undef TEST_NUM_SUB_OFFSET

    #define TEST_NUM_SUB_OFFSET(TAG, NUM_1, POS, NUM_2)     \
    {                                                       \
        TEST_CASE_OPEN(TAG)                                 \
        {                                                   \
            num_p num_1 = num_create_immed(ARG_OPEN NUM_1); \
            num_p num_2 = num_create_immed(ARG_OPEN NUM_2); \
            TEST_REVERT_OPEN                                \
            {                                               \
                num_sub_offset(num_1, POS, num_2);          \
            }                                               \
            TEST_REVERT_CLOSE                               \
            num_free(num_1);                                \
            num_free(num_2);                                \
        }                                                   \
        TEST_CASE_CLOSE                                     \
    }

    TEST_NUM_SUB_OFFSET(7, (0), 0, (1, 1))
    TEST_NUM_SUB_OFFSET(8, (2, 2, 1), 1, (1, 3))
    TEST_NUM_SUB_OFFSET(9, (2, 2, 1), 1, (2, 1, 2))

    #undef TEST_NUM_SUB_OFFSET

    TEST_FN_CLOSE
}



static void test_num_shl_core(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_SHL_CORE(TAG, NUM_BEF, BITS, NUM_AFT)  \
    {                                                       \
        TEST_CASE_OPEN(TAG)                                 \
        {                                                   \
            num_p num = num_create_immed(ARG_OPEN NUM_BEF); \
            num = num_expand_to(num, num->count + 1);       \
            num_shl_core(num, BITS);                        \
            assert(num_immed(num, ARG_OPEN NUM_AFT));       \
        }                                                   \
        TEST_CASE_CLOSE                                     \
    }

    TEST_NUM_SHL_CORE(1, (0), 0, (0))
    TEST_NUM_SHL_CORE(2, (0), 63, (0))
    TEST_NUM_SHL_CORE(3, (1, 1), 1, (1, 2))
    TEST_NUM_SHL_CORE(4, (1, 1), 1, (1, 2))
    TEST_NUM_SHL_CORE(5, (1, 1), 63, (1, 0x8000000000000000))
    TEST_NUM_SHL_CORE(6, (1, 2), 63, (2, 1, 0))
    TEST_NUM_SHL_CORE(7, (2, 1, 2), 63, (2, 0x8000000000000001, 0))

    #undef TEST_NUM_SHL_CORE

    #define TEST_NUM_SHL_CORE(TAG, NUM_BEF, BITS)           \
    {                                                       \
        TEST_CASE_OPEN(TAG)                                 \
        {                                                   \
            num_p num = num_create_immed(ARG_OPEN NUM_BEF); \
            TEST_REVERT_OPEN                                \
            {                                               \
                num_shl_core(num, BITS);              \
            }                                               \
            TEST_REVERT_CLOSE                               \
        }                                                   \
        TEST_CASE_CLOSE                                     \
    }

    TEST_NUM_SHL_CORE(8, (1, 1), 64)

    #undef TEST_NUM_SHL_CORE

    TEST_FN_CLOSE
}

static void test_num_shr_core(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_SHR_CORE(TAG, NUM_BEF, BITS, NUM_AFT)  \
    {                                                       \
        TEST_CASE_OPEN(TAG)                                 \
        {                                                   \
            num_p num = num_create_immed(ARG_OPEN NUM_BEF); \
            num_shr_core(num, BITS);                  \
            assert(num_immed(num, ARG_OPEN NUM_AFT));       \
        }                                                   \
        TEST_CASE_CLOSE                                     \
    }

    TEST_NUM_SHR_CORE(1, (0), 1, (0))
    TEST_NUM_SHR_CORE(2, (1, 1), 0, (1, 1))
    TEST_NUM_SHR_CORE(3, (1, 1), 1, (0))
    TEST_NUM_SHR_CORE(4, (1, 2), 1, (1, 1))
    TEST_NUM_SHR_CORE(5, (1, UINT64_MAX), 63, (1, 1))
    TEST_NUM_SHR_CORE(6, (2, 1, 0x8000000000000000), 63, (1, 3))
    TEST_NUM_SHR_CORE(7,
        (4, 4, UINT64_MAX, UINT64_MAX, UINT64_MAX - 3), 0,
        (4, 4, UINT64_MAX, UINT64_MAX, UINT64_MAX - 3)
    )

    #undef TEST_NUM_SHR_CORE

    TEST_CASE_OPEN(8)
    {
        num_p num = num_create_immed(1, 1);
        TEST_REVERT_OPEN
        {
            num_shr_core(num, 64);
        }
        TEST_REVERT_CLOSE
        num_free(num);
    }
    TEST_CASE_CLOSE

    TEST_FN_CLOSE
}

static void test_num_mul_uint(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_MUL_UINT(TAG, NUM_BEF, VALUE, NUM_AFT) \
    {                                                       \
        TEST_CASE_OPEN(TAG)                                 \
        {                                                   \
            num_p num = num_create_immed(ARG_OPEN NUM_BEF); \
            num = num_mul_uint(num, VALUE);                 \
            assert(num_immed(num, ARG_OPEN NUM_AFT));       \
        }                                                   \
        TEST_CASE_CLOSE                                     \
    }

    TEST_NUM_MUL_UINT(1, (0), 0, (0))
    TEST_NUM_MUL_UINT(2, (0), 1, (0))
    TEST_NUM_MUL_UINT(3, (1, 2), 0, (0))
    TEST_NUM_MUL_UINT(4, (1, 2), 3, (1, 6))
    TEST_NUM_MUL_UINT(5, (1, 2), UINT64_MAX, (2, 1, UINT64_MAX - 1))
    TEST_NUM_MUL_UINT(6, (2, 2, 3), 4, (2, 8, 12))
    TEST_NUM_MUL_UINT(7,
        (2, UINT64_MAX, UINT64_MAX), UINT64_MAX,
        (3, UINT64_MAX - 1, UINT64_MAX, 1)
    )
    TEST_NUM_MUL_UINT(8, (2, 2, 0), 3, (2, 6, 0))
    TEST_NUM_MUL_UINT(9,
        (2, 6, 0xea7db545decb57a4), 123427205323,
        (2, 0xc6bfba060d, 0xdf4c110fd2f33f0c)
    )

    #undef TEST_NUM_MUL_UINT

    TEST_FN_CLOSE
}



static void test_smm_bit_inv(bool show)
{
    TEST_FN_OPEN

    #define TEST_SMM_BIT_INV(TAG, I, K, RES)    \
    {                                           \
        TEST_CASE_OPEN(TAG)                     \
        {                                       \
            uint64_t res = ssm_bit_inv(I, K);   \
            assert(uint64(res, RES))            \
        }                                       \
        TEST_CASE_CLOSE                         \
    }

    TEST_SMM_BIT_INV(1, 0, 2, 0)
    TEST_SMM_BIT_INV(2, 1, 2, 1)
    TEST_SMM_BIT_INV(3, 1, 4, 2)
    TEST_SMM_BIT_INV(4, 2, 4, 1)
    TEST_SMM_BIT_INV(5, 3, 4, 3)
    TEST_SMM_BIT_INV(6, 5, 16, 10)

    TEST_FN_CLOSE
}

static void test_num_ssm_add_mod(bool show)
{
    TEST_FN_OPEN

    #define TEST_SSM_ADD_MOD(TAG, NUM, N, RES)          \
    {                                                   \
        TEST_CASE_OPEN(TAG)                             \
        {                                               \
            num_p num = num_create_immed(ARG_OPEN NUM); \
            num_ssm_add_mod_immed(num, 0, num, N, N);   \
            assert(num_immed(num, ARG_OPEN RES));       \
        }                                               \
        TEST_CASE_CLOSE                                 \
    }

    TEST_SSM_ADD_MOD(1,
        (6, 0, 0, 1, 0, 0, 2), 3,
        (6, 0, 0, 1, 0, 0, 3)
    )
    TEST_SSM_ADD_MOD(2,
        (6, 0, B(63), 0, 0, B(63), 0), 3,
        (6, 0, B(63), 0, 1, 0, 0)
    )
    TEST_SSM_ADD_MOD(3,
        (6, 1, 0, 0, 0, 0, 1), 3,
        (6, 1, 0, 0, 0, 0, 0)
    )
    TEST_SSM_ADD_MOD(4,
        (6, 1, 0, 0, 1, 0, 0), 3,
        (6, 1, 0, 0, 0, UINT64_MAX, UINT64_MAX)
    )
    TEST_SSM_ADD_MOD(5,
        (6, 0, UINT64_MAX, UINT64_MAX, 1, 0, 0), 3,
        (6, 0, UINT64_MAX, UINT64_MAX, 0, UINT64_MAX, UINT64_MAX - 1)
    )
    TEST_SSM_ADD_MOD(6,
        (6, 0, UINT64_MAX, UINT64_MAX, 0, UINT64_MAX, UINT64_MAX), 3,
        (6, 0, UINT64_MAX, UINT64_MAX, 0, UINT64_MAX, UINT64_MAX - 2)
    )

    #undef TEST_SSM_ADD_MOD

    TEST_FN_CLOSE
}

static void test_num_ssm_sub_mod(bool show)
{
    TEST_FN_OPEN

    #define TEST_SSM_SUB_MOD(TAG, NUM, POS_1, POS_2, N, RES)        \
    {                                                               \
        TEST_CASE_OPEN(TAG)                                         \
        {                                                           \
            num_p num = num_create_immed(ARG_OPEN NUM);             \
            num_p num_res = num_create_rand(N);                     \
            num_ssm_sub_mod(num_res, 0, num, POS_1, num, POS_2, N); \
            assert(num_immed(num_res, ARG_OPEN RES));               \
            assert(num_immed(num, ARG_OPEN NUM));                   \
        }                                                           \
        TEST_CASE_CLOSE                                             \
    }

    TEST_SSM_SUB_MOD(1,
        (6, 0, 0, 1, 0, 0, 2), 0, 3, 3,
        (3, 0, 0, 1)
    )
    TEST_SSM_SUB_MOD(2,
        (6, 0, 0, 2, 0, 0, 2), 0, 3, 3,
        (3, 0, 0, 0)
    )
    TEST_SSM_SUB_MOD(3,
        (6, 0, 0, 3, 0, 0, 2), 0, 3, 3,
        (3, 1, 0, 0)
    )
    TEST_SSM_SUB_MOD(4,
        (6, 0, 0, 1, 1, 0, 0), 0, 3, 3,
        (3, 0, UINT64_MAX, UINT64_MAX)
    )
    TEST_SSM_SUB_MOD(5,
        (6, 1, 0, 0, 0, 0, 1), 0, 3, 3,
        (3, 0, 0, 2)
    )
    TEST_SSM_SUB_MOD(6,
        (6, 0, 0, 1, 0, 0, 0), 0, 3, 3,
        (3, 1, 0, 0)
    )
    TEST_SSM_SUB_MOD(7,
        (6, 1, 0, 0, 0, UINT64_MAX, UINT64_MAX), 0, 3, 3,
        (3, 1, 0, 0)
    )

    #undef TEST_SSM_SUB_MOD

    TEST_FN_CLOSE
}

static void test_num_ssm_opposite(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_SSM_OPPOSITE(TAG, NUM, N, RES)     \
    {                                                   \
        TEST_CASE_OPEN(TAG)                             \
        {                                               \
            num_p num = num_create_immed(ARG_OPEN NUM); \
            num_ssm_opposite(num, 0, N);                \
            assert(num_immed(num, ARG_OPEN RES));       \
        }                                               \
        TEST_CASE_CLOSE                                 \
    }

    TEST_NUM_SSM_OPPOSITE(1,
        (2, 0, 1), 2,
        (2, 1, 0)
    )
    TEST_NUM_SSM_OPPOSITE(2,
        (2, 1, 0), 2,
        (2, 0, 1)
    )
    TEST_NUM_SSM_OPPOSITE(3,
        (2, 0, 2), 2,
        (2, 0, UINT64_MAX)
    )
    TEST_NUM_SSM_OPPOSITE(4,
        (2, 0, UINT64_MAX), 2,
        (2, 0, 2)
    )
    TEST_NUM_SSM_OPPOSITE(5,
        (3, 0, 0, 1), 3,
        (3, 1, 0, 0)
    )
    TEST_NUM_SSM_OPPOSITE(6,
        (3, 1, 0, 0), 3,
        (3, 0, 0, 1)
    )
    TEST_NUM_SSM_OPPOSITE(7,
        (3, 0, 0, 2), 3,
        (3, 0, UINT64_MAX, UINT64_MAX)
    )
    TEST_NUM_SSM_OPPOSITE(8,
        (3, 0, UINT64_MAX, UINT64_MAX), 3,
        (3, 0, 0, 2)
    )

    TEST_FN_CLOSE
}

static void test_num_ssm_pad_no_wrap(bool show)
{
    TEST_FN_OPEN

    #define TEST_SSM_PAD(TAG, NUM, Mv, Nv, Kv, RES)         \
    {                                                       \
        TEST_CASE_OPEN(TAG)                                 \
        {                                                   \
            num_p num = num_create_immed(ARG_OPEN NUM);     \
            ssm_params_t p = (ssm_params_t)                 \
            {                                               \
                .M = (Mv),                                  \
                .K = (Kv),                                  \
                .n = (Nv)                                   \
            };                                              \
            num_p num_res = num_ssm_pad_no_wrap(num, &p);   \
            num_res->count = (Nv) * (Kv);                   \
            assert(num_immed(num_res, ARG_OPEN RES))        \
            assert(num_immed(num, ARG_OPEN NUM))            \
        }                                                   \
        TEST_CASE_CLOSE                                     \
    }

    TEST_SSM_PAD(1,
        (1, 1), 1, 2, 1,
        (2, 0, 1)
    )
    TEST_SSM_PAD(2,
        (1, 1), 1, 2, 2,
        (4, 0, 0, 0, 1)
    )
    TEST_SSM_PAD(3,
        (2, 2, 1), 1, 2, 4,
        (8, 0, 0, 0, 0, 0, 2, 0, 1)
    )
    TEST_SSM_PAD(4,
        (4, 4, 3, 2, 1), 2, 4, 4,
        (16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 3, 0, 0, 2, 1)
    )
    TEST_SSM_PAD(5,
        (2, 1, 0), 1, 2, 1,
        (2, 1, 0)
    )
    TEST_SSM_PAD(6,
        (3, 1, 0, 0), 1, 2, 2,
        (4, 1, 0, 0, 0)
    )
    TEST_SSM_PAD(7,
        (3, 0, UINT64_MAX, UINT64_MAX), 1, 2, 2,
        (4, 0, UINT64_MAX, 0, UINT64_MAX)
    )
    TEST_SSM_PAD(8,
        (9, 1, 0, 0, 0, 0, 0, 0, 0, 0),
        1, 4, 8,
        (32,
            0, 0, 1, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0
        )
    )

    #undef TEST_SSM_PAD

    TEST_FN_CLOSE
}

static void test_num_ssm_shl(bool show)
{
    TEST_FN_OPEN

    #define TEST_SSM_SHL(TAG, NUM, POS, N, BITS, RES)   \
    {                                                   \
        TEST_CASE_OPEN(TAG)                             \
        {                                               \
            num_p num = num_create_immed(ARG_OPEN NUM); \
            num_p num_res = num_create_rand(N);         \
            num_ssm_shl(num_res, 0, num, POS, N, BITS); \
            assert(num_immed(num_res, ARG_OPEN RES));   \
            assert(num_immed(num, ARG_OPEN NUM));       \
        }                                               \
        TEST_CASE_CLOSE                                 \
    }

    TEST_SSM_SHL(1,
        (3, 0, 0, 1), 0, 3, 0,
        (3, 0, 0, 1)
    )
    TEST_SSM_SHL(2,
        (3, 0, 0, 1), 0, 3, 1,
        (3, 0, 0, 2)
    )
    TEST_SSM_SHL(3,
        (3, 0, 0, 1), 0, 3, 64,
        (3, 0, 1, 0)
    )
    TEST_SSM_SHL(4,
        (3, 0, 0, 1), 0, 3, 65,
        (3, 0, 2, 0)
    )
    TEST_SSM_SHL(5,
        (3, UINT64_MAX, UINT64_MAX, UINT64_MAX), 0, 3, 0,
        (3, UINT64_MAX, UINT64_MAX, UINT64_MAX)
    )
    TEST_SSM_SHL(6,
        (3, UINT64_MAX, UINT64_MAX, UINT64_MAX), 0, 3, 1,
        (3, UINT64_MAX, UINT64_MAX, UINT64_MAX << 1)
    )
    TEST_SSM_SHL(7,
        (3, UINT64_MAX, UINT64_MAX, UINT64_MAX), 0, 3, 64,
        (3, UINT64_MAX, UINT64_MAX, 0)
    )
    TEST_SSM_SHL(8,
        (3, UINT64_MAX, UINT64_MAX, UINT64_MAX), 0, 3, 65,
        (3, UINT64_MAX, UINT64_MAX << 1, 0)
    )
    TEST_SSM_SHL(9,
        (3, 0x3333333333333333, 0x2222222222222222, 0x1111111111111111), 0, 3, 0,
        (3, 0x3333333333333333, 0x2222222222222222, 0x1111111111111111)
    )
    TEST_SSM_SHL(10,
        (3, 0x3333333333333333, 0x2222222222222222, 0x1111111111111111), 0, 3, 4,
        (3, 0x3333333333333332, 0x2222222222222221, 0x1111111111111110)
    )
    TEST_SSM_SHL(11,
        (3, 0x3333333333333333, 0x2222222222222222, 0x1111111111111111), 0, 3, 64,
        (3, 0x2222222222222222, 0x1111111111111111, 0)
    )
    TEST_SSM_SHL(12,
        (3, 0x3333333333333333, 0x2222222222222222, 0x1111111111111111), 0, 3, 68,
        (3, 0x2222222222222221, 0x1111111111111110, 0)
    )

    #undef TEST_SSM_SHL

    TEST_FN_CLOSE
}

static void test_num_ssm_shr(bool show)
{
    TEST_FN_OPEN

    #define TEST_SSM_SHR(TAG, NUM, POS, N, BITS, RES)   \
    {                                                   \
        TEST_CASE_OPEN(TAG)                             \
        {                                               \
            num_p num = num_create_immed(ARG_OPEN NUM); \
            num_p num_res = num_create_rand(N);         \
            num_ssm_shr(num_res, 0, num, POS, N, BITS); \
            assert(num_immed(num_res, ARG_OPEN RES));   \
            assert(num_immed(num, ARG_OPEN NUM));       \
        }                                               \
        TEST_CASE_CLOSE                                 \
    }

    TEST_SSM_SHR(1,
        (3, 2, 0, 0), 0, 3, 0,
        (3, 2, 0, 0)
    )
    TEST_SSM_SHR(2,
        (3, 2, 0, 0), 0, 3, 1,
        (3, 1, 0, 0)
    )
    TEST_SSM_SHR(3,
        (3, 2, 0, 0), 0, 3, 64,
        (3, 0, 2, 0)
    )
    TEST_SSM_SHR(4,
        (3, 2, 0, 0), 0, 3, 65,
        (3, 0, 1, 0)
    )
    TEST_SSM_SHR(5,
        (3, UINT64_MAX, UINT64_MAX, UINT64_MAX), 0, 3, 0,
        (3, UINT64_MAX, UINT64_MAX, UINT64_MAX)
    )
    TEST_SSM_SHR(6,
        (3, UINT64_MAX, UINT64_MAX, UINT64_MAX), 0, 3, 1,
        (3, UINT64_MAX >> 1, UINT64_MAX, UINT64_MAX)
    )
    TEST_SSM_SHR(7,
        (3, UINT64_MAX, UINT64_MAX, UINT64_MAX), 0, 3, 64,
        (3, 0, UINT64_MAX, UINT64_MAX)
    )
    TEST_SSM_SHR(8,
        (3, UINT64_MAX, UINT64_MAX, UINT64_MAX), 0, 3, 65,
        (3, 0, UINT64_MAX >> 1, UINT64_MAX)
    )
    TEST_SSM_SHR(9,
        (3, 0x3333333333333333, 0x2222222222222222, 0x1111111111111111), 0, 3, 0,
        (3, 0x3333333333333333, 0x2222222222222222, 0x1111111111111111)
    )
    TEST_SSM_SHR(10,
        (3, 0x3333333333333333, 0x2222222222222222, 0x1111111111111111), 0, 3, 4,
        (3, 0x0333333333333333, 0x3222222222222222, 0x2111111111111111)
    )
    TEST_SSM_SHR(11,
        (3, 0x3333333333333333, 0x2222222222222222, 0x1111111111111111), 0, 3, 64,
        (3, 0, 0x3333333333333333, 0x2222222222222222)
    )
    TEST_SSM_SHR(12,
        (3, 0x3333333333333333, 0x2222222222222222, 0x1111111111111111), 0, 3, 68,
        (3, 0, 0x0333333333333333, 0x3222222222222222)
    )

    #undef TEST_SSM_SHR

    TEST_FN_CLOSE
}

static void test_num_ssm_shl_mod(bool show)
{
    TEST_FN_OPEN

    #define TEST_SSM_SHL_MOD(TAG, NUM, BITS, RES)       \
    {                                                   \
        TEST_CASE_OPEN(TAG)                             \
        {                                               \
            num_p num = num_create_immed(ARG_OPEN NUM); \
            num_p num_aux = num_create_rand(6);         \
            num_ssm_shl_mod(num_aux, num, 0, 3, BITS);  \
            num_free(num_aux);                          \
            assert(num_immed(num, ARG_OPEN RES));       \
        }                                               \
        TEST_CASE_CLOSE                                 \
    }

    TEST_SSM_SHL_MOD(1,
        (3, 0, 0, 0), 1,
        (3, 0, 0, 0)
    )
    TEST_SSM_SHL_MOD(2,
        (3, 0, 0, 1), 0,
        (3, 0, 0, 1)
    )
    TEST_SSM_SHL_MOD(3,
        (3, 0, 0, 1), 8,
        (3, 0, 0, 0x100)
    )
    TEST_SSM_SHL_MOD(4,
        (3, 0, 0, 1), 64,
        (3, 0, 1, 0)
    )
    TEST_SSM_SHL_MOD(5,
        (3, 0, 0, 1), 65,
        (3, 0, 2, 0)
    )
    TEST_SSM_SHL_MOD(6,
        (3, 0, 0, 1), 128,
        (3, 1, 0, 0)
    )
    TEST_SSM_SHL_MOD(7,
        (3, 1, 0, 0), 1,
        (3, 0, UINT64_MAX, UINT64_MAX)
    )
    TEST_SSM_SHL_MOD(8,
        (3, 0, B(63), 1), 1,
        (3, 0, 0, 1)
    )
    TEST_SSM_SHL_MOD(9,
        (3, 0, UINT64_MAX, UINT64_MAX), 1,
        (3, 0, UINT64_MAX, UINT64_MAX - 2)
    )
    TEST_SSM_SHL_MOD(10,
        (3, 0, UINT64_MAX, UINT64_MAX), 64,
        (3, 0, UINT64_MAX - 1, 1)
    )
    TEST_SSM_SHL_MOD(11,
        (3, 0, UINT64_MAX, UINT64_MAX), 65,
        (3, 0, UINT64_MAX - 3, 1)
    )
    TEST_SSM_SHL_MOD(12,
        (3, 0, UINT64_MAX, UINT64_MAX), 128,
        (3, 0, 0, 2)
    )
    TEST_SSM_SHL_MOD(13,
        (3, 0, 0, 1), 128,
        (3, 1, 0, 0)
    )
    TEST_SSM_SHL_MOD(14,
        (3, 1, 0, 0), 128,
        (3, 0, 0, 1)
    )

    #undef TEST_SSM_SHL_MOD

    TEST_FN_CLOSE
}

static void test_num_ssm_shr_mod(bool show)
{
    TEST_FN_OPEN

    #define TEST_SSM_SHR_MOD(TAG, NUM, BITS, RES)       \
    {                                                   \
        TEST_CASE_OPEN(TAG)                             \
        {                                               \
            num_p num = num_create_immed(ARG_OPEN NUM); \
            num_p num_aux = num_create_rand(6);         \
            num_ssm_shr_mod(num_aux, num, 0, 3, BITS);  \
            num_free(num_aux);                          \
            assert(num_immed(num, ARG_OPEN RES));       \
        }                                               \
        TEST_CASE_CLOSE                                 \
    }

    TEST_SSM_SHR_MOD(1,
        (3, 0, 0, 0), 1,
        (3, 0, 0, 0)
    )
    TEST_SSM_SHR_MOD(2,
        (3, 0, 2, 0), 0,
        (3, 0, 2, 0)
    )
    TEST_SSM_SHR_MOD(3,
        (3, 0, 2, 0), 1,
        (3, 0, 1, 0)
    )
    TEST_SSM_SHR_MOD(4,
        (3, 0, 2, 0), 64,
        (3, 0, 0, 2)
    )
    TEST_SSM_SHR_MOD(5,
        (3, 0, 2, 0), 65,
        (3, 0, 0, 1)
    )
    TEST_SSM_SHR_MOD(6,
        (3, 1, 0, 0), 128,
        (3, 0, 0, 1)
    )
    TEST_SSM_SHR_MOD(7,
        (3, 0, 0, 1), 1,
        (3, 0, B(63), 1)
    )
    TEST_SSM_SHR_MOD(8,
        (3, 0, UINT64_MAX, UINT64_MAX), 1,
        (3, 1, 0, 0)
    )
    TEST_SSM_SHR_MOD(9,
        (3, 0, UINT64_MAX, UINT64_MAX), 2,
        (3, 0, B(63), 0)
    )
    TEST_SSM_SHR_MOD(10,
        (3, 0, UINT64_MAX, UINT64_MAX), 64,
        (3, 0, 2, 0)
    )
    TEST_SSM_SHR_MOD(11,
        (3, 0, UINT64_MAX, UINT64_MAX), 65,
        (3, 0, 1, 0)
    )
    TEST_SSM_SHR_MOD(12,
        (3, 0, UINT64_MAX, UINT64_MAX), 128,
        (3, 0, 0, 2)
    )
    TEST_SSM_SHR_MOD(13,
        (3, 0, UINT64_MAX, UINT64_MAX), 128,
        (3, 0, 0, 2)
    )
    TEST_SSM_SHR_MOD(14,
        (3, 1, 0, 0), 128,
        (3, 0, 0, 1)
    )
    TEST_SSM_SHR_MOD(15,
        (3, 0, 0, 1), 128,
        (3, 1, 0, 0)
    )

    #undef TEST_SSM_SHR_MOD

    TEST_FN_CLOSE
}

static void test_num_ssm_fft(bool show)
{
    TEST_FN_OPEN

    #define TEST_SSM_FFT(TAG, NUM, Nv, Kv, RES)         \
    {                                                   \
        TEST_CASE_OPEN(TAG)                             \
        {                                               \
            num_p num = num_create_immed(ARG_OPEN NUM); \
            uint64_t Q = 64 * ((Nv) - 1) / (Kv);        \
            ssm_params_t p = (ssm_params_t)             \
            {                                           \
                .K = (Kv),                              \
                .Q = Q,                                 \
                .n = (Nv),                              \
            };                                          \
            num_p num_aux = num_create_rand(2 * (Nv));  \
            num_ssm_fft_fwd(num_aux, num, &p);          \
            num_free(num_aux);                          \
            assert(num_immed(num, ARG_OPEN RES));       \
        }                                               \
        TEST_CASE_CLOSE                                 \
    }

    TEST_SSM_FFT(1,
        (4, 0, 0, 0, 1), 2, 2,
        (4, 0, 1, 0, 1)
    )
    TEST_SSM_FFT(2,
        (4, 0, 1, 0, 0), 2, 2,
        (4, 0, 0xffffffff00000001, 0, B(32))
    )
    TEST_SSM_FFT(3,
        (4, 0, 1, 0, 1), 2, 2,
        (4, 0, 0xffffffff00000002, 0, 0x100000001)
    )
    TEST_SSM_FFT(4,
        (8, 0, 0, 0, 0, 0, 0, 0, 1), 2, 4,
        (8, 0, 1, 0, 1, 0, 1, 0, 1)
    )
    TEST_SSM_FFT(5,
        (8, 0, 0, 0, 0, 0, 1, 0, 0), 2, 4,
        (8, 0, 0xffff000000000001, 0, B(48), 0, 0xffffffffffff0001, 0, B(16))
    )
    TEST_SSM_FFT(6,
        (8, 0, 0, 0, 1, 0, 0, 0, 0), 2, 4,
        (8, 0, 0xffffffff00000001, 0, 0xffffffff00000001, 0, B(32), 0, B(32))
    )

    #undef TEST_SSM_FFT

    TEST_FN_CLOSE
}

static void test_num_ssm_depad_wrap(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_SSM_DEPAD_WRAP(TAG, NUM_FFT, N, NUM_RES)                   \
    {                                                                           \
        TEST_CASE_OPEN(TAG)                                                     \
        {                                                                       \
            num_p num_fft = num_create_immed(ARG_OPEN NUM_FFT);                 \
            ssm_params_t p = ssm_get_params_wrap(N);                            \
            num_p num_aux_1 = num_create_rand(N);                               \
            num_p num_aux_2 = num_create_rand(2 * (N));                         \
            num_p num_res = num_create_rand(N);                                 \
            num_ssm_depad_wrap(num_aux_1, num_aux_2, num_res, 0, num_fft, &p);  \
            assert(num_immed(num_res, ARG_OPEN NUM_RES));                       \
            num_free(num_aux_1);                                                \
            num_free(num_aux_2);                                                \
            num_free(num_fft);                                                  \
        }                                                                       \
        TEST_CASE_CLOSE                                                         \
    }

    TEST_NUM_SSM_DEPAD_WRAP(1,
        (32,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 1
        ),
        9,
        (9, 0, 0, 0, 0, 0, 0, 0, 0, 1)
    )
    TEST_NUM_SSM_DEPAD_WRAP(2,
        (32,
            0, 0, 0, 1,
            0, 0, 0, 1,
            0, 0, 0, 1,
            0, 0, 0, 1,
            0, 0, 0, 1,
            0, 0, 0, 1,
            0, 0, 0, 1,
            0, 0, 0, 1
        ),
        9,
        (9, 0, 1, 1, 1, 1, 1, 1, 1, 1)
    )
    TEST_NUM_SSM_DEPAD_WRAP(3,
        (32,
            0, 0, 0, 8,
            0, 0, 0, 7,
            0, 0, 0, 6,
            0, 0, 0, 5,
            0, 0, 0, 4,
            0, 0, 0, 3,
            0, 0, 0, 2,
            0, 0, 0, 1
        ),
        9,
        (9, 0, 8, 7, 6, 5, 4, 3, 2, 1)
    )
    TEST_NUM_SSM_DEPAD_WRAP(4,
        (32,
            0, 0, 1, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0
        ),
        9,
        (9, 1, 0, 0, 0, 0, 0, 0, 0, 0)
    )

    TEST_CASE_OPEN(6)
    TEST_CASE_CLOSE

    TEST_FN_CLOSE
}

static void test_num_ssm_mul_wrap(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_SSM_MUL_WRAP(TAG, N, NUM_FFT_1, NUM_FFT_2, RES)            \
    {                                                                           \
        TEST_CASE_OPEN(TAG)                                                     \
        {                                                                       \
            num_p num_fft_1 = num_create_immed(ARG_OPEN NUM_FFT_1);             \
            num_p num_fft_2 = num_create_immed(ARG_OPEN NUM_FFT_2);             \
            num_p num_aux_1 = num_create_rand(N);                               \
            num_p num_aux_2 = num_create_rand(2 * (N));                         \
            ssm_params_t p = ssm_get_params_wrap(N);                            \
            num_p num_fft_1_next = num_create_dirty(CLU_ARGS(p.n * p.K, 0));    \
            num_p num_fft_2_next = num_create_dirty(CLU_ARGS(p.n * p.K, 0));    \
            num_ssm_mul_wrap(                                                   \
                num_aux_1,                                                      \
                num_aux_2,                                                      \
                num_fft_1_next,                                                 \
                num_fft_2_next,                                                 \
                num_fft_1,                                                      \
                num_fft_2,                                                      \
                0,                                                              \
                &p                                                              \
            );                                                                  \
            assert(num_immed(num_fft_1, ARG_OPEN RES))                          \
            num_free(num_aux_1);                                                \
            num_free(num_aux_2);                                                \
            num_free(num_fft_1_next);                                           \
            num_free(num_fft_2_next);                                           \
            num_free(num_fft_2);                                                \
        }                                                                       \
        TEST_CASE_CLOSE                                                         \
    }

    TEST_NUM_SSM_MUL_WRAP(1, 9,
        (9, 0, 0, 0, 0, 0, 0, 0, 0, 1),
        (9, 0, 0, 0, 0, 0, 0, 0, 0, 1),
        (9, 0, 0, 0, 0, 0, 0, 0, 0, 1)
    )
    TEST_NUM_SSM_MUL_WRAP(2, 9,
        (9, 0, 0, 0, 0, 0, 0, 0, 1, 0),
        (9, 0, 0, 0, 0, 0, 0, 0, 1, 0),
        (9, 0, 0, 0, 0, 0, 0, 1, 0, 0)
    )
    TEST_NUM_SSM_MUL_WRAP(3, 9,
        (9, 0, 0, 0, 0, 0, 1, 0, 0, 0),
        (9, 0, 0, 0, 0, 0, 1, 0, 0, 0),
        (9, 0, 0, 1, 0, 0, 0, 0, 0, 0)
    )
    TEST_NUM_SSM_MUL_WRAP(4, 9,
        (9, 0, 0, 0, 0, 1, 0, 0, 0, 0),
        (9, 0, 0, 0, 0, 1, 0, 0, 0, 0),
        (9, 1, 0, 0, 0, 0, 0, 0, 0, 0)
    )
    TEST_NUM_SSM_MUL_WRAP(5, 9,
        (9, 1, 0, 0, 0, 0, 0, 0, 0, 0),
        (9, 0, 0, 0, 0, 0, 0, 0, 0, 1),
        (9, 1, 0, 0, 0, 0, 0, 0, 0, 0)
    )
    TEST_NUM_SSM_MUL_WRAP(6, 9,
        (9, 1, 0, 0, 0, 0, 0, 0, 0, 0),
        (9, 1, 0, 0, 0, 0, 0, 0, 0, 0),
        (9, 0, 0, 0, 0, 0, 0, 0, 0, 1)
    )
    TEST_NUM_SSM_MUL_WRAP(7, 9,
        (9, 0, 0, 0, 0, 0, 0, 0, 0, UINT64_MAX),
        (9, 0, 0, 0, 0, 0, 0, 0, 0, UINT64_MAX),
        (9, 0, 0, 0, 0, 0, 0, 0, UINT64_MAX - 1, 1)
    )
    TEST_NUM_SSM_MUL_WRAP(8, 9,
        (
            9,
                     0, UINT64_MAX, UINT64_MAX,
            UINT64_MAX, UINT64_MAX, UINT64_MAX,
            UINT64_MAX, UINT64_MAX, UINT64_MAX
        ),
        (
            9,
                     0, UINT64_MAX, UINT64_MAX,
            UINT64_MAX, UINT64_MAX, UINT64_MAX,
            UINT64_MAX, UINT64_MAX, UINT64_MAX
        ),
        (9, 0, 0, 0, 0, 0, 0, 0, 0, 4)
    )
    TEST_NUM_SSM_MUL_WRAP(9, 9,
        (9, 0, 1, 0, 0, 0, 0, 0, 0, 0),
        (9, 0, 1, 0, 0, 0, 0, 0, 0, 0),
        (9, 0, UINT64_MAX, UINT64_MAX, 0, 0, 0, 0, 0, 1)
    )

    TEST_FN_CLOSE
}



static void test_num_div_normalize(bool show)
{
    TEST_FN_OPEN

    #define TEST_DIV_NORMALIZE(TAG, NUM_1, NUM_2, RES_1, RES_2) \
    {                                                           \
        TEST_CASE_OPEN(TAG)                                     \
        {                                                       \
            num_p num_1 = num_create_immed(ARG_OPEN NUM_1);     \
            num_p num_2 = num_create_immed(ARG_OPEN NUM_2);     \
            num_div_normalize(&num_1, &num_2);                  \
            assert(num_immed(num_1, ARG_OPEN RES_1))            \
            assert(num_immed(num_2, ARG_OPEN RES_2))            \
        }                                                       \
        TEST_CASE_CLOSE                                         \
    }

    TEST_DIV_NORMALIZE(1, (1, 1), (1, 1), (1, B(63)), (1, B(63)))

    TEST_FN_CLOSE
}



static void test_num_is_zero(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_IS_ZERO(TAG, NUM, IS_ZERO)         \
    {                                                   \
        TEST_CASE_OPEN(TAG)                             \
        {                                               \
            num_p num = num_create_immed(ARG_OPEN NUM); \
            bool res = num_is_zero(num);                \
            assert(res == (IS_ZERO));                   \
            num_free(num);                              \
        }                                               \
        TEST_CASE_CLOSE                                 \
    }

    TEST_NUM_IS_ZERO(1, (0), true)
    TEST_NUM_IS_ZERO(2, (1, 1), false)
    TEST_NUM_IS_ZERO(3, (2, 1, 2), false)

    #undef TEST_NUM_IS_ZERO

    TEST_FN_CLOSE
}



static void test_num_shl(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_SHL(TAG, NUM_IN, BITS, NUM_OUT)        \
    {                                                       \
        TEST_CASE_OPEN(TAG)                                 \
        {                                                   \
            num_p num = num_create_immed(ARG_OPEN NUM_IN);  \
            num = num_shl(num, BITS);                       \
            assert(num_immed(num, ARG_OPEN NUM_OUT));       \
        }                                                   \
        TEST_CASE_CLOSE                                     \
    }

    TEST_NUM_SHL(1, (0), 0, (0))
    TEST_NUM_SHL(2, (0), 1, (0))
    TEST_NUM_SHL(3, (0), 64, (0))
    TEST_NUM_SHL(4, (1, 1), 1, (1, 2))
    TEST_NUM_SHL(5, (1, 1), 64, (2, 1, 0))
    TEST_NUM_SHL(6, (1, 1), 65, (2, 2, 0))
    TEST_NUM_SHL(7, (1, 0x8000000000000000), 1, (2, 1, 0))

    #undef TEST_NUM_SHL

    TEST_FN_CLOSE
}

static void test_num_shr(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_SHR(TAG, NUM_IN, BITS, NUM_OUT)        \
    {                                                       \
        TEST_CASE_OPEN(TAG)                                 \
        {                                                   \
            num_p num = num_create_immed(ARG_OPEN NUM_IN);  \
            num = num_shr(num, BITS);                       \
            assert(num_immed(num, ARG_OPEN NUM_OUT));       \
        }                                                   \
        TEST_CASE_CLOSE                                     \
    }

    TEST_NUM_SHR(1, (0), 0, (0))
    TEST_NUM_SHR(2, (0), 1, (0))
    TEST_NUM_SHR(3, (1, 1), 1, (0))
    TEST_NUM_SHR(4, (1, 2), 1, (1, 1))
    TEST_NUM_SHR(5, (2, 1, 0), 64, (1, 1))
    TEST_NUM_SHR(6, (2, 1, 0), 65, (0))
    TEST_NUM_SHR(7, (2, 2, 0), 65, (1, 1))

    #undef TEST_NUM_SHR

    TEST_FN_CLOSE
}



static void test_num_add(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_ADD(TAG, NUM_1, NUM_2, NUM_OUT)        \
    {                                                       \
        TEST_CASE_OPEN(TAG)                                 \
        {                                                   \
            num_p num_1 = num_create_immed(ARG_OPEN NUM_1); \
            num_p num_2 = num_create_immed(ARG_OPEN NUM_2); \
            num_1 = num_add(num_1, num_2);                  \
            assert(num_immed(num_1, ARG_OPEN NUM_OUT));     \
        }                                                   \
        TEST_CASE_CLOSE                                     \
    }

    TEST_NUM_ADD(1,
        (0),
        (0),
        (0)
    )
    TEST_NUM_ADD(2,
        (1, 1),
        (0),
        (1, 1)
    )
    TEST_NUM_ADD(3,
        (0),
        (1, 1),
        (1, 1)
    )
    TEST_NUM_ADD(4,
        (1, 1),
        (1, 2),
        (1, 3)
    )
    TEST_NUM_ADD(5,
        (2, 2, 1),
        (1, 2),
        (2, 2, 3)
    )
    TEST_NUM_ADD(6,
        (1, 1),
        (2, 2, 3),
        (2, 2, 4)
    )
    TEST_NUM_ADD(7,
        (2, UINT64_MAX, UINT64_MAX),
        (1, 1),
        (3, 1, 0, 0)
    )
    TEST_NUM_ADD(8,
        (1, 1),
        (2, UINT64_MAX, UINT64_MAX),
        (3, 1, 0, 0)
    )

    #undef TEST_NUM_ADD

    TEST_FN_CLOSE
}

static void test_num_sub(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_SUB(TAG, NUM_1, NUM_2, NUM_OUT)        \
    {                                                       \
        TEST_CASE_OPEN(TAG)                                 \
        {                                                   \
            num_p num_1 = num_create_immed(ARG_OPEN NUM_1); \
            num_p num_2 = num_create_immed(ARG_OPEN NUM_2); \
            num_1 = num_sub(num_1, num_2);                  \
            assert(num_immed(num_1, ARG_OPEN NUM_OUT));     \
        }                                                   \
        TEST_CASE_CLOSE                                     \
    }

    TEST_NUM_SUB(1,
        (0),
        (0),
        (0)
    )
    TEST_NUM_SUB(2,
        (1, 1),
        (0),
        (1, 1)
    )
    TEST_NUM_SUB(3,
        (1, 2),
        (1, 1),
        (1, 1)
    )
    TEST_NUM_SUB(4,
        (1, 2),
        (1, 2),
        (0)
    )
    TEST_NUM_SUB(5,
        (2, 1, 0),
        (1, 1),
        (1, UINT64_MAX)
    )
    TEST_NUM_SUB(6,
        (2, 4, 3),
        (2, 1, 2),
        (2, 3, 1)
    )
    TEST_NUM_SUB(7,
        (2, 1, 0),
        (2, 1, 0),
        (0)
    )
    TEST_NUM_SUB(8,
        (5, 1, 0, 0, 0, 0),
        (4, 0x8000000000000000, 0, 0, 0x8000000000000000),
        (4, UINT64_MAX >> 1, UINT64_MAX, UINT64_MAX, 0x8000000000000000)
    )

    #undef TEST_NUM_SUB

    TEST_FN_CLOSE
}

static void test_num_mul(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_MUL(TAG, FN, NUM_1, NUM_2, RES)        \
    {                                                       \
        TEST_CASE_OPEN(TAG)                                 \
        {                                                   \
            num_p num_1 = num_create_immed(ARG_OPEN NUM_1); \
            num_p num_2 = num_create_immed(ARG_OPEN NUM_2); \
            num_p num_res = FN(num_1, num_2);               \
            assert(num_immed(num_res, ARG_OPEN RES))        \
            num_free(num_1);                                \
            num_free(num_2);                                \
        }                                                   \
        TEST_CASE_CLOSE                                     \
    }                                                       \

    #define TEST_NUM_MUL_BATCH(TAG, NUM_1, NUM_2, RES)                          \
    {                                                                           \
        TEST_NUM_MUL((10 * (TAG)) + 1, num_mul_classic, NUM_1, NUM_2, RES)      \
        TEST_NUM_MUL((10 * (TAG)) + 2, num_mul_ssm, NUM_1, NUM_2, RES)          \
        TEST_NUM_MUL((10 * (TAG)) + 3, num_mul_core, NUM_1, NUM_2, RES)         \
    }

    TEST_NUM_MUL_BATCH(1,
        (0),
        (0),
        (0)
    )
    TEST_NUM_MUL_BATCH(2,
        (1, 1),
        (0),
        (0)
    )
    TEST_NUM_MUL_BATCH(3,
        (0),
        (1, 1),
        (0)
    )
    TEST_NUM_MUL_BATCH(4,
        (1, 2),
        (1, 3),
        (1, 6)
    )
    TEST_NUM_MUL_BATCH(5,
        (1, UINT64_MAX),
        (1, UINT64_MAX),
        (2, UINT64_MAX - 1, 1)
    )
    TEST_NUM_MUL_BATCH(6,
        (2, 2, 3),
        (1, 4),
        (2, 8, 12)
    )
    TEST_NUM_MUL_BATCH(7,
        (1, 2),
        (2, 3, 4),
        (2, 6, 8)
    )
    TEST_NUM_MUL_BATCH(8,
        (2, 2, 3),
        (2, 4, 5),
        (3, 8, 22, 15)
    )
    TEST_NUM_MUL_BATCH(9,
        (2, 1, 0),
        (1, 1),
        (2, 1, 0)
    )
    TEST_NUM_MUL_BATCH(10,
        (1, 1),
        (2, 1, 0),
        (2, 1, 0)
    )
    TEST_NUM_MUL_BATCH(11,
        (2, 1, 0),
        (2, 1, 0),
        (3, 1, 0, 0)
    )
    TEST_NUM_MUL_BATCH(12,
        (1, 2),
        (2, 2, UINT64_MAX),
        (2, 5, UINT64_MAX - 1)
    )
    TEST_NUM_MUL_BATCH(13,
        (2, UINT64_MAX, UINT64_MAX),
        (2, UINT64_MAX, UINT64_MAX),
        (4, UINT64_MAX, UINT64_MAX - 1, 0, 1)
    )
    TEST_NUM_MUL_BATCH(14,
        (3, 2, 3, 4),
        (3, 5, 6, 7),
        (5, 10, 27, 52, 45, 28)
    )
    TEST_NUM_MUL_BATCH(15,
        (3, 2, 3, 4),
        (3, 5, 6, 7),
        (5, 10, 27, 52, 45, 28)
    )
    TEST_NUM_MUL_BATCH(16,
        (5, 1, 0, 0, 0, 0),
        (5, 1, 0, 0, 0, 0),
        (9, 1, 0, 0, 0, 0, 0, 0, 0, 0)
    )
    TEST_NUM_MUL_BATCH(17,
        (5, 1, 0, 0, 0, 1),
        (5, 1, 0, 0, 0, 1),
        (9, 1, 0, 0, 0, 2, 0, 0, 0, 1)
    )
    TEST_NUM_MUL_BATCH(18,
        (5, UINT64_MAX, 0, 0, 0, 1),
        (5, UINT64_MAX, 0, 0, 0, 1),
        (10, UINT64_MAX - 1, 1, 0, 0, 1, UINT64_MAX - 1, 0, 0, 0, 1)
    )
    TEST_NUM_MUL_BATCH(19,
        (5, UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX),
        (5, UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX),
        (10, UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX - 1, 0, 0, 0, 0, 1)
    )
    TEST_NUM_MUL_BATCH(20,
        (
            19,
            UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
            UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
            UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
            UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
            UINT64_MAX, UINT64_MAX, UINT64_MAX
        ),
        (
            19,
            UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
            UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
            UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
            UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
            UINT64_MAX, UINT64_MAX, UINT64_MAX
        ),
        (
            38,
            UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
            UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
            UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
            UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
            UINT64_MAX, UINT64_MAX, UINT64_MAX -1, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
        )
    )

    #undef TEST_NUM_MUL

    TEST_FN_CLOSE
}

static void test_num_sqr(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_SQR_FN(TAG, FN, NUM, RES)          \
    {                                                   \
        TEST_CASE_OPEN(TAG)                             \
        {                                               \
            num_p num = num_create_immed(ARG_OPEN NUM); \
            num = FN(num);                              \
            assert(num_immed(num, ARG_OPEN RES))        \
        }                                               \
        TEST_CASE_CLOSE                                 \
    }

    #define TEST_NUM_SQR(TAG, NUM, RES)                                 \
    {                                                                   \
        TEST_NUM_SQR_FN((10 * (TAG)) + 1, num_sqr_classic, NUM, RES)    \
        TEST_NUM_SQR_FN((10 * (TAG)) + 2, num_sqr_ssm, NUM, RES)        \
        TEST_NUM_SQR_FN((10 * (TAG)) + 3, num_sqr, NUM, RES)            \
    }

    TEST_NUM_SQR(1,
        (0),
        (0)
    )
    TEST_NUM_SQR(2,
        (1, 1),
        (1, 1)
    )
    TEST_NUM_SQR(3,
        (1, 2),
        (1, 4)
    )
    TEST_NUM_SQR(4,
        (1, (uint64_t)(UINT32_MAX)),
        (1, 0xfffffffe00000001)
    )
    TEST_NUM_SQR(5,
        (1, B(32)),
        (2, 1, 0)
    )
    TEST_NUM_SQR(6,
        (2, 1, 0),
        (3, 1, 0, 0)
    )
    TEST_NUM_SQR(7,
        (2, 2, 3),
        (3, 4, 12, 9)
    )
    TEST_NUM_SQR(8,
        (2, UINT64_MAX, UINT64_MAX),
        (4, UINT64_MAX, UINT64_MAX - 1, 0, 1)
    )

    #undef TEST_NUM_SQR

    TEST_FN_CLOSE
}

static void test_num_div_mod(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_DIV_MOD(TAG, NUM_1, NUM_2, NUM_Q, NUM_R)   \
    {                                                           \
        TEST_CASE_OPEN(TAG)                                     \
        {                                                       \
            num_p num_q, num_r;                                 \
            num_p num_1 = num_create_immed(ARG_OPEN NUM_1);     \
            num_p num_2 = num_create_immed(ARG_OPEN NUM_2);     \
            num_div_mod(&num_q, &num_r, num_1, num_2);          \
            assert(num_immed(num_q, ARG_OPEN NUM_Q));           \
            assert(num_immed(num_r, ARG_OPEN NUM_R));           \
        }                                                       \
        TEST_CASE_CLOSE                                         \
    }

    TEST_NUM_DIV_MOD(1,
        (0),
        (1, 1),
        (0),
        (0)
    )
    TEST_NUM_DIV_MOD(2,
        (1, 4),
        (1, 2),
        (1, 2),
        (0)
    )
    TEST_NUM_DIV_MOD(3,
        (1, 5),
        (1, 2),
        (1, 2),
        (1, 1)
    )
    TEST_NUM_DIV_MOD(4,
        (1, 5),
        (1, 5),
        (1, 1),
        (0)
    )
    TEST_NUM_DIV_MOD(5,
        (1, 9),
        (1, 3),
        (1, 3),
        (0)
    )
    TEST_NUM_DIV_MOD(6,
        (3, 1, 0, 0),
        (2, 1, 0),
        (2, 1, 0),
        (0)
    )
    TEST_NUM_DIV_MOD(7,
        (1, 1),
        (2, 1, 0),
        (0),
        (1, 1)
    )
    TEST_NUM_DIV_MOD(8,
        (2, 4, UINT64_MAX),
        (2, 2, 0),
        (1, 2),
        (1, UINT64_MAX)
    )
    TEST_NUM_DIV_MOD(9,
        (2, 4, 0),
        (2, 2, UINT64_MAX),
        (1, 1),
        (2, 1, 1)
    )
    TEST_NUM_DIV_MOD(10,
        (2, 1, 0),
        (1, UINT64_MAX),
        (1, 1),
        (1, 1)
    )
    TEST_NUM_DIV_MOD(11,
        (2, UINT64_MAX, 0),
        (2, 1, UINT64_MAX),
        (1, UINT64_MAX >> 1),
        (2, 1, UINT64_MAX >> 1)
    )
    TEST_NUM_DIV_MOD(12,
        (2, 0xc929d7d593, 0xb7090a859117cfa4),
        (2, 6, 0xea7db545decb57a4),
        (1, 0x0000001d1635b735),
        (1, 0x88c80995d8646eb0)
    )
    TEST_NUM_DIV_MOD(13,
        (3, UINT64_MAX, 0, UINT64_MAX),
        (1, UINT64_MAX),
        (3, 1, 0, 1),
        (0)
    )
    TEST_NUM_DIV_MOD(14,
        (5, UINT64_MAX, 0, 0, 0, UINT64_MAX),
        (1, UINT64_MAX),
        (5, 1, 0, 0, 0, 1),
        (0)
    )
    TEST_NUM_DIV_MOD(15,
        (6, 4, 0, 8, 4, 0, 0),
        (4, 4, 0, 0, 4),
        (3, 1, 0, 1),
        (4, 3, UINT64_MAX, UINT64_MAX, UINT64_MAX - 3)
    )
    TEST_NUM_DIV_MOD(16,
        (3, 1, 1, 0),
        (2, 1, 2),
        (1, UINT64_MAX),
        (1, 2)
    )
    TEST_NUM_DIV_MOD(17,
        (3, UINT64_MAX, UINT64_MAX, UINT64_MAX),
        (2, UINT64_MAX, UINT64_MAX),
        (2, 1, 0),
        (1, UINT64_MAX)
    )
    TEST_NUM_DIV_MOD(18,
        (3, 1, 0, 0),
        (2, 1, UINT64_MAX),
        (1, 0x8000000000000000),
        (1, 0x8000000000000000)
    )
    TEST_NUM_DIV_MOD(19,
        (4, 1, 0, 0, 0),
        (3, 1, 0, UINT64_MAX),
        (1, UINT64_MAX),
        (2, 1, UINT64_MAX)
    )
    TEST_NUM_DIV_MOD(20,
        (3, 1, UINT64_MAX - 1, 0),
        (2, 1, UINT64_MAX),
        (1, UINT64_MAX),
        (1, UINT64_MAX)
    )
    TEST_NUM_DIV_MOD(21,
        (4, 1, UINT64_MAX, UINT64_MAX - 1, 0),
        (3, 1, UINT64_MAX, UINT64_MAX),
        (1, UINT64_MAX),
        (3, 1, UINT64_MAX - 1, UINT64_MAX)
    )
    TEST_NUM_DIV_MOD(22,
        (3, UINT64_MAX, 0, 0),
        (2, UINT64_MAX, UINT64_MAX),
        (1, UINT64_MAX),
        (1, UINT64_MAX)
    )
    TEST_NUM_DIV_MOD(23,
        (3, 2, 0, 0),
        (2, 2, UINT64_MAX),
        (1, 0xAAAAAAAAAAAAAAAA),
        (2, 2, 0xAAAAAAAAAAAAAAAA)
    )
    TEST_NUM_DIV_MOD(24,
        (5, 1, 0, 0, 0, 0),
        (4, 0x8000000000000000, 0, 0, 0x8000000000000000),
        (1, 1),
        (4, UINT64_MAX >> 1, UINT64_MAX, UINT64_MAX, 0x8000000000000000)
    )
    TEST_NUM_DIV_MOD(25,
        (6, 0x8000000000000000, 1, 0, 0x8000000000000000, 0, 0),
        (4, 0x8000000000000000, 0, 0, 0x8000000000000000),
        (3, 1, 0, 1),
        (4, UINT64_MAX >> 1, UINT64_MAX, UINT64_MAX, 0x8000000000000000)
    )
    TEST_NUM_DIV_MOD(26,
        (8, UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX),
        (2, UINT64_MAX, UINT64_MAX),
        (7, 1, 0, 1, 0, 1, 0, 1),
        (0)
    )
    TEST_NUM_DIV_MOD(27,
        (4, 0xa, 0, 0, 0xc),
        (4, 1, 0, 0, 1),
        (1, 0xa),
        (1, 2)
    )

    #undef TEST_NUM_DIV_MOD

    TEST_CASE_OPEN(25)
    {
        num_p num_1 = num_create_immed(1, 1);
        num_p num_2 = num_create_immed(0);
        num_p num_q, num_r;
        TEST_REVERT_OPEN
        {
            num_div_mod(&num_q, &num_r, num_1, num_2);
        }
        TEST_REVERT_CLOSE
        num_free(num_1);
        num_free(num_2);
    }
    TEST_CASE_CLOSE

    TEST_FN_CLOSE
}

static void test_num_gcd(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_GDC(TAG, NUM_1, NUM_2, NUM_OUT)        \
    {                                                       \
        TEST_CASE_OPEN(TAG)                                 \
        {                                                   \
            num_p num_1 = num_create_immed(ARG_OPEN NUM_1); \
            num_p num_2 = num_create_immed(ARG_OPEN NUM_2); \
            num_1 = num_gcd(num_1, num_2);                  \
            assert(num_immed(num_1, ARG_OPEN NUM_OUT));     \
        }                                                   \
        TEST_CASE_CLOSE                                     \
    }

    TEST_NUM_GDC(1,
        (1, 3),
        (1, 2),
        (1, 1)
    )
    TEST_NUM_GDC(2,
        (1, 4),
        (1, 2),
        (1, 2)
    )
    TEST_NUM_GDC(3,
        (1, 6),
        (1, 4),
        (1, 2)
    )

    #undef TEST_NUM_GDC

    TEST_FN_CLOSE
}



static void test_num_div_mod_uint(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_DIV_MOD_UINT(TAG, NUM, VALUE, NUM_Q, R)    \
    {                                                           \
        TEST_CASE_OPEN(TAG)                                     \
        {                                                       \
            num_p num = num_create_immed(ARG_OPEN NUM);         \
            num_p num_q = num_div_mod_uint(num, VALUE);         \
            assert(num_immed(num_q, ARG_OPEN NUM_Q));           \
            assert(num_eq_dbg(num, num_wrap(R)));               \
        }                                                       \
        TEST_CASE_CLOSE                                         \
    }

    TEST_NUM_DIV_MOD_UINT(1,
        (0),
        1,
        (0),
        0
    )
    TEST_NUM_DIV_MOD_UINT(2,
        (1, 4),
        2,
        (1, 2),
        (0)
    )
    TEST_NUM_DIV_MOD_UINT(3,
        (1, 5),
        2,
        (1, 2),
        1
    )
    TEST_NUM_DIV_MOD_UINT(4,
        (1, 5),
        5,
        (1, 1),
        0
    )
    TEST_NUM_DIV_MOD_UINT(5,
        (1, 9),
        3,
        (1, 3),
        0
    )
    TEST_NUM_DIV_MOD_UINT(6,
        (2, 1, 0),
        UINT64_MAX,
        (1, 1),
        1
    )
    TEST_NUM_DIV_MOD_UINT(7,
        (3, UINT64_MAX, 0, UINT64_MAX),
        UINT64_MAX,
        (3, 1, 0, 1),
        0
    )
    TEST_NUM_DIV_MOD_UINT(8,
        (5, UINT64_MAX, 0, 0, 0, UINT64_MAX),
        UINT64_MAX,
        (5, 1, 0, 0, 0, 1),
        0
    )

    #undef TEST_NUM_DIV_MOD_UINT

    TEST_CASE_OPEN(9)
    {
        num_p num = num_create_immed(1, 1);
        TEST_REVERT_OPEN
        {
            num_div_mod_uint(num, 0);
        }
        TEST_REVERT_CLOSE
        num_free(num);
    }
    TEST_CASE_CLOSE

    TEST_FN_CLOSE
}



static void test_num_base_to(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_BASE_TO(TAG, NUM_IN, NUM_OUT)          \
    {                                                       \
        TEST_CASE_OPEN(TAG)                                 \
        {                                                   \
            num_p num = num_create_immed(ARG_OPEN NUM_IN);  \
            num = num_base_to(num, 10);                     \
            assert(num_immed(num, ARG_OPEN NUM_OUT));       \
        }                                                   \
        TEST_CASE_CLOSE                                     \
    }

    TEST_NUM_BASE_TO(1,
        (0),
        (0)
    )
    TEST_NUM_BASE_TO(2,
        (1, 1),
        (1, 1)
    )
    TEST_NUM_BASE_TO(3,
        (1, 9),
        (1, 9)
    )
    TEST_NUM_BASE_TO(4,
        (1, 10),
        (2, 1, 0)
    )
    TEST_NUM_BASE_TO(5,
        (1, 100),
        (3, 1, 0, 0)
    )
    TEST_NUM_BASE_TO(6,
        (1, 1000),
        (4, 1, 0, 0, 0)
    )
    TEST_NUM_BASE_TO(7,
        (1, 10000),
        (5, 1, 0, 0, 0, 0)
    )
    TEST_NUM_BASE_TO(8,
        (1, 99),
        (2, 9, 9)
    )
    TEST_NUM_BASE_TO(9,
        (2, 5, 0x6bc75e2d63100000),
        (21, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    )
    TEST_NUM_BASE_TO(10,
        (2, 1, 0),
        (20, 1, 8, 4, 4, 6, 7, 4, 4, 0, 7, 3, 7, 0, 9, 5, 5, 1, 6, 1, 6)
    )

    #undef TEST_NUM_BASE_TO

    TEST_FN_CLOSE
}

static void test_num_base_from(bool show)
{
    TEST_FN_OPEN

    #define TEST_NUM_BASE_FROM(TAG, NUM_IN, NUM_OUT)        \
    {                                                       \
        TEST_CASE_OPEN(TAG)                                 \
        {                                                   \
            num_p num = num_create_immed(ARG_OPEN NUM_IN);  \
            num = num_base_from(num, 10);                   \
            assert(num_immed(num, ARG_OPEN NUM_OUT));       \
        }                                                   \
        TEST_CASE_CLOSE                                     \
    }

    TEST_NUM_BASE_FROM(1,
        (0),
        (0)
    )
    TEST_NUM_BASE_FROM(2,
        (1, 1),
        (1, 1)
    )
    TEST_NUM_BASE_FROM(3,
        (1, 9),
        (1, 9)
    )
    TEST_NUM_BASE_FROM(4,
        (2, 1, 0),
        (1, 10)
    )
    TEST_NUM_BASE_FROM(5,
        (21, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        (2, 5, 0x6bc75e2d63100000)
    )

    #undef TEST_NUM_BASE_FROM

    TEST_FN_CLOSE
}



static void test_fuzz_num_ssm_sh(bool show)
{
    TEST_FN_OPEN

    #define TEST_FUZZ_NUM_SSM_SH(TAG, N, BITS, RUNS)                \
    {                                                               \
        TEST_FUZZ_CASE_OPEN(TAG, RUNS)                              \
        {                                                           \
            num_p num = num_create_rand(N);                         \
            num->chunk[(N) - 1] = 0;                                \
            num_p num_res = num_copy(num);                          \
            num_p num_aux = num_create_rand(2 *(N));                \
            num_ssm_shl_mod(num_aux, num_res, 0, N, BITS);          \
            num_ssm_shr_mod(num_aux, num_res, 0, N, BITS);          \
            num_free(num_aux);                                      \
            if(!num_eq_dbg(num_copy(num_res), num_copy(num)))       \
            {                                                       \
                printf("\ncase: (n: %d) (bits: %d)", N, BITS);      \
                printf("\nentrie");                                 \
                num_display_span_full("num", num, N, 1);            \
                printf("\nround trip results in");                  \
                num_display_span_full("num_res", num_res, N, 1);    \
                assert(false);                                      \
            }                                                       \
            num_free(num_res);                                      \
            num_free(num);                                          \
        }                                                           \
        TEST_FUZZ_CASE_CLOSE                                        \
    }

    TEST_FUZZ_NUM_SSM_SH(1, 2,  1, 100)
    TEST_FUZZ_NUM_SSM_SH(2, 3, 65, 100)
    TEST_FUZZ_NUM_SSM_SH(3, 4, 80, 100)
    TEST_FUZZ_NUM_SSM_SH(4, 5, 80, 100)

    TEST_FN_CLOSE
}

static void test_fuzz_num_ssm_pad_no_wrap_round_trip(bool show)
{
    TEST_FN_OPEN

    #define TEST_FUZZ_NUM_SSM_PAD_NO_WRAP_ROUND_TRIP(TAG, COUNT, RUNS)  \
    {                                                                   \
        TEST_FUZZ_CASE_OPEN(TAG, RUNS)                                  \
        {                                                               \
            num_p num_in = num_create_rand(COUNT);                      \
            ssm_params_t p = ssm_get_params(COUNT);                     \
            num_p num_middle = num_ssm_pad_no_wrap(num_in, &p);         \
            num_p num_out = num_ssm_depad_no_wrap(num_middle, &p);      \
            assert(num_eq_dbg(num_in, num_out));                        \
        }                                                               \
        TEST_FUZZ_CASE_CLOSE                                            \
    }

    TEST_FUZZ_NUM_SSM_PAD_NO_WRAP_ROUND_TRIP(1, 256,  100);

    TEST_FN_CLOSE
}

static void test_fuzz_num_ssm_pad_wrap_round_trip(bool show)
{
    TEST_FN_OPEN

    #define TEST_FUZZ_NUM_SSM_PAD_WRAP(N)                                       \
    {                                                                           \
        ssm_params_t p = ssm_get_params_wrap(N);                                \
        num_p num_middle = num_create_dirty(CLU_ARGS(p.n * p.K, 0));            \
        num_ssm_pad_wrap(num_middle, num_in, 0, &p);                            \
        num_p num_aux_1 = num_create_rand(N);                                   \
        num_p num_aux_2 = num_create_rand(2 * (N));                             \
        num_p num_out = num_create_rand(N);                                     \
        num_ssm_depad_wrap(num_aux_1, num_aux_2, num_out, 0, num_middle, &p);   \
        num_out->count = N;                                                     \
        assert(num_eq_dbg(num_in, num_out));                                    \
        num_free(num_aux_1);                                                    \
        num_free(num_aux_2);                                                    \
        num_free(num_middle);                                                   \
    }

    TEST_CASE_OPEN(1)
    {
        uint64_t N = 9;
        num_p num_in = num_create_immed(9, 1, 0, 0, 0, 0, 0, 0, 0, 0);
        TEST_FUZZ_NUM_SSM_PAD_WRAP(N)
    }
    TEST_CASE_CLOSE

    #define TEST_FUZZ_NUM_SSM_PAD_WRAP_ROUND_TRIP(TAG, N, RUNS) \
    {                                                           \
        TEST_FUZZ_CASE_OPEN(TAG, RUNS)                          \
        {                                                       \
            num_p num_in = num_create_rand(N);                  \
            num_in->chunk[(N) - 1] = 0;                         \
            TEST_FUZZ_NUM_SSM_PAD_WRAP(N)                       \
        }                                                       \
        TEST_FUZZ_CASE_CLOSE                                    \
    }

    TEST_FUZZ_NUM_SSM_PAD_WRAP_ROUND_TRIP(2, 9, 100)
    TEST_FUZZ_NUM_SSM_PAD_WRAP_ROUND_TRIP(3, 17, 100)
    TEST_FUZZ_NUM_SSM_PAD_WRAP_ROUND_TRIP(4, 257, 100)
    TEST_FUZZ_NUM_SSM_PAD_WRAP_ROUND_TRIP(5, 1025, 100)
    TEST_FUZZ_NUM_SSM_PAD_WRAP_ROUND_TRIP(6, 1048577, 10)

    TEST_FN_CLOSE
}

static void test_fuzz_num_ssm_fft(bool show)
{
    TEST_FN_OPEN

    #define TEST_FUZZ_NUM_SSM_FFT(TAG, Nv, Kv, RUNS)                    \
    {                                                                   \
        TEST_FUZZ_CASE_OPEN(TAG, RUNS)                                  \
        {                                                               \
            num_p num = num_create_rand(((Nv)-1) * (Kv));               \
            uint64_t Q = 64 * ((Nv)-1) / (Kv);                          \
            ssm_params_t p = (ssm_params_t)                             \
            {                                                           \
                .M = (Nv)-1,                                            \
                .K = (Kv),                                              \
                .Q = Q,                                                 \
                .n = (Nv),                                              \
            };                                                          \
            num_p num_fft = num_ssm_pad_no_wrap(num, &p);               \
            num_free(num);                                              \
            num_fft->count = (Nv) * (Kv);                               \
            num_p num_res = num_copy(num_fft);                          \
            num_p num_aux = num_create_rand(2 * (Nv));                  \
            num_ssm_fft_fwd(num_aux, num_res, &p);                      \
            num_ssm_fft_inv(num_aux, num_res, &p);                      \
            num_free(num_aux);                                          \
            if(!num_eq_dbg(num_copy(num_res), num_copy(num_fft)))       \
            {                                                           \
                printf("\ncase: (K: %d) (n: %d)", (Kv), (Nv));          \
                printf("\nentrie");                                     \
                num_display_span_full("num_fft", num_fft, (Nv), (Kv));  \
                printf("\nroundrip results in");                        \
                num_display_span_full("num_res", num_res, (Nv), (Kv));  \
                assert(false);                                          \
            }                                                           \
            num_free(num_res);                                          \
            num_free(num_fft);                                          \
        }                                                               \
        TEST_FUZZ_CASE_CLOSE                                            \
    }

    TEST_FUZZ_NUM_SSM_FFT(1, 2, 4, 100)
    TEST_FUZZ_NUM_SSM_FFT(2, 2, 8, 100)
    TEST_FUZZ_NUM_SSM_FFT(3, 2, 16, 100)
    TEST_FUZZ_NUM_SSM_FFT(4, 3, 16, 100)
    TEST_FUZZ_NUM_SSM_FFT(5, 10, 128, 100)

    TEST_FN_CLOSE
}

static void test_fuzz_num_ssm_mul(bool show)
{
    TEST_FN_OPEN

    #define TEST_FUZZ_NUM_SSM_MUL_COUNT(COUNT_1, COUNT_2)   \
    {                                                       \
        num_p num_1 = num_create_rand(COUNT_1);             \
        num_p num_2 = num_create_rand(COUNT_2);             \
        num_p num_res_1 = num_mul_ssm(num_1, num_2);        \
        num_p num_res_2 = num_mul_classic(num_1, num_2);    \
        assert(num_eq_dbg(num_res_1, num_res_2));           \
        num_free(num_1);                                    \
        num_free(num_2);                                    \
    }

    #define TEST_FUZZ_NUM_SSM_MUL(TAG, COUNT, RUNS)     \
    {                                                   \
        TEST_FUZZ_CASE_OPEN(TAG, RUNS)                  \
        {                                               \
            TEST_FUZZ_NUM_SSM_MUL_COUNT(COUNT, COUNT)   \
        }                                               \
        TEST_FUZZ_CASE_CLOSE                            \
    }

    TEST_FUZZ_NUM_SSM_MUL(1, 500, 100)
    TEST_FUZZ_NUM_SSM_MUL(2, 1000, 10)
    TEST_FUZZ_NUM_SSM_MUL(3, 5000,  2)
    TEST_FUZZ_NUM_SSM_MUL(4, 80000, 1)

    #undef TEST_FUZZ_NUM_SSM_MUL

    #define TEST_FUZZ_NUM_SSM_MUL(TAG, COUNT_MIN, COUNT_MAX, RUNS)  \
    {                                                               \
        TEST_FUZZ_CASE_OPEN(TAG, RUNS)                              \
        {                                                           \
            uint64_t count_1 = rand_64_range(COUNT_MIN, COUNT_MAX); \
            uint64_t count_2 = rand_64_range(COUNT_MIN, COUNT_MAX); \
            TEST_FUZZ_NUM_SSM_MUL_COUNT(count_1, count_2)           \
        }                                                           \
        TEST_FUZZ_CASE_CLOSE                                        \
    }

    TEST_FUZZ_NUM_SSM_MUL(5, 10, 20, 100)
    TEST_FUZZ_NUM_SSM_MUL(6, 50, 100, 100)
    TEST_FUZZ_NUM_SSM_MUL(7, 500, 5000, 100)

    #undef TEST_FUZZ_NUM_SSM_MUL

    TEST_FN_CLOSE
}

static void test_fuzz_num_ssm_sqr(bool show)
{
    TEST_FN_OPEN

    #define TEST_FUZZ_NUM_SSM_SQR_COUNT(COUNT)          \
    {                                                   \
        num_p num = num_create_rand(COUNT);             \
        num_p num_res_1 = num_sqr_ssm(num_copy(num));   \
        num_p num_res_2 = num_sqr_classic(num);         \
        assert(num_eq_dbg(num_res_1, num_res_2));       \
    }

    #define TEST_FUZZ_NUM_SSM_SQR(TAG, COUNT, RUNS) \
    {                                               \
        TEST_FUZZ_CASE_OPEN(TAG, RUNS)              \
        {                                           \
            TEST_FUZZ_NUM_SSM_SQR_COUNT(COUNT)      \
        }                                           \
        TEST_FUZZ_CASE_CLOSE                        \
    }

    TEST_FUZZ_NUM_SSM_SQR(1,  500, 100)
    TEST_FUZZ_NUM_SSM_SQR(2, 1000,  10)
    TEST_FUZZ_NUM_SSM_SQR(3, 5000,   2)

    #undef TEST_FUZZ_NUM_SSM_MUL

    TEST_FN_CLOSE
}

static void test_fuzz_num_bz_div(bool show)
{
    TEST_FN_OPEN

    #define TEST_FUZZ_NUM_SSM_DIV(TAG, COUNT_1, COUNT_2, RUNS)              \
    {                                                                       \
        TEST_FUZZ_CASE_OPEN(TAG, RUNS)                                      \
        {                                                                   \
            num_p num_1 = num_create_rand(COUNT_1);                         \
            num_p num_2 = num_create_rand(COUNT_2);                         \
            num_p num_q, num_r;                                             \
            num_div_mod(&num_q, &num_r, num_copy(num_1), num_copy(num_2));  \
            num_p num_aux = num_mul(num_copy(num_q), num_copy(num_2));      \
            num_aux = num_add(num_aux, num_copy(num_r));                    \
            if(!num_eq_dbg(num_copy(num_aux), num_copy(num_1)))             \
            {                                                               \
                printf("\nentries");                                        \
                num_display_full("num_1", num_1);                           \
                num_display_full("num_2", num_2);                           \
                printf("\nreconstructed give");                             \
                num_display_full("num_aux", num_aux);                       \
                assert(false);                                              \
            }                                                               \
            num_free(num_1);                                                \
            num_free(num_2);                                                \
            num_free(num_aux);                                              \
            num_free(num_q);                                                \
            num_free(num_r);                                                \
        }                                                                   \
        TEST_FUZZ_CASE_CLOSE                                                \
    }

    TEST_FUZZ_NUM_SSM_DIV( 1,  3,  1, 100)
    TEST_FUZZ_NUM_SSM_DIV( 1,  3,  2, 100)
    TEST_FUZZ_NUM_SSM_DIV( 2,  4,  2, 100)
    TEST_FUZZ_NUM_SSM_DIV( 3,  5,  2, 100)
    TEST_FUZZ_NUM_SSM_DIV( 4,  8,  5, 100)
    TEST_FUZZ_NUM_SSM_DIV( 5, 10,  7, 100)
    TEST_FUZZ_NUM_SSM_DIV( 6, 32, 19, 100)
    TEST_FUZZ_NUM_SSM_DIV( 7, 20, 20,   5)
    TEST_FUZZ_NUM_SSM_DIV( 8, 30, 20,   5)
    TEST_FUZZ_NUM_SSM_DIV( 9, 40, 20,   5)
    TEST_FUZZ_NUM_SSM_DIV(10, 50, 20,   5)

    #undef TEST_FUZZ_NUM_SSM_DIV

    TEST_FN_CLOSE
}

[[maybe_unused]]
static void test_all(bool show)
{
    test_uint_from_char(show);
    test_uint128(show);

    test_num_create(show);
    test_num_expand_to(show);

    test_num_normalize(show);
    test_num_break(show);

    test_num_wrap(show);
    test_num_wrap_dec(show);
    test_num_wrap_hex(show);
    test_num_wrap_str(show);
    test_num_read_dec(show);
    test_num_unwrap(show);
    test_num_copy(show);

    test_num_add_uint_offset(show);
    test_num_sub_uint_offset(show);
    test_num_cmp_offset(show);
    test_num_sub_offset(show);

    test_num_shl_core(show);
    test_num_shr_core(show);
    test_num_mul_uint(show);

    test_smm_bit_inv(show);
    test_num_ssm_add_mod(show);
    test_num_ssm_sub_mod(show);
    test_num_ssm_opposite(show);
    test_num_ssm_pad_no_wrap(show);
    test_num_ssm_shl(show);
    test_num_ssm_shr(show);
    test_num_ssm_shl_mod(show);
    test_num_ssm_shr_mod(show);
    test_num_ssm_fft(show);
    test_num_ssm_depad_wrap(show);
    test_num_ssm_mul_wrap(show);

    test_num_div_normalize(show);

    test_num_is_zero(show);

    test_num_shl(show);
    test_num_shr(show);

    test_num_add(show);
    test_num_sub(show);
    test_num_mul(show);
    test_num_sqr(show);
    test_num_div_mod(show);
    test_num_gcd(show);

    test_num_div_mod_uint(show);

    test_num_base_to(show);
    test_num_base_from(show);

    test_fuzz_num_ssm_sh(show);
    test_fuzz_num_ssm_pad_no_wrap_round_trip(show);
    test_fuzz_num_ssm_pad_wrap_round_trip(show);
    test_fuzz_num_ssm_fft(show);
    test_fuzz_num_ssm_mul(show);
    test_fuzz_num_ssm_sqr(show);
    test_fuzz_num_bz_div(show);
}
