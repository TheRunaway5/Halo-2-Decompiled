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
import tempfile
import zipfile

from bundle_movies import bundle as bundle_movies, read_manifest, verify_file
from mcc_movies import (ROLES, SOURCE_MANIFEST, _cancellation, _owned, _publish_new,
                        source_manifest)
from mcc_prompts import SOURCE as PROMPT_SOURCE, verify_source as verify_prompt_source

from packaged_shader_defaults import (native_runtime_wrapper_body,
                                     relocate_compiled_shader_default, shader_inventory)


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


def verify_bundled_movies(root):
    """Validate strict paired metadata and both full sources before staging."""
    root = Path(root).resolve()
    manifest_sha, files = source_manifest(root)
    metadata = read_manifest(root / SOURCE_MANIFEST)
    if sha(root / SOURCE_MANIFEST) != manifest_sha or metadata["files"] != files:
        raise ValueError("Bundled movie source manifest changed during verification")
    rows = {}
    for name, record in files.items():
        source = _owned(root / "resources/movies" / name, root)
        if not source.is_file():
            raise ValueError("Missing bundled movie source: " + str(source))
        before = source.stat()
        verify_file(source, record)
        after = source.stat()
        if (before.st_size, before.st_mtime_ns) != (after.st_size, after.st_mtime_ns):
            raise ValueError("Bundled movie source changed during verification")
        rows["resources/movies/" + name] = dict(record)
    for name in (SOURCE_MANIFEST, "resources/movies/manifest.json"):
        path = root / name
        rows[name] = dict(bytes=path.stat().st_size, sha256=sha(path))
    # A same-size source metadata change must not be paired with old payload
    # records. No output has been created at this boundary.
    if source_manifest(root) != (manifest_sha, files):
        raise ValueError("Bundled movie source manifests changed during verification")
    return dict(schema="H2_PACKAGED_BUNDLED_MOVIES_V1", source_schema=metadata["schema"],
                source_provenance="Port-supplied user-provided AI upscaled intro and credits; preserved without transcoding",
                files=rows, roles={role: "resources/movies/" + name for role, name in ROLES.items()},
                movie_payload_bytes=sum(row["bytes"] for row in files.values()),
                cache_import_destination="presentation/movies", attract_assigned=False,
                external_vista_or_xbox_required=False)


def copy_bundled_movies(root, stage, verified):
    """Copy exactly the two assigned roles and both source metadata files."""
    root, stage = Path(root), Path(stage)
    if verify_bundled_movies(root) != verified:
        raise ValueError("Bundled movies changed after package preflight")
    manifest = read_manifest(root / SOURCE_MANIFEST)
    bundle_movies(root / "resources/movies", stage / "resources/movies", manifest)
    # Preserve the exact paired input bytes, including harmless formatting;
    # bundle_movies has already verified the complete copied video payloads.
    for name in (SOURCE_MANIFEST, "resources/movies/manifest.json"):
        copy(root / name, stage / name)
    if source_manifest(stage)[1] != manifest["files"]:
        raise ValueError("Packaged movie metadata differs from its payload records")
    for name in (SOURCE_MANIFEST, "resources/movies/manifest.json"):
        if sha(stage / name) != verified["files"][name]["sha256"]:
            raise ValueError("Packaged movie source metadata changed")
    return verified


def verify_bundled_prompts(root):
    """Authenticate all original masters and runtime files before staging."""
    raw, metadata, _ = verify_prompt_source(root)
    records = {**metadata["runtime_files"], **metadata["artwork_files"]}
    files = {PROMPT_SOURCE + "/" + name: dict(row) for name, row in records.items()}
    files[PROMPT_SOURCE + "/manifest.json"] = dict(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest())
    return dict(schema="H2_PACKAGED_AUTHORED_PROMPTS_V1", source_schema=metadata["source_schema"],
                source_provenance=metadata["source_provenance"], files=files,
                source_file_count=len(files), source_bytes=sum(row["bytes"] for row in files.values()),
                svg_masters=82, runtime_files=7, cache_files=9,
                cache_import_destination="presentation/prompts", cache_binding="relative dot",
                replaces_MCC_fonts=False, external_vista_or_xbox_required=False)


def copy_bundled_prompts(root, stage, verified):
    """Preserve every verified source-relative prompt path, then rehash copy."""
    root, stage = Path(root), Path(stage)
    if verify_bundled_prompts(root) != verified:
        raise ValueError("Authored prompt source changed after package preflight")
    for name in sorted(verified["files"]):
        copy(root / name, stage / name)
    if verify_bundled_prompts(stage) != verified:
        raise ValueError("Packaged authored prompt source differs from its verified originals")
    return verified


def write_archive(stage, pending, movies):
    with zipfile.ZipFile(pending, "x", compression=zipfile.ZIP_DEFLATED, compresslevel=6,
                         allowZip64=True) as archive:
        for path in sorted(stage.rglob("*")):
            if path.is_file():
                relative = path.relative_to(stage).as_posix()
                # The exact assigned MP4s are already compressed. Store their
                # original bytes; retain the normal compression for all else.
                archive.write(path, stage.name + "/" + relative,
                              compress_type=zipfile.ZIP_STORED if relative in movies["roles"].values()
                              else zipfile.ZIP_DEFLATED)


def _assemble(root, runtime, launchers, output, shader_directory, shaders, stage, pending, movies, prompts):
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
    payload = packaged_runtime.read_bytes()
    payload, shader_relocation = relocate_compiled_shader_default(payload, shader_directory)
    packaged_runtime.write_bytes(payload)
    shutil.copytree(shader_directory, stage / "bin/shaders")
    if shader_inventory(stage / "bin/shaders") != shaders:
        raise RuntimeError("Packaged shader bytes differ from the existing bank")
    for source in (root / "tools").glob("*.py"):
        if not source.is_symlink():
            copy(source, stage / "tools" / source.name)
    for source in (root / "tools/reference").glob("*.tsv"):
        if not source.is_symlink():
            copy(source, stage / "tools/reference" / source.name)
    shutil.copytree(root / "tools/data", stage / "tools/data", ignore=shutil.ignore_patterns("__pycache__", ".git"))
    copy(root / "resources/launcher.svg", stage / "resources/launcher.svg")
    shutil.copytree(root / "resources/licenses", stage / "resources/licenses")
    copy_bundled_movies(root, stage, movies)
    copy_bundled_prompts(root, stage, prompts)
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
    wrapper(stage / "bin/halo2_native", native_runtime_wrapper_body())
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
        "runtime_shader_relocation": shader_relocation,
        "runtime_wrapper_sha256": sha(stage / "bin/halo2_native"),
        "shaders": shaders,
        "shared_libraries": {name: sha(stage / "lib" / name) for name in sorted(library_sources)},
        "bundled_movies": movies,
        "authored_prompts": prompts,
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
        "The ZIP also includes the port-supplied user-provided AI upscaled intro and credits, without transcoding.\n"
        f"These two bundled MP4s add {movies['movie_payload_bytes']:,} bytes (about 2.54 GB for the supplied release movies).\n"
        "Setup copies them into the private presentation cache; allow another 2.54 GB there. Attract is unassigned.\n"
        "The port's authored button prompts include all 82 SVG masters, reconstruction provenance and seven runtime files.\n"
        f"Their complete {prompts['source_file_count']}-file source bank adds {prompts['source_bytes']:,} bytes to the ZIP.\n"
        "Setup copies the runtime prompt bank into presentation/prompts and preserves the MCC font tree. No Vista fonts are included.\n"
        "Select a map, then Play. Play currently opens the MCC content preview.\n"
        "The preview lets testers inspect maps, textures, materials and rendering. Native gameplay\n"
        "and complete Xbox TU1.5 conversion remain in progress; this package does not claim parity.\n"
        "This contains the existing compiled runtime; newer source-only compatibility fixes are not included.\n\n"
        "To capture a test: ./Halo2-Launcher --launch-game --map lockout.map --frames 3 --screenshot lockout.ppm\n"
        "Runtime output can be collected with: ./Halo2-Launcher --launch-game > test.log 2>&1\n"
        "To update this precompiled package, extract a newer test ZIP. No compilation is needed.\n")
    files = sorted(p for p in stage.rglob("*") if p.is_file())
    (stage / "SHA256SUMS").write_text("".join(f"{sha(p)}  {p.relative_to(stage).as_posix()}\n" for p in files))
    write_archive(stage, pending, movies)
    return {"zip": str(output), "bytes": pending.stat().st_size, "sha256": sha(pending),
            "stage_retained": False, "bundled_movie_payload_bytes": movies["movie_payload_bytes"],
            "authored_prompt_source_files": prompts["source_file_count"],
            "authored_prompt_source_bytes": prompts["source_bytes"],
            "zip64_enabled": True, "mode": "MCC content preview; no new gameplay/frontend acceptance"}


def package(root, runtime, launchers, output, shader_directory=None):
    root, runtime, launchers = Path(root).resolve(), Path(runtime).resolve(), Path(launchers).resolve()
    output = Path(output)
    if output.is_symlink() or output.exists():
        raise ValueError("Refusing to replace an existing ZIP")
    output = output.absolute()
    shader_directory = Path(shader_directory or runtime.parent / "shaders").resolve(strict=True)
    shaders = shader_inventory(shader_directory)
    movies = verify_bundled_movies(root)
    prompts = verify_bundled_prompts(root)
    output.parent.mkdir(parents=True, exist_ok=True)
    # Only the complete requested ZIP persists. The private stage (including
    # the 2.54 GB movie copy) is removed on success, failure and cancellation.
    with _cancellation(), tempfile.TemporaryDirectory(prefix=".mcc-package-", dir=output.parent) as temporary:
        temporary = Path(temporary)
        stage, pending = temporary / output.stem, temporary / "complete.zip"
        result = _assemble(root, runtime, launchers, output, shader_directory, shaders, stage, pending, movies, prompts)
        _publish_new(pending, output)
    return result


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runtime", type=Path, required=True)
    parser.add_argument("--launchers", type=Path, required=True)
    parser.add_argument("--shader-directory", type=Path,
                        help="actual compiled default shader bank (defaults to RUNTIME_PARENT/shaders)")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(package(root, args.runtime, args.launchers, args.output, args.shader_directory)))


if __name__ == "__main__":
    main()
