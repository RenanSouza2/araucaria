#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <stdbool.h>

#include "struct.h"

file_t file_write_open(const char file_path[], uint64_t amount);
file_t file_write_open_resume(const char file_path[], uint64_t amount);
void file_write_close(file_p fp);
void file_write_uint64(file_p fp, uint64_t value);
void file_write_int64(file_p fp, int64_t value);
void file_write_start(file_p fp);
void file_write_end(file_p fp);

FILE* file_read_open(const char file_path[]);
void file_read_move_to_index(FILE *fp, uint64_t index);
uint64_t file_read_uint64(FILE *fp);
int64_t file_read_int64(FILE *fp);

#endif
