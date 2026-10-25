// LD_PRELOAD main-thread CPU-time sampler. Records interrupted RIP plus an
// unwound stack every H2_SAMPLE_US microseconds of main-thread CPU time,
// starting H2_SAMPLE_DELAY seconds after load. Dumps on exit to H2_SAMPLE_OUT.
#define _GNU_SOURCE
#include <execinfo.h>
#include <setjmp.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>
#include <ucontext.h>
#include <unistd.h>

#define DEPTH 48
#define CAPACITY 200000
typedef struct { uint64_t t; uint32_t n; uint64_t pc[DEPTH]; } Sample;
static Sample *samples;
static volatile uint32_t count;
static pid_t main_tid;
static timer_t timer;
static const char *out_path;
static int dumped;
static int unwind;
static sigjmp_buf unwind_escape;
static volatile sig_atomic_t unwinding;
static void unwind_fault(int sig) { (void)sig; if (unwinding) siglongjmp(unwind_escape, 1); }

static void handler(int sig, siginfo_t *info, void *context) {
    (void)sig; (void)info;
    if (syscall(SYS_gettid) != main_tid) return;
    uint32_t i = count;
    if (i >= CAPACITY) return;
    Sample *s = &samples[i];
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    s->t = (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
    ucontext_t *uc = (ucontext_t *)context;
    s->pc[0] = (uint64_t)uc->uc_mcontext.gregs[REG_RIP];
    void *frames[DEPTH + 4];
    int n = 0;
    if (unwind) {
        // Unwinding through translated code can fault; recover and keep only
        // the interrupted PC rather than taking the process down.
        struct sigaction fault, old_segv, old_bus;
        memset(&fault, 0, sizeof fault);
        fault.sa_handler = unwind_fault; fault.sa_flags = SA_NODEFER;
        sigaction(SIGSEGV, &fault, &old_segv); sigaction(SIGBUS, &fault, &old_bus);
        unwinding = 1;
        if (!sigsetjmp(unwind_escape, 1)) n = backtrace(frames, DEPTH + 4);
        else n = 0;
        unwinding = 0;
        sigaction(SIGSEGV, &old_segv, NULL); sigaction(SIGBUS, &old_bus, NULL);
    }
    // Skip handler/trampoline frames up to and including the interrupted PC.
    int start = 0;
    if (!n) { s->n = 1; count = i + 1; return; }
    for (int k = 0; k < n && k < 6; ++k)
        if ((uint64_t)frames[k] == s->pc[0]) { start = k + 1; break; }
    if (!start) start = n < 3 ? n : 3;
    uint32_t m = 1;
    for (int k = start; k < n && m < DEPTH; ++k) s->pc[m++] = (uint64_t)frames[k];
    s->n = m;
    count = i + 1;
}

static void dump(void) {
    if (dumped || !samples || !out_path) return;
    dumped = 1;
    timer_delete(timer);
    char path[4096];
    snprintf(path, sizeof path, "%s.samples", out_path);
    FILE *f = fopen(path, "wb");
    if (!f) return;
    uint32_t c = count;
    fwrite(&c, 4, 1, f);
    for (uint32_t i = 0; i < c; ++i) {
        fwrite(&samples[i].t, 8, 1, f);
        fwrite(&samples[i].n, 4, 1, f);
        fwrite(samples[i].pc, 8, samples[i].n, f);
    }
    fclose(f);
    snprintf(path, sizeof path, "%s.maps", out_path);
    FILE *in = fopen("/proc/self/maps", "r"), *o = fopen(path, "w");
    if (in && o) { char buf[8192]; size_t r; while ((r = fread(buf, 1, sizeof buf, in))) fwrite(buf, 1, r, o); }
    if (in) fclose(in);
    if (o) fclose(o);
}

static void *arm(void *arg) {
    pthread_t main_thread = (pthread_t)arg;
    const char *delay = getenv("H2_SAMPLE_DELAY");
    const char *us = getenv("H2_SAMPLE_US");
    struct timespec d = {delay ? atoi(delay) : 0, 0};
    nanosleep(&d, NULL);
    clockid_t clock;
    if (pthread_getcpuclockid(main_thread, &clock)) return NULL;
    struct sigevent ev; memset(&ev, 0, sizeof ev);
    ev.sigev_notify = SIGEV_THREAD_ID;
    ev.sigev_signo = SIGRTMIN + 3;
    ev._sigev_un._tid = main_tid;
    if (timer_create(clock, &ev, &timer)) return NULL;
    long interval = us ? atol(us) : 1000;
    struct itimerspec it = {{interval / 1000000, (interval % 1000000) * 1000},
                            {interval / 1000000, (interval % 1000000) * 1000}};
    timer_settime(timer, 0, &it, NULL);
    return NULL;
}

__attribute__((constructor)) static void init(void) {
    out_path = getenv("H2_SAMPLE_OUT");
    if (!out_path) return;
    unsetenv("LD_PRELOAD");
    samples = calloc(CAPACITY, sizeof(Sample));
    unwind = !getenv("H2_SAMPLE_NO_UNWIND");
    main_tid = (pid_t)syscall(SYS_gettid);
    void *warm[4]; backtrace(warm, 4);  // load libgcc_s before signals
    struct sigaction sa; memset(&sa, 0, sizeof sa);
    sa.sa_sigaction = handler; sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGRTMIN + 3, &sa, NULL);
    atexit(dump);
    pthread_t t; pthread_create(&t, NULL, arm, (void *)pthread_self()); pthread_detach(t);
}
__attribute__((destructor)) static void fini(void) { dump(); }
