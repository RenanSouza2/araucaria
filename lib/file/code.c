#include <stdio.h>
#include <stdlib.h>

#include "debug.h" // IWYU pragma: keep
#include "../../mods/clu/header.h" // IWYU pragma: keep
#include "../../mods/macros/assert.h" // IWYU pragma: keep



#ifdef DEBUG
#endif



// Layout: [amount][end_0..end_{amount-1}][entry_0]..[entry_n][MAGIC_FILE].
// Slot i holds the offset one past entry i, 0 until committed, so entry i
// spans [i ? end_{i-1} : header, end_i). MAGIC_ENTRY, written after the
// payload, is what certifies an entry; the slot is not.
static constexpr uint64_t MAGIC_FILE = 0xd0bbe;
static constexpr uint64_t MAGIC_ENTRY = 0xe10be;

static uint64_t file_header_size(uint64_t amount)
{
    return (amount + 1) * sizeof(uint64_t);
}

static uint64_t file_slot_pos(uint64_t index)
{
    return (index + 1) * sizeof(uint64_t);
}

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
    assert(res >= 0);
    return (uint64_t)res;
}

static uint64_t file_size(FILE *fp)
{
    fseek_safe(fp, 0, SEEK_END);
    return ftell_safe(fp);
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
        .pos = file_header_size(amount)
    };
    file_write_uint64(&res, amount);
    for(uint64_t i=0; i<amount; i++)
    {
        file_write_uint64(&res, 0);
    }
    return res;
}

void file_write_close(file_p fp)
{
    assert(fp->amount == fp->count);

    fseek_safe(fp->fp, (long)fp->pos, SEEK_SET);
    file_write_uint64(fp, MAGIC_FILE);
    fclose(fp->fp);
}

void file_write_start(file_p fp)
{
    assert(fp->count < fp->amount);

    fseek_safe(fp->fp, (long)fp->pos, SEEK_SET);
}

// Ordering: the seek flushes the payload before the slot offset that certifies
// it, and the offset must be flushed too, never left in the stream buffer.
void file_write_end(file_p fp)
{
    file_write_uint64(fp, MAGIC_ENTRY);

    uint64_t end = ftell_safe(fp->fp);
    fseek_safe(fp->fp, (long)file_slot_pos(fp->count), SEEK_SET);
    file_write_uint64(fp, end);
    fflush(fp->fp);

    fp->pos = end;
    fp->count++;
}



// Entries are written in order, so the first invalid slot ends the run.
static uint64_t file_read_count(FILE *fp, uint64_t amount, uint64_t size)
{
    uint64_t pos = file_header_size(amount);
    for(uint64_t i=0; i<amount; i++)
    {
        fseek_safe(fp, (long)file_slot_pos(i), SEEK_SET);
        uint64_t end = file_read_uint64(fp);
        if(end < pos + sizeof(uint64_t) || end > size)
        {
            return i;
        }

        fseek_safe(fp, (long)(end - sizeof(uint64_t)), SEEK_SET);
        if(file_read_uint64(fp) != MAGIC_ENTRY)
        {
            return i;
        }

        pos = end;
    }

    return amount;
}

// Falls back to file_write_open when the file is missing, unreadable, or was
// written for a different amount.
file_t file_write_open_resume(const char file_path[], uint64_t amount)
{
    FILE *fp = fopen(file_path, "r+b");
    if(fp == nullptr)
    {
        return file_write_open(file_path, amount);
    }

    uint64_t size = file_size(fp);
    if(size < file_header_size(amount))
    {
        fclose(fp);
        return file_write_open(file_path, amount);
    }

    fseek_safe(fp, 0, SEEK_SET);
    if(file_read_uint64(fp) != amount)
    {
        fclose(fp);
        return file_write_open(file_path, amount);
    }

    uint64_t count = file_read_count(fp, amount, size);
    uint64_t pos = file_header_size(amount);
    if(count > 0)
    {
        fseek_safe(fp, (long)file_slot_pos(count - 1), SEEK_SET);
        pos = file_read_uint64(fp);
    }

    return (file_t)
    {
        .fp = fp,
        .amount = amount,
        .count = count,
        .pos = pos
    };
}



FILE* file_read_open(const char file_path[])
{
    FILE *fp = fopen(file_path, "rb");
    if(fp == nullptr)
    {
        return nullptr;
    }

    uint64_t size = file_size(fp);
    if(size < 2 * sizeof(uint64_t))
    {
        fclose(fp);
        return nullptr;
    }

    fseek_safe(fp, -(long)sizeof(uint64_t), SEEK_END);
    uint64_t code = file_read_uint64(fp);

    if(code != MAGIC_FILE)
    {
        fclose(fp);
        return nullptr;
    }

    fseek_safe(fp, 0, SEEK_SET);
    uint64_t amount = file_read_uint64(fp);
    if(amount == 0 || amount > size / sizeof(uint64_t))
    {
        fclose(fp);
        return nullptr;
    }

    // The last entry must end where MAGIC_FILE begins; a payload word equal
    // to MAGIC_FILE is not on its own proof of completeness.
    fseek_safe(fp, (long)file_slot_pos(amount - 1), SEEK_SET);
    if(file_read_uint64(fp) != size - sizeof(uint64_t))
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

    uint64_t pos = file_header_size(amount);
    if(index > 0)
    {
        fseek_safe(fp, (long)file_slot_pos(index - 1), SEEK_SET);
        pos = file_read_uint64(fp);
    }

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
