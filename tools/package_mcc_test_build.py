#!/usr/bin/env python3
"""Package already compiled Linux test binaries and their runtime dependencies."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import sysconfig
import zipfile


def sha(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def copy(source, target):
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, target)


def wrapper(path, body):
    path.write_text("#!/bin/sh\nset -eu\n" + body)
    path.chmod(0o755)


def dependencies(path):
    result = subprocess.run(["ldd", str(path)], capture_output=True, text=True, check=True)
    if "not found" in result.stdout:
        raise RuntimeError("Unresolved runtime dependency: " + result.stdout)
    return [Path(match) for match in re.findall(r"(?:=>\s+|^\s*)(/\S+)\s+\(", result.stdout, re.M)]


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runtime", type=Path, required=True)
    parser.add_argument("--launchers", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    runtime, launchers, output = args.runtime.resolve(), args.launchers.resolve(), args.output.resolve()
    if output.exists():
        parser.error("Refusing to replace an existing ZIP")
    stage = output.parent / output.stem
    if stage.exists():
        parser.error("Refusing to replace an existing staging directory")
    stage.mkdir(parents=True)
    (stage / "assets/imported").mkdir(parents=True)
    (stage / "bin").mkdir()
    executable_sources = [runtime, launchers / "Halo2-Launcher", launchers / "Halo2-Title-Launcher"]
    copied_elf = []
    for source in executable_sources:
        destination = stage / "bin" / (source.name + ".real")
        copy(source, destination)
        subprocess.run(["strip", "--strip-debug", str(destination)], check=True)
        copied_elf.append(destination)
    # The existing renderer embeds its build-time directory as a fixed-length
    # path constant. Relocate that data constant without changing instructions,
    # field widths or any shader bytes. Redundant ./ segments retain its length.
    packaged_runtime = stage / "bin/halo2_native.real"
    original_path = str(runtime.parent / "shaders").encode()
    relative_path = b"bin/shaders"
    padding = len(original_path) - len(relative_path)
    if padding < 0:
        raise RuntimeError("Compiled shader path is too short to relocate")
    if padding % 2:
        relative_path = b"bin//shaders"
        padding -= 1
    relative_path = b"./" * (padding // 2) + relative_path
    payload = packaged_runtime.read_bytes()
    if len(relative_path) != len(original_path) or payload.count(original_path) != 1:
        raise RuntimeError("Compiled shader path is not an unambiguous fixed-length constant")
    packaged_runtime.write_bytes(payload.replace(original_path, relative_path))
    shutil.copytree(runtime.parent / "shaders", stage / "bin/shaders")
    for source in (root / "tools").glob("*.py"):
        if not source.is_symlink():
            copy(source, stage / "tools" / source.name)
    for source in (root / "tools/reference").glob("*.tsv"):
        if not source.is_symlink():
            copy(source, stage / "tools/reference" / source.name)
    shutil.copytree(root / "tools/data", stage / "tools/data", ignore=shutil.ignore_patterns("__pycache__", ".git"))
    copy(root / "resources/launcher.svg", stage / "resources/launcher.svg")
    shutil.copytree(root / "resources/licenses", stage / "resources/licenses")
    for name in ("LICENSE", "LICENSE.md", "LICENSE.txt", "COPYING"):
        if (root / name).is_file():
            copy(root / name, stage / name)
    custom = root / "assets/native-loading-custom"
    if custom.is_dir():
        shutil.copytree(custom, stage / "assets/native-loading-custom")

    # Ship the interpreter, Tk and Pillow used by onboarding. No compiler,
    # development checkout, local cache, credentials or original game assets.
    stdlib = Path(sysconfig.get_path("stdlib"))
    shutil.copytree(stdlib, stage / "python/lib" / stdlib.name,
                    ignore=shutil.ignore_patterns("site-packages", "__pycache__", "test", "tests", "idlelib", "ensurepip"))
    import PIL
    shutil.copytree(Path(PIL.__path__[0]), stage / "python/lib" / stdlib.name / "site-packages/PIL",
                    ignore=shutil.ignore_patterns("__pycache__"))
    python_binary = stage / "python/bin/python3.real"
    copy(Path(sys.executable).resolve(), python_binary)
    copied_elf.append(python_binary)
    for directory in (Path("/usr/lib/tcl8.6"), Path("/usr/lib/tk8.6"), Path("/usr/lib/tcl8")):
        if directory.is_dir():
            shutil.copytree(directory, stage / "python" / directory.name)
    for source in (stage / "python").rglob("*.so"):
        copied_elf.append(source)
    library_sources = {}
    for binary in copied_elf:
        for source in dependencies(binary):
            library_sources.setdefault(source.name, source)
    loader = Path("/lib64/ld-linux-x86-64.so.2")
    library_sources[loader.name] = loader
    for name, source in library_sources.items():
        copy(source, stage / "lib" / name)
    # Keep license notices for the redistributed host shared libraries.
    packages = set()
    if shutil.which("pacman"):
        result = subprocess.run(["pacman", "-Qoq", *map(str, library_sources.values())], capture_output=True, text=True)
        packages.update(result.stdout.splitlines())
    for package in sorted(packages):
        notices = Path("/usr/share/licenses") / package
        if notices.is_dir():
            shutil.copytree(notices, stage / "resources/licenses/runtime" / package, symlinks=False)
    for name in ("Halo2-Launcher", "Halo2-Title-Launcher"):
        wrapper(stage / name,
                'root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)\n'
                'export PATH="$root/python/bin:$PATH"\n'
                'exec "$root/lib/ld-linux-x86-64.so.2" --library-path "$root/lib" "$root/bin/' + name + '.real" "$@"\n')
    wrapper(stage / "bin/halo2_native",
            'root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)\n'
            'cd "$root"\n'
            'exec "$root/lib/ld-linux-x86-64.so.2" --library-path "$root/lib" "$root/bin/halo2_native.real" "$@"\n')
    wrapper(stage / "python/bin/python3",
            'root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)\n'
            'export PYTHONHOME="$root/python"\n'
            'export TCL_LIBRARY="$root/python/tcl8.6" TK_LIBRARY="$root/python/tk8.6"\n'
            'exec "$root/lib/ld-linux-x86-64.so.2" --library-path "$root/lib" "$root/python/bin/python3.real" "$@"\n')
    manifest = {
        "schema": "H2_PRECOMPILED_MCC_TEST_V1", "platform": "Linux x86-64",
        "mode": "MCC content preview", "native_gameplay_complete": False,
        "full_tu15_parity": False, "compilation_required": False,
        "external_game_data": "Tester's own Halo MCC installation",
        "runtime_input_sha256": sha(runtime), "runtime_packaged_sha256": sha(stage / "bin/halo2_native.real"),
        "runtime_source_changes_rebuilt": False,
        "runtime_shader_directory_relocated": True,
        "shaders": {p.name: sha(p) for p in sorted((stage / "bin/shaders").glob("*.spv"))},
        "shared_libraries": {name: sha(stage / "lib" / name) for name in sorted(library_sources)},
    }
    subprocess.run([str(stage / "bin/halo2_native"), "--mcc-preview", "--help"],
                   check=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, timeout=30)
    (stage / "PRECOMPILED_TEST_BUILD.json").write_text(json.dumps(manifest, indent=2) + "\n")
    (stage / "README.txt").write_text(
        "Halo 2 MCC content test build — Linux x86-64\n\n"
        "Extract the entire ZIP into a writable folder. Run ./Halo2-Launcher.\n"
        "The game, shaders, Python, Tk and native libraries are already compiled/bundled.\n"
        "Keep all folders beside the launcher. A Linux desktop and working Vulkan driver are required.\n\n"
        "First launch detects your installed Halo MCC and copies Halo 2 content to assets/imported.\n"
        "If detection fails, select your MCC installation or its halo2 folder.\n"
        "Allow space for the Halo 2 data plus decoded resources. No Xbox ISO, TU patch or Vista bank is needed.\n"
        "Select a map, then Play. Play currently opens the MCC content preview.\n"
        "The preview lets testers inspect maps, textures, materials and rendering. Native gameplay\n"
        "and complete Xbox TU1.5 conversion remain in progress; this package does not claim parity.\n"
        "This contains the existing compiled runtime; newer source-only compatibility fixes are not included.\n\n"
        "To capture a test: ./Halo2-Launcher --launch-game --map lockout.map --frames 3 --screenshot lockout.ppm\n"
        "Runtime output can be collected with: ./Halo2-Launcher --launch-game > test.log 2>&1\n"
        "To update this precompiled package, extract a newer test ZIP. No compilation is needed.\n")
    files = sorted(p for p in stage.rglob("*") if p.is_file())
    (stage / "SHA256SUMS").write_text("".join(f"{sha(p)}  {p.relative_to(stage).as_posix()}\n" for p in files))
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for path in sorted(stage.rglob("*")):
            if path.is_file():
                archive.write(path, stage.name + "/" + path.relative_to(stage).as_posix())
    print(json.dumps({"zip": str(output), "bytes": output.stat().st_size, "sha256": sha(output), "stage": str(stage)}))


if __name__ == "__main__":
    main()
