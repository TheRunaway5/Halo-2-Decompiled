#!/usr/bin/env bash
set -euo pipefail
task_root="$(cd "$(dirname "$0")/.." && pwd)"
task_build="${1:?Specify a private build directory}"
task_mode="${2:-normal}"
mkdir -p "$task_build/shaders"
glslangValidator -V --target-env vulkan1.1 "$task_root/shaders/source_weather_depth_alias.comp" -o "$task_build/shaders/source_weather_depth_alias.comp.spv"
glslangValidator -V --target-env vulkan1.1 "$task_root/tests/source_weather_depth_alias_sample.comp" -o "$task_build/shaders/source_weather_depth_alias_sample.comp.spv"
task_flags=(-O2)
if [ "$task_mode" = asan ]; then task_flags=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer);
elif [ "$task_mode" != normal ]; then exit 2; fi
c++ -std=c++20 "${task_flags[@]}" -I"$task_root/include" -I"$task_root/tests" \
    "-DHALO2_SOURCE_DIR=\"$task_root\"" "-DHALO2_WEATHER_DEPTH_ALIAS_SHADER_DIR=\"$task_build/shaders\"" \
    "$task_root/src/rendering/weather/source_weather_depth_alias.cpp" "$task_root/tests/source_weather_depth_alias_tests.cpp" \
    -lvulkan -o "$task_build/source_weather_depth_alias_tests"
