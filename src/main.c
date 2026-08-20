#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

#if defined(__linux__)
#include <sched.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/thread_policy.h>
#include <pthread.h>
#endif

#include "../mods/clu/header.h"
#include "../mods/macros/assert.h"
#include "../mods/macros/fork.h"
#include "../mods/macros/time.h"

#include "../lib/fxd/header.h"
#include "../lib/flt/header.h"
#include "../lib/mod/header.h"
#include "../lib/num/header.h"
#include "../lib/num/struct.h"
#include "../lib/sig/header.h"



[[maybe_unused]]
static int64_t get_arg(int argc, char** argv)
{
    assert(argc > 1);
    // NOLINTNEXTLINE(cert-err34-c)
    return atoi(argv[1]);
}



[[maybe_unused]]
static num_p num_generate_1_step(num_p num, uint64_t salt)
{
    num = num_add(num, num_wrap(salt));
    return num_sqr(num);
}

[[maybe_unused]]
static num_p num_generate_1(uint64_t max, uint64_t salt)
{
    num_p num = num_wrap(2);
    for(uint64_t i=0; i<max; i++)
    {
        num = num_generate_1_step(num, salt);
    }
    return num;
}

[[maybe_unused]]
static num_p num_generate_2_step(num_p num, uint64_t salt)
{
    constexpr uint64_t seed = 0xe6503424c62eef89;
    num = num_add_uint(num, salt);
    return num_mul_uint(num, seed);
}

[[maybe_unused]]
static num_p num_generate_2(uint64_t index, uint64_t salt)
{
    constexpr uint64_t seed = 0xe6503424c62eef89;
    num_p num = num_wrap(seed);
    for(uint64_t i=1; i<index; i++)
    {
        num = num_generate_2_step(num, salt);
    }

    return num;
}

[[maybe_unused]]
static void time_1(uint64_t begin, uint64_t end)
{
    assert(begin);
    num_p num_1 = num_generate_1(begin, 1);
    num_p num_2 = num_generate_1(begin - 1, 2);
    for(uint64_t i=begin; i<end; i++)
    {
        printf("\n" U64P() "", i);

        TIME_SETUP
        num_1 = num_generate_1_step(num_1, 1);
        TIME_END(t1)
        printf("\t%10.3f", (double)t1 / 1e9);

        TIME_RESET
        num_2 = num_generate_1_step(num_2, 2);
        TIME_END(t2)
        printf("\t%10.3f", (double)t2 / 1e9);

        num_p num_1_copy = num_copy(num_1);
        num_p num_2_copy = num_copy(num_2);

        TIME_RESET
        num_p num = num_div(num_1_copy, num_2_copy);
        TIME_END(t3)
        printf("\t%10.3f", (double)t3 / 1e9);

        num_free(num);
    }
    num_free(num_1);
    num_free(num_2);
}

[[maybe_unused]]
static void ssm_params_display_main(ssm_params_t p)
{
    printf("\t" U64P() "", p.M);
    printf("\t" U64P() "", p.K);
}

[[maybe_unused]]
static void time_2(int argc, char** argv, uint64_t max, uint64_t jumps)
{
    uint64_t id = argc > 1 ? (uint64_t)get_arg(argc, argv) : 0;
    num_p num_1 = num_generate_1(max, 2);

    // printf("\nid: " U64P() "", id);
    // num_display_tag("num_1", num_1);
    // printf("\nN\ttime\tM\tK\tQ\tn\tdepth\tlast_n");

    uint64_t first = jumps;
    num_p num_2 = num_generate_2(first, 2);
    uint64_t threads = 1;
    for(uint64_t i=id + first; num_2->count < num_1->count; i += threads * jumps)
    {
        for(uint64_t j=0; j<threads * jumps; j++)
        {
            num_2 = num_generate_2_step(num_2, 2);
        }

        printf("\n" U64P() "", i);

        uint64_t res = 0;
        uint64_t repeat = 1;
        for(uint64_t j=0; j<repeat; j ++)
        {
            num_p num_aux_1 = num_copy(num_1);
            num_p num_aux_2 = num_copy(num_2);

            uint64_t begin = get_time();
            num_aux_1 = num_mul(num_aux_1, num_aux_2);
            uint64_t end = get_time();
            num_free(num_aux_1);

            res += end - begin;
        }

        printf("\t%10.9lf", ((double)res / (double)repeat) / 1e9);
    }
}

[[maybe_unused]]
static void time_2_total(int argc, char** argv)
{
    printf("\ncount\ttime");

    time_2(argc, argv, 17, 10);
    time_2(argc, argv, 18, 20);
    time_2(argc, argv, 19, 40);
    time_2(argc, argv, 20, 80);
    time_2(argc, argv, 21, 160);
    time_2(argc, argv, 22, 320);
}

[[maybe_unused]]
static void time_3()
{
    uint64_t i_last = 1;
    num_p num_1 = num_generate_2(i_last, 2);
    num_p num_2 = num_generate_2(i_last, 3);
    uint64_t threshold = 1000;
    uint64_t max = 200000;
    uint64_t p_1 = 102;
    uint64_t p_2 = 100;
    // uint64_t threshold = 1000;
    // uint64_t max = 200000;
    // uint64_t p_1 = 1002;
    // uint64_t p_2 = 1000;
    assert(threshold * p_1 / p_2 > threshold);
    for(uint64_t i=1; i<threshold; i+=1)
    {
        TIME_SETUP

        for(uint64_t j=i_last; j<i; j++)
        {
            num_1 = num_generate_2_step(num_1, 2);
            num_2 = num_generate_2_step(num_2, 3);
        }
        i_last = i;

        printf("\n" U64P() "", num_1->count);
        num_p num_1_copy = num_copy(num_1);
        num_p num_2_copy = num_copy(num_2);
        TIME_RESET
        num_p num_res = num_mul(num_1_copy, num_2_copy);
        TIME_END(t3);
        printf("\t%.5f", (double)t3 / 1e9);
        num_free(num_res);
    }

    for(uint64_t i=threshold; i<max; i = i * p_1 / p_2)
    {
         TIME_SETUP

        for(uint64_t j=i_last; j<i; j++)
        {
            num_1 = num_generate_2_step(num_1, 2);
            num_2 = num_generate_2_step(num_2, 3);
        }
        i_last = i;

        printf("\n" U64P() "", num_1->count);
        num_p num_1_copy = num_copy(num_1);
        num_p num_2_copy = num_copy(num_2);
        TIME_RESET
        num_p num_res = num_mul(num_1_copy, num_2_copy);
        TIME_END(t3);
        printf("\t%.5f", (double)t3 / 1e9);
        num_free(num_res);
    }
}

[[maybe_unused]]
static void fibonacci_1()
{
    num_p num_a = num_wrap(1);
    num_p num_b = num_wrap(1);

    uint64_t space = 10'000;
    for(uint64_t i=0; ; i++)
    {
        num_p num_c = num_add(num_a, num_copy(num_b));

        if(i%space == 0)
        {
            printf("\n" U64P() " num: ", i);
            num_display(num_c);
        }

        num_a = num_b;
        num_b = num_c;
    }
}

[[maybe_unused]]
static void fibonacci_2(uint64_t min, uint64_t max)
{
    num_p num_a = num_wrap(0);
    num_p num_b = num_wrap(1);
    num_p num_c = num_wrap(1);

    printf("\n");
    for(uint64_t i=0; i < max; i++)
    {
        if(i > min)
        {
            printf("\n" U64P() "", i);
        }

        uint64_t begin = get_time();
        num_p num_a_2 = num_sqr(num_copy(num_a));
        num_p num_b_2 = num_sqr(num_copy(num_b));
        num_p num_c_2 = num_sqr(num_copy(num_c));

        num_a = num_add(num_a, num_c);
        num_b = num_mul(num_b, num_a);

        num_a = num_add(num_a_2, num_copy(num_b_2));
        num_c = num_add(num_b_2, num_c_2);
        uint64_t end = get_time();

        if(i > min)
        {
            printf("\t%10.3f", (double)(end - begin) / 1e3);
        }
    }
}

[[maybe_unused]]
static void fibonacci_3(uint64_t, uint64_t max)
{
    num_p num_a = num_wrap(0);
    num_p num_b = num_wrap(1);

    printf("\n");
    for(uint64_t i=0; i < max; i++)
    {
        // if(i > min)
        //     printf("\n" U64P() "", i);

        // uint64_t begin = get_time();

        num_p num_c = num_add(num_copy(num_a), num_copy(num_b));

        num_a = num_sqr(num_a);
        num_b = num_sqr(num_b);
        num_c = num_sqr(num_c);

        num_p num_a_new = num_add(num_copy(num_a), num_b);
        num_p num_b_new = num_sub(num_c, num_a);

        num_a = num_a_new;
        num_b = num_b_new;

        // uint64_t end = get_time();

        // if(i > min)
        //     printf("\t%5.3f", (double)(end - begin) / 1e9);

        num_display_full("a", num_b);

        // uint64_t size = 10;
        // flt_num_t f_a = flt_num_wrap_num(num_copy(num_a), size);
        // flt_num_t f_b = flt_num_wrap_num(num_copy(num_b), size);
        // flt_num_t f_p = flt_num_div(f_b, f_a);
        // printf("\n");
        // flt_num_display_dec(f_p);
    }
}

[[maybe_unused]]
static void factorial()
{
    num_p num = num_wrap(1);
    for(uint64_t i=1; ; i++)
    {
        num = num_mul(num, num_wrap(i));

        if(i%1000 == 0)
        {
            num_display_full("num", num);
            printf("\ni: " U64P() "\t", i);
        }
    }
}

[[maybe_unused]]
static num_p fib_1(uint64_t index)
{
    if(index < 2)
    {
        return num_wrap(1);
    }

    num_p num_a = num_wrap(1);
    num_p num_b = num_wrap(1);
    for(uint64_t i=1; i<index; i++)
    {
        num_p num_c = num_add(num_a, num_copy(num_b));
        num_a = num_b;
        num_b = num_c;
    }
    num_free(num_a);
    return num_b;
}

[[maybe_unused]]
static num_p fib_2(uint64_t index)
{
    num_p num_a = num_wrap(1);
    num_p num_b = num_wrap(0);
    num_p num_c = num_wrap(1);
    for(uint64_t mask = 0x8000000000000000; mask; mask >>= 1)
    {
        num_p num_a_2 = num_sqr(num_copy(num_a));
        num_p num_b_2 = num_sqr(num_copy(num_b));
        num_p num_c_2 = num_sqr(num_copy(num_c));

        num_a = num_add(num_a, num_c);
        num_b = num_mul(num_b, num_a);

        num_a = num_add(num_a_2, num_copy(num_b_2));
        num_c = num_add(num_b_2, num_c_2);

        if((mask & index) == 0)
        {
            continue;
        }

        num_c_2 = num_add(num_copy(num_b), num_copy(num_c));
        num_free(num_a);
        num_a = num_b;
        num_b = num_c;
        num_c = num_c_2;
    }
    num_free(num_a);
    num_free(num_b);
    return num_c;
}



[[maybe_unused]]
static void mod_num_fib(mod_num_p mod_a, mod_num_p mod_b)
{
    mod_num_t mod_c = mod_num_add(*mod_a, mod_num_copy(*mod_b));

    *mod_a = *mod_b;
    *mod_b = mod_c;
}

[[maybe_unused]]
static void time_dec()
{
    printf("\n");
    num_p num = num_generate_1(20, 2);
    printf("\ngenerated\n");
    printf("\n");
    uint64_t begin = get_time();
    num_display_dec(num);
    uint64_t end = get_time();
    printf("\n");
    printf("\ntime: %.3f", (double)(end - begin) / 1e3);
}



[[maybe_unused]]
static void pi_1()
{
    uint64_t pos = 3;
    fxd_num_t fxd = fxd_num_wrap(0, pos);
    for(uint64_t i=1; ; i++)
    {
        int64_t base = (int64_t)((16 * i * i) - (16 * i) + 3);
        fxd_num_t fxd_1 = fxd_num_wrap(8, pos);
        fxd_num_t fxd_2 = fxd_num_wrap(base, pos);
        fxd_1 = fxd_num_div(fxd_1, fxd_2);
        fxd = fxd_num_add(fxd, fxd_num_copy(fxd_1));

        if(i%1000000 == 0)
        {
            printf("\ni: " U64P() "\t", i);
            fxd_num_display_dec(fxd);
            printf("\t");
            fxd_num_display_dec(fxd_1);
            fxd_num_free(fxd_1);
        }
    }
}

[[maybe_unused]]
static void e()
{
    uint64_t pos = 1000;
    fxd_num_t fxd = fxd_num_wrap(0, pos);
    fxd_num_t base = fxd_num_wrap(1, pos);
    for(uint64_t i=1; ; i++)
    {
        fxd_num_t fxd_2 = fxd_num_wrap((int64_t)i, pos);
        base = fxd_num_div(base, fxd_2);
        fxd = fxd_num_add(fxd, fxd_num_copy(base));

        if(i%1000 == 0)
        {
            fprintf(stderr, "\ni: " U64P() "", i);
            fxd_num_display_dec(fxd);
        }
    }
}

[[maybe_unused]]
static void pi_2()
{
    uint64_t pos = 2;
    fxd_num_t fxd = fxd_num_wrap(2, pos);
    for(uint64_t i=1; ; i++)
    {
        uint64_t base = 4 * i * i;
        fxd = fxd_num_mul(fxd, fxd_num_wrap((int64_t)base, pos));
        fxd = fxd_num_div(fxd, fxd_num_wrap((int64_t)base - 1, pos));

        if(i%1000000 == 0)
        {
            printf("\n");
            fxd_num_display_dec(fxd);
        }
    }
}

[[maybe_unused]]
static void flt_num_pi_1()
{
    uint64_t size = 3;
    flt_num_t flt = flt_num_wrap(2, size);

    flt_num_t i_m_8 = flt_num_wrap(0, size);
    flt_num_t i_p_2 = flt_num_copy(i_m_8);
    flt_num_t flt_1 = flt_num_wrap(1, size);
    flt_num_t flt_4 = flt_num_wrap(4, size);
    flt_num_t flt_8 = flt_num_wrap(8, size);
    for(uint64_t i=0; ; i++)
    {
        i_p_2 = flt_num_add(i_p_2, flt_num_copy(flt_4));
        i_p_2 = flt_num_add(i_p_2, flt_num_copy(i_m_8));

        i_m_8 = flt_num_add(i_m_8, flt_num_copy(flt_8));

        flt = flt_num_mul(flt, flt_num_copy(i_p_2));

        flt_num_t flt_base = flt_num_sub(
            flt_num_copy(i_p_2),
            flt_num_copy(flt_1)
        );
        flt = flt_num_div(flt, flt_base);

        if(i%1000000 == 0)
        {
            printf("\n");
            flt_num_display_dec(flt);
        }
    }
}

[[maybe_unused]]
static void flt_num_pi_2(uint64_t size)
{
    flt_num_t flt = flt_num_wrap(3, size);
    flt_num_t flt_1_4 = flt_num_div(
        flt_num_wrap(1, size),
        flt_num_wrap(4, size)
    );
    flt_num_t flt_m_3_8 = flt_num_div(
        flt_num_wrap(-3, size),
        flt_num_wrap(8, size)
    );
    flt_num_t flt_1 = flt_num_wrap(1, size);
    flt_num_t flt_m_1_2 = flt_num_div(
        flt_num_wrap(-1, size),
        flt_num_wrap(2, size)
    );

    flt_num_t flt_a = flt_num_wrap(6, size);
    for(uint64_t i=1; ; i++)
    {
        flt_num_t flt_tmp = flt_num_wrap((int64_t)i, size);
        flt_tmp = flt_num_div(flt_num_copy(flt_m_3_8), flt_tmp);
        flt_tmp = flt_num_add(flt_num_copy(flt_1_4), flt_tmp);
        flt_a = flt_num_mul(flt_a, flt_tmp);

        flt_tmp = flt_num_wrap((int64_t)((2*i) + 1), size);
        flt_tmp = flt_num_div(flt_num_copy(flt_1), flt_tmp);
        flt_tmp = flt_num_add(flt_tmp, flt_num_copy(flt_m_1_2));

        flt_tmp = flt_num_mul(flt_tmp, flt_num_copy(flt_a));
        if(!flt_num_safe_add(flt, flt_tmp))
        {
            flt_num_free(flt_tmp);
            break;
        }

        flt = flt_num_add(flt, flt_tmp);

        if(i%1000 == 0)
        {
            printf("\n");
            flt_num_display_dec(flt);
        }
    }

    printf("\n");
    printf("\n");
    flt_num_display_dec(flt);
    flt_num_free(flt);

    flt_num_free(flt_a);
    flt_num_free(flt_1_4);
    flt_num_free(flt_m_3_8);
    flt_num_free(flt_1);
    flt_num_free(flt_m_1_2);
}

[[maybe_unused]]
static void flt_num_pi_3(uint64_t size)
{
    flt_num_t flt_a = flt_num_wrap(6, size);
    flt_num_t flt_pi = flt_num_wrap(3, size);

    for(uint64_t i=1; ; i++)
    {
        flt_a = flt_num_mul_sig(flt_a, sig_num_wrap((int64_t)(2 * i) - 3));
        flt_a = flt_num_div_sig(flt_a, sig_num_wrap((int64_t)(8 * i)));

        if(i%1000 == 0)
        {
            fprintf(stderr, "\nexp: " D64P() "", -((int64_t)flt_a.size + flt_a.exponent));
        }

        flt_num_t flt_b = flt_num_copy(flt_a);
        flt_b = flt_num_mul_sig(flt_b, sig_num_wrap(1 - (int64_t)(2 * i)));
        flt_b = flt_num_div_sig(flt_b, sig_num_wrap((int64_t)(4 * i) + 2));

        if(!flt_num_safe_add(flt_pi, flt_b))
        {
            flt_num_free(flt_b);
            break;
        }

        flt_pi = flt_num_add(flt_pi, flt_b);
    }

    printf("\n");
    printf("\n");
    flt_num_display_dec(flt_pi);
    flt_num_free(flt_pi);
    flt_num_free(flt_a);
}



[[maybe_unused]]
static void display_bit(uint64_t value)
{
    for(uint64_t i=0; i<64; i++, value <<= 1)
    {
        printf("" U64PX "", value >> 63);
    }
}



[[maybe_unused]]
static fxd_num_t fxd_step(fxd_num_t fxd, uint64_t pos)
{
    fxd_num_t fxd_a = fxd_num_shr(fxd_num_copy(fxd), 1);
    fxd = fxd_num_div(fxd_num_wrap(1, pos), fxd);
    return fxd_num_add(fxd, fxd_a);
}

[[maybe_unused]]
static void sqrt_2()
{
    fxd_num_t fxd_x = fxd_num_wrap(1, 1);
    num_p num = num_wrap(10);

    for(uint64_t i=0; ; i++)
    {
        printf("\ni: " U64P() "", i);
        // fprintf(stderr, "\ni: %lu", i);

        fxd_x = fxd_step(fxd_x, 1);

        fxd_num_t fxd_2 = fxd_num_sqr(fxd_num_copy(fxd_x));
        fxd_2 = fxd_num_sub(fxd_2, fxd_num_wrap(2, 1));
        bool res = num_cmp(fxd_2.sig.num, num) < 0;
        fxd_num_free(fxd_2);
        if(res)
        {
            break;
        }
    }

    for(uint64_t i = 2; ; i *= 2)
    {
        fxd_x = fxd_num_reposition(fxd_x, i);
        fxd_x = fxd_step(fxd_x, i);

        // if(fork())
        //     continue;

        printf("\n\n");
        fxd_num_display_full(fxd_x);
        printf("\n");
        fxd_num_display_dec(fxd_x);
        printf("\n\npos: " U64P() "", i * 2);

        // exit(EXIT_SUCCESS);
    }

    // fxd_num_display_dec(fxd_x);

    // fxd_num_free(fxd_x);
    // num_free(num);
}



#ifdef DEBUG
[[maybe_unused]]
static void mem_1(uint64_t index)
{
    num_p num_1 = num_generate_1(index, 2);
    num_p num_2 = num_generate_1(index, 3);

    clu_clean_max_occupancy();
    uint64_t count_bef = clu_get_register_count();
    num_1 = num_mul(num_1, num_2);
    uint64_t count_aft = clu_get_register_count();

    printf("\nregiter_count: " U64P() "", count_aft - count_bef);
    printf("\tmax occupancy: " U64P() "", clu_get_max_occupancy());

    num_free(num_1);
}
#else
[[maybe_unused]]
static void mem_1(uint64_t)
{
    printf("\n\tfunction should be run with ./run_debug");
    revert()
}
#endif



[[maybe_unused]]
static void time_assembly_mul()
{
#ifdef DEBUG
    uint64_t base = 22;
#else
    uint64_t base = 30;
#endif

    tprintf("base: " U64P() "", base);

    num_p num_1 = num_generate_1(base, 2);
    num_p num_2 = num_add(num_copy(num_1), num_wrap(1));

    tprintf("num_1->count: " U64P() "", num_1->count);
    tprintf("num_2->count: " U64P() "", num_2->count);

    num_p num_1_c = num_copy(num_1);

    TIME_SETUP
    // clu_log_level_set(CLU_LOG_DYNAMIC);
    num_p num_res = num_mul(num_1_c, num_2);
    TIME_END(t1)
    tprintf("time mul: %.3f", dtime(t1));

    // TIME_RESET
    // num_res = num_div(num_res, num_1); // NOLINT(readability-suspicious-call-argument)
    // TIME_END(t2)
    // tprintf("time div: %.3f", dtime(t2));

    num_free(num_res);

#ifdef DEBUG
    uint64_t count = clu_get_register_count();
    tprintf("total allocations : " U64P() "", count);
    tprintf("max occupancy     : " U64P() "", clu_get_max_occupancy());
    assert(clu_mem_is_empty());
#endif
}

[[maybe_unused]]
static void time_threads_mul()
{
#ifdef DEBUG
    uint64_t base = 22;
#else
    uint64_t base = 30;
#endif

    tprintf("base: " U64P() "", base);

    num_p num_1 = num_generate_1(base, 2);
    num_p num_2 = num_add(num_copy(num_1), num_wrap(1));

    tprintf("num_1->count: " U64P() "", num_1->count);
    tprintf("num_2->count: " U64P() "", num_2->count);

    uint64_t thread_counts[] = {1, 2, 4, 8, 16};
    num_p num_res_ref = nullptr;

    for(uint64_t i=0; i<sizeof(thread_counts)/sizeof(thread_counts[0]); i++)
    {
        uint64_t threads = thread_counts[i];

        num_p num_1_c = num_copy(num_1);
        num_p num_2_c = num_copy(num_2);

        TIME_SETUP
        num_p num_res = num_mul_threads(num_1_c, num_2_c, threads);
        TIME_END(t1)
        tprintf("threads: " U64P(2) "  time: %.3f", threads, dtime(t1));

        if(num_res_ref == nullptr)
        {
            num_res_ref = num_copy(num_res);
        }
        else
        {
            assert(num_cmp(num_res, num_res_ref) == 0)
        }

        num_free(num_res);
    }

    num_free(num_res_ref);
    num_free(num_1);
    num_free(num_2);

#ifdef DEBUG
    uint64_t count = clu_get_register_count();
    tprintf("total allocations : " U64P() "", count);
    tprintf("max occupancy     : " U64P() "", clu_get_max_occupancy());
    assert(clu_mem_is_empty());
#endif
}

[[maybe_unused]]
static void time_assembly_sqr()
{
#ifdef DEBUG
    uint64_t base = 22;
#else
    uint64_t base = 28;
#endif

    tprintf("base: " U64P() "", base);

    num_p num = num_generate_1(base, 2);

    tprintf("num->count: " U64P() "", num->count);

    num_p num_res, num_c;

    TIME_SETUP

    // TIME_RESET
    // // clu_log_level_set(CLU_LOG_DYNAMIC);
    // num_res = num_mul_classic(num, num);
    // TIME_END(t1)
    // tprintf("time mul classic : %.3f", dtime(t1));
    // num_free(num_res);

    // num_c = num_copy(num);
    // TIME_RESET
    // // clu_log_level_set(CLU_LOG_DYNAMIC);
    // num_res = num_sqr_classic(num_c);
    // TIME_END(t2)
    // tprintf("time sqr classic : %.3f", dtime(t2));
    // num_free(num_res);

    num_p num_c_1 = num_copy(num);
    num_p num_c_2 = num_copy(num);
    TIME_RESET
    // clu_log_level_set(CLU_LOG_DYNAMIC);
    num_res = num_mul(num_c_1, num_c_2);
    TIME_END(t3)
    tprintf("time mul ssm     : %.3f", dtime(t3));
    num_free(num_res);

    num_c = num_copy(num);
    TIME_RESET
    // clu_log_level_set(CLU_LOG_DYNAMIC);
    num_res = num_sqr(num_c);
    TIME_END(t4)
    tprintf("time sqr ssm     : %.3f", dtime(t4));
    num_free(num_res);

#ifdef DEBUG
    uint64_t count = clu_get_register_count();
    tprintf("total allocations : " U64P() "", count);
    tprintf("max occupancy     : " U64P() "", clu_get_max_occupancy());
    assert(clu_mem_is_empty());
#endif
}

[[maybe_unused]]
static void time_assembly_div()
{
#ifdef DEBUG
    uint64_t base = 28;
#else
    uint64_t base = 28;
#endif

    tprintf("base: " U64P() "", base);

    num_p num_2 = num_generate_1(base, 2);
    num_p num_1 = num_generate_1(base + 1, 3);

    tprintf("num_1->count: " U64P() "", num_1->count);
    tprintf("num_2->count: " U64P() "", num_2->count);

    TIME_SETUP

    num_p num_1_c = num_copy(num_1);
    num_p num_2_c = num_copy(num_2);

    TIME_RESET
    // clu_log_level_set(CLU_LOG_DYNAMIC);
    num_p num_q = num_div(num_1_c, num_2_c);
    TIME_END(t1)

    tprintf("time div         : %.3f", dtime(t1));

    num_free(num_q);
    num_free(num_1);
    num_free(num_2);

#ifdef DEBUG
    uint64_t count = clu_get_register_count();
    tprintf("total allocations : " U64P() "", count);
    tprintf("max occupancy     : " U64P() "", clu_get_max_occupancy());
    assert(clu_mem_is_empty());
#endif
}



[[maybe_unused]]
static void time_threads_div()
{
#ifdef DEBUG
    uint64_t base = 22;
#else
    uint64_t base = 28;
#endif

    tprintf("base: " U64P() "", base);

    num_p num_2 = num_generate_1(base, 2);
    num_p num_1 = num_generate_1(base + 1, 3);

    tprintf("num_1->count: " U64P() "", num_1->count);
    tprintf("num_2->count: " U64P() "", num_2->count);

    uint64_t thread_counts[] = {1, 2, 4, 8, 16};
    num_p num_res_ref = nullptr;

    for(uint64_t i=0; i<sizeof(thread_counts)/sizeof(thread_counts[0]); i++)
    {
        uint64_t threads = thread_counts[i];

        num_p num_1_c = num_copy(num_1);
        num_p num_2_c = num_copy(num_2);

        TIME_SETUP
        num_p num_res = num_div_threads(num_1_c, num_2_c, threads);
        TIME_END(t1)
        tprintf("threads: " U64P(2) "  time: %.3f", threads, dtime(t1));

        if(num_res_ref == nullptr)
        {
            num_res_ref = num_copy(num_res);
        }
        else
        {
            assert(num_cmp(num_res, num_res_ref) == 0)
        }

        num_free(num_res);
    }

    num_free(num_res_ref);
    num_free(num_1);
    num_free(num_2);

#ifdef DEBUG
    uint64_t count = clu_get_register_count();
    tprintf("total allocations : " U64P() "", count);
    tprintf("max occupancy     : " U64P() "", clu_get_max_occupancy());
    assert(clu_mem_is_empty());
#endif
}


constexpr uint64_t processor_siblings = 2;

static uint64_t processor_of(uint64_t index)
{
    uint64_t cores = (uint64_t)sysconf(_SC_NPROCESSORS_ONLN) / processor_siblings;
    if(index < cores)
    {
        return processor_siblings * index;
    }
    return (processor_siblings * (index - cores)) + 1;
}

static void lock_processor([[maybe_unused]] uint64_t index)
{
#if defined(__linux__)

    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(index, &cpu_set);
    assert(sched_setaffinity(0, sizeof(cpu_set), &cpu_set) == 0)

#elif defined(__APPLE__)

    thread_affinity_policy_data_t policy = { .affinity_tag = (integer_t)index };
    thread_policy_set(
        pthread_mach_thread_np(pthread_self()),
        THREAD_AFFINITY_POLICY,
        (thread_policy_t)&policy,
        THREAD_AFFINITY_POLICY_COUNT
    );

#endif
}


[[maybe_unused]]
static void time_procs_mul()
{
#ifdef DEBUG
    uint64_t base = 22;
#else
    uint64_t base = 28;
#endif

    tprintf("base: " U64P() "", base);

    num_p num_1 = num_generate_1(base, 2);
    num_p num_2 = num_add(num_copy(num_1), num_wrap(1));

    tprintf("num_1->count: " U64P() "", num_1->count);
    tprintf("num_2->count: " U64P() "", num_2->count);

    num_p num_res_ref = num_mul_threads(num_copy(num_1), num_copy(num_2), 1);

    constexpr uint64_t procs_max = 16;
    uint64_t proc_counts[] = {1, 2, 4, 8, 16};

    double * times = mmap(
        nullptr,
        procs_max * sizeof(double),
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0
    );
    assert(times != MAP_FAILED);

    double solo = 0;

    for(uint64_t i=0; i<sizeof(proc_counts)/sizeof(proc_counts[0]); i++)
    {
        uint64_t procs = proc_counts[i];
        pid_t pid[procs_max];

        for(uint64_t p=0; p<procs; p++)
        {
            pid[p] = fork_safe();
            if(pid[p] == 0)
            {
                lock_processor(processor_of(p));

                num_p num_1_c = num_copy(num_1);
                num_p num_2_c = num_copy(num_2);

                TIME_SETUP
                num_p num_res = num_mul_threads(num_1_c, num_2_c, 1);
                TIME_END(t1)

                assert(num_cmp(num_res, num_res_ref) == 0)
                times[p] = dtime(t1);

                num_free(num_res);
                exit(EXIT_SUCCESS);
            }
        }

        for(uint64_t p=0; p<procs; p++)
        {
            waitpid_safe(pid[p], nullptr);
        }

        double total = 0;
        double worst = 0;
        for(uint64_t p=0; p<procs; p++)
        {
            total += times[p];
            worst = times[p] > worst ? times[p] : worst;
        }

        double mean = total / (double)procs;
        if(i == 0)
        {
            solo = mean;
        }

        tprintf(
            "procs: " U64P(2) "  mean: %.3f  max: %.3f  vs solo: %.2fx  throughput: %.2fx",
            procs, mean, worst, mean / solo, ((double)procs * solo) / mean
        );
    }

    assert(munmap(times, procs_max * sizeof(double)) == 0);
    num_free(num_res_ref);
    num_free(num_1);
    num_free(num_2);

#ifdef DEBUG
    uint64_t count = clu_get_register_count();
    tprintf("total allocations : " U64P() "", count);
    tprintf("max occupancy     : " U64P() "", clu_get_max_occupancy());
    assert(clu_mem_is_empty());
#endif
}



[[maybe_unused]]
static void affinity_span([[maybe_unused]] uint64_t threads, [[maybe_unused]] bool spread)
{
#if defined(__linux__)

    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    for(uint64_t i = 0; i < threads; i++)
    {
        CPU_SET(spread ? (processor_siblings * i) : i, &cpu_set);
    }
    assert(sched_setaffinity(0, sizeof(cpu_set), &cpu_set) == 0)

#endif
}

[[maybe_unused]]
static void affinity_all()
{
#if defined(__linux__)

    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    for(uint64_t i = 0; i < (uint64_t)sysconf(_SC_NPROCESSORS_ONLN); i++)
    {
        CPU_SET(i, &cpu_set);
    }
    assert(sched_setaffinity(0, sizeof(cpu_set), &cpu_set) == 0)

#endif
}

[[maybe_unused]]
static void time_smt_mul()
{
#ifdef DEBUG
    uint64_t base = 22;
#else
    uint64_t base = 28;
#endif

    tprintf("base: " U64P() "", base);

    num_p num_1 = num_generate_1(base, 2);
    num_p num_2 = num_add(num_copy(num_1), num_wrap(1));

    tprintf("num_1->count: " U64P() "", num_1->count);

    num_p num_res_ref = num_mul_threads(num_copy(num_1), num_copy(num_2), 1);

    uint64_t thread_counts[] = {2, 8};

    for(uint64_t i=0; i<sizeof(thread_counts)/sizeof(thread_counts[0]); i++)
    {
        uint64_t threads = thread_counts[i];

        for(uint64_t s=0; s<2; s++)
        {
            bool spread = (bool)s;
            affinity_span(threads, spread);

            num_p num_1_c = num_copy(num_1);
            num_p num_2_c = num_copy(num_2);

            TIME_SETUP
            num_p num_res = num_mul_threads(num_1_c, num_2_c, threads);
            TIME_END(t1)

            assert(num_cmp(num_res, num_res_ref) == 0)
            num_free(num_res);

            tprintf(
                "threads: " U64P(2) "  cores: " U64P(2) "  %-6s  time: %.3f",
                threads,
                spread ? threads : ((threads + processor_siblings - 1) / processor_siblings),
                spread ? "spread" : "packed",
                dtime(t1)
            );
        }
    }

    affinity_all();
    num_free(num_res_ref);
    num_free(num_1);
    num_free(num_2);

#ifdef DEBUG
    uint64_t count = clu_get_register_count();
    tprintf("total allocations : " U64P() "", count);
    tprintf("max occupancy     : " U64P() "", clu_get_max_occupancy());
    assert(clu_mem_is_empty());
#endif
}



// int main(int argc, char** argv)
int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    srand((unsigned int)time(nullptr)); // NOLINT(cert-msc51-cpp, cert-msc32-c)
    printf("\nbegin\n\n");

    // clu_log_level_set(CLU_LOG_ALL);

    // num_generate(21, 2);
    // time_1(16, 28);
    // time_1(16, 17);
    // time_2(argc, argv, 25, 10000);
    // time_2_total(argc, argv);
    // time_3();
    // time_4();

    // fibonacci();
    // fibonacci_2(16, 23);
    // fibonacci_3(16, 40);
    // sqrt_2();
    // e();
    // pi_2();
    // flt_num_pi_1();
    // flt_num_pi_2(1000);
    // flt_num_pi_3(1000);
    // mem_1(21);
    // time_assembly_mul();
    // time_threads_mul();
    time_threads_div();
    // time_procs_mul();
    // time_smt_mul();
    // time_assembly_sqr();
    // time_assembly_div();

    printf("\n");
    return 0;
}
