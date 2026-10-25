#!/usr/bin/env bash
# Interleaved A/B FPS: ab.sh LABEL PAIRS RUNTIME_A RUNTIME_B [players]
set -euo pipefail
label=$1 pairs=$2 a=$3 b=$4 players=${5:-4}
S=$(dirname "$(readlink -f "$0")")
reports=${H2_PERF_REPORTS:?set H2_PERF_REPORTS to a report directory}
mkdir -p "$reports"
for ((i = 1; i <= pairs; ++i)); do
  if ((i % 2)); then order="A B"; else order="B A"; fi
  for role in $order; do
    runtime=$a; [[ $role == B ]] && runtime=$b
    result=$(H2_CPUMON=1 "$S/profile_run.sh" "$runtime" "$reports" "$label-$role-$i" "$players" 300 "" 2>&1)
    fps=$(sed -n 's/^frames=[0-9]* fps=//p' <<<"$result"); cpu=$(sed -n 's/^cpu_ms_per_frame=\([0-9.]*\).*/\1/p' <<<"$result")
    echo "$label pair$i $role fps=$fps cpu_ms=$cpu load=$(cut -d' ' -f1 /proc/loadavg)"
  done
done
