#!/usr/bin/env python3
"""Profile an installed Linux SDL/Vulkan multiplayer runtime without rebuilding it.

Samples the main thread between real presentation boundaries. Renderer timings,
CPU stacks, main-thread utilization and optional NVIDIA device utilization stay
separate: host renderer timings are not GPU execution timestamps. Raw samples,
the preload library and runtime logs use run_test.py's owned temporary storage.
Only a bounded JSON report is retained. Sampling runs are diagnostic, not FPS
acceptance measurements; use benchmark_framerate.py for the latter.
"""
from __future__ import annotations

import argparse
import bisect
from collections import Counter
import csv
import ctypes
import hashlib
import json
import math
import os
from pathlib import Path
import re
import statistics
import struct
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
RECORD = struct.Struct('=QQII48Q')


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


class Nvidia:
    """Optional device-wide utilization; includes unrelated GPU workloads."""
    class Utilization(ctypes.Structure):
        _fields_ = [('gpu', ctypes.c_uint), ('memory', ctypes.c_uint)]

    def __init__(self):
        self.library = None
        try:
            library = ctypes.CDLL('libnvidia-ml.so.1')
            if library.nvmlInit_v2() != 0:
                return
            self.library = library
            self.handle = ctypes.c_void_p()
            if library.nvmlDeviceGetHandleByIndex_v2(0, ctypes.byref(self.handle)) != 0:
                self.close()
                return
            name = ctypes.create_string_buffer(128)
            self.name = (name.value.decode() if library.nvmlDeviceGetName(
                self.handle, name, len(name)) == 0 else 'NVIDIA device 0')
        except OSError:
            self.close()

    def sample(self):
        if not self.library:
            return None
        utilization = self.Utilization()
        if self.library.nvmlDeviceGetUtilizationRates(self.handle, ctypes.byref(utilization)):
            return None
        return {'gpu_percent': utilization.gpu, 'memory_percent': utilization.memory}

    def close(self):
        if self.library:
            self.library.nvmlShutdown()
            self.library = None


def summarize(values):
    values = sorted(values)
    if not values:
        return None
    return {'mean': statistics.mean(values), 'median': statistics.median(values),
            'p95': values[int(.95 * (len(values) - 1))], 'max': values[-1]}


def cpu_functions(runtime, scratch, warmup, frames):
    modules = (scratch / 'modules.tsv').read_text().splitlines()
    boundaries = {name: int(value) for name, value in re.findall(r'(\w+)=(\d+)', modules[-1])}
    if (boundaries.get('presented') != frames or not boundaries.get('begin_ns')
            or boundaries.get('end_ns', 0) <= boundaries['begin_ns']):
        raise RuntimeError('Main-thread presentation boundaries did not match the requested workload')
    base, end = next((int(row[0], 16), int(row[1], 16)) for line in modules
                     if not line.startswith('#') and (row := line.split('\t'))[2] == '/proc/self/exe')
    symbols = []
    result = subprocess.run(['nm', '-nC', '--defined-only', str(runtime)],
                            text=True, capture_output=True, check=True)
    for line in result.stdout.splitlines():
        if match := re.match(r'([0-9a-f]+) [TtWw] (.+)', line):
            symbols.append((int(match[1], 16), match[2]))
    if not symbols:
        raise RuntimeError('Runtime has no function symbols; use an unstripped development runtime')
    starts = [address for address, _ in symbols]
    leaf, inclusive, sampled_frames = Counter(), Counter(), Counter()
    data = (scratch / 'samples.bin').read_bytes()
    if len(data) % RECORD.size:
        raise RuntimeError('Incomplete CPU sample record')
    samples = outside = external = 0
    first_stack = None
    for wall, cpu, frame, depth, *addresses in RECORD.iter_unpack(data):
        if not warmup <= frame < frames:
            outside += 1
            continue
        samples += 1
        if first_stack is None:
            first_stack = {'depth': depth, 'addresses': [hex(address) for address in addresses[:depth]]}
        sampled_frames[frame] += 1
        executable = [address - base for address in addresses[:depth] if base <= address < end]
        names = [symbols[index][1] for address in executable
                 if (index := bisect.bisect_right(starts, address) - 1) >= 0]
        if not names:
            external += 1
            continue
        leaf[names[0]] += 1
        inclusive.update(set(names))
    if not samples or outside:
        raise RuntimeError(f'CPU sampler boundary failed: {samples} in range, {outside} outside')
    if not leaf:
        raise RuntimeError('CPU samples did not resolve into the selected executable')
    def top(counts):
        return [{'function': name, 'samples': count, 'percent': 100 * count / samples}
                for name, count in counts.most_common(70)]
    return {'samples': samples, 'sampled_frame_count': len(sampled_frames),
            'sample_limit_reached': samples >= 100000,
            'first_sampled_frame': min(sampled_frames), 'last_sampled_frame': max(sampled_frames),
            'outside_requested_frames': outside, 'external_only_samples': external,
            'executable_range': [hex(base), hex(end)], 'first_stack': first_stack,
            'leaf_executable_function_or_external_caller': top(leaf), 'inclusive': top(inclusive),
            'boundaries': boundaries,
            'limits': 'Statistical main-thread CPU time. Inclusive rows overlap. '
                      'An external-library sample is charged to its first executable caller.'}


def worker(args):
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    library = scratch / 'native_cpu_profiler.so'
    timing = scratch / 'renderer.tsv'
    args.output.parent.mkdir(parents=True, exist_ok=True)
    command = [str(args.runtime), '--multiplayer', '--map', str(args.map),
               '--shared-maps', str(args.shared_maps), '--local-players', str(args.players),
               '--frames', str(args.frames), '--viewport', str(args.width), str(args.height)]
    if not args.audio:
        command.append('--mute')
    environment = dict(os.environ)
    environment.update(HALO2_NATIVE_PROFILE_DIRECTORY=str(scratch),
                       HALO2_NATIVE_PROFILE_EXECUTABLE=str(args.runtime),
                       HALO2_NATIVE_PROFILE_WARMUP=str(args.warmup),
                       HALO2_NATIVE_PROFILE_FRAMES=str(args.frames),
                       HALO2_NATIVE_PROFILE_HZ=str(args.sample_hz),
                       HALO2_RENDER_TIMINGS=str(timing),
                       SDL_VIDEODRIVER=args.video_driver,
                       SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT='0x0000/0x0000')
    environment.update(HALO2_PIPELINE_CACHE_DIRECTORY=str(scratch / 'vulkan-cache'),
                       MESA_SHADER_CACHE_DIR=str(scratch / 'mesa-cache'),
                       __GL_SHADER_DISK_CACHE_PATH=str(scratch / 'nvidia-cache'))
    environment['LD_PRELOAD'] = str(library) + (':' + environment['LD_PRELOAD']
                                               if environment.get('LD_PRELOAD') else '')
    report = {'schema': 'H2_MULTIPLAYER_PROFILE_V1', 'completed': False,
              'runtime': str(args.runtime), 'runtime_sha256': digest(args.runtime),
              'map': str(args.map), 'map_sha256': digest(args.map), 'players': args.players,
              'frames': args.frames, 'warmup': args.warmup, 'sample_hz': args.sample_hz,
              'command': command, 'scratch': str(scratch),
              'reference_controls': {name: environment.get(name) for name in
                  ('HALO2_REFERENCE_REPEAT_VEHICLE_SKINS', 'HALO2_REFERENCE_INSTRUCTION_CALLBACKS',
                   'HALO2_REFERENCE_WRITE_DISPATCH', 'HALO2_REFERENCE_LITERAL_INSTRUCTIONS',
                   'HALO2_REFERENCE_TEXTURE_FINGERPRINTS', 'HALO2_REFERENCE_SMALL_MEMORY_TRANSFERS',
                   'HALO2_REFERENCE_REPEAT_UNIT_SKINS', 'HALO2_REFERENCE_SCALAR_VECTOR_READS')},
              'host': {'architecture': os.uname().machine, 'logical_cpus': os.cpu_count(),
                       'load_average_at_start': os.getloadavg(),
                       'cpu': next((line.split(':', 1)[1].strip() for line in
                                    Path('/proc/cpuinfo').read_text().splitlines()
                                    if line.startswith('model name')), None)},
              'limits': 'Visible idle multiplayer diagnostic on this host. Sampling adds overhead; '
                        'FPS targets require separate unsampled benchmarks. No graphics or '
                        'simulation settings are reduced. Device utilization includes other applications.'}
    gpu = Nvidia()
    samples = []
    try:
        subprocess.run(['cc', '-shared', '-fPIC', '-O2', str(ROOT / 'tools/native_cpu_profiler.c'),
                        '-o', str(library), '-ldl', '-pthread', '-lrt'], check=True)
        report['sampler_source_sha256'] = digest(ROOT / 'tools/native_cpu_profiler.c')
        deadline = time.monotonic() + args.timeout
        previous = None
        ticks = os.sysconf('SC_CLK_TCK')
        with (scratch / 'runtime.log').open('w') as log:
            process = subprocess.Popen(command, env=environment, cwd=ROOT, stdout=log,
                                       stderr=subprocess.STDOUT)
            try:
                while process.poll() is None:
                    if time.monotonic() > deadline:
                        raise TimeoutError('Profile exceeded its declared runtime timeout')
                    # The first binary sample marks completion of presentation warmup.
                    if (scratch / 'samples.bin').exists() and (scratch / 'samples.bin').stat().st_size:
                        try:
                            fields = Path(f'/proc/{process.pid}/task/{process.pid}/stat').read_text().rsplit(')', 1)[1].split()
                            now = time.monotonic()
                            cpu_ticks = int(fields[11]) + int(fields[12])
                            sample = {'monotonic_seconds': now, 'gpu': gpu.sample()}
                            if previous:
                                sample['main_thread_cpu_percent'] = 100 * (cpu_ticks - previous[1]) / ticks / (now - previous[0])
                            previous = now, cpu_ticks
                            samples.append(sample)
                        except FileNotFoundError:
                            pass
                    time.sleep(.2)
                report['exit_code'] = process.returncode
            finally:
                if process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait()
        report['log_tail'] = (scratch / 'runtime.log').read_text(errors='replace')[-12000:]
        if report['exit_code']:
            raise RuntimeError(f'Runtime failed: exit {report["exit_code"]}')
        with timing.open() as stream:
            rows = list(csv.DictReader(stream, delimiter='\t'))
        if len(rows) != args.frames:
            raise RuntimeError(f'Expected {args.frames} actual renderer frames, got {len(rows)}')
        sampled = rows[args.warmup:]
        report['renderer_host_ms'] = {key: summarize([float(row[key]) for row in sampled])
                                      for key in sampled[0] if key.endswith('_ms')}
        report['render_sizes'] = sorted({(int(row['render_width']), int(row['render_height'])) for row in sampled})
        report['present_sizes'] = sorted({(int(row['present_width']), int(row['present_height'])) for row in sampled})
        report['cpu'] = cpu_functions(args.runtime, scratch, args.warmup, args.frames)
        bounds = report['cpu']['boundaries']
        samples = [sample for sample in samples if bounds['begin_ns'] <=
                   int(sample['monotonic_seconds'] * 1000000000) < bounds['end_ns']]
        report['gpu_device'] = getattr(gpu, 'name', None)
        report['host_samples'] = samples
        report['main_thread_cpu_percent'] = summarize([s['main_thread_cpu_percent'] for s in samples if 'main_thread_cpu_percent' in s])
        report['gpu_utilization_percent'] = summarize([s['gpu']['gpu_percent'] for s in samples if s['gpu']])
        report['completed'] = True
        print(f'Profile complete: {report["cpu"]["samples"]} CPU samples across '
              f'{report["cpu"]["sampled_frame_count"]} frames', flush=True)
        for row in report['cpu']['leaf_executable_function_or_external_caller'][:10]:
            print(f'{row["percent"]:6.2f}% {row["function"][:200]}', flush=True)
    except Exception as error:
        report['error'] = str(error)
        raise
    finally:
        gpu.close()
        text = json.dumps(report, indent=2) + '\n'
        if len(text.encode()) > 1024 * 1024:
            raise RuntimeError('Profiler report exceeds its bounded 1 MiB report budget')
        args.output.write_text(text)
        print(f'Profiler report: {args.output}', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--map', type=Path, required=True)
    parser.add_argument('--shared-maps', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--players', type=int, choices=(1, 2, 3, 4), default=2)
    parser.add_argument('--frames', type=int, default=240)
    parser.add_argument('--warmup', type=int, default=30)
    parser.add_argument('--width', type=int, default=1280)
    parser.add_argument('--height', type=int, default=720)
    parser.add_argument('--sample-hz', type=int, default=250)
    parser.add_argument('--video-driver', choices=('x11', 'wayland'), default='x11')
    parser.add_argument('--timeout', type=float, default=180)
    parser.add_argument('--audio', action='store_true')
    parser.add_argument('--worker', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    if sys.platform != 'linux':
        parser.error('Installed-runtime CPU sampling currently supports Linux')
    if not (0 < args.warmup < args.frames <= 100000 and 50 <= args.sample_hz <= 1000
            and 64 <= args.width <= 4096 and 64 <= args.height <= 4096
            and math.isfinite(args.timeout) and args.timeout > 0):
        parser.error('Require positive warmup below frames, 50..1000 Hz, 64..4096 dimensions and a finite positive timeout')
    args.runtime = args.runtime.resolve(strict=True)
    args.map = args.map.resolve(strict=True)
    args.shared_maps = (args.shared_maps or args.map.parent).resolve(strict=True)
    args.output = args.output.resolve()
    if args.worker:
        worker(args)
        return 0
    return subprocess.call([sys.executable, '-B', str(ROOT / 'tools/run_test.py'),
                            '--scratch-root', str(ROOT / '.local/test-scratch'), '--',
                            sys.executable, '-B', str(Path(__file__).resolve()),
                            '--worker', *sys.argv[1:]])


if __name__ == '__main__':
    raise SystemExit(main())
