// Raw file I/O throughput against the scratch volume, shaped like the staged
// transform's transfers: T threads moving fixed-size elements of one unlinked file.
//
// diskbw <dir> <file MB> <elem KB> <threads> <pattern> <flags> <ops>
//   pattern: seq     thread t owns one contiguous range
//            stride  thread t takes elements t, t+T, t+2T ... (interleaved)
//            wide    thread t takes blocks of 512 elements spaced by a large stride
//   flags:   any of  n = uncached I/O (F_NOCACHE on macOS, O_DIRECT on Linux),
//                    p = preallocate, b = 40 byte offset bias (not with n on Linux),
//                    a = keep the whole file mapped MAP_SHARED while it runs,
//                    s = Linux: start writeback of each element as it is written and
//                        drop the previous one from the page cache once it is on disk,
//                    d = Linux: drop each element from the page cache after reading it,
//                    f = one fd per thread, F = one file per thread (the file size is
//                    split between them), m = one write at a time (user mutex),
//                    M = one read at a time too, - = none
//   ops:     string of w (write pass), r (read pass) and x (odd threads write while
//            even threads read), run in order

// builds on macOS and Linux:  cc -O2 -o diskbw diskbw.c -pthread
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#else
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

typedef struct
{
    int fd;
    int write;
    uint64_t thread;
    uint64_t threads;
    uint64_t elems;
    uint64_t elem_bytes;
    uint64_t bias;
    int lock_write;
    int lock_read;
    int range_sync;
    int drop_read;
    off_t prev_off;
    const char * pattern;
    uint64_t io_ns;
    uint64_t bytes;
} worker_t;

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

static uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * 1000000000ull) + (uint64_t)ts.tv_nsec;
}

static void io_all(worker_t * w, char * buf, uint64_t elem)
{
    uint64_t left = w->elem_bytes;
    off_t off = (off_t)(w->bias + (elem * w->elem_bytes));
    char * p = buf;
    int lock = w->write ? w->lock_write : w->lock_read;
    uint64_t t0 = now_ns();
    if(lock)
    {
        pthread_mutex_lock(&g_lock);
    }
    while(left)
    {
        ssize_t n = w->write ? pwrite(w->fd, p, left, off) : pread(w->fd, p, left, off);
        if(n <= 0)
        {
            perror("io");
            exit(1);
        }
        left -= (uint64_t)n;
        p += n;
        off += n;
    }
    if(lock)
    {
        pthread_mutex_unlock(&g_lock);
    }

#ifdef __linux__
    off_t start = (off_t)(w->bias + (elem * w->elem_bytes));
    if(w->write && w->range_sync)
    {
        sync_file_range(w->fd, start, (off_t)w->elem_bytes, SYNC_FILE_RANGE_WRITE);
        if(w->prev_off >= 0)
        {
            sync_file_range(
                w->fd, w->prev_off, (off_t)w->elem_bytes,
                SYNC_FILE_RANGE_WAIT_BEFORE | SYNC_FILE_RANGE_WRITE | SYNC_FILE_RANGE_WAIT_AFTER
            );
            posix_fadvise(w->fd, w->prev_off, (off_t)w->elem_bytes, POSIX_FADV_DONTNEED);
        }
        w->prev_off = start;
    }
    if(!w->write && w->drop_read)
    {
        posix_fadvise(w->fd, start, (off_t)w->elem_bytes, POSIX_FADV_DONTNEED);
    }
#endif
    w->io_ns += now_ns() - t0;
    w->bytes += w->elem_bytes;
}

static void * worker(void * arg)
{
    worker_t * w = arg;
    char * buf;
    if(posix_memalign((void **)&buf, 16384, w->elem_bytes + 16384))
    {
        exit(1);
    }
    for(uint64_t i = 0; i < w->elem_bytes; i += 8)
    {
        uint64_t v = (i * 0x9e3779b97f4a7c15ull) ^ (w->thread << 56);
        memcpy(buf + i, &v, 8);
    }

    if(strcmp(w->pattern, "seq") == 0)
    {
        uint64_t start = (w->elems * w->thread) / w->threads;
        uint64_t end = (w->elems * (w->thread + 1)) / w->threads;
        for(uint64_t e = start; e < end; e++)
        {
            io_all(w, buf, e);
        }
    }
    else if(strcmp(w->pattern, "stride") == 0)
    {
        for(uint64_t e = w->thread; e < w->elems; e += w->threads)
        {
            io_all(w, buf, e);
        }
    }
    else
    {
        // blocks of `width` elements with stride gl = elems / width, like a first
        // fused pass: block a holds elements a, a + gl, a + 2 gl ...
        uint64_t width = 512;
        uint64_t gl = w->elems / width;
        for(uint64_t a = w->thread; a < gl; a += w->threads)
        {
            for(uint64_t b = 0; b < width; b++)
            {
                io_all(w, buf, a + (gl * b));
            }
        }
    }

    free(buf);
    return NULL;
}

static double tv_s(struct timeval tv)
{
    return (double)tv.tv_sec + ((double)tv.tv_usec / 1e6);
}

int main(int argc, char ** argv)
{
    if(argc < 8)
    {
        fprintf(stderr, "usage: diskbw <dir> <file MB> <elem KB> <threads> <seq|stride|wide> <flags> <ops>\n");
        return 2;
    }

    const char * dir = argv[1];
    uint64_t file_bytes = strtoull(argv[2], NULL, 10) << 20;
    uint64_t elem_bytes = strtoull(argv[3], NULL, 10) << 10;
    uint64_t threads = strtoull(argv[4], NULL, 10);
    const char * pattern = argv[5];
    const char * flags = argv[6];
    const char * ops = argv[7];

    int nocache = strchr(flags, 'n') != NULL;
    int prealloc = strchr(flags, 'p') != NULL;
    int per_thread_fd = strchr(flags, 'f') != NULL;
    uint64_t bias = strchr(flags, 'b') ? 40 : 0;
    int lock_read = strchr(flags, 'M') != NULL;
    int lock_write = lock_read || strchr(flags, 'm') != NULL;
    uint64_t elems = file_bytes / elem_bytes;

    char path[1024];
    snprintf(path, sizeof(path), "%s/diskbw_XXXXXX", dir);
    int fd = mkstemp(path);
    if(fd < 0)
    {
        perror("mkstemp");
        return 1;
    }

    if(prealloc)
    {
        uint64_t t0 = now_ns();
#ifdef __APPLE__
        fstore_t fst =
        {
            .fst_flags = F_ALLOCATEALL,
            .fst_posmode = F_PEOFPOSMODE,
            .fst_offset = 0,
            .fst_length = (off_t)(file_bytes + bias),
        };
        if(fcntl(fd, F_PREALLOCATE, &fst) == -1)
        {
            perror("F_PREALLOCATE");
        }
#else
        int err = posix_fallocate(fd, 0, (off_t)(file_bytes + bias));
        if(err)
        {
            fprintf(stderr, "posix_fallocate: %s\n", strerror(err));
        }
#endif
        printf("prealloc %.3f s\n", (double)(now_ns() - t0) / 1e9);
    }
    if(ftruncate(fd, (off_t)(file_bytes + bias)))
    {
        perror("ftruncate");
        return 1;
    }

    void * map = MAP_FAILED;
    if(strchr(flags, 'a'))
    {
        map = mmap(NULL, file_bytes + bias, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if(map == MAP_FAILED)
        {
            perror("mmap");
            return 1;
        }
    }

    int per_thread_file = strchr(flags, 'F') != NULL;
    if(per_thread_file)
    {
        per_thread_fd = 1;
        file_bytes /= threads;
        elems = file_bytes / elem_bytes;
    }

    int * fds = malloc(threads * sizeof(int));
    for(uint64_t t = 0; t < threads; t++)
    {
        fds[t] = per_thread_fd ? open(path, O_RDWR) : fd;
        if(per_thread_file)
        {
            char own[1024];
            snprintf(own, sizeof(own), "%s/diskbw_XXXXXX", dir);
            fds[t] = mkstemp(own);
            unlink(own);
            if(fds[t] >= 0 && ftruncate(fds[t], (off_t)(file_bytes + bias)))
            {
                perror("ftruncate");
                return 1;
            }
        }
        if(fds[t] < 0)
        {
            perror("open");
            return 1;
        }
#ifdef __APPLE__
        if(nocache && fcntl(fds[t], F_NOCACHE, 1) == -1)
        {
            perror("F_NOCACHE");
        }
#else
        if(nocache && fcntl(fds[t], F_SETFL, fcntl(fds[t], F_GETFL) | O_DIRECT) == -1)
        {
            perror("O_DIRECT");
        }
#endif
    }
    unlink(path);

    pthread_t * ids = malloc(threads * sizeof(pthread_t));
    worker_t * ws = malloc(threads * sizeof(worker_t));

    for(const char * op = ops; *op; op++)
    {
        struct rusage ru0, ru1;
        getrusage(RUSAGE_SELF, &ru0);
        uint64_t t0 = now_ns();
        for(uint64_t t = 0; t < threads; t++)
        {
            ws[t] = (worker_t)
            {
                .fd = fds[t],
                .write = *op == 'w' || (*op == 'x' && (t & 1)),
                .thread = per_thread_file ? 0 : t,
                .threads = per_thread_file ? 1 : threads,
                .elems = elems,
                .elem_bytes = elem_bytes,
                .bias = bias,
                .lock_write = lock_write,
                .lock_read = lock_read,
                .range_sync = strchr(flags, 's') != NULL,
                .drop_read = strchr(flags, 'd') != NULL,
                .prev_off = -1,
                .pattern = pattern,
            };
            pthread_create(&ids[t], NULL, worker, &ws[t]);
        }
        uint64_t io_ns = 0;
        uint64_t bytes = 0;
        uint64_t bytes_wr = 0;
        for(uint64_t t = 0; t < threads; t++)
        {
            pthread_join(ids[t], NULL);
            io_ns += ws[t].io_ns;
            bytes += ws[t].bytes;
            bytes_wr += ws[t].write ? ws[t].bytes : 0;
        }
        double wall = (double)(now_ns() - t0) / 1e9;
        getrusage(RUSAGE_SELF, &ru1);
        if(*op == 'x')
        {
            printf(
                "x  mixed: read %8.1f MB/s  write %8.1f MB/s   ",
                ((double)(bytes - bytes_wr) / (1024.0 * 1024.0)) / wall,
                ((double)bytes_wr / (1024.0 * 1024.0)) / wall
            );
        }

        printf(
            "%c  file %6llu MB  elem %6llu KB  thr %2llu  %-6s flags %-5s  %8.1f MB/s  wall %7.2f s"
            "  user %6.2f  sys %6.2f  in-call %5.2f thr\n",
            *op,
            (unsigned long long)(file_bytes >> 20),
            (unsigned long long)(elem_bytes >> 10),
            (unsigned long long)threads,
            pattern,
            flags,
            ((double)bytes / (1024.0 * 1024.0)) / wall,
            wall,
            tv_s(ru1.ru_utime) - tv_s(ru0.ru_utime),
            tv_s(ru1.ru_stime) - tv_s(ru0.ru_stime),
            ((double)io_ns / 1e9) / wall
        );
        fflush(stdout);
    }

    uint64_t t0 = now_ns();
    if(map != MAP_FAILED)
    {
        munmap(map, file_bytes + bias);
    }
    for(uint64_t t = 0; t < threads; t++)
    {
        if(per_thread_fd)
        {
            close(fds[t]);
        }
    }
    close(fd);
    printf("close %.3f s\n", (double)(now_ns() - t0) / 1e9);
    return 0;
}
