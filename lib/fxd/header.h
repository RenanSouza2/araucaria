#ifndef FXD_H
#define FXD_H

#include <stdbool.h>

#include "struct.h"
#include "../file/struct.h"

void fxd_num_display_dec(fxd_num_t fxd);
void fxd_num_display(fxd_num_t fxd);
void fxd_num_display_tag(const char tag[], fxd_num_t fxd);
void fxd_num_display_full(fxd_num_t fxd);

fxd_num_t fxd_num_reposition(fxd_num_t fxd, uint64_t pos);

fxd_num_t fxd_num_wrap(int64_t value, uint64_t pos);
fxd_num_t fxd_num_wrap_sig(sig_num_t sig, uint64_t pos);
fxd_num_t fxd_num_copy(fxd_num_t fxd);
void fxd_num_free(fxd_num_t fxd);

fxd_num_t fxd_num_base_to(fxd_num_t fxd, uint64_t base);

int64_t fxd_num_cmp(fxd_num_t fxd_1, fxd_num_t fxd_2);
bool fxd_num_is_zero(fxd_num_t fxd);

fxd_num_t fxd_num_shl(fxd_num_t fxd, uint64_t value);
fxd_num_t fxd_num_shr(fxd_num_t fxd, uint64_t value);

fxd_num_t fxd_num_add(fxd_num_t fxd_1, fxd_num_t fxd_2);
fxd_num_t fxd_num_sub(fxd_num_t fxd_1, fxd_num_t fxd_2);
fxd_num_t fxd_num_mul(fxd_num_t fxd_1, fxd_num_t fxd_2);
fxd_num_t fxd_num_mul_threads(fxd_num_t fxd_1, fxd_num_t fxd_2, uint64_t threads);
fxd_num_t fxd_num_sqr(fxd_num_t fxd);
fxd_num_t fxd_num_div(fxd_num_t fxd_1, fxd_num_t fxd_2);
fxd_num_t fxd_num_div_threads(fxd_num_t fxd_1, fxd_num_t fxd_2, uint64_t threads);

fxd_num_t fxd_num_mul_sig(fxd_num_t fxd, sig_num_t sig);
fxd_num_t fxd_num_mul_sig_threads(fxd_num_t fxd, sig_num_t sig, uint64_t threads);
fxd_num_t fxd_num_div_sig(fxd_num_t fxd, sig_num_t sig);
fxd_num_t fxd_num_div_sig_threads(fxd_num_t fxd, sig_num_t sig, uint64_t threads);

void file_write_fxd_num_raw(file_p fp, fxd_num_t fxd);
void file_write_fxd_num(file_p fp, fxd_num_t fxd);
void fxd_num_save(const char file_path[], fxd_num_t fxd);

fxd_num_t file_read_fxd_num_raw(FILE *fp);
fxd_num_t file_read_fxd_num(FILE *fp, uint64_t index);
fxd_num_t fxd_num_load(const char file_path[]);

#endif
