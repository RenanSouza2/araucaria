#include "../debug.h"
#include "../../../testrc.h"
#include "../../../mods/macros/test.h"

#include "../../num/debug.h"
#include "../../sig/debug.h"



static void test_fxd_div(bool show)
{
    TEST_FN_OPEN

    #define TEST_FXD_NUM_DIV(TAG, FXD_1, FXD_2, RES)                \
    {                                                               \
        TEST_CASE_OPEN(TAG)                                         \
        {                                                           \
            fxd_num_t fxd_1 = fxd_num_create_immed(ARG_OPEN FXD_1); \
            fxd_num_t fxd_2 = fxd_num_create_immed(ARG_OPEN FXD_2); \
            fxd_1 = fxd_num_div(fxd_1, fxd_2);                      \
            assert(fxd_num_immed(fxd_1, ARG_OPEN RES))              \
        }                                                           \
        TEST_CASE_CLOSE                                             \
    }

    TEST_FXD_NUM_DIV(1,
        (1, POSITIVE, 2, 6, 0),
        (1, POSITIVE, 2, 3, 0),
        (1, POSITIVE, 2, 2, 0)
    );

    TEST_FN_CLOSE
}


// Coverage for the threaded entry points. See the long note in lib/sig/test/test.c for
// why the small tables assert absolute results instead of comparing against the plain
// function, and why the large cases do the opposite. The same reasoning applies here,
// with fxd's repositioning taking the place of sig's signal handling as the wrapping
// being pinned.
constexpr uint64_t threads_mul_count = 65536;
constexpr uint64_t threads_mul_n = 4;
constexpr uint64_t threads_div_count = 32768;
constexpr uint64_t threads_div_n = 2;
constexpr uint64_t threads_pos = 8;

static void test_fxd_num_mul_threads(bool show)
{
    TEST_FN_OPEN

    #define TEST_FXD_NUM_MUL_THREADS(TAG, FXD_1, FXD_2, THREADS, RES) \
    {                                                                 \
        TEST_CASE_OPEN(TAG)                                           \
        {                                                             \
            fxd_num_t fxd = fxd_num_mul_threads(                      \
                fxd_num_create_immed(ARG_OPEN FXD_1),                 \
                fxd_num_create_immed(ARG_OPEN FXD_2),                 \
                THREADS                                               \
            );                                                        \
            assert(fxd_num_immed(fxd, ARG_OPEN RES))                  \
        }                                                             \
        TEST_CASE_CLOSE                                               \
    }

    TEST_FXD_NUM_MUL_THREADS(1,
        (1, POSITIVE, 2, 6, 0), (1, POSITIVE, 2, 3, 0), 4,
        (1, POSITIVE, 2, 18, 0)
    );
    TEST_FXD_NUM_MUL_THREADS(2,
        (1, NEGATIVE, 2, 6, 0), (1, POSITIVE, 2, 3, 0), 4,
        (1, NEGATIVE, 2, 18, 0)
    );
    TEST_FXD_NUM_MUL_THREADS(3,
        (1, POSITIVE, 2, 6, 0), (1, ZERO, 0), 4,
        (1, ZERO, 0)
    );
    TEST_FXD_NUM_MUL_THREADS(4,
        (1, POSITIVE, 2, 6, 0), (1, POSITIVE, 2, 3, 0), 1,
        (1, POSITIVE, 2, 18, 0)
    );

    // Product wider than the operands, so the trim back down to pos actually removes a
    // limb. The cases above all stay at two limbs, where trimming is a no-op.
    TEST_FXD_NUM_MUL_THREADS(5,
        (1, POSITIVE, 2, 6, 0), (1, POSITIVE, 2, 5, 0), 4,
        (1, POSITIVE, 2, 30, 0)
    );

    #undef TEST_FXD_NUM_MUL_THREADS

    // Operands large enough that num actually fans the multiply out instead of clamping
    // the request back to one worker.
    TEST_CASE_OPEN_TIMEOUT(6, 0)
    {
        fxd_num_t fxd_1 = fxd_num_wrap_sig(sig_num_create_rand(threads_mul_count), threads_pos);
        fxd_num_t fxd_2 = fxd_num_wrap_sig(sig_num_create_rand(threads_mul_count), threads_pos);

        fxd_num_t ref = fxd_num_mul(fxd_num_copy(fxd_1), fxd_num_copy(fxd_2));
        fxd_num_t thr = fxd_num_mul_threads(
            fxd_num_copy(fxd_1),
            fxd_num_copy(fxd_2),
            threads_mul_n
        );
        assert(fxd_num_eq_dbg(ref, thr));

        fxd_num_free(fxd_1);
        fxd_num_free(fxd_2);
    }
    TEST_CASE_CLOSE

    TEST_FN_CLOSE
}

static void test_fxd_num_div_threads(bool show)
{
    TEST_FN_OPEN

    #define TEST_FXD_NUM_DIV_THREADS(TAG, FXD_1, FXD_2, THREADS, RES) \
    {                                                                 \
        TEST_CASE_OPEN(TAG)                                           \
        {                                                             \
            fxd_num_t fxd = fxd_num_div_threads(                      \
                fxd_num_create_immed(ARG_OPEN FXD_1),                 \
                fxd_num_create_immed(ARG_OPEN FXD_2),                 \
                THREADS                                               \
            );                                                        \
            assert(fxd_num_immed(fxd, ARG_OPEN RES))                  \
        }                                                             \
        TEST_CASE_CLOSE                                               \
    }

    TEST_FXD_NUM_DIV_THREADS(1,
        (1, POSITIVE, 2, 6, 0), (1, POSITIVE, 2, 3, 0), 4,
        (1, POSITIVE, 2, 2, 0)
    );
    TEST_FXD_NUM_DIV_THREADS(2,
        (1, NEGATIVE, 2, 6, 0), (1, POSITIVE, 2, 3, 0), 4,
        (1, NEGATIVE, 2, 2, 0)
    );
    TEST_FXD_NUM_DIV_THREADS(3,
        (1, ZERO, 0), (1, POSITIVE, 2, 3, 0), 4,
        (1, ZERO, 0)
    );
    TEST_FXD_NUM_DIV_THREADS(4,
        (1, POSITIVE, 2, 6, 0), (1, POSITIVE, 2, 3, 0), 1,
        (1, POSITIVE, 2, 2, 0)
    );

    #undef TEST_FXD_NUM_DIV_THREADS

    // Dividend twice the divisor so Burnikel-Ziegler actually recurses.
    TEST_CASE_OPEN_TIMEOUT(5, 0)
    {
        fxd_num_t fxd_1 = fxd_num_wrap_sig(
            sig_num_create_rand(2 * threads_div_count),
            threads_pos
        );
        fxd_num_t fxd_2 = fxd_num_wrap_sig(
            sig_num_create_rand(threads_div_count),
            threads_pos
        );

        fxd_num_t ref = fxd_num_div(fxd_num_copy(fxd_1), fxd_num_copy(fxd_2));
        fxd_num_t thr = fxd_num_div_threads(
            fxd_num_copy(fxd_1),
            fxd_num_copy(fxd_2),
            threads_div_n
        );
        assert(fxd_num_eq_dbg(ref, thr));

        fxd_num_free(fxd_1);
        fxd_num_free(fxd_2);
    }
    TEST_CASE_CLOSE

    TEST_FN_CLOSE
}

static void test_fxd_num_mul_sig_threads(bool show)
{
    TEST_FN_OPEN

    #define TEST_FXD_NUM_MUL_SIG_THREADS(TAG, FXD, SIG, THREADS, RES) \
    {                                                                 \
        TEST_CASE_OPEN(TAG)                                           \
        {                                                             \
            fxd_num_t fxd = fxd_num_mul_sig_threads(                  \
                fxd_num_create_immed(ARG_OPEN FXD),                   \
                sig_num_create_immed(ARG_OPEN SIG),                   \
                THREADS                                               \
            );                                                        \
            assert(fxd_num_immed(fxd, ARG_OPEN RES))                  \
        }                                                             \
        TEST_CASE_CLOSE                                               \
    }

    TEST_FXD_NUM_MUL_SIG_THREADS(1,
        (1, POSITIVE, 2, 6, 0), (POSITIVE, 1, 3), 4,
        (1, POSITIVE, 2, 18, 0)
    );
    TEST_FXD_NUM_MUL_SIG_THREADS(2,
        (1, POSITIVE, 2, 6, 0), (NEGATIVE, 1, 3), 4,
        (1, NEGATIVE, 2, 18, 0)
    );
    TEST_FXD_NUM_MUL_SIG_THREADS(3,
        (1, POSITIVE, 2, 6, 0), (ZERO, 0), 4,
        (1, ZERO, 0)
    );
    TEST_FXD_NUM_MUL_SIG_THREADS(4,
        (1, POSITIVE, 2, 6, 0), (POSITIVE, 1, 3), 1,
        (1, POSITIVE, 2, 18, 0)
    );

    // Unlike fxd_num_mul, fxd_num_mul_sig does not trim, so the product keeps its third
    // limb -- pinned here so the two are not silently made to behave the same.
    TEST_FXD_NUM_MUL_SIG_THREADS(5,
        (1, POSITIVE, 2, 6, 0), (POSITIVE, 2, 5, 0), 4,
        (1, POSITIVE, 3, 30, 0, 0)
    );

    #undef TEST_FXD_NUM_MUL_SIG_THREADS

    TEST_CASE_OPEN_TIMEOUT(6, 0)
    {
        fxd_num_t fxd = fxd_num_wrap_sig(sig_num_create_rand(threads_mul_count), threads_pos);
        sig_num_t sig = sig_num_create_rand(threads_mul_count);

        fxd_num_t ref = fxd_num_mul_sig(fxd_num_copy(fxd), sig_num_copy(sig));
        fxd_num_t thr = fxd_num_mul_sig_threads(
            fxd_num_copy(fxd),
            sig_num_copy(sig),
            threads_mul_n
        );
        assert(fxd_num_eq_dbg(ref, thr));

        fxd_num_free(fxd);
        sig_num_free(sig);
    }
    TEST_CASE_CLOSE

    TEST_FN_CLOSE
}

static void test_fxd_num_div_sig_threads(bool show)
{
    TEST_FN_OPEN

    #define TEST_FXD_NUM_DIV_SIG_THREADS(TAG, FXD, SIG, THREADS, RES) \
    {                                                                 \
        TEST_CASE_OPEN(TAG)                                           \
        {                                                             \
            fxd_num_t fxd = fxd_num_div_sig_threads(                  \
                fxd_num_create_immed(ARG_OPEN FXD),                   \
                sig_num_create_immed(ARG_OPEN SIG),                   \
                THREADS                                               \
            );                                                        \
            assert(fxd_num_immed(fxd, ARG_OPEN RES))                  \
        }                                                             \
        TEST_CASE_CLOSE                                               \
    }

    TEST_FXD_NUM_DIV_SIG_THREADS(1,
        (1, POSITIVE, 2, 6, 0), (POSITIVE, 1, 3), 4,
        (1, POSITIVE, 2, 2, 0)
    );
    TEST_FXD_NUM_DIV_SIG_THREADS(2,
        (1, NEGATIVE, 2, 6, 0), (POSITIVE, 1, 3), 4,
        (1, NEGATIVE, 2, 2, 0)
    );
    TEST_FXD_NUM_DIV_SIG_THREADS(3,
        (1, ZERO, 0), (POSITIVE, 1, 3), 4,
        (1, ZERO, 0)
    );
    TEST_FXD_NUM_DIV_SIG_THREADS(4,
        (1, POSITIVE, 2, 6, 0), (POSITIVE, 1, 3), 1,
        (1, POSITIVE, 2, 2, 0)
    );

    #undef TEST_FXD_NUM_DIV_SIG_THREADS

    TEST_CASE_OPEN_TIMEOUT(5, 0)
    {
        fxd_num_t fxd = fxd_num_wrap_sig(
            sig_num_create_rand(2 * threads_div_count),
            threads_pos
        );
        sig_num_t sig = sig_num_create_rand(threads_div_count);

        fxd_num_t ref = fxd_num_div_sig(fxd_num_copy(fxd), sig_num_copy(sig));
        fxd_num_t thr = fxd_num_div_sig_threads(
            fxd_num_copy(fxd),
            sig_num_copy(sig),
            threads_div_n
        );
        assert(fxd_num_eq_dbg(ref, thr));

        fxd_num_free(fxd);
        sig_num_free(sig);
    }
    TEST_CASE_CLOSE

    TEST_FN_CLOSE
}


static void test_fxd()
{
    TEST_LIB

    bool show = false;

    test_fxd_div(show);

    test_fxd_num_mul_threads(show);
    test_fxd_num_div_threads(show);
    test_fxd_num_mul_sig_threads(show);
    test_fxd_num_div_sig_threads(show);

    TEST_ASSERT_MEM_EMPTY
}



int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    test_fxd();
    printf("\n\n\tTest successful\n\n");
    return 0;
}
