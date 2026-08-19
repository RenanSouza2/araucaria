#ifndef SIG_H
#define SIG_H

#include <stdio.h>
#include <stdbool.h>

#include "struct.h"
#include "../file/struct.h"

void sig_num_display(sig_num_t sig, bool full);
void sig_num_display_tag(const char tag[], sig_num_t sig);
void sig_num_display_full(sig_num_t sig);
void sig_num_display_dec(sig_num_t sig);

sig_num_t sig_num_create(uint64_t signal, num_p num);

void sig_num_free(sig_num_t sig);

sig_num_t sig_num_wrap(int64_t value);
sig_num_t sig_num_wrap_int128(int128_t value);
sig_num_t sig_num_wrap_num(num_p num);
sig_num_t sig_num_wrap_str(const char str[]);
sig_num_t sig_num_copy(sig_num_t sig);
sig_num_t sig_num_realloc_disk(sig_num_t sig);

bool sig_num_is_zero(sig_num_t sig);
int64_t sig_num_cmp(sig_num_t sig_1, sig_num_t sig_2);

sig_num_t sig_num_shl(sig_num_t sig, uint64_t bits);
sig_num_t sig_num_shr(sig_num_t sig, uint64_t bits);

sig_num_t sig_num_opposite(sig_num_t sig);
sig_num_t sig_num_add(sig_num_t sig_1, sig_num_t sig_2);
sig_num_t sig_num_sub(sig_num_t sig_1, sig_num_t sig_2);
sig_num_t sig_num_mul(sig_num_t sig_1, sig_num_t sig_2);
sig_num_t sig_num_mul_threads(sig_num_t sig_1, sig_num_t sig_2, uint64_t threads);
sig_num_t sig_num_sqr(sig_num_t sig);
sig_num_t sig_num_div(sig_num_t sig_1, sig_num_t sig_2);
sig_num_t sig_num_div_threads(sig_num_t sig_1, sig_num_t sig_2, uint64_t threads);

sig_num_t sig_num_mul_int(sig_num_t sig, int64_t value);

void file_write_sig_num_raw(file_p fp, sig_num_t sig);
void file_write_sig_num(file_p fp, sig_num_t sig);
void sig_num_save(const char file_path[], sig_num_t sig);

sig_num_t file_read_sig_num_raw(FILE *fp);
sig_num_t file_read_sig_num(FILE *fp, uint64_t index);
sig_num_t sig_num_load(char file_path[]);

#endif
