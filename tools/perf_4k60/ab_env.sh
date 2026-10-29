#!/usr/bin/env bash
# Interleaved A/B of one runtime with/without environment assignments: ab_env.sh LABEL PAIRS RUNTIME VAR=VALUE [VAR=VALUE ...]
set -euo pipefail
if (( $# < 4 )); then
  echo 'Usage: ab_env.sh LABEL PAIRS RUNTIME VAR=VALUE [VAR=VALUE ...]' >&2
  exit 2
fi
label=$1 pairs=$2 runtime=$3
assignments=("${@:4}")
S=$(dirname "$(readlink -f "$0")")
reports=${H2_PERF_REPORTS:?set H2_PERF_REPORTS to a report directory}
mkdir -p "$reports"
for ((i = 1; i <= pairs; ++i)); do
  if ((i % 2)); then order="A B"; else order="B A"; fi
  for role in $order; do
    if [[ $role == B ]]; then result=$(env "${assignments[@]}" H2_CPUMON=1 "$S/profile_run.sh" "$runtime" "$reports" "$label-$role-$i" 4 300 "" 2>&1)
    else result=$(H2_CPUMON=1 "$S/profile_run.sh" "$runtime" "$reports" "$label-$role-$i" 4 300 "" 2>&1); fi
    fps=$(sed -n 's/^frames=[0-9]* fps=//p' <<<"$result"); cpu=$(sed -n 's/^cpu_ms_per_frame=\([0-9.]*\).*/\1/p' <<<"$result")
    echo "$label pair$i $role fps=$fps cpu_ms=$cpu load=$(cut -d' ' -f1 /proc/loadavg)"
  done
done
