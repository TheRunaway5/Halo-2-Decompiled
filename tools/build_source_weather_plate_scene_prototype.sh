#!/usr/bin/env bash
set -euo pipefail
task_root="$(cd "$(dirname "$0")/.." && pwd)"
task_build="${1:?Specify a private build directory}"
task_archive="${2:?Specify the coherent halo2_cache archive}"
task_mode="${3:-normal}"
mkdir -p "$task_build/shaders"
for task_shader in source_weather_plate.vert source_weather_plate.frag; do
    glslangValidator -V --target-env vulkan1.1 -I"$task_root/shaders" "$task_root/shaders/$task_shader" -o "$task_build/shaders/$task_shader.spv"
done
task_flags=(-O2)
if [ "$task_mode" = asan ]; then task_flags=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer);
elif [ "$task_mode" != normal ]; then exit 2; fi
c++ -std=c++20 -ffp-contract=off -fno-fast-math "${task_flags[@]}" \
    -I"$task_root/include" -I"$task_root/tests" \
    "-DHALO2_SOURCE_DIR=\"$task_root\"" "-DHALO2_WEATHER_PLATE_SHADER_DIR=\"$task_build/shaders\"" \
    "$task_root/src/source_weather_plate_render.cpp" "$task_root/tests/source_weather_plate_scene_tests.cpp" \
    "$task_archive" -lvulkan -lz -llz4 -o "$task_build/source_weather_plate_scene_tests"
