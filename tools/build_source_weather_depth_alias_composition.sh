#!/usr/bin/env bash
set -euo pipefail
task_root="$(cd "$(dirname "$0")/.." && pwd)"
task_build="${1:?Specify a private build directory}"
task_mode="${2:-normal}"
task_render_archive="$task_root/analysis/multiplayer-shading/weather-graphics-cmake-integration/build/libhalo2_weather_render.a"
task_cache_archive="$task_root/analysis/multiplayer-shading/resolution-bitmap-frozen-source/build/libhalo2_cache.a"
mkdir -p "$task_build/shaders"
for task_shader in source_weather_depth_alias.comp source_weather_plate.vert source_weather_plate.frag; do
    glslangValidator -V --target-env vulkan1.1 -I"$task_root/shaders" "$task_root/shaders/$task_shader" -o "$task_build/shaders/$task_shader.spv"
done
task_flags=(-O2)
task_render_input=("$task_render_archive")
if [ "$task_mode" = asan ]; then
    task_flags=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer)
    # Instrument the existing plate module from its unchanged source as well.
    task_render_input=("$task_root/src/rendering/weather/source_weather_plate_render.cpp")
elif [ "$task_mode" != normal ]; then exit 2; fi
c++ -std=c++20 -ffp-contract=off -fno-fast-math "${task_flags[@]}" -I"$task_root/include" -I"$task_root/tests" \
    "-DHALO2_SOURCE_DIR=\"$task_root\"" "-DHALO2_WEATHER_DEPTH_ALIAS_SHADER_DIR=\"$task_build/shaders\"" "-DHALO2_WEATHER_PLATE_SHADER_DIR=\"$task_build/shaders\"" \
    "$task_root/src/rendering/weather/source_weather_depth_alias.cpp" "$task_root/tests/source_weather_depth_alias_composition_tests.cpp" \
    "${task_render_input[@]}" "$task_cache_archive" -lvulkan -lz -llz4 -o "$task_build/source_weather_depth_alias_composition_tests"
