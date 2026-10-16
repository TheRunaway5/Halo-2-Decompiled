#!/usr/bin/env python3
"""Freeze current sources and seed a private pStencil build from local objects.

Reflink copies never change the base checkout/build. CMake regenerates all
commands against the frozen source; compiler dependencies are relocated too.
Unchanged flag files retain their timestamps so their compatible object files
can be reused. Changed source/header dependencies still rebuild normally.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base-build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--base-source", type=Path, help="Optional retained source matching the seed build")
    args = parser.parse_args()
    root = args.base_source.resolve() if args.base_source else Path(__file__).resolve().parents[2]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    source, build = output / "source", output / "build"
    source.mkdir()
    for name in ("src", "include", "cmake", "shaders", "external", "tools", "tests", "resources", "data"):
        subprocess.run(["cp", "-a", "--reflink=auto", str(root / name), str(source / name)], check=True)
    for name in ("CMakeLists.txt", "LICENSE", "README.md"):
        subprocess.run(["cp", "-a", "--reflink=auto", str(root / name), str(source / name)], check=True)
    for name in ("assets", "analysis", ".tools", ".local", "Halo 2 Vista installer", "docs"):
        (source / name).symlink_to(root / name, target_is_directory=True)
    subprocess.run(["cp", "-a", "--reflink=auto", str(args.base_build.resolve()), str(build)], check=True)
    old_build = str(args.base_build.resolve())
    token = "__PSTENCIL_PRIVATE_BUILD__"
    flags = {}
    for path in build.rglob("*"):
        if not path.is_file() or not (path.suffix in (".make", ".cmake", ".d", ".txt") or path.name == "Makefile"):
            continue
        try:
            text = path.read_text()
        except UnicodeError:
            continue
        relocated = text.replace(old_build, token).replace(str(root), str(source)).replace(token, str(build))
        if relocated != text:
            stat = path.stat();path.write_text(relocated);os.utime(path, ns=(stat.st_atime_ns, stat.st_mtime_ns))
        if path.name in ("flags.make", "compiler_depend.ts"):
            flags[path] = (path.read_bytes(), path.stat().st_mtime_ns)
    manifest = []
    for directory in ("src", "include", "cmake", "shaders", "tests", "tools"):
        for path in sorted((source / directory).rglob("*")):
            if path.is_file() and not path.is_symlink():
                manifest.append(dict(path=str(path.relative_to(source)), sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
    (output / "source-manifest.json").write_text(json.dumps(dict(source=str(source), files=manifest), indent=2) + "\n")
    with (output / "configure.log").open("w") as log:
        arguments = ["cmake", "-S", str(source), "-B", str(build),
                     "-DPSTENCIL_2003_BUILD=/mnt/SSD4TB/Leaks/Halo 2/2003"]
        reference_python = root / ".tools/python/bin/python"
        if reference_python.is_file():
            arguments.append("-DPSTENCIL_REFERENCE_PYTHON=" + str(reference_python))
        subprocess.run(arguments, stdout=log, stderr=subprocess.STDOUT, check=True)
    for path, (before, timestamp) in flags.items():
        if path.exists() and path.read_bytes() == before:
            os.utime(path, ns=(path.stat().st_atime_ns, timestamp))
    print(build, flush=True)


if __name__ == "__main__":
    main()
