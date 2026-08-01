#include "../debug.h"
#include "../../../testrc.h"
#include "../../../mods/macros/test.h"

#include "../../num/debug.h"



static void test_file_num()
{
    TEST_LIB

    TEST_ASSERT_MEM_EMPTY
}



int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    test_file_num();
    printf("\n\n\tTest successful\n\n");
    return 0;
}
