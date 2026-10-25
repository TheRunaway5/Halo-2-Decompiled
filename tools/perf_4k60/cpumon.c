// LD_PRELOAD monitor: correlates rows appended to HALO2_RENDER_TIMINGS with
// main-thread CPU time, so per-frame CPU cost survives host contention.
// Writes "frames cpu_ns wall_ns" rows to H2_CPU_MONITOR_OUT on exit.
#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define CAP 4096
static struct { long frames; uint64_t cpu, wall; } rows[CAP];
static volatile int nrows, stop;
static const char *tsv, *out;
static clockid_t main_clock;

static uint64_t now(clockid_t c) { struct timespec t; clock_gettime(c, &t); return (uint64_t)t.tv_sec * 1000000000ull + t.tv_nsec; }

static void *watch(void *arg) {
    (void)arg;
    long lines = 0, offset = 0, last = -1;
    char buf[65536];
    while (!stop) {
        struct stat st;
        if (stat(tsv, &st) == 0 && st.st_size < offset) { offset = 0; lines = 0; }
        FILE *f = fopen(tsv, "r");
        if (f) {
            fseek(f, offset, SEEK_SET);
            size_t n;
            while ((n = fread(buf, 1, sizeof buf, f)) > 0) {
                for (size_t i = 0; i < n; ++i) lines += buf[i] == '\n';
                offset += (long)n;
            }
            fclose(f);
        }
        long frames = lines > 0 ? lines - 1 : 0;
        if (frames != last && nrows < CAP) {
            rows[nrows].frames = frames; rows[nrows].cpu = now(main_clock); rows[nrows].wall = now(CLOCK_MONOTONIC);
            ++nrows; last = frames;
        }
        struct timespec d = {0, 500000}; nanosleep(&d, NULL);
    }
    return NULL;
}

static void dump(void) {
    stop = 1;
    FILE *f = fopen(out, "w");
    if (!f) return;
    for (int i = 0; i < nrows; ++i) fprintf(f, "%ld %llu %llu\n", rows[i].frames, (unsigned long long)rows[i].cpu, (unsigned long long)rows[i].wall);
    fclose(f);
}

__attribute__((constructor)) static void init(void) {
    tsv = getenv("HALO2_RENDER_TIMINGS"); out = getenv("H2_CPU_MONITOR_OUT");
    if (!tsv || !out) return;
    unsetenv("LD_PRELOAD");
    pthread_getcpuclockid(pthread_self(), &main_clock);
    atexit(dump);
    pthread_t t; pthread_create(&t, NULL, watch, NULL); pthread_detach(t);
}
