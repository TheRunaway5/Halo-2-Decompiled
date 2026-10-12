#!/usr/bin/env bash
set -euo pipefail
repo_dir="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="${1:-$repo_dir/analysis/multiplayer-shading/weather-plate-caller-profile/build}"
mkdir -p "$build_dir"
extra_flags=()
if [[ "${ASAN:-0}" == 1 ]]; then
    extra_flags=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie)
else
    extra_flags=(-O2)
fi
c++ -std=c++20 -ffp-contract=off -fno-fast-math -Wall -Wextra -Werror \
    "${extra_flags[@]}" -I"$repo_dir/include" \
    "$repo_dir/src/source_weather_plate_program.cpp" \
    "$repo_dir/src/source_weather_plate_caller.cpp" \
    "$repo_dir/tests/source_weather_plate_caller_tests.cpp" \
    -o "$build_dir/source_weather_plate_caller_tests"
