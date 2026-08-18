#ifndef NUM_STRUCT_H
#define NUM_STRUCT_H

#include <stdbool.h>

#include "../../mods/macros/struct.h"
#include "../../mods/macros/uint.h" // IWYU pragma: keep

typedef uint64_t * chunk_p;
[[maybe_unused]]
constexpr uint64_t chunk_bits_log_2 = 6;
[[maybe_unused]]
constexpr uint64_t chunk_bits = 64;

STRUCT(araucaria_disk_config)
{
    uint64_t disk_threshold_bytes;
    const char* disk_path;
    bool is_set;
};

STRUCT(araucaria_thread_config)
{
    uint64_t thread_count;
    bool is_set;
};

STRUCT(num)
{
    uint64_t size;
    uint64_t count;
    bool is_mmap;
    chunk_p chunk;
};

STRUCT(ssm_params)
{
    uint64_t count;
    uint64_t M;
    uint64_t K;
    uint64_t Q;
    uint64_t n;
};

#endif
