#include "../debug.h"
#include "../../../testrc.h"
#include "../../../mods/macros/test.h"

#include "../../num/debug.h"
#include "../../sig/debug.h"



#define FLT_NUM_ZERO(SIZE) (0, SIZE, ZERO, 0)

static void test_int64_get_sign(bool show)
{
    TEST_FN_OPEN

    #define TEST_INT64_GET_SIGN(TAG, IN, RES)   \
    {                                           \
        TEST_CASE_OPEN(TAG)                     \
        {                                       \
            uint64_t res = int64_get_sign(IN);  \
            assert(uint64(res, RES));           \
        }                                       \
        TEST_CASE_CLOSE                         \
    }

    TEST_INT64_GET_SIGN(1, INT64_MIN, NEGATIVE);
    TEST_INT64_GET_SIGN(2, -1, NEGATIVE);
    TEST_INT64_GET_SIGN(3, 0, ZERO);
    TEST_INT64_GET_SIGN(4, 1, POSITIVE);
    TEST_INT64_GET_SIGN(5, INT64_MAX, POSITIVE);

    #undef TEST_INT64_GET_SIGN

    TEST_FN_CLOSE
}

static void test_int64_add(bool show)
{
    TEST_FN_OPEN

    #define TEST_INT64_ADD(TAG, A, B, RES)  \
    {                                       \
        TEST_CASE_OPEN(TAG)                 \
        {                                   \
            int64_t res = int64_add(A, B);  \
            assert(int64(res, RES));        \
        }                                   \
        TEST_CASE_CLOSE                     \
    }

    for(int64_t a=-2; a<3; a++)
    for(int64_t b=-2; b<3; b++)
    {
        TEST_INT64_ADD(13 + 5*a + b, a, b, a + b);
    }

    TEST_INT64_ADD(26, INT64_MIN, 0, INT64_MIN);
    TEST_INT64_ADD(27, INT64_MIN + 1, -1, INT64_MIN);
    TEST_INT64_ADD(28, INT64_MAX, 0, INT64_MAX);
    TEST_INT64_ADD(29, INT64_MAX - 1, 1, INT64_MAX);
    TEST_INT64_ADD(31, INT64_MIN, INT64_MAX, -1);
    TEST_INT64_ADD(32, INT64_MAX, INT64_MIN, -1);

    #undef TEST_INT64_ADD

    #define TEST_INT64_ADD(TAG, A, B)   \
    {                                   \
        TEST_CASE_OPEN(TAG)             \
        {                               \
            TEST_REVERT_OPEN            \
            {                           \
                int64_add(A, B);        \
            }                           \
            TEST_REVERT_CLOSE           \
        }                               \
        TEST_CASE_CLOSE                 \
    }

    TEST_INT64_ADD(33, INT64_MIN, -1);
    TEST_INT64_ADD(34, INT64_MAX, 1);
    TEST_INT64_ADD(35, INT64_MIN, INT64_MIN);
    TEST_INT64_ADD(36, INT64_MAX, INT64_MAX);

    #undef TEST_INT64_ADD

    TEST_FN_CLOSE
}



static void test_flt_num_normalize(bool show)
{
    TEST_FN_OPEN

    #define TEST_FLT_NUM_NORMALIZE(TAG, FLT, EXPONENT, SIZE, SIG) \
    {                                                               \
        TEST_CASE_OPEN(TAG)                                         \
        {                                                           \
            flt_num_t flt = flt_num_create_immed(ARG_OPEN FLT); \
            flt = flt_num_normalize(flt);                         \
            assert(int64(flt.exponent, EXPONENT));                  \
            assert(uint64(flt.size, SIZE));                         \
            assert(sig_num_immed(flt.sig, ARG_OPEN SIG));           \
        }                                                           \
        TEST_CASE_CLOSE                                             \
    }

    TEST_FLT_NUM_NORMALIZE(1, FLT_NUM_ZERO(2), 0, 2, (ZERO, 0));
    TEST_FLT_NUM_NORMALIZE(2, (0, 2, POSITIVE, 2, 1, 0), 0, 2, (POSITIVE, 2, 1, 0));
    TEST_FLT_NUM_NORMALIZE(3, (0, 2, POSITIVE, 1, 1), -1, 2, (POSITIVE, 2, 1, 0));
    TEST_FLT_NUM_NORMALIZE(4, (0, 2, NEGATIVE, 1, 1), -1, 2, (NEGATIVE, 2, 1, 0));
    TEST_FLT_NUM_NORMALIZE(5, (0, 2, POSITIVE, 3, 1, 2, 3), 1, 2, (POSITIVE, 2, 1, 2));

    #undef TEST_FLT_NUM_NORMALIZE

    #define TEST_FLT_NUM_NORMALIZE(TAG, FLT)                      \
    {                                                               \
        TEST_CASE_OPEN(TAG)                                         \
        {                                                           \
            flt_num_t flt = flt_num_create_immed(ARG_OPEN FLT); \
            TEST_REVERT_OPEN                                        \
            {                                                       \
                flt_num_normalize(flt);                           \
            }                                                       \
            TEST_REVERT_CLOSE                                       \
        }                                                           \
        TEST_CASE_CLOSE                                             \
    }

    TEST_FLT_NUM_NORMALIZE(6, (INT64_MIN, 2, POSITIVE, 1, 1));
    TEST_FLT_NUM_NORMALIZE(7, (INT64_MAX, 2, POSITIVE, 3, 1, 0, 1));

    #undef TEST_FLT_NUM_NORMALIZE

    TEST_FN_CLOSE
}



static void test_flt_num_set_exponent(bool show)
{
    TEST_FN_OPEN

    #define TEST_FLT_NUM_SET_EXPONENT(TAG, FLT, EXPONENT, RES)    \
    {                                                               \
        TEST_CASE_OPEN(TAG)                                         \
        {                                                           \
            flt_num_t flt = flt_num_create_immed(ARG_OPEN FLT); \
            flt = flt_num_set_exponent(flt, EXPONENT);            \
            assert(flt_num_immed(flt, ARG_OPEN RES));             \
        }                                                           \
        TEST_CASE_CLOSE                                             \
    }

    TEST_FLT_NUM_SET_EXPONENT(1, FLT_NUM_ZERO(2), 0, FLT_NUM_ZERO(2));
    TEST_FLT_NUM_SET_EXPONENT(2, FLT_NUM_ZERO(2), 1, (1, 2, ZERO, 0));
    TEST_FLT_NUM_SET_EXPONENT(3, FLT_NUM_ZERO(2), -1, (-1, 2, ZERO, 0));
    TEST_FLT_NUM_SET_EXPONENT(4, (0, 2, POSITIVE, 2, 1, 2), 0, (0, 2, POSITIVE, 2, 1, 2));
    TEST_FLT_NUM_SET_EXPONENT(5, (0, 2, POSITIVE, 2, 1, 2), -1, (-1, 2, POSITIVE, 3, 1, 2, 0));
    TEST_FLT_NUM_SET_EXPONENT(6, (0, 2, POSITIVE, 2, 1, 2), 2, (2, 2, ZERO, 0));
    TEST_FLT_NUM_SET_EXPONENT(7, (0, 3, POSITIVE, 3, 1, 2, 3), 1, (1, 3, POSITIVE, 2, 1, 2));

    #undef TEST_FLT_NUM_SET_EXPONENT

    TEST_FN_CLOSE
}



static void test_flt_num_is_zero(bool show)
{
    TEST_FN_OPEN

    #define TEST_FLT_NUM_IS_ZERO(TAG, FLT, RES)                   \
    {                                                               \
        TEST_CASE_OPEN(TAG)                                         \
        {                                                           \
            flt_num_t flt = flt_num_create_immed(ARG_OPEN FLT); \
            bool res = flt_num_is_zero(flt);                      \
            assert(res == RES);                                     \
            flt_num_free(flt);                                    \
        }                                                           \
        TEST_CASE_CLOSE                                             \
    }

    TEST_FLT_NUM_IS_ZERO(1, FLT_NUM_ZERO(2), true);
    TEST_FLT_NUM_IS_ZERO(2, (0, 2, POSITIVE, 2, 1, 0), false);

    #undef TEST_FLT_NUM_IS_ZERO

    TEST_FN_CLOSE
}


static void test_flt_num_shr(bool show)
{
    TEST_FN_OPEN

    #define TEST_FLT_NUM_shr(TAG, FLT, BITS, RES)                  \
    {                                                                   \
        TEST_CASE_OPEN(TAG)                                             \
        {                                                               \
            flt_num_t flt = flt_num_create_immed(ARG_OPEN FLT); \
            flt = flt_num_shr(flt, BITS);                        \
            assert(flt_num_immed(flt, ARG_OPEN RES))                \
        }                                                               \
        TEST_CASE_CLOSE                                                 \
    }

    TEST_FLT_NUM_shr(1, FLT_NUM_ZERO(2), 0, FLT_NUM_ZERO(2));
    TEST_FLT_NUM_shr(2, FLT_NUM_ZERO(2), 1, FLT_NUM_ZERO(2));
    TEST_FLT_NUM_shr(3, FLT_NUM_ZERO(2), 64, FLT_NUM_ZERO(2));
    TEST_FLT_NUM_shr(4, FLT_NUM_ZERO(2), 65, FLT_NUM_ZERO(2));
    TEST_FLT_NUM_shr(5, (0, 2, POSITIVE, 2, 1, 0), 0, (0, 2, POSITIVE, 2, 1, 0));
    TEST_FLT_NUM_shr(6, (0, 2, POSITIVE, 2, 1, 0), 1, (-1, 2, POSITIVE, 2, 0x8000000000000000, 0));
    TEST_FLT_NUM_shr(7, (0, 2, POSITIVE, 2, 1, 0), 64,(-1, 2, POSITIVE, 2, 1, 0));
    TEST_FLT_NUM_shr(8, (0, 2, POSITIVE, 2, 1, 0), 65, (-2, 2, POSITIVE, 2, 0x8000000000000000, 0));
    TEST_FLT_NUM_shr(9, (0, 2, POSITIVE, 2, 2, 0), 0, (0, 2, POSITIVE, 2, 2, 0));
    TEST_FLT_NUM_shr(10, (0, 2, POSITIVE, 2, 2, 0), 1, (0, 2, POSITIVE, 2, 1, 0));
    TEST_FLT_NUM_shr(11, (0, 2, POSITIVE, 2, 2, 0), 2, (-1, 2, POSITIVE, 2, 0x8000000000000000, 0));
    TEST_FLT_NUM_shr(12, (0, 2, POSITIVE, 2, 3, 0), 0, (0, 2, POSITIVE, 2, 3, 0));
    TEST_FLT_NUM_shr(13, (0, 2, POSITIVE, 2, 3, 0), 1, (0, 2, POSITIVE, 2, 1, 0x8000000000000000));
    TEST_FLT_NUM_shr(14, (0, 2, POSITIVE, 2, 3, 0), 2, (-1, 2, POSITIVE, 2, 0xc000000000000000, 0));
    TEST_FLT_NUM_shr(15,
        (0, 2, POSITIVE, 2, 2, 1),
        1,
        (0, 2, POSITIVE, 2, 1, 0)
    );
    TEST_FLT_NUM_shr(16,
        (0, 2, POSITIVE, 2, 3, 0x8000000000000000),
        1,
        (0, 2, POSITIVE, 2, 1, 0xc000000000000000)
    );

    #undef TEST_FLT_NUM_shr

    TEST_FN_CLOSE
}



static void test_flt_num_add(bool show)
{
    TEST_FN_OPEN

    #define TEST_FLT_NUM_ADD(TAG, FLT_1, FLT_2, RES)                  \
    {                                                                   \
        TEST_CASE_OPEN(TAG)                                             \
        {                                                               \
            flt_num_t flt_1 = flt_num_create_immed(ARG_OPEN FLT_1); \
            flt_num_t flt_2 = flt_num_create_immed(ARG_OPEN FLT_2); \
            flt_1 = flt_num_add(flt_1, flt_2);                        \
            assert(flt_num_immed(flt_1, ARG_OPEN RES))                \
        }                                                               \
        TEST_CASE_CLOSE                                                 \
    }

    TEST_FLT_NUM_ADD(1,
        FLT_NUM_ZERO(2),
        FLT_NUM_ZERO(2),
        FLT_NUM_ZERO(2)
    );
    TEST_FLT_NUM_ADD(2,
        FLT_NUM_ZERO(2),
        (0, 2, POSITIVE, 1, 1, 0),
        (0, 2, POSITIVE, 1, 1, 0)
    );
    TEST_FLT_NUM_ADD(3,
        (0, 2, POSITIVE, 2, 1, 0),
        FLT_NUM_ZERO(2),
        (0, 2, POSITIVE, 2, 1, 0)
    );
    TEST_FLT_NUM_ADD(4,
        (0, 2, POSITIVE, 2, 1, 0),
        (0, 2, POSITIVE, 2, 2, 0),
        (0, 2, POSITIVE, 2, 3, 0)
    );
    TEST_FLT_NUM_ADD(5,
        ( 0, 2, POSITIVE, 2, 1, 0),
        ( 0, 2, NEGATIVE, 2, 1, 0),
        (-1, 2, ZERO, 0)
    );
    TEST_FLT_NUM_ADD(6,
        (2, 2, POSITIVE, 2, 1, 0),
        (0, 2, POSITIVE, 2, 1, 0),
        (2, 2, POSITIVE, 2, 1, 0)
    );
    TEST_FLT_NUM_ADD(7,
        (1, 2, POSITIVE, 2, 1, 0),
        (0, 2, POSITIVE, 2, 1, 0),
        (1, 2, POSITIVE, 2, 1, 1)
    );
    TEST_FLT_NUM_ADD(8,
        (1, 2, POSITIVE, 2, 1, 1),
        (1, 2, NEGATIVE, 2, 1, 0),
        (0, 2, POSITIVE, 2, 1, 0)
    );
    TEST_FLT_NUM_ADD(9,
        (0, 2, POSITIVE, 2, 1, 0),
        (2, 2, POSITIVE, 2, 1, 0),
        (2, 2, POSITIVE, 2, 1, 0)
    );
    TEST_FLT_NUM_ADD(10,
        (0, 2, POSITIVE, 2, 1, 0),
        (1, 2, POSITIVE, 2, 1, 0),
        (1, 2, POSITIVE, 2, 1, 1)
    );
    TEST_FLT_NUM_ADD(11,
        (0, 2, POSITIVE, 2, 1, 0),
        (1, 2, POSITIVE, 2, 1, 0),
        (1, 2, POSITIVE, 2, 1, 1)
    );

    #undef TEST_FLT_NUM_ADD

    TEST_FN_CLOSE
}

static void test_flt_num_sub(bool show)
{
    TEST_FN_OPEN

    #define TEST_FLT_NUM_SUB(TAG, FLT_1, FLT_2, RES)                  \
    {                                                                   \
        TEST_CASE_OPEN(TAG)                                             \
        {                                                               \
            flt_num_t flt_1 = flt_num_create_immed(ARG_OPEN FLT_1); \
            flt_num_t flt_2 = flt_num_create_immed(ARG_OPEN FLT_2); \
            flt_1 = flt_num_sub(flt_1, flt_2);                        \
            assert(flt_num_immed(flt_1, ARG_OPEN RES))                \
        }                                                               \
        TEST_CASE_CLOSE                                                 \
    }

    TEST_FLT_NUM_SUB(1,
        FLT_NUM_ZERO(2),
        FLT_NUM_ZERO(2),
        FLT_NUM_ZERO(2)
    );
    TEST_FLT_NUM_SUB(2,
        FLT_NUM_ZERO(2),
        (0, 2, POSITIVE, 2, 1, 0),
        (0, 2, NEGATIVE, 2, 1, 0)
    );

    #undef TEST_FLT_NUM_SUB

    TEST_FN_CLOSE
}

static void test_flt_num_mul(bool show)
{
    TEST_FN_OPEN

    #define TEST_FLT_NUM_MUL(TAG, FLT_1, FLT_2, RES)                \
    {                                                               \
        TEST_CASE_OPEN(TAG)                                         \
        {                                                           \
            flt_num_t flt_1 = flt_num_create_immed(ARG_OPEN FLT_1); \
            flt_num_t flt_2 = flt_num_create_immed(ARG_OPEN FLT_2); \
            flt_1 = flt_num_mul(flt_1, flt_2);                      \
            assert(flt_num_immed(flt_1, ARG_OPEN RES))              \
        }                                                           \
        TEST_CASE_CLOSE                                             \
    }

    TEST_FLT_NUM_MUL(1,
        FLT_NUM_ZERO(2),
        FLT_NUM_ZERO(2),
        (0, 2, ZERO, 0)
    );
    TEST_FLT_NUM_MUL(2,
        FLT_NUM_ZERO(2),
        (-1, 2, POSITIVE, 2, 1, 0),
        (-1, 2, ZERO, 0)
    );
    TEST_FLT_NUM_MUL(3,
        (-1, 2, POSITIVE, 2, 1, 0),
        FLT_NUM_ZERO(2),
        (-1, 2, ZERO, 0)
    );
    TEST_FLT_NUM_MUL(4,
        (-1, 2, POSITIVE, 2, 1, 0),
        (-1, 2, POSITIVE, 2, 1, 0),
        (-1, 2, POSITIVE, 2, 1, 0)
    );

    #undef TEST_FLT_NUM_MUL

    TEST_FN_CLOSE
}

static void test_flt_num_sqr(bool show)
{
    TEST_FN_OPEN

    #define TEST_FLT_NUM_SQR(TAG, FLT, RES)                     \
    {                                                           \
        TEST_CASE_OPEN(TAG)                                     \
        {                                                       \
            flt_num_t flt = flt_num_create_immed(ARG_OPEN FLT); \
            flt = flt_num_sqr(flt);                             \
            assert(flt_num_immed(flt, ARG_OPEN RES))            \
        }                                                       \
        TEST_CASE_CLOSE                                         \
    }

    TEST_FLT_NUM_SQR(1,
        (-1, 2, POSITIVE, 2, 3, 0),
        (-1, 2, POSITIVE, 2, 9, 0)
    );
    TEST_FLT_NUM_SQR(2,
        (-2, 2, NEGATIVE, 2, 3, 0),
        (-3, 2, POSITIVE, 2, 9, 0)
    );

    #undef TEST_FLT_NUM_SQR

    #define TEST_FLT_NUM_SQR(TAG, FLT)                          \
    {                                                           \
        TEST_CASE_OPEN(TAG)                                     \
        {                                                       \
            flt_num_t flt = flt_num_create_immed(ARG_OPEN FLT); \
            TEST_REVERT_OPEN                                    \
            {                                                   \
                flt_num_sqr(flt);                               \
            }                                                   \
            TEST_REVERT_CLOSE                                   \
        }                                                       \
        TEST_CASE_CLOSE                                         \
    }

    TEST_FLT_NUM_SQR(3, (INT64_MAX, 2, POSITIVE, 2, 1, 0));

    #undef TEST_FLT_NUM_SQR

    TEST_FN_CLOSE
}

static void test_flt_num_div(bool show)
{
    TEST_FN_OPEN

    #define TEST_FLT_NUM_DIV(TAG, FLT_1, FLT_2, RES)                \
    {                                                               \
        TEST_CASE_OPEN(TAG)                                         \
        {                                                           \
            flt_num_t flt_1 = flt_num_create_immed(ARG_OPEN FLT_1); \
            flt_num_t flt_2 = flt_num_create_immed(ARG_OPEN FLT_2); \
            flt_1 = flt_num_div(flt_1, flt_2);                      \
            assert(flt_num_immed(flt_1, ARG_OPEN RES))              \
        }                                                           \
        TEST_CASE_CLOSE                                             \
    }

    TEST_FLT_NUM_DIV(1,
        (-1, 2, POSITIVE, 2, 6, 0),
        (-1, 2, POSITIVE, 2, 3, 0),
        (-1, 2, POSITIVE, 2, 2, 0)
    );

    #undef TEST_FLT_NUM_DIV

    TEST_FN_CLOSE
}



static void test_flt_num_div_sig(bool show)
{
    TEST_FN_OPEN

    #define TEST_FLT_NUM_DIV_SIG(TAG, FLT, SIG, RES)            \
    {                                                           \
        TEST_CASE_OPEN(TAG)                                     \
        {                                                       \
            flt_num_t flt = flt_num_create_immed(ARG_OPEN FLT); \
            sig_num_t sig = sig_num_create_immed(ARG_OPEN SIG); \
            flt = flt_num_div_sig(flt, sig);                    \
            assert(flt_num_immed(flt, ARG_OPEN RES))            \
        }                                                       \
        TEST_CASE_CLOSE                                         \
    }

    TEST_FLT_NUM_DIV_SIG(1,
        (0, 2, POSITIVE, 2, 6, 0),
        (POSITIVE, 1, 3),
        (0, 2, POSITIVE, 2, 2, 0)
    );
    TEST_FLT_NUM_DIV_SIG(2,
        (0, 2, POSITIVE, 2, 6, 0),
        (POSITIVE, 2, 3, 0),
        (-1, 2, POSITIVE, 2, 2, 0)
    );
    TEST_FLT_NUM_DIV_SIG(3,
        (0, 2, POSITIVE, 2, 5, 0),
        (POSITIVE, 1, 3),
        (0, 2, POSITIVE, 2, 1, 0xAAAAAAAAAAAAAAAA)
    );
    TEST_FLT_NUM_DIV_SIG(4,
        (0, 2, POSITIVE, 2, 2, 0),
        (POSITIVE, 1, 3),
        (-1, 2, POSITIVE, 2, 0xAAAAAAAAAAAAAAAA, 0xAAAAAAAAAAAAAAAA)
    );

    #undef TEST_FLT_NUM_DIV_SIG

    TEST_FN_CLOSE
}


constexpr uint64_t threads_mul_count = 65536;
constexpr uint64_t threads_mul_n = 4;
constexpr uint64_t threads_div_count = 32768;
constexpr uint64_t threads_div_n = 2;

static void test_flt_num_mul_threads(bool show)
{
    TEST_FN_OPEN

    #define TEST_FLT_NUM_MUL_THREADS(TAG, FLT_1, FLT_2, THREADS, RES) \
    {                                                                 \
        TEST_CASE_OPEN(TAG)                                           \
        {                                                             \
            flt_num_t flt = flt_num_mul_threads(                      \
                flt_num_create_immed(ARG_OPEN FLT_1),                 \
                flt_num_create_immed(ARG_OPEN FLT_2),                 \
                THREADS                                               \
            );                                                        \
            assert(flt_num_immed(flt, ARG_OPEN RES))                  \
        }                                                             \
        TEST_CASE_CLOSE                                               \
    }

    TEST_FLT_NUM_MUL_THREADS(1,
        FLT_NUM_ZERO(2), FLT_NUM_ZERO(2), 4,
        (0, 2, ZERO, 0)
    );
    TEST_FLT_NUM_MUL_THREADS(2,
        FLT_NUM_ZERO(2), (-1, 2, POSITIVE, 2, 1, 0), 4,
        (-1, 2, ZERO, 0)
    );
    TEST_FLT_NUM_MUL_THREADS(3,
        (-1, 2, POSITIVE, 2, 1, 0), FLT_NUM_ZERO(2), 4,
        (-1, 2, ZERO, 0)
    );
    TEST_FLT_NUM_MUL_THREADS(4,
        (-1, 2, POSITIVE, 2, 1, 0), (-1, 2, POSITIVE, 2, 1, 0), 4,
        (-1, 2, POSITIVE, 2, 1, 0)
    );
    TEST_FLT_NUM_MUL_THREADS(5,
        (-1, 2, NEGATIVE, 2, 1, 0), (-1, 2, POSITIVE, 2, 1, 0), 4,
        (-1, 2, NEGATIVE, 2, 1, 0)
    );
    TEST_FLT_NUM_MUL_THREADS(6,
        (-1, 2, POSITIVE, 2, 1, 0), (-1, 2, POSITIVE, 2, 1, 0), 1,
        (-1, 2, POSITIVE, 2, 1, 0)
    );

    #undef TEST_FLT_NUM_MUL_THREADS

    TEST_CASE_OPEN_TIMEOUT(7, 0)
    {
        flt_num_t flt_1 = flt_num_create_rand(0, threads_mul_count);
        flt_num_t flt_2 = flt_num_create_rand(0, threads_mul_count);

        flt_num_t ref = flt_num_mul(flt_num_copy(flt_1), flt_num_copy(flt_2));
        flt_num_t thr = flt_num_mul_threads(
            flt_num_copy(flt_1),
            flt_num_copy(flt_2),
            threads_mul_n
        );
        assert(flt_num_eq_dbg(ref, thr));

        flt_num_free(flt_1);
        flt_num_free(flt_2);
    }
    TEST_CASE_CLOSE

    TEST_FN_CLOSE
}

static void test_flt_num_div_threads(bool show)
{
    TEST_FN_OPEN

    #define TEST_FLT_NUM_DIV_THREADS(TAG, FLT_1, FLT_2, THREADS, RES) \
    {                                                                 \
        TEST_CASE_OPEN(TAG)                                           \
        {                                                             \
            flt_num_t flt = flt_num_div_threads(                      \
                flt_num_create_immed(ARG_OPEN FLT_1),                 \
                flt_num_create_immed(ARG_OPEN FLT_2),                 \
                THREADS                                               \
            );                                                        \
            assert(flt_num_immed(flt, ARG_OPEN RES))                  \
        }                                                             \
        TEST_CASE_CLOSE                                               \
    }

    TEST_FLT_NUM_DIV_THREADS(1,
        (-1, 2, POSITIVE, 2, 6, 0), (-1, 2, POSITIVE, 2, 3, 0), 4,
        (-1, 2, POSITIVE, 2, 2, 0)
    );
    TEST_FLT_NUM_DIV_THREADS(2,
        (-1, 2, NEGATIVE, 2, 6, 0), (-1, 2, POSITIVE, 2, 3, 0), 4,
        (-1, 2, NEGATIVE, 2, 2, 0)
    );
    TEST_FLT_NUM_DIV_THREADS(3,
        (-1, 2, POSITIVE, 2, 6, 0), (-1, 2, POSITIVE, 2, 3, 0), 1,
        (-1, 2, POSITIVE, 2, 2, 0)
    );

    #undef TEST_FLT_NUM_DIV_THREADS

    TEST_CASE_OPEN_TIMEOUT(4, 0)
    {
        flt_num_t flt_1 = flt_num_create_rand(0, threads_div_count);
        flt_num_t flt_2 = flt_num_create_rand(0, threads_div_count);

        flt_num_t ref = flt_num_div(flt_num_copy(flt_1), flt_num_copy(flt_2));
        flt_num_t thr = flt_num_div_threads(
            flt_num_copy(flt_1),
            flt_num_copy(flt_2),
            threads_div_n
        );
        assert(flt_num_eq_dbg(ref, thr));

        flt_num_free(flt_1);
        flt_num_free(flt_2);
    }
    TEST_CASE_CLOSE

    TEST_FN_CLOSE
}

static void test_flt_num_mul_sig_threads(bool show)
{
    TEST_FN_OPEN

    #define TEST_FLT_NUM_MUL_SIG_THREADS(TAG, FLT, SIG, THREADS, RES) \
    {                                                                 \
        TEST_CASE_OPEN(TAG)                                           \
        {                                                             \
            flt_num_t flt = flt_num_mul_sig_threads(                  \
                flt_num_create_immed(ARG_OPEN FLT),                   \
                sig_num_create_immed(ARG_OPEN SIG),                   \
                THREADS                                               \
            );                                                        \
            assert(flt_num_immed(flt, ARG_OPEN RES))                  \
        }                                                             \
        TEST_CASE_CLOSE                                               \
    }

    TEST_FLT_NUM_MUL_SIG_THREADS(1,
        (0, 2, POSITIVE, 2, 6, 0), (POSITIVE, 1, 3), 4,
        (0, 2, POSITIVE, 2, 18, 0)
    );
    TEST_FLT_NUM_MUL_SIG_THREADS(2,
        (0, 2, POSITIVE, 2, 6, 0), (NEGATIVE, 1, 3), 4,
        (0, 2, NEGATIVE, 2, 18, 0)
    );
    TEST_FLT_NUM_MUL_SIG_THREADS(3,
        (0, 2, POSITIVE, 2, 6, 0), (ZERO, 0), 4,
        (0, 2, ZERO, 0)
    );
    TEST_FLT_NUM_MUL_SIG_THREADS(4,
        FLT_NUM_ZERO(2), (POSITIVE, 1, 3), 4,
        (0, 2, ZERO, 0)
    );
    TEST_FLT_NUM_MUL_SIG_THREADS(5,
        (0, 2, POSITIVE, 2, 6, 0), (POSITIVE, 1, 3), 1,
        (0, 2, POSITIVE, 2, 18, 0)
    );

    TEST_FLT_NUM_MUL_SIG_THREADS(6,
        (0, 2, POSITIVE, 2, 6, 0), (POSITIVE, 2, 5, 0), 4,
        (1, 2, POSITIVE, 2, 30, 0)
    );
    TEST_FLT_NUM_MUL_SIG_THREADS(7,
        (0, 2, NEGATIVE, 2, 6, 0), (POSITIVE, 2, 5, 0), 4,
        (1, 2, NEGATIVE, 2, 30, 0)
    );

    #undef TEST_FLT_NUM_MUL_SIG_THREADS

    TEST_CASE_OPEN_TIMEOUT(8, 0)
    {
        flt_num_t flt = flt_num_create_rand(0, threads_mul_count);
        sig_num_t sig = sig_num_create_rand(threads_mul_count);

        flt_num_t ref = flt_num_mul_sig(flt_num_copy(flt), sig_num_copy(sig));
        flt_num_t thr = flt_num_mul_sig_threads(
            flt_num_copy(flt),
            sig_num_copy(sig),
            threads_mul_n
        );
        assert(flt_num_eq_dbg(ref, thr));

        flt_num_free(flt);
        sig_num_free(sig);
    }
    TEST_CASE_CLOSE

    TEST_FN_CLOSE
}

static void test_flt_num_div_sig_threads(bool show)
{
    TEST_FN_OPEN

    #define TEST_FLT_NUM_DIV_SIG_THREADS(TAG, FLT, SIG, THREADS, RES) \
    {                                                                 \
        TEST_CASE_OPEN(TAG)                                           \
        {                                                             \
            flt_num_t flt = flt_num_div_sig_threads(                  \
                flt_num_create_immed(ARG_OPEN FLT),                   \
                sig_num_create_immed(ARG_OPEN SIG),                   \
                THREADS                                               \
            );                                                        \
            assert(flt_num_immed(flt, ARG_OPEN RES))                  \
        }                                                             \
        TEST_CASE_CLOSE                                               \
    }

    TEST_FLT_NUM_DIV_SIG_THREADS(1,
        (0, 2, POSITIVE, 2, 6, 0), (POSITIVE, 1, 3), 4,
        (0, 2, POSITIVE, 2, 2, 0)
    );
    TEST_FLT_NUM_DIV_SIG_THREADS(2,
        (0, 2, POSITIVE, 2, 6, 0), (POSITIVE, 2, 3, 0), 4,
        (-1, 2, POSITIVE, 2, 2, 0)
    );
    TEST_FLT_NUM_DIV_SIG_THREADS(3,
        (0, 2, NEGATIVE, 2, 5, 0), (POSITIVE, 1, 3), 4,
        (0, 2, NEGATIVE, 2, 1, 0xAAAAAAAAAAAAAAAA)
    );
    TEST_FLT_NUM_DIV_SIG_THREADS(4,
        (0, 2, POSITIVE, 2, 6, 0), (POSITIVE, 1, 3), 1,
        (0, 2, POSITIVE, 2, 2, 0)
    );

    TEST_FLT_NUM_DIV_SIG_THREADS(5,
        (0, 2, POSITIVE, 2, 6, 0), (POSITIVE, 2, 5, 0), 4,
        (-1, 2, POSITIVE, 2, 1, 0x3333333333333333)
    );

    #undef TEST_FLT_NUM_DIV_SIG_THREADS

    TEST_CASE_OPEN_TIMEOUT(6, 0)
    {
        flt_num_t flt = flt_num_create_rand(0, threads_div_count);
        sig_num_t sig = sig_num_create_rand(threads_div_count);

        flt_num_t ref = flt_num_div_sig(flt_num_copy(flt), sig_num_copy(sig));
        flt_num_t thr = flt_num_div_sig_threads(
            flt_num_copy(flt),
            sig_num_copy(sig),
            threads_div_n
        );
        assert(flt_num_eq_dbg(ref, thr));

        flt_num_free(flt);
        sig_num_free(sig);
    }
    TEST_CASE_CLOSE

    TEST_FN_CLOSE
}


static void test_flt()
{
    TEST_LIB

    bool show = false;

    test_int64_get_sign(show);
    test_int64_add(show);

    test_flt_num_normalize(show);

    test_flt_num_set_exponent(show);

    test_flt_num_is_zero(show);

    test_flt_num_shr(show);

    test_flt_num_add(show);
    test_flt_num_sub(show);
    test_flt_num_mul(show);
    test_flt_num_sqr(show);
    test_flt_num_div(show);

    test_flt_num_div_sig(show);

    test_flt_num_mul_threads(show);
    test_flt_num_div_threads(show);
    test_flt_num_mul_sig_threads(show);
    test_flt_num_div_sig_threads(show);

    TEST_ASSERT_MEM_EMPTY
}



int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    test_flt();
    printf("\n\n\tTest successful\n\n");
    return 0;
}
