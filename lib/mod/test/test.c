#include "../debug.h"
#include "../../../testrc.h"
#include "../../../mods/macros/test.h"

#include "../../num/debug.h"



static void test_mod_num_create(bool show)
{
    TEST_FN_OPEN

    num_p p = num_wrap(7);

    #define TEST_MOD_NUM_CREATE(TAG, NUM, RES)          \
    {                                                   \
        TEST_CASE_OPEN(TAG)                             \
        {                                               \
            num_p num = num_create_immed(ARG_OPEN NUM); \
            mod_num_t mod = mod_num_create(num, p);     \
            assert(mod_num_immed(mod, ARG_OPEN RES));   \
        }                                               \
        TEST_CASE_CLOSE                                 \
    }

    TEST_MOD_NUM_CREATE(1, (0), (0));
    TEST_MOD_NUM_CREATE(2, (1, 1), (1, 1));
    TEST_MOD_NUM_CREATE(3, (1, 7), (0));
    TEST_MOD_NUM_CREATE(4, (1, 8), (1, 1));

    #undef TEST_MOD_NUM_CREATE

    num_free(p);

    TEST_FN_CLOSE
}

static void test_mod_num_wrap(bool show)
{
    TEST_FN_OPEN

    num_p p = num_wrap(7);

    #define TEST_MOD_NUM_WRAP(TAG, VALUE, RES)          \
    {                                                   \
        TEST_CASE_OPEN(TAG)                             \
        {                                               \
            mod_num_t mod = mod_num_wrap(VALUE, p);     \
            assert(mod_num_immed(mod, ARG_OPEN RES));   \
        }                                               \
        TEST_CASE_CLOSE                                 \
    }

    TEST_MOD_NUM_WRAP(1, 0, (0));
    TEST_MOD_NUM_WRAP(2, 1, (1, 1));
    TEST_MOD_NUM_WRAP(3, 7, (0));
    TEST_MOD_NUM_WRAP(4, 8, (1, 1));

    #undef TEST_MOD_NUM_WRAP

    num_free(p);

    TEST_FN_CLOSE
}



static void test_mod_num_add(bool show)
{
    TEST_FN_OPEN

    num_p p = num_wrap(7);

    #define TEST_MOD_NUM_ADD(TAG, VALUE_1, VALUE_2, RES)    \
    {                                                       \
        TEST_CASE_OPEN(TAG)                                 \
        {                                                   \
            mod_num_t mod_1 = mod_num_wrap(VALUE_1, p);     \
            mod_num_t mod_2 = mod_num_wrap(VALUE_2, p);     \
            mod_num_t mod_res = mod_num_add(mod_1, mod_2);  \
            assert(mod_num_immed(mod_res, ARG_OPEN RES));   \
        }                                                   \
        TEST_CASE_CLOSE                                     \
    }

    TEST_MOD_NUM_ADD(1, 0, 0, (0));
    TEST_MOD_NUM_ADD(2, 1, 2, (1, 3));
    TEST_MOD_NUM_ADD(3, 1, 6, (0));
    TEST_MOD_NUM_ADD(4, 6, 6, (1, 5));

    #undef TEST_MOD_NUM_ADD

    num_free(p);

    TEST_FN_CLOSE
}

static void test_mod_num_sub(bool show)
{
    TEST_FN_OPEN

    num_p p = num_wrap(7);

    #define TEST_MOD_NUM_SUB(TAG, VALUE_1, VALUE_2, RES)    \
    {                                                       \
        TEST_CASE_OPEN(TAG)                                 \
        {                                                   \
            mod_num_t mod_1 = mod_num_wrap(VALUE_1, p);     \
            mod_num_t mod_2 = mod_num_wrap(VALUE_2, p);     \
            mod_num_t mod_res = mod_num_sub(mod_1, mod_2);  \
            assert(mod_num_immed(mod_res, ARG_OPEN RES));   \
        }                                                   \
        TEST_CASE_CLOSE                                     \
    }

    TEST_MOD_NUM_SUB(1, 0, 0, (0));
    TEST_MOD_NUM_SUB(2, 0, 1, (1, 6));
    TEST_MOD_NUM_SUB(3, 2, 0, (1, 2));
    TEST_MOD_NUM_SUB(4, 3, 2, (1, 1));
    TEST_MOD_NUM_SUB(5, 2, 5, (1, 4));

    #undef TEST_MOD_NUM_SUB

    num_free(p);

    TEST_FN_CLOSE
}

static void test_mod_num_mul(bool show)
{
    TEST_FN_OPEN

    num_p p = num_wrap(7);

    #define TEST_MOD_NUM_MUL(TAG, VALUE_1, VALUE_2, RES)    \
    {                                                       \
        TEST_CASE_OPEN(TAG)                                 \
        {                                                   \
            mod_num_t mod_1 = mod_num_wrap(VALUE_1, p);     \
            mod_num_t mod_2 = mod_num_wrap(VALUE_2, p);     \
            mod_num_t mod_res = mod_num_mul(mod_1, mod_2);  \
            assert(mod_num_immed(mod_res, ARG_OPEN RES));   \
        }                                                   \
        TEST_CASE_CLOSE                                     \
    }

    TEST_MOD_NUM_MUL(1, 0, 0, (0));
    TEST_MOD_NUM_MUL(2, 0, 1, (0));
    TEST_MOD_NUM_MUL(3, 1, 0, (0));
    TEST_MOD_NUM_MUL(4, 2, 3, (1, 6));
    TEST_MOD_NUM_MUL(5, 3, 4, (1, 5));
    TEST_MOD_NUM_MUL(6, 5, 6, (1, 2));

    #undef TEST_MOD_NUM_MUL

    TEST_CASE_OPEN(7)
    {
        num_p p_6 = num_wrap(6);

        mod_num_t mod_1 = mod_num_wrap(3, p_6);
        mod_num_t mod_2 = mod_num_wrap(2, p_6);
        mod_num_t mod_res = mod_num_mul(mod_1, mod_2);
        assert(mod_num_immed(mod_res, 0));

        num_free(p_6);
    }
    TEST_CASE_CLOSE

    num_free(p);

    TEST_FN_CLOSE
}

static void test_mod_num_div(bool show)
{
    TEST_FN_OPEN

    #define TEST_MOD_NUM_DIV(TAG, MOD, VALUE_1, VALUE_2, RES)   \
    {                                                           \
        TEST_CASE_OPEN(TAG)                                     \
        {                                                       \
            num_p p = num_wrap(MOD);                            \
            mod_num_t mod_1 = mod_num_wrap(VALUE_1, p);         \
            mod_num_t mod_2 = mod_num_wrap(VALUE_2, p);         \
            mod_num_t mod_res = mod_num_div(mod_1, mod_2);      \
            assert(mod_num_immed(mod_res, ARG_OPEN RES));       \
            num_free(p);                                        \
        }                                                       \
        TEST_CASE_CLOSE                                         \
    }

    TEST_MOD_NUM_DIV(1, 7, 0, 2, (0));
    TEST_MOD_NUM_DIV(2, 7, 6, 2, (1, 3));
    TEST_MOD_NUM_DIV(3, 7, 6, 4, (1, 5));
    TEST_MOD_NUM_DIV(4, 29, 1, 9, (1, 13));

    #undef TEST_MOD_NUM_DIV

    #define TEST_MOD_NUM_DIV(TAG, MOD, VALUE_1, VALUE_2)    \
    {                                                       \
        TEST_CASE_OPEN(TAG)                                 \
        {                                                   \
            num_p p = num_wrap(MOD);                        \
            mod_num_t mod_1 = mod_num_wrap(VALUE_1, p);     \
            mod_num_t mod_2 = mod_num_wrap(VALUE_2, p);     \
            TEST_REVERT_OPEN                                \
            {                                               \
                mod_num_div(mod_1, mod_2);                  \
            }                                               \
            TEST_REVERT_CLOSE                               \
        }                                                   \
        TEST_CASE_CLOSE                                     \
    }

    TEST_MOD_NUM_DIV(5, 6, 1, 0);
    TEST_MOD_NUM_DIV(6, 6, 1, 2);

    #undef TEST_MOD_NUM_DIV

    TEST_FN_CLOSE
}



static void test_mod()
{
    TEST_LIB

    bool show = false;

    test_mod_num_create(show);
    test_mod_num_wrap(show);

    test_mod_num_add(show);
    test_mod_num_sub(show);
    test_mod_num_mul(show);
    test_mod_num_div(show);

    TEST_ASSERT_MEM_EMPTY
}



int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    test_mod();
    printf("\n\n\tTest successful\n\n");
    return 0;
}
