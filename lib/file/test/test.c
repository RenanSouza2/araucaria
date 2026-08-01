#include "../debug.h"
#include "../../../testrc.h"
#include "../../../mods/macros/test.h"

#include "../../num/debug.h"



static void test_sig_num_create(bool show)
{
    TEST_FN_OPEN

    #define TEST_SIG_NUM_CREATE(TAG, NUM, SIGNAL, SIGNAL_RES)   \
    {                                                           \
        TEST_CASE_OPEN(TAG)                                     \
        {                                                       \
            num_p num = num_create_immed(ARG_OPEN NUM);         \
            sig_num_t sig = sig_num_create(SIGNAL, num);        \
            assert(uint64(sig.signal, SIGNAL_RES));             \
            assert(uint64(sig.num->count, num->count));         \
            sig_num_free(sig);                                  \
        }                                                       \
        TEST_CASE_CLOSE                                         \
    }

    TEST_SIG_NUM_CREATE(1, (1, 1), POSITIVE, POSITIVE);
    TEST_SIG_NUM_CREATE(2, (1, 1), NEGATIVE, NEGATIVE);
    TEST_SIG_NUM_CREATE(3, (0), POSITIVE, ZERO);
    TEST_SIG_NUM_CREATE(4, (0), ZERO, ZERO);
    TEST_SIG_NUM_CREATE(5, (0), NEGATIVE, ZERO);

    TEST_CASE_OPEN(6)
    {
        num_p num = num_create_immed(1, 2);
        TEST_REVERT_OPEN
        {
            sig_num_create(ZERO, num);
        }
        TEST_REVERT_CLOSE
    }
    TEST_CASE_CLOSE

    TEST_FN_CLOSE
}

static void test_sig_num_create_immed(bool show)
{
    TEST_FN_OPEN

    TEST_CASE_OPEN(1)
    {
        sig_num_t sig = sig_num_create_immed(POSITIVE, 1, 1);
        assert(sig.signal == POSITIVE);
        assert(num_immed(sig.num, 1, 1));
    }
    TEST_CASE_CLOSE

    TEST_CASE_OPEN(2)
    {
        sig_num_t sig = sig_num_create_immed(NEGATIVE, 1, 1);
        assert(sig.signal == NEGATIVE);
        assert(num_immed(sig.num, 1, 1));
    }
    TEST_CASE_CLOSE

    TEST_CASE_OPEN(3)
    {
        sig_num_t sig = sig_num_create_immed(POSITIVE, 0);
        assert(sig.signal == ZERO);
        assert(num_immed(sig.num, 0));
    }
    TEST_CASE_CLOSE

    TEST_CASE_OPEN(4)
    {
        sig_num_t sig = sig_num_create_immed(ZERO, 0);
        assert(sig.signal == ZERO);
        assert(num_immed(sig.num, 0));
    }
    TEST_CASE_CLOSE

    TEST_CASE_OPEN(5)
    {
        sig_num_t sig = sig_num_create_immed(NEGATIVE, 0);
        assert(sig.signal == ZERO);
        assert(num_immed(sig.num, 0));
    }
    TEST_CASE_CLOSE

    TEST_FN_CLOSE
}



static void test_sig_num_wrap(bool show)
{
    TEST_FN_OPEN

    #define TEST_SIG_NUM_WRAP(TAG, VALUE, RES)          \
    {                                                   \
        TEST_CASE_OPEN(TAG)                             \
        {                                               \
            sig_num_t sig = sig_num_wrap(VALUE);        \
            assert(sig_num_immed(sig, ARG_OPEN RES));   \
        }                                               \
        TEST_CASE_CLOSE                                 \
    }

    TEST_SIG_NUM_WRAP(1,  0, (ZERO, 0))
    TEST_SIG_NUM_WRAP(2,  1, (POSITIVE, 1, 1))
    TEST_SIG_NUM_WRAP(3, -1, (NEGATIVE, 1, 1))
    TEST_SIG_NUM_WRAP(4, INT64_MAX, (POSITIVE, 1, INT64_MAX))
    TEST_SIG_NUM_WRAP(5, INT64_MIN, (NEGATIVE, 1, INT64_MIN))

    #undef TEST_SIG_NUM_WRAP

    TEST_FN_CLOSE
}

static void test_sig_num_wrap_int128(bool show)
{
    TEST_FN_OPEN

    #define TEST_SIG_NUM_WRAP_INT128(TAG, VALUE, RES)               \
    {                                                               \
        TEST_CASE_OPEN(TAG)                                         \
        {                                                           \
            sig_num_t sig = sig_num_wrap_int128((int128_t)(VALUE)); \
            assert(sig_num_immed(sig, ARG_OPEN RES));               \
        }                                                           \
        TEST_CASE_CLOSE                                             \
    }

    TEST_SIG_NUM_WRAP_INT128(1,  0, (ZERO, 0))
    TEST_SIG_NUM_WRAP_INT128(2,  1, (POSITIVE, 1, 1))
    TEST_SIG_NUM_WRAP_INT128(3,
        B(63),
        (POSITIVE, 1,  B(63))
    )
    TEST_SIG_NUM_WRAP_INT128(4,
        B(63) + 1,
        (POSITIVE, 1,  B(63) + 1)
    )
    TEST_SIG_NUM_WRAP_INT128(5,
        UINT64_MAX,
        (POSITIVE, 1, UINT64_MAX)
    )
    TEST_SIG_NUM_WRAP_INT128(6,
        B128(64),
        (POSITIVE, 2, 1, 0)
    )
    TEST_SIG_NUM_WRAP_INT128(7,
        B128(126),
        (POSITIVE, 2, B128(62), 0)
    )
    TEST_SIG_NUM_WRAP_INT128(8,
        INT128_MAX,
        (POSITIVE, 2, UINT64_MAX / 2, UINT64_MAX)
    )
    TEST_SIG_NUM_WRAP_INT128(9,
        -1,
        (NEGATIVE, 1, 1)
    )
    TEST_SIG_NUM_WRAP_INT128(10,
        INT64_MIN + 1,
        (NEGATIVE, 1,  INT64_MAX)
    )
    TEST_SIG_NUM_WRAP_INT128(11,
        INT64_MIN,
        (NEGATIVE, 1,  INT64_MIN)
    )
    TEST_SIG_NUM_WRAP_INT128(12,
        -B128(64),
        (NEGATIVE, 2, 1, 0)
    )
    TEST_SIG_NUM_WRAP_INT128(13,
        -B128(126),
        (NEGATIVE, 2, B128(62), 0)
    )
    TEST_SIG_NUM_WRAP_INT128(14,
        INT128_MIN,
        (NEGATIVE, 2, B(63), 0)
    )

    #undef TEST_SIG_NUM_WRAP_INT128

    TEST_FN_CLOSE
}

static void test_sig_num_wrap_str(bool show)
{
    TEST_FN_OPEN

    #define TEST_SIG_WRAP_STR(TAG, STR, ...)        \
    {                                               \
        TEST_CASE_OPEN(TAG)                         \
        {                                           \
            sig_num_t sig = sig_num_wrap_str(STR);       \
            assert(sig_num_immed(sig, __VA_ARGS__));  \
        }                                           \
        TEST_CASE_CLOSE                             \
    }

    TEST_SIG_WRAP_STR( 1, "", ZERO, 0);
    TEST_SIG_WRAP_STR( 2, "0", ZERO, 0);
    TEST_SIG_WRAP_STR( 3, "1", POSITIVE, 1, 1);
    TEST_SIG_WRAP_STR( 4, "10", POSITIVE, 1, 10);
    TEST_SIG_WRAP_STR( 5, "+", ZERO, 0);
    TEST_SIG_WRAP_STR( 6, "+0", ZERO, 0);
    TEST_SIG_WRAP_STR( 7, "+1", POSITIVE, 1, 1);
    TEST_SIG_WRAP_STR( 8, "+10", POSITIVE, 1, 10);
    TEST_SIG_WRAP_STR( 9, "-", ZERO, 0);
    TEST_SIG_WRAP_STR(10, "-0", ZERO, 0);
    TEST_SIG_WRAP_STR(11, "-1", NEGATIVE, 1, 1);
    TEST_SIG_WRAP_STR(12, "-10", NEGATIVE, 1, 10);
    TEST_SIG_WRAP_STR(13, "0x", ZERO, 0);
    TEST_SIG_WRAP_STR(14, "0x0", ZERO, 0);
    TEST_SIG_WRAP_STR(15, "0x1", POSITIVE, 1, 1);
    TEST_SIG_WRAP_STR(16, "0x10", POSITIVE, 1, 16);
    TEST_SIG_WRAP_STR(17, "+0x", ZERO, 0);
    TEST_SIG_WRAP_STR(18, "+0x0", ZERO, 0);
    TEST_SIG_WRAP_STR(19, "+0x1", POSITIVE, 1, 1);
    TEST_SIG_WRAP_STR(20, "+0x10", POSITIVE, 1, 16);
    TEST_SIG_WRAP_STR(21, "-0x", ZERO, 0);
    TEST_SIG_WRAP_STR(22, "-0x0", ZERO, 0);
    TEST_SIG_WRAP_STR(23, "-0x1", NEGATIVE, 1, 1);
    TEST_SIG_WRAP_STR(24, "-0x10", NEGATIVE, 1, 16);

    #undef TEST_SIG_WRAP_STR

    TEST_FN_CLOSE
}

static void test_sig_num_copy(bool show)
{
    TEST_FN_OPEN

    #define TEST_SIG_COPY(TAG, ...)                             \
    {                                                           \
        TEST_CASE_OPEN(TAG)                                     \
        {                                                       \
            sig_num_t sig = sig_num_create_immed(__VA_ARGS__);  \
            sig_num_t sig_num_res = sig_num_copy(sig);          \
            assert(sig_num_immed(sig_num_res, __VA_ARGS__))     \
            sig_num_free(sig);                                  \
        }                                                       \
        TEST_CASE_CLOSE                                         \
    }

    TEST_SIG_COPY(1, POSITIVE, 0);

    TEST_SIG_COPY(2, POSITIVE, 1, 1);
    TEST_SIG_COPY(3, POSITIVE, 2, 1, 2);

    TEST_SIG_COPY(4, NEGATIVE, 1, 1);
    TEST_SIG_COPY(5, NEGATIVE, 2, 1, 2);

    #undef TEST_SIG_COPY

    TEST_FN_CLOSE
}



static void test_sig_num_is_zero(bool show)
{
    TEST_FN_OPEN

    #define TEST_SIG_IS_ZERO(TAG, NUM, RES)     \
    {                                           \
        TEST_CASE_OPEN(TAG)                     \
        {                                       \
            sig_num_t sig = sig_num_wrap(NUM);  \
            int64_t res = sig_num_is_zero(sig); \
            assert(int64(res, RES));            \
            sig_num_free(sig);                  \
        }                                       \
        TEST_CASE_CLOSE                         \
    }

    TEST_SIG_IS_ZERO(1,  1, false);
    TEST_SIG_IS_ZERO(2,  0, true);
    TEST_SIG_IS_ZERO(3, -1, false);

    #undef TEST_SIG_IS_ZERO

    TEST_FN_CLOSE
}

static void test_sig_num_cmp(bool show)
{
    TEST_FN_OPEN

    #define TEST_SIG_CMP(TAG, NUM_1, NUM_2, RES)        \
    {                                                   \
        TEST_CASE_OPEN(TAG)                             \
        {                                               \
            sig_num_t sig_1 = sig_num_wrap(NUM_1);      \
            sig_num_t sig_2 = sig_num_wrap(NUM_2);      \
            int64_t res = sig_num_cmp(sig_1, sig_2);    \
            assert(int64(res, RES));                    \
            sig_num_free(sig_1);                        \
            sig_num_free(sig_2);                        \
        }                                               \
        TEST_CASE_CLOSE                                 \
    }

    TEST_SIG_CMP( 1,  2,  3, -1);
    TEST_SIG_CMP( 2,  2,  2,  0);
    TEST_SIG_CMP( 3,  2,  1,  1);
    TEST_SIG_CMP( 4,  2,  0,  1);
    TEST_SIG_CMP( 5,  2, -1,  1);
    TEST_SIG_CMP( 6,  2, -2,  1);
    TEST_SIG_CMP( 7,  2, -3,  1);

    TEST_SIG_CMP( 8,  0,  1, -1);
    TEST_SIG_CMP( 9,  0,  0,  0);
    TEST_SIG_CMP(10,  0, -1,  1);

    TEST_SIG_CMP(11, -2,  2, -1);
    TEST_SIG_CMP(12, -2,  3, -1);
    TEST_SIG_CMP(13, -2,  1, -1);
    TEST_SIG_CMP(14, -2,  0, -1);
    TEST_SIG_CMP(15, -2, -1, -1);
    TEST_SIG_CMP(16, -2, -2,  0);
    TEST_SIG_CMP(17, -2, -3,  1);

    #undef TEST_SIG_CMP

    TEST_FN_CLOSE
}



static void test_sig_num_shl(bool show)
{
    TEST_FN_OPEN

    #define TEST_SIG_NUM_SHL(TAG, SIG_NUM_BEF, BITS, SIG_NUM_AFT)       \
    {                                                                   \
        TEST_CASE_OPEN(TAG)                                             \
        {                                                               \
            sig_num_t sig = sig_num_create_immed(ARG_OPEN SIG_NUM_BEF); \
            sig = sig_num_shl(sig, BITS);                               \
            assert(sig_num_immed(sig, ARG_OPEN SIG_NUM_AFT));           \
        }                                                               \
        TEST_CASE_CLOSE                                                 \
    }

    TEST_SIG_NUM_SHL(1, (ZERO, 0), 0, (ZERO, 0));
    TEST_SIG_NUM_SHL(2, (ZERO, 0), 1, (ZERO, 0));
    TEST_SIG_NUM_SHL(3, (ZERO, 0), 64, (ZERO, 0));
    TEST_SIG_NUM_SHL(4, (ZERO, 0), 65, (ZERO, 0));
    TEST_SIG_NUM_SHL(5, (POSITIVE, 1, 1), 0, (POSITIVE, 1, 1));
    TEST_SIG_NUM_SHL(6, (POSITIVE, 1, 1), 1, (POSITIVE, 1, 2));
    TEST_SIG_NUM_SHL(7, (POSITIVE, 1, 1), 63, (POSITIVE, 1, 0x8000000000000000));
    TEST_SIG_NUM_SHL(8, (POSITIVE, 1, 1), 64, (POSITIVE, 2, 1, 0));
    TEST_SIG_NUM_SHL(9, (POSITIVE, 1, 1), 65, (POSITIVE, 2, 2, 0));

    #undef TEST_SIG_NUM_SHL

    TEST_FN_CLOSE
}

static void test_sig_num_shr(bool show)
{
    TEST_FN_OPEN

    #define TEST_SIG_NUM_SHR(TAG, SIG_NUM_BEF, BITS, SIG_NUM_AFT)       \
    {                                                                   \
        TEST_CASE_OPEN(TAG)                                             \
        {                                                               \
            sig_num_t sig = sig_num_create_immed(ARG_OPEN SIG_NUM_BEF); \
            sig = sig_num_shr(sig, BITS);                               \
            assert(sig_num_immed(sig, ARG_OPEN SIG_NUM_AFT));           \
        }                                                               \
        TEST_CASE_CLOSE                                                 \
    }

    TEST_SIG_NUM_SHR(1, (ZERO, 0), 0, (ZERO, 0));
    TEST_SIG_NUM_SHR(2, (ZERO, 0), 1, (ZERO, 0));
    TEST_SIG_NUM_SHR(3, (ZERO, 0), 63, (ZERO, 0));
    TEST_SIG_NUM_SHR(4, (ZERO, 0), 64, (ZERO, 0));
    TEST_SIG_NUM_SHR(5, (ZERO, 0), 65, (ZERO, 0));
    TEST_SIG_NUM_SHR(6, (POSITIVE, 1, 1), 0, (POSITIVE, 1, 1));
    TEST_SIG_NUM_SHR(7, (POSITIVE, 1, 1), 1, (ZERO, 0));
    TEST_SIG_NUM_SHR(8, (POSITIVE, 2, 2, 0), 0, (POSITIVE, 2, 2, 0));
    TEST_SIG_NUM_SHR(9, (POSITIVE, 2, 2, 0), 1, (POSITIVE, 2, 1, 0));
    TEST_SIG_NUM_SHR(10, (POSITIVE, 2, 2, 0), 2, (POSITIVE, 1, 0x8000000000000000));
    TEST_SIG_NUM_SHR(11, (POSITIVE, 2, 2, 0), 64, (POSITIVE, 1, 2));
    TEST_SIG_NUM_SHR(11, (POSITIVE, 2, 2, 0), 65, (POSITIVE, 1, 1));

    #undef TEST_SIG_NUM_SHR

    TEST_FN_CLOSE
}



static void test_sig_num_opposite(bool show)
{
    TEST_FN_OPEN

    #define TEST_SIG_OPPOSITE(TAG, SIGNAL_IN, SIGNAL_OUT, ...)              \
    {                                                                       \
        TEST_CASE_OPEN(TAG)                                                 \
        {                                                                   \
            sig_num_t sig = sig_num_create_immed(SIGNAL_IN, __VA_ARGS__);   \
            sig = sig_num_opposite(sig);                                    \
            assert(sig_num_immed(sig, SIGNAL_OUT, __VA_ARGS__));            \
        }                                                                   \
        TEST_CASE_CLOSE                                                     \
    }

    TEST_SIG_OPPOSITE(1, ZERO, ZERO, 0);
    TEST_SIG_OPPOSITE(2, POSITIVE, NEGATIVE, 1, 1);
    TEST_SIG_OPPOSITE(3, NEGATIVE, POSITIVE, 1, 1);

    TEST_FN_CLOSE
}

static void test_sig_num_add(bool show)
{
    TEST_FN_OPEN

    #define TEST_SIG_NUM_ADD(TAG, SIG_NUM_1, SIG_NUM_2, SIG_NUM_OUT)    \
    {                                                                   \
        TEST_CASE_OPEN(TAG)                                             \
        {                                                               \
            sig_num_t sig_1 = sig_num_create_immed(ARG_OPEN SIG_NUM_1); \
            sig_num_t sig_2 = sig_num_create_immed(ARG_OPEN SIG_NUM_2); \
            sig_1 = sig_num_add(sig_1, sig_2);                          \
            assert(sig_num_immed(sig_1, ARG_OPEN SIG_NUM_OUT));         \
        }                                                               \
        TEST_CASE_CLOSE                                                 \
    }

    TEST_SIG_NUM_ADD(1,
        (ZERO, 0),
        (ZERO, 0),
        (ZERO, 0)
    );
    TEST_SIG_NUM_ADD(2,
        (ZERO, 0),
        (POSITIVE, 1, 1),
        (POSITIVE, 1, 1)
    );
    TEST_SIG_NUM_ADD(3,
        (ZERO, 0),
        (NEGATIVE, 1, 1),
        (NEGATIVE, 1, 1)
    );
    TEST_SIG_NUM_ADD(4,
        (POSITIVE, 1, 2),
        (POSITIVE, 1, 1),
        (POSITIVE, 1, 3)
    );
    TEST_SIG_NUM_ADD(5,
        (POSITIVE, 1, 2),
        (ZERO, 0),
        (POSITIVE, 1, 2)
    );
    TEST_SIG_NUM_ADD(6,
        (POSITIVE, 1, 2),
        (NEGATIVE, 1, 1),
        (POSITIVE, 1, 1)
    );
    TEST_SIG_NUM_ADD(7,
        (POSITIVE, 1, 2),
        (NEGATIVE, 1, 2),
        (ZERO, 0)
    );
    TEST_SIG_NUM_ADD(8,
        (POSITIVE, 1, 2),
        (NEGATIVE, 1, 3),
        (NEGATIVE, 1, 1)
    );
    TEST_SIG_NUM_ADD(9,
        (NEGATIVE, 1, 2),
        (NEGATIVE, 1, 1),
        (NEGATIVE, 1, 3)
    );
    TEST_SIG_NUM_ADD(10,
        (NEGATIVE, 1, 2),
        (ZERO, 0),
        (NEGATIVE, 1, 2)
    );
    TEST_SIG_NUM_ADD(11,
        (NEGATIVE, 1, 2),
        (POSITIVE, 1, 1),
        (NEGATIVE, 1, 1)
    );
    TEST_SIG_NUM_ADD(12,
        (NEGATIVE, 1, 2),
        (POSITIVE, 1, 2),
        (ZERO, 0)
    );
    TEST_SIG_NUM_ADD(13,
        (NEGATIVE, 1, 2),
        (POSITIVE, 1, 3),
        (POSITIVE, 1, 1)
    );

    #undef TEST_SIG_NUM_ADD

    TEST_FN_CLOSE
}

static void test_sig_num_sub(bool show)
{
    TEST_FN_OPEN

    #define TESTS_SIG_NUM_SUB(TAG, SIG_NUM_1, SIG_NUM_2, SIG_NUM_OUT)   \
    {                                                                   \
        TEST_CASE_OPEN(TAG)                                             \
        {                                                               \
            sig_num_t sig_1 = sig_num_create_immed(ARG_OPEN SIG_NUM_1); \
            sig_num_t sig_2 = sig_num_create_immed(ARG_OPEN SIG_NUM_2); \
            sig_1 = sig_num_sub(sig_1, sig_2);                          \
            assert(sig_num_immed(sig_1, ARG_OPEN SIG_NUM_OUT));         \
        }                                                               \
        TEST_CASE_CLOSE                                                 \
    }

    TESTS_SIG_NUM_SUB(1,
        (POSITIVE, 1, 3),
        (POSITIVE, 1, 1),
        (POSITIVE, 1, 2)
    );
    TESTS_SIG_NUM_SUB(2,
        (POSITIVE, 1, 3),
        (ZERO, 0),
        (POSITIVE, 1, 3)
    );
    TESTS_SIG_NUM_SUB(1,
        (POSITIVE, 1, 3),
        (NEGATIVE, 1, 1),
        (POSITIVE, 1, 4)
    );

    #undef TESTS_SIG_NUM_SUB

    TEST_FN_CLOSE
}

static void test_sig_num_mul(bool show)
{
    TEST_FN_OPEN

    #define TEST_SIG_NUM_MUL(TAG, SIG_NUM_1, SIG_NUM_2, SIG_NUM_OUT)    \
    {                                                                   \
        TEST_CASE_OPEN(TAG)                                             \
        {                                                               \
            sig_num_t sig_1 = sig_num_create_immed(ARG_OPEN SIG_NUM_1); \
            sig_num_t sig_2 = sig_num_create_immed(ARG_OPEN SIG_NUM_2); \
            sig_1 = sig_num_mul(sig_1, sig_2);                          \
            assert(sig_num_immed(sig_1, ARG_OPEN SIG_NUM_OUT));         \
        }                                                               \
        TEST_CASE_CLOSE                                                 \
    }

    TEST_SIG_NUM_MUL(1,
        (POSITIVE, 1, 2),
        (POSITIVE, 1, 3),
        (POSITIVE, 1, 6)
    );
    TEST_SIG_NUM_MUL(2,
        (POSITIVE, 1, 2),
        (ZERO, 0),
        (ZERO, 0)
    );
    TEST_SIG_NUM_MUL(3,
        (NEGATIVE, 1, 2),
        (ZERO, 0),
        (ZERO, 0)
    );
    TEST_SIG_NUM_MUL(4,
        (ZERO, 0),
        (POSITIVE, 1, 2),
        (ZERO, 0)
    );
    TEST_SIG_NUM_MUL(5,
        (ZERO, 0),
        (NEGATIVE, 1, 2),
        (ZERO, 0)
    );
    TEST_SIG_NUM_MUL(6,
        (POSITIVE, 1, 2),
        (NEGATIVE, 1, 3),
        (NEGATIVE, 1, 6)
    );
    TEST_SIG_NUM_MUL(7,
        (NEGATIVE, 1, 2),
        (POSITIVE, 1, 3),
        (NEGATIVE, 1, 6)
    );
    TEST_SIG_NUM_MUL(8,
        (NEGATIVE, 1, 2),
        (NEGATIVE, 1, 3),
        (POSITIVE, 1, 6)
    );

    #undef TEST_SIG_NUM_MUL

    TEST_FN_CLOSE
}

static void test_sig_num_div(bool show)
{
    TEST_FN_OPEN

    #define TEST_SIG_NUM_DIV(TAG, SIG_NUM_1, SIG_NUM_2, SIG_NUM_OUT)    \
    {                                                                   \
        TEST_CASE_OPEN(TAG)                                             \
        {                                                               \
            sig_num_t sig_1 = sig_num_create_immed(ARG_OPEN SIG_NUM_1); \
            sig_num_t sig_2 = sig_num_create_immed(ARG_OPEN SIG_NUM_2); \
            sig_1 = sig_num_div(sig_1, sig_2);                          \
            assert(sig_num_immed(sig_1, ARG_OPEN SIG_NUM_OUT));         \
        }                                                               \
        TEST_CASE_CLOSE                                                 \
    }

    TEST_SIG_NUM_DIV(1,
        (POSITIVE, 1, 2),
        (POSITIVE, 1, 3),
        (ZERO, 0)
    );
    TEST_SIG_NUM_DIV(2,
        (POSITIVE, 1, 2),
        (NEGATIVE, 1, 3),
        (ZERO, 0)
    );
    TEST_SIG_NUM_DIV(3,
        (POSITIVE, 1, 3),
        (POSITIVE, 1, 2),
        (POSITIVE, 1, 1)
    );
    TEST_SIG_NUM_DIV(4,
        (ZERO, 0),
        (POSITIVE, 1, 2),
        (ZERO, 0)
    );
    TEST_SIG_NUM_DIV(5,
        (NEGATIVE, 1, 3),
        (POSITIVE, 1, 2),
        (NEGATIVE, 1, 1)
    );

    #undef TEST_SIG_NUM_DIV

    TEST_FN_CLOSE
}



static void test_sig_num()
{
    TEST_LIB

    bool show = false;

    test_sig_num_create(show);
    test_sig_num_create_immed(show);

    test_sig_num_wrap(show);
    test_sig_num_wrap_int128(show);
    test_sig_num_wrap_str(show);
    test_sig_num_copy(show);

    test_sig_num_is_zero(show);
    test_sig_num_cmp(show);

    test_sig_num_shl(show);
    test_sig_num_shr(show);

    test_sig_num_opposite(show);
    test_sig_num_add(show);
    test_sig_num_sub(show);
    test_sig_num_mul(show);
    test_sig_num_div(show);

    TEST_ASSERT_MEM_EMPTY
}



int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    test_sig_num();
    printf("\n\n\tTest successful\n\n");
    return 0;
}
