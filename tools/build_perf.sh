#!/usr/bin/env bash
# Shared build of the perf tree (build-perf-4k60, Release halo2_native).
#
# Every session builds the shared tree only through this script: builds are
# serialized by a lock in the build directory, the configure step re-runs the
# operand specialization (so the binders in tools/native_*_replacements.py are
# re-applied), and each build keeps its log beside the tree.
#
#   tools/build_perf.sh                 build halo2_native
#   JOBS=8 tools/build_perf.sh          fewer compile jobs (default 12)
#   tools/build_perf.sh --pin NAME      also copy the result to halo2_native.NAME
#
# A pinned copy is about 350 MB; delete it after its live checks.
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
build=${HALO2_PERF_BUILD:-$root/build-perf-4k60}
jobs=${JOBS:-12}
pin=

while (($#)); do
    case $1 in
        --pin) pin=${2:?--pin needs a name}; shift 2 ;;
        *) echo "usage: $0 [--pin NAME]" >&2; exit 2 ;;
    esac
done

[[ -f $build/CMakeCache.txt ]] || { echo "no configured tree at $build" >&2; exit 1; }
log=$build/build-$(date +%Y%m%d-%H%M%S).log

exec 9>"$build/.build_perf.lock"
if ! flock -n 9; then
    echo "waiting for another build_perf.sh on $build" >&2
    flock 9
fi

start=$(date +%s)
{
    echo "== configure $(date -Is)"
    cmake -S "$root" -B "$build"
    echo "== build halo2_native -j$jobs $(date -Is)"
    cmake --build "$build" --target halo2_native -j "$jobs"
} >"$log" 2>&1 || { status=$?; tail -40 "$log" >&2; echo "build failed ($status), log $log" >&2; exit "$status"; }

echo "built $build/halo2_native in $(( $(date +%s) - start ))s, log $log"
sha256sum "$build/halo2_native"
if [[ -n $pin ]]; then
    cp --reflink=auto "$build/halo2_native" "$build/halo2_native.$pin"
    echo "pinned $build/halo2_native.$pin"
fi
