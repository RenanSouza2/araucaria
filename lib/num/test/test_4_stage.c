#define NAME "stage"
#include "behavior.c"



// 2^64 - 59
constexpr uint64_t stage_residue_prime = UINT64_MAX - 58;

// NUM mod stage_residue_prime
static uint64_t num_residue(num_p num)
{
    uint64_t res = 0;
    for(uint64_t i=num->count; i>0; i--)
    {
        res = LOW(U128HL(res, num->chunk[i-1]) % stage_residue_prime);
    }
    return res;
}

// 16 threads need 131072 limbs in the shorter operand (num_mul_threads_ceiling)
static void test_fuzz_num_ssm_stage_wide(bool show)
{
    TEST_FN_OPEN

    #define TEST_FUZZ_NUM_SSM_STAGE_WIDE_MUL(TAG, COUNT_1, COUNT_2, RUNS, THREADS)  \
    {                                                                               \
        TEST_FUZZ_CASE_OPEN(TAG, RUNS)                                              \
        {                                                                           \
            fuzz_seed(_tag);                                                        \
            num_p num_1 = num_create_rand(COUNT_1);                                 \
            num_p num_2 = num_create_rand(COUNT_2);                                 \
            uint128_t res = MUL(num_residue(num_1), num_residue(num_2));            \
            num_p num_res = num_mul_threads(num_1, num_2, THREADS);                 \
            assert(uint64(num_residue(num_res), LOW(res % stage_residue_prime)));   \
            num_free(num_res);                                                      \
        }                                                                           \
        TEST_FUZZ_CASE_CLOSE                                                        \
    }

    TEST_FUZZ_NUM_SSM_STAGE_WIDE_MUL(1, 131072, 131072, 2, 16)
    TEST_FUZZ_NUM_SSM_STAGE_WIDE_MUL(2, 300000, 300000, 2, 16)
    TEST_FUZZ_NUM_SSM_STAGE_WIDE_MUL(3, 140000, 400000, 2, 16)
    TEST_FUZZ_NUM_SSM_STAGE_WIDE_MUL(4, 200000, 200000, 2, 12)

    #undef TEST_FUZZ_NUM_SSM_STAGE_WIDE_MUL

    #define TEST_FUZZ_NUM_SSM_STAGE_WIDE_SQR(TAG, COUNT, RUNS, THREADS)             \
    {                                                                               \
        TEST_FUZZ_CASE_OPEN(TAG, RUNS)                                              \
        {                                                                           \
            fuzz_seed(_tag);                                                        \
            num_p num = num_create_rand(COUNT);                                     \
            uint64_t res_num = num_residue(num);                                    \
            uint128_t res = MUL(res_num, res_num);                                  \
            num_p num_res = num_sqr_threads(num, THREADS);                          \
            assert(uint64(num_residue(num_res), LOW(res % stage_residue_prime)));   \
            num_free(num_res);                                                      \
        }                                                                           \
        TEST_FUZZ_CASE_CLOSE                                                        \
    }

    TEST_FUZZ_NUM_SSM_STAGE_WIDE_SQR(5, 131072, 2, 16)
    TEST_FUZZ_NUM_SSM_STAGE_WIDE_SQR(6, 300000, 2, 16)
    TEST_FUZZ_NUM_SSM_STAGE_WIDE_SQR(7, 200000, 2, 12)

    #undef TEST_FUZZ_NUM_SSM_STAGE_WIDE_SQR

    TEST_FN_CLOSE
}



static void test_num_stage()
{
    TEST_LIB

    bool show = false;

    araucaria_disk_config_t config = {
        .disk_path = "./cache",
        .disk_threshold_bytes = 8192,
        .ram_budget_bytes = U64(8) * 1024 * 1024
    };
    araucaria_disk_config_set(&config);

    test_all(show);

    // keeps every worker's share above the 1 MiB cache block for 16 workers
    config.ram_budget_bytes = U64(32) * 1024 * 1024;
    araucaria_disk_config_set(&config);

    test_fuzz_num_ssm_stage_wide(show);

    TEST_ASSERT_MEM_EMPTY
}



int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    test_seed_init();
    test_num_stage();
    printf("\n\n\tTest successful\n\n");
    return 0;
}
