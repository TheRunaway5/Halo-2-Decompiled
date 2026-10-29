"""Compile small owned fixtures for the preload monitor's exit lifecycle."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = r'''
#define _GNU_SOURCE
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
static int controlled_clock(clockid_t, struct timespec *);
#define clock_gettime controlled_clock
#include "cpumon.c"
#undef clock_gettime
static sem_t sampled, release_sample;
static atomic_int blocked, dumped, failures;
static int controlled_clock(clockid_t clock, struct timespec *value) {
    if (clock == main_clock && !atomic_exchange(&blocked, 1)) {
        sem_post(&sampled);
        while (sem_wait(&release_sample) != 0) {}
    }
    return clock_gettime(clock, value);
}
static void *release_after_stop(void *unused) {
    (void)unused;
    while (!atomic_load(&stop)) {
        struct timespec delay = {0, 100000}; nanosleep(&delay, NULL);
    }
    struct timespec delay = {0, 2000000}; nanosleep(&delay, NULL);
    if (atomic_load(&dumped)) atomic_fetch_add(&failures, 1);
    sem_post(&release_sample);
    return NULL;
}
int main(int argc, char **argv) {
    if (argc != 3) return 1;
    setenv("HALO2_RENDER_TIMINGS", argv[1], 1);
    setenv("H2_CPU_MONITOR_OUT", argv[2], 1);
    if (sem_init(&sampled, 0, 0) || sem_init(&release_sample, 0, 0)) return 2;
    for (int iteration = 0; iteration < 32; ++iteration) {
        FILE *input = fopen(argv[1], "w");
        if (!input) return 3;
        fputs("frame\n1\n", input); fclose(input);
        nrows = 0; atomic_store(&stop, 0); atomic_store(&blocked, 0); atomic_store(&dumped, 0);
        init(); if (!watcher_started) return 4;
        while (sem_wait(&sampled) != 0) {}
        pthread_t releaser;
        if (pthread_create(&releaser, NULL, release_after_stop, NULL)) return 5;
        dump(); atomic_store(&dumped, 1);
        if (pthread_join(releaser, NULL)) return 6;
        if (watcher_started || nrows != 1 || atomic_load(&failures)) return 7;
        FILE *output = fopen(argv[2], "r");
        if (!output) return 8;
        long frames; unsigned long long cpu, wall;
        int fields = fscanf(output, "%ld %llu %llu", &frames, &cpu, &wall);
        fclose(output);
        if (fields != 3 || frames != 1 || !cpu || !wall) return 9;
    }
    sem_destroy(&sampled); sem_destroy(&release_sample);
    puts("32 blocked in-flight samples joined before dump; complete final rows");
    return 0;
}
'''
HOST = r'''
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(void) {
    FILE *f = fopen(getenv("HALO2_RENDER_TIMINGS"), "w");
    if (!f) return 1;
    fputs("frame\n", f); fflush(f);
    for (int frame = 1; frame <= 40; ++frame) {
        fprintf(f, "%d\n", frame); fflush(f);
        clock_t until = clock() + CLOCKS_PER_SEC / 10000;
        while (clock() < until) {}
        struct timespec delay = {0, 100000}; nanosleep(&delay, NULL);
    }
    fclose(f); return 0;
}
'''


@unittest.skipUnless(sys.platform == 'linux' and shutil.which('gcc'),
                     'Linux and gcc are required for the monitor fixture')
class CpuMonitorLifecycleTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.owned = tempfile.TemporaryDirectory(prefix='cpumon-', dir=os.environ.get('HALO2_TEST_SCRATCH'))
        cls.addClassCleanup(cls.owned.cleanup)
        cls.root = Path(cls.owned.name)
        cls.fixture = cls.root / 'fixture.c'
        cls.fixture.write_text(FIXTURE)
        cls.host = cls.root / 'host.c'
        cls.host.write_text(HOST)
        cls.env = {key: value for key, value in os.environ.items()
                   if key not in ('LD_PRELOAD', 'HALO2_RENDER_TIMINGS', 'H2_CPU_MONITOR_OUT')}

    def compile(self, output, source, *options):
        command = ['gcc', '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                   '-pthread', '-I', str(ROOT / 'tools/perf_4k60'),
                   *options, str(source), '-o', str(output)]
        result = subprocess.run(command, capture_output=True, text=True, env=self.env, timeout=30)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def fixture_run(self, executable):
        return subprocess.run([str(executable), str(self.root / 'timing.tsv'), str(self.root / 'cpu.txt')],
                              capture_output=True, text=True, env=self.env, timeout=20)

    def test_dump_waits_for_in_flight_final_sample(self):
        executable = self.root / 'lifecycle'
        self.compile(executable, self.fixture)
        result = self.fixture_run(executable)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_repeated_preload_exit_has_complete_monotonic_rows(self):
        library = self.root / 'cpumon.so'
        executable = self.root / 'host'
        self.compile(library, ROOT / 'tools/perf_4k60/cpumon.c', '-shared', '-fPIC')
        self.compile(executable, self.host)
        for iteration in range(24):
            output = self.root / f'cpu-{iteration}.txt'
            env = dict(self.env, LD_PRELOAD=str(library), HALO2_RENDER_TIMINGS=str(self.root / 'timing.tsv'),
                       H2_CPU_MONITOR_OUT=str(output))
            result = subprocess.run([str(executable)], capture_output=True, text=True, env=env, timeout=10)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            rows = [tuple(map(int, line.split())) for line in output.read_text().splitlines()]
            self.assertGreater(len(rows), 1)
            self.assertTrue(all(len(row) == 3 and row[1] > 0 and row[2] > 0 for row in rows))
            self.assertTrue(all(a[0] < b[0] and a[1] <= b[1] and a[2] <= b[2]
                                for a, b in zip(rows, rows[1:])))

    def test_lifecycle_under_thread_sanitizer(self):
        executable = self.root / 'lifecycle-tsan'
        self.compile(executable, self.fixture, '-fsanitize=thread', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie')
        result = self.fixture_run(executable)
        if 'ThreadSanitizer: unexpected memory mapping' in result.stderr:
            self.skipTest('ThreadSanitizer cannot reserve its shadow mapping on this host')
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertNotIn('WARNING: ThreadSanitizer', result.stderr)


if __name__ == '__main__':
    unittest.main()
