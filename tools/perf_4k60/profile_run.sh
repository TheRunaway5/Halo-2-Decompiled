#!/usr/bin/env bash
# Usage: profile_run.sh RUNTIME REPORT_DIR LABEL PLAYERS FRAMES [SAMPLE_DELAY] [extra runtime args...]
# Env: H2_MAP (default coagulation), H2_WINDOW (profile seconds), H2_CPUMON=1 (CPU ms/frame).
# Runs inside tools/run_test.py; raw samples/builds live in HALO2_TEST_SCRATCH.
set -euo pipefail
runtime=$1 report=$2 label=$3 players=$4 frames=$5 delay=${6:-}
shift 6 || shift $#
here=$(dirname "$(readlink -f "$0")")
root=$(cd "$here/../.." && pwd)
maps=$root/assets/imported/mcc-dire-valley-polish-20261023/halo2/h2_maps_win64_dx11
map=${H2_MAP:-coagulation}
scratch=$HALO2_TEST_SCRATCH
mkdir -p "$report"
env_extra=()
if [[ -n "$delay" ]]; then
  gcc -O2 -shared -fPIC -o "$scratch/sampler.so" "$here/sampler.c" -lpthread
  env_extra=(LD_PRELOAD="$scratch/sampler.so" H2_SAMPLE_OUT="$scratch/prof" H2_SAMPLE_DELAY="$delay" H2_SAMPLE_US=1000)
fi
if [[ -n "${H2_CPUMON:-}" && -z "$delay" ]]; then
  gcc -O2 -shared -fPIC -o "$scratch/cpumon.so" "$here/cpumon.c" -lpthread
  env_extra=(LD_PRELOAD="$scratch/cpumon.so" H2_CPU_MONITOR_OUT="$scratch/cpumon.txt")
fi
rm -f "$scratch/timing.tsv" "$scratch/cpumon.txt"
start=$(date +%s.%N)
env HALO2_RENDER_TIMINGS="$scratch/timing.tsv" HALO2_MENU_FOG_SEED=1 \
  SDL_WINDOW_NO_ACTIVATION_WHEN_SHOWN=1 SDL_WINDOW_ACTIVATE_WHEN_SHOWN=0 SDL_WINDOW_ACTIVATE_WHEN_RAISED=0 \
  SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT=0x0000/0x0000 "${env_extra[@]}" \
  "$runtime" --multiplayer --map "$maps/$map.map" --shared-maps "$maps" \
  --mp-starting-weapon 5 --local-players "$players" --frames "$frames" \
  --viewport 1280 720 --render-size 3840 2160 --mute "$@" > "$scratch/run.log" 2>&1 || {
    echo "runtime failed: $?"; tail -30 "$scratch/run.log"; exit 1; }
end=$(date +%s.%N)
python3 -c "print('elapsed %.1f s' % ($end - $start))" | tee "$report/$label.elapsed"
python3 - "$scratch/timing.tsv" "$report/$label" <<'EOF'
import csv, statistics, sys, json
rows = list(csv.DictReader(open(sys.argv[1]), delimiter='\t'))
rows = rows[30:]
out = {}
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
cp "$scratch/timing.tsv" "$report/$label.timing.tsv"
grep -a "Native main cull" "$scratch/run.log" | tail -40 > "$report/$label.cull.txt" || true
if [[ -f "$scratch/cpumon.txt" ]]; then
  python3 - "$scratch/cpumon.txt" <<'PY' | tee "$report/$label.cpu.txt"
import sys
rows = [tuple(map(int, l.split())) for l in open(sys.argv[1]) if l.strip()]
first = next((r for r in rows if r[0] >= 30), rows[0])
last = max((r for r in rows if r[0] > first[0]), key=lambda r: r[0])
frames = last[0] - first[0]
print(f'cpu_ms_per_frame={(last[1]-first[1])/frames/1e6:.3f} wall_ms_per_frame={(last[2]-first[2])/frames/1e6:.3f} frames={frames}')
PY
fi
if [[ -n "$delay" ]]; then
  python3 "$here/analyze.py" "$scratch/prof" "$runtime" 60 "${H2_WINDOW:-0}" > "$report/$label.profile.txt"
  cp "$scratch/prof.samples" "$scratch/prof.maps" "$report/" && mv "$report/prof.samples" "$report/$label.samples" && mv "$report/prof.maps" "$report/$label.maps"
  head -70 "$report/$label.profile.txt"
fi
