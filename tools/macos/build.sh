#!/usr/bin/env bash
set -euo pipefail
if [[ ${1:-} == --help || ${1:-} == -h ]]; then
    printf '%s\n' 'Usage: ./build-macos.sh [extra CMake configure arguments]' \
        'Builds an arm64+x86_64 app with bundled MoltenVK on macOS.' \
        'Set HALO2_MOLTENVK_LIBRARY and HALO2_MOLTENVK_LICENSE.' \
        'SDL is built for both architectures by default; external SDL also needs HALO2_SDL_LICENSE.' \
        'All linked dependencies must supply both architectures.' \
        'HALO2_BUILD_DIR defaults to build-macos; HALO2_PYTHON defaults to python3.' \
        'HALO2_MACOS_TARGET defaults to halo2_native; graphics bring-up:' \
        '  HALO2_MACOS_TARGET=halo2_macos_graphics_probe ./build-macos.sh -DHALO2_MACOS_BRINGUP_ONLY=ON' \
        'See docs/MACOS_PORT.md for current engine blockers and disposable validation.'
    exit 0
fi
if [[ $(uname -s) != Darwin ]]; then
    printf '%s\n' 'Run build-macos.sh on a Mac with Xcode Command Line Tools.' >&2
    exit 2
fi
repo_dir=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
build_dir=${HALO2_BUILD_DIR:-build-macos}
if [[ $build_dir != /* ]]; then build_dir="$repo_dir/$build_dir"; fi
cmake -S "$repo_dir" -B "$build_dir" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' \
    -DPython3_EXECUTABLE="${HALO2_PYTHON:-python3}" \
    -DHALO2_MOLTENVK_LIBRARY="${HALO2_MOLTENVK_LIBRARY:?Set the universal MoltenVK dylib path}" \
    -DHALO2_MOLTENVK_LICENSE="${HALO2_MOLTENVK_LICENSE:?Set MoltenVK LICENSE path}" \
    -DHALO2_SDL_LICENSE="${HALO2_SDL_LICENSE:-}" "$@"
cmake --build "$build_dir" --target "${HALO2_MACOS_TARGET:-halo2_native}" \
    --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
