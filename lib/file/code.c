#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "../../mods/clu/header.h"
#include "../../mods/macros/assert.h" // IWYU pragma: keep

#include "../num/header.h"
#include "../num/struct.h"



#ifdef DEBUG

#include "../num/debug.h"

#endif



static void fseek_safe(FILE *fp, long pos, int whence)
{
    if (fseek(fp, pos, whence) != 0)
    {
        exit(EXIT_FAILURE);
    }
}

static uint64_t ftell_safe(FILE *fp)
{
    int64_t res = ftell(fp);
    if (res < 0)
    {
        exit(EXIT_FAILURE);
    }

    return (uint64_t)res;
}

void file_write_uint64(file_p fp, uint64_t value)
{
    fwrite(&value, sizeof(uint64_t), 1, fp->fp);
}

void file_write_int64(file_p fp, int64_t value)
{
    fwrite(&value, sizeof(int64_t), 1, fp->fp);
}

file_t file_write_open(const char file_path[], uint64_t amount)
{
    FILE *fp = fopen(file_path, "wb");
    assert(fp);


    file_t res = (file_t)
    {
        .fp = fp,
        .amount = amount,
        .count = 0,
        .pos = (amount + 1) * sizeof(uint64_t)
    };
    file_write_uint64(&res, amount);
    return res;
}

static constexpr uint64_t MAGIC = 0xd0bbe;

void file_write_close(file_p fp)
{
    assert(fp->amount == fp->count);

    file_write_uint64(fp, MAGIC);
    fclose(fp->fp);
}

void file_write_start(file_p fp)
{
    assert(fp->count < fp->amount);

    fseek_safe(fp->fp, (long)((fp->count + 1) * sizeof(uint64_t)), SEEK_SET);
    file_write_uint64(fp, fp->pos);
    fseek_safe(fp->fp, (long)fp->pos, SEEK_SET);
}

void file_write_end(file_p fp)
{
    fp->pos = ftell_safe(fp->fp);
    fp->count++;
}



FILE* file_read_open(const char file_path[])
{
    FILE *fp = fopen(file_path, "rb");
    if(fp == nullptr)
    {
        return nullptr;
    }

    fseek_safe(fp, 0, SEEK_END);
    uint64_t size = ftell_safe(fp);
    if(size < sizeof(uint64_t))
    {
        fclose(fp);
        return nullptr;
    }

    fseek_safe(fp, -(long)sizeof(uint64_t), SEEK_END);
    uint64_t code = file_read_uint64(fp);

    if(code != MAGIC)
    {
        fclose(fp);
        return nullptr;
    }

    return fp;
}

void file_read_move_to_index(FILE *fp, uint64_t index)
{
    fseek_safe(fp, 0, SEEK_SET);
    uint64_t amount = file_read_uint64(fp);
    assert(index < amount);

    fseek_safe(fp, (long)((index + 1) * sizeof(uint64_t)), SEEK_SET);
    uint64_t pos = file_read_uint64(fp);

    fseek_safe(fp, (long)pos, SEEK_SET);
}

uint64_t file_read_uint64(FILE *fp)
{
    uint64_t res;
    assert(fread(&res, sizeof(uint64_t), 1, fp) == 1);
    return res;
}

int64_t file_read_int64(FILE *fp)
{
    int64_t res;
    assert(fread(&res, sizeof(int64_t), 1, fp) == 1);
    return res;
}
