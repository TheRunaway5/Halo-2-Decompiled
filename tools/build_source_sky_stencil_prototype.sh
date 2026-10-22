#!/usr/bin/env bash
set -euo pipefail
task_root="$(cd "$(dirname "$0")/.." && pwd)"
task_build="${1:?Specify a new private build directory}"
task_mode="${2:-normal}"
mkdir -p "$task_build/shaders"
for task_shader in source_sky_stencil.vert source_sky_stencil.geom source_sky_stencil.frag source_sky_stencil_passthrough.vert; do
    glslangValidator -V --target-env vulkan1.1 "$task_root/shaders/$task_shader" -o "$task_build/shaders/$task_shader.spv"
    spirv-val --target-env vulkan1.1 "$task_build/shaders/$task_shader.spv"
done
task_flags=(-O2)
if [ "$task_mode" = asan ]; then task_flags=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer);
elif [ "$task_mode" != normal ]; then exit 2; fi
c++ -std=c++20 -ffp-contract=off -fno-fast-math "${task_flags[@]}" -I"$task_root/include" -I"$task_root/tests" \
    "-DHALO2_SOURCE_DIR=\"$task_root\"" "-DHALO2_SKY_STENCIL_SHADER_DIR=\"$task_build/shaders\"" \
    "$task_root/src/rendering/world/source_sky_stencil.cpp" "$task_root/tests/source_sky_stencil_tests.cpp" -lvulkan -o "$task_build/source_sky_stencil_tests"
