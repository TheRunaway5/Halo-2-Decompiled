#!/usr/bin/env bash
set -euo pipefail
task_root="$(cd "$(dirname "$0")/.." && pwd)"
task_build="${1:?Specify a private CPU build directory}"
task_mode="${2:-normal}"
mkdir -p "$task_build"
task_flags=(-O2)
if [ "$task_mode" = asan ]; then task_flags=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer);
elif [ "$task_mode" != normal ]; then exit 2; fi
c++ -std=c++20 -ffp-contract=off -fno-fast-math "${task_flags[@]}" \
    -I"$task_root/include" \
    "$task_root/src/source_weather_plate_program.cpp" "$task_root/tests/source_weather_plate_program_tests.cpp" \
    -o "$task_build/source_weather_plate_program_tests"
