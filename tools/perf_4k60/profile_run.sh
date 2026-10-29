#!/usr/bin/env bash
# Usage: profile_run.sh RUNTIME REPORT_DIR LABEL PLAYERS FRAMES [SAMPLE_DELAY] [extra runtime args...]
# Env: H2_MAPS (MCC map directory), H2_MAP (default coagulation), H2_WINDOW (profile seconds), H2_CPUMON=1 (CPU ms/frame), H2_GPU_TIMINGS=1 (same-queue GPU intervals).
# Runs inside tools/run_test.py; raw samples/builds live in HALO2_TEST_SCRATCH.
set -euo pipefail
runtime=$1 report=$2 label=$3 players=$4 frames=$5 delay=${6:-}
runtime=$(readlink -f "$runtime")
shift 6 || shift $#
here=$(dirname "$(readlink -f "$0")")
root=$(cd "$here/../.." && pwd)
maps=${H2_MAPS:-$root/assets/imported/mcc-dire-valley-polish-20261023/halo2/h2_maps_win64_dx11}
maps=$(readlink -f "$maps")
map=${H2_MAP:-coagulation}
scratch=$HALO2_TEST_SCRATCH
mkdir -p "$report"
# A failed retry must not leave the previous invocation's successful metrics
# under the same label. These suffixes are owned exclusively by this helper.
for suffix in timing.txt timing.tsv gpu.tsv failure.log elapsed cpu.txt cull.txt profile.txt samples maps; do
  rm -f -- "$report/$label.$suffix"
done
env_extra=()
if [[ -n "$delay" ]]; then
  gcc -O2 -shared -fPIC -o "$scratch/sampler.so" "$here/sampler.c" -lpthread
  env_extra=(LD_PRELOAD="$scratch/sampler.so" H2_SAMPLE_OUT="$scratch/prof" H2_SAMPLE_DELAY="$delay" H2_SAMPLE_US=1000)
fi
if [[ -n "${H2_CPUMON:-}" && -z "$delay" ]]; then
  gcc -O2 -shared -fPIC -o "$scratch/cpumon.so" "$here/cpumon.c" -lpthread
  env_extra=(LD_PRELOAD="$scratch/cpumon.so" H2_CPU_MONITOR_OUT="$scratch/cpumon.txt")
fi
if [[ -n "${H2_GPU_TIMINGS:-}" ]]; then
  env_extra+=(HALO2_RENDER_GPU_TIMINGS="$scratch/gpu.tsv")
fi
rm -f "$scratch/timing.tsv" "$scratch/cpumon.txt" "$scratch/gpu.tsv"
start=$(date +%s.%N)
(
cd "$(dirname "$runtime")"
env HALO2_RENDER_TIMINGS="$scratch/timing.tsv" HALO2_MENU_FOG_SEED=1 \
  HALO2_PIPELINE_CACHE_DIRECTORY="$scratch/vulkan-cache" \
  MESA_SHADER_CACHE_DIR="$scratch/mesa-cache" __GL_SHADER_DISK_CACHE_PATH="$scratch/nvidia-cache" \
  XDG_DATA_HOME="$scratch/xdg-data" XDG_CONFIG_HOME="$scratch/xdg-config" \
  SDL_WINDOW_NO_ACTIVATION_WHEN_SHOWN=1 SDL_WINDOW_ACTIVATE_WHEN_SHOWN=0 SDL_WINDOW_ACTIVATE_WHEN_RAISED=0 \
  SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT=0x0000/0x0000 "${env_extra[@]}" \
  "$runtime" --multiplayer --graphics-mode Original --map "$maps/$map.map" --shared-maps "$maps" \
  --mp-starting-weapon 5 --local-players "$players" --frames "$frames" \
  --viewport 1280 720 --render-size 3840 2160 --mute "$@"
) > "$scratch/run.log" 2>&1 || {
    result=$?
    echo "runtime failed: $result"
    tail -c 12000 "$scratch/run.log" | tee "$report/$label.failure.log"
    exit "$result"
  }
end=$(date +%s.%N)
python3 - "$scratch/timing.tsv" "$report/$label" "${H2_GPU_TIMINGS:-}" "$scratch/gpu.tsv" "$players" "$frames" <<'EOF'
import csv, json, math, statistics, sys
from pathlib import Path
prefix = Path(sys.argv[2])
dimensions = ('render_width', 'render_height', 'present_width', 'present_height')
expected = (3840, 2160, 1280, 720)
def fail(reason, **details):
    diagnostic = {'passed': False, 'error': reason,
                  'expected_dimensions': dict(zip(dimensions, expected)), **details}
    Path(str(prefix) + '.failure.log').write_text(json.dumps(diagnostic, indent=2) + '\n')
    print('INVALID 4K workload: ' + reason, file=sys.stderr)
    raise SystemExit(1)
try:
    with open(sys.argv[1]) as stream:
        reader = csv.DictReader(stream, delimiter='\t')
        missing = [name for name in dimensions if name not in (reader.fieldnames or [])]
        if missing:
            fail('timing dimensions are unavailable', missing_fields=missing)
        all_rows = list(reader)
except OSError as error:
    fail('timing data is unavailable', detail=str(error)[-1500:])
if len(all_rows) != int(sys.argv[6]):
    fail('completed timing row count differs from requested frames',
         total_rows=len(all_rows), requested_frames=int(sys.argv[6]))
bad_count = 0
bad_samples = []
for index, row in enumerate(all_rows):
    try:
        actual = tuple(int(row[name]) for name in dimensions)
    except (TypeError, ValueError):
        actual = None
    if actual != expected:
        bad_count += 1
        if len(bad_samples) < 8:
            bad_samples.append({'timing_row': index + 1, 'dimensions':
                                {name: str(row.get(name))[:32] for name in dimensions}})
if bad_count:
    fail('recorded render/presentation dimensions differ from 3840x2160/1280x720',
         mismatched_rows=bad_count, total_rows=len(all_rows), samples=bad_samples)
rows = all_rows[30:]
if not rows:
    fail('no measured timing rows remain after 30 warmup frames', total_rows=len(all_rows))
out = {}
if sys.argv[3]:
    try:
        gpu_rows = list(csv.DictReader(Path(sys.argv[4]).open(), delimiter='\t'))
        if len(gpu_rows) != len(all_rows):
            fail('GPU timestamp sample count differs from completed CPU frames',
                 gpu_rows=len(gpu_rows), cpu_rows=len(all_rows))
        gpu_fields = ('gpu_total_ms', 'gpu_compute_ms', 'gpu_scene_ms', 'gpu_present_copy_ms')
        for index, row in enumerate(gpu_rows):
            actual = tuple(int(row[k]) for k in ('render_width', 'render_height', 'views'))
            values = tuple(float(row[k]) for k in gpu_fields)
            if actual != (3840, 2160, int(sys.argv[5])) or int(row['frame']) != index:
                fail('GPU timestamp metadata differs from the measured workload', gpu_row=index)
            if any(not math.isfinite(v) or v < 0 for v in values) or not math.isclose(values[0], sum(values[1:]), abs_tol=0.000001):
                fail('GPU timestamp intervals are invalid', gpu_row=index)
        for k in gpu_fields:
            v = sorted(float(r[k]) for r in gpu_rows[30:])
            out[k] = (statistics.mean(v), statistics.median(v), v[int(.95*(len(v)-1))])
        Path(str(prefix) + '.gpu.tsv').write_bytes(Path(sys.argv[4]).read_bytes())
    except (OSError, KeyError, TypeError, ValueError) as error:
        fail('GPU timestamps are missing or malformed', detail=str(error)[-1500:])
for k in rows[0]:
    if k.endswith('_ms'):
        v = sorted(float(r[k]) for r in rows)
        out[k] = (statistics.mean(v), statistics.median(v), v[int(.95*(len(v)-1))])
fps = 1000 / out['interval_ms'][0]
lines = [f'frames={len(rows)} fps={fps:.2f}']
lines += [f'{k:40s} mean {m:8.3f} med {d:8.3f} p95 {p:8.3f}' for k, (m, d, p) in sorted(out.items(), key=lambda x: -x[1][0])]
open(sys.argv[2] + '.timing.txt', 'w').write('\n'.join(lines) + '\n')
print('\n'.join(lines[:25]))
EOF
python3 -c "print('elapsed %.1f s' % ($end - $start))" | tee "$report/$label.elapsed"
cp "$scratch/timing.tsv" "$report/$label.timing.tsv"
grep -a "Native main cull" "$scratch/run.log" | tail -40 > "$report/$label.cull.txt" || true
if [[ -f "$scratch/cpumon.txt" ]]; then
  python3 - "$scratch/cpumon.txt" "$frames" <<'PY' | tee "$report/$label.cpu.txt"
import sys
rows = [tuple(map(int, l.split())) for l in open(sys.argv[1]) if l.strip()]
first = next((r for r in rows if r[0] >= 30), rows[0])
# Keep the final samples away from renderer destruction and its file close.
# The watcher correlates flushed rows with CPU time; it does not timestamp
# frame boundaries inside the game thread itself.
last = max((r for r in rows if first[0] < r[0] <= int(sys.argv[2]) - 5), key=lambda r: r[0])
frames = last[0] - first[0]
print(f'cpu_ms_per_frame={(last[1]-first[1])/frames/1e6:.3f} wall_ms_per_frame={(last[2]-first[2])/frames/1e6:.3f} frames={frames} first_frame={first[0]} last_frame={last[0]} samples={len(rows)}')
PY
fi
if [[ -n "$delay" ]]; then
  python3 "$here/analyze.py" "$scratch/prof" "$runtime" 60 "${H2_WINDOW:-0}" > "$report/$label.profile.txt"
  cp "$scratch/prof.samples" "$scratch/prof.maps" "$report/" && mv "$report/prof.samples" "$report/$label.samples" && mv "$report/prof.maps" "$report/$label.maps"
  head -70 "$report/$label.profile.txt"
fi
