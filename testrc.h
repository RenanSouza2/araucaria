#include "./mods/clu/header.h"
#include "./mods/macros/assert.h"

#define TEST_ASSERT_MEM_EMPTY assert(clu_mem_is_empty());

// Applies to TEST_CASE_OPEN only; TEST_FUZZ_CASE_OPEN passes 0 explicitly
// because its large-operand cases legitimately run for minutes.
#define TEST_CASE_TIMEOUT_MS 10'000
