#define NAME "stage"
#include "behavior.c"



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
