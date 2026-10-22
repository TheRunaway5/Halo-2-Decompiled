/* Optional Linux main-thread sampler for profile_multiplayer.py.
 * Presentation boundaries exclude loading, warmup and device destruction.
 * All output belongs to the enclosing run_test.py scratch owner. */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <execinfo.h>
#include <fcntl.h>
#include <link.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>
#include <vulkan/vulkan.h>

struct sample {
    uint64_t wall_ns, cpu_ns;
    uint32_t frame, depth;
    uint64_t addresses[48];
};
static int sample_fd = -1;
static FILE *modules;
static timer_t cpu_timer;
static volatile sig_atomic_t frame, samples, active;
static unsigned warmup, frames, hz;
static pid_t main_tid;
static uint64_t begin_ns, end_ns;
static int timer_ready;
static PFN_vkQueuePresentKHR present;

static uint64_t now(clockid_t clock) {
    struct timespec time;
    clock_gettime(clock, &time);
    return (uint64_t)time.tv_sec * 1000000000ull + time.tv_nsec;
}
static void sample_cpu(int unused) {
    (void)unused;
    if (!active || samples >= 100000) return;
    void *addresses[48];
    struct sample record = {0};
    record.wall_ns = now(CLOCK_MONOTONIC);
    record.cpu_ns = now(CLOCK_THREAD_CPUTIME_ID);
    record.frame = (uint32_t)frame;
    record.depth = (uint32_t)backtrace(addresses, 48);
    for (unsigned i = 0; i < record.depth; ++i)
        record.addresses[i] = (uintptr_t)addresses[i];
    if (write(sample_fd, &record, sizeof(record)) == sizeof(record)) ++samples;
}
static int module(struct dl_phdr_info *info, size_t size, void *unused) {
    (void)size; (void)unused;
    uintptr_t end = 0;
    for (unsigned i = 0; i < info->dlpi_phnum; ++i) {
        const ElfW(Phdr) *p = &info->dlpi_phdr[i];
        if (p->p_type == PT_LOAD && p->p_vaddr + p->p_memsz > end)
            end = p->p_vaddr + p->p_memsz;
    }
    fprintf(modules, "%lx\t%lx\t%s\n", (unsigned long)info->dlpi_addr,
            (unsigned long)(info->dlpi_addr + end),
            *info->dlpi_name ? info->dlpi_name : "/proc/self/exe");
    return 0;
}
__attribute__((constructor)) static void initialize(void) {
    const char *directory = getenv("HALO2_NATIVE_PROFILE_DIRECTORY");
    if (!directory) return;
    const char *expected = getenv("HALO2_NATIVE_PROFILE_EXECUTABLE");
    char executable[4096];
    const ssize_t length = readlink("/proc/self/exe", executable, sizeof(executable) - 1);
    if (length < 0 || !expected) return;
    executable[length] = 0;
    /* Runtime helpers inherit LD_PRELOAD; they must never truncate this
     * process's samples or replace its executable relocation information. */
    if (strcmp(executable, expected)) return;
    const char *w = getenv("HALO2_NATIVE_PROFILE_WARMUP");
    const char *f = getenv("HALO2_NATIVE_PROFILE_FRAMES");
    const char *h = getenv("HALO2_NATIVE_PROFILE_HZ");
    if (!w || !f || !h) _exit(125);
    warmup = (unsigned)strtoul(w, NULL, 10);
    frames = (unsigned)strtoul(f, NULL, 10);
    hz = (unsigned)strtoul(h, NULL, 10);
    main_tid = (pid_t)syscall(SYS_gettid);
    char path[4096];
    snprintf(path, sizeof(path), "%s/samples.bin", directory);
    sample_fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    snprintf(path, sizeof(path), "%s/modules.tsv", directory);
    modules = fopen(path, "w");
    if (sample_fd < 0 || !modules || !hz || frames <= warmup) _exit(125);
    dl_iterate_phdr(module, NULL);
    fflush(modules);
    /* Resolve unwinding before a signal can interrupt allocation/loader work. */
    void *prime[48]; backtrace(prime, 48);
    struct sigaction action = {0};
    action.sa_handler = sample_cpu;
    action.sa_flags = SA_RESTART;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGRTMIN + 4, &action, NULL)) _exit(125);
    struct sigevent event = {0};
    event.sigev_notify = SIGEV_THREAD_ID;
    event.sigev_signo = SIGRTMIN + 4;
    event._sigev_un._tid = main_tid;
    if (timer_create(CLOCK_THREAD_CPUTIME_ID, &event, &cpu_timer)) _exit(125);
    timer_ready = 1;
    present = (PFN_vkQueuePresentKHR)dlsym(RTLD_NEXT, "vkQueuePresentKHR");
    if (!present) _exit(125);
}
VKAPI_ATTR VkResult VKAPI_CALL vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *info) {
    if (!present) present = (PFN_vkQueuePresentKHR)dlsym(RTLD_NEXT, "vkQueuePresentKHR");
    if (!present) return VK_ERROR_INITIALIZATION_FAILED;
    const VkResult result = present(queue, info);
    if (!timer_ready || syscall(SYS_gettid) != main_tid) return result;
    ++frame;
    if ((unsigned)frame == warmup) {
        const long interval = (long)(1000000000u / hz);
        struct itimerspec period = {{0, interval}, {0, interval}};
        begin_ns = now(CLOCK_MONOTONIC);
        active = 1;
        if (timer_settime(cpu_timer, 0, &period, NULL)) _exit(125);
    }
    if ((unsigned)frame >= frames) {
        active = 0;
        if (!end_ns) end_ns = now(CLOCK_MONOTONIC);
        struct itimerspec stopped = {0};
        timer_settime(cpu_timer, 0, &stopped, NULL);
    }
    return result;
}
__attribute__((destructor)) static void finish(void) {
    active = 0;
    if (timer_ready) timer_delete(cpu_timer);
    if (modules) {
        fprintf(modules, "# presented=%d samples=%d record_bytes=%zu begin_ns=%llu end_ns=%llu\n",
                frame, samples, sizeof(struct sample),
                (unsigned long long)begin_ns, (unsigned long long)end_ns);
        fclose(modules);
    }
    if (sample_fd >= 0) close(sample_fd);
}
