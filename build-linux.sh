#!/usr/bin/env bash
set -euo pipefail

if [[ ${1:-} == --help || ${1:-} == -h ]]; then
    printf '%s\n' 'Usage: ./build-linux.sh [build_runtime.py options]' \
        'Builds the supported Linux x86_64 runtime and launchers.' \
        'HALO2_BUILD_DIR: build directory, relative to the repo or absolute (default: build).' \
        'HALO2_PYTHON: Python 3 executable (default: python3).' \
        'CMAKE_BUILD_PARALLEL_LEVEL: optional build job limit.' \
        'Latest binaries: dist/builds/linux-x86_64/.'
    exit 0
fi

repo_dir=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
build_dir=${HALO2_BUILD_DIR:-build}
python_command=${HALO2_PYTHON:-python3}
if [[ $build_dir != /* ]]; then
    build_dir="$repo_dir/$build_dir"
fi

if [[ $(uname -s) != Linux ]]; then
    printf '%s\n' 'This script builds Linux x86_64. Windows and macOS builds are not supported yet.' >&2
    exit 2
fi
case $(uname -m) in
    x86_64|amd64) ;;
    *) printf '%s\n' 'Only Linux x86_64 is supported for now; Linux ARM64 builds are skipped.' >&2; exit 2 ;;
esac
if ! command -v "$python_command" >/dev/null 2>&1; then
    printf '%s\n' "Python was not found: $python_command. Install Python 3.11+ or set HALO2_PYTHON." >&2
    exit 1
fi

exec "$python_command" -B "$repo_dir/tools/build_runtime.py" \
    --root "$repo_dir" --build-dir "$build_dir" --no-gui "$@"
