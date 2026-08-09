#define NAME "ram"
#include "behavior.c"



static void test_num_ram()
{
    TEST_LIB

    bool show = false;

    test_all(show);

    TEST_ASSERT_MEM_EMPTY
}



int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    test_seed_init();
    test_num_ram();
    printf("\n\n\tTest successful\n\n");
    return 0;
}
