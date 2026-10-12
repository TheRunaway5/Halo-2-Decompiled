#!/usr/bin/env bash
set -euo pipefail
task_root="$(cd "$(dirname "$0")/.." && pwd)"
task_build="${1:?Specify a private build directory}"
task_mode="${2:-normal}"
task_native="${3:-$task_root/analysis/multiplayer-shading/depth-encoding-gpu-prototype/historical-world.frag}"
if [ ! -f "$task_native" ]; then task_native="$task_root/shaders/world.frag"; fi
mkdir -p "$task_build/shaders"
"$task_root/.tools/python/bin/python" "$task_root/tools/prepare_source_depth_encoding_gpu.py" "$task_build/shaders" --native "$task_native"
glslangValidator -V --target-env vulkan1.1 "$task_root/tests/source_depth_encoding.vert" -o "$task_build/shaders/source_depth_encoding.vert.spv"
glslangValidator -V --target-env vulkan1.1 -I"$task_build/shaders" "$task_root/tests/source_depth_encoding.frag" -o "$task_build/shaders/source_depth_encoding.frag.spv"
glslangValidator -V --target-env vulkan1.1 -DDEPTH_COLOR_CONTROL=1 -I"$task_build/shaders" "$task_root/tests/source_depth_encoding.frag" -o "$task_build/shaders/source_depth_encoding_color.frag.spv"
for task_shader in "$task_build"/shaders/*.spv; do spirv-val --target-env vulkan1.1 "$task_shader"; done
task_flags=(-O2 -ffp-contract=off -fno-fast-math)
if [ "$task_mode" = asan ]; then task_flags=(-O1 -g -ffp-contract=off -fno-fast-math -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie);
elif [ "$task_mode" != normal ]; then exit 2; fi
c++ -std=c++20 "${task_flags[@]}" -Wall -Wextra -Wno-missing-field-initializers -I"$task_root/include" -I"$task_root/tests" \
    "-DHALO2_DEPTH_ENCODING_DIR=\"$task_build/shaders\"" \
    "$task_root/tests/source_depth_encoding_gpu_tests.cpp" -lvulkan -o "$task_build/source_depth_encoding_gpu_tests"
"$task_build/source_depth_encoding_gpu_tests" --cpu
