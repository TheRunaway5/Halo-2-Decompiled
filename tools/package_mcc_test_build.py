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

from mcc_movies import _cancellation, _publish_new
from installation_paths import resource_root
from native_variant_assets import source_bundle as variant_sources
from custom_assets import inventory as custom_asset_inventory, install as install_custom_assets
import mcc_presentation
from mcc_ui_layout import SOURCE as UI_LAYOUT_SOURCE, source_profile as verify_ui_layout
from mcc_loading_presentation import (bundled_ambience, AMBIENCE, TONE,
    converter_dependencies, converter_wrapper, CODEBOOK, CONVERTER_MANIFEST,
    CONVERTER_SCHEMA, CONVERTER_WRAPPER, CONVERTER_PAYLOAD, CONVERTER_LOADER)

from packaged_shader_defaults import (native_runtime_wrapper_body,
                                     relocate_compiled_shader_default, shader_inventory)

# Authored-family sources remain distinct from native MCC font profiles.
PROMPT_SOURCE = "resources/presentation/prompts"


def sha(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def copy(source, target):
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, target)


def wrapper(path, body):
    path.write_text("#!/bin/sh\nset -eu\n" + body)
    path.chmod(0o755)


def qualified_frontend(runtime, cache):
    """Admit the existing raw-binary proof before creating a portable owner."""
    from native_gameplay_test import _check
    cache = Path(cache)
    if cache.is_symlink():
        raise ValueError("Qualified frontend cache requires its direct private owner")
    cache = cache.resolve(strict=True)
    if cache.parent.name != "imported" or cache.parent.parent.name != "assets":
        raise ValueError("Qualified frontend cache must be under assets/imported")
    runtime = Path(runtime).resolve(strict=True)
    _check(cache.parents[2], runtime, cache, frontend=True)
    sidecar = runtime.with_name(runtime.name + ".frontend-test.json")
    before = sha(sidecar)
    report = json.loads(sidecar.read_text())
    if (report["shaders"].get("binding") != "compiled-default" or
            report["runtime"].get("sha256") != sha(runtime) or sha(sidecar) != before):
        raise ValueError("Portable frontend requires the qualified raw binary and its compiled shader owner")
    runtime_adjacent_data(runtime, report)
    return dict(report=report, source_capability_sha256=before, cache=cache)


RUNTIME_BANK_SCHEMA = "H2_RUNTIME_ADJACENT_SLAYER_BANK_V1"
RUNTIME_BANK_FILES = frozenset({
    "data/tags/variants/fiesta-slayer-v1.json",
    "data/tags/variants/infection-slayer-v1.json",
})


def runtime_adjacent_data(runtime, report):
    """Authenticate only the qualified runtime's explicit two-Slayer bank.

    The selected older loader prefers executable-adjacent variants. The normal
    source-root bank remains independent and may support newer engine modes.
    """
    declaration = report.get("runtime_adjacent_data")
    if declaration is None:
        return None
    if (not isinstance(declaration, dict) or declaration.get("schema") != RUNTIME_BANK_SCHEMA or
            not isinstance(declaration.get("files"), dict) or
            set(declaration["files"]) != RUNTIME_BANK_FILES):
        raise ValueError("Invalid qualified runtime-adjacent Slayer bank inventory")
    root = Path(runtime).resolve(strict=True).parent
    directory = root / "data/tags/variants"
    for relative in ("data", "data/tags", "data/tags/variants"):
        path = root / relative
        if path.is_symlink() or not path.is_dir() or path.resolve(strict=True) != path:
            raise ValueError("Runtime-adjacent bank requires direct owned directories")
    if {p.name for p in directory.iterdir()} != {Path(name).name for name in RUNTIME_BANK_FILES}:
        raise ValueError("Runtime-adjacent bank has undeclared files")
    for name, expected in declaration["files"].items():
        if (not isinstance(expected, dict) or set(expected) != {"bytes", "sha256"} or
                type(expected["bytes"]) is not int or not 0 < expected["bytes"] <= 16384 or
                not isinstance(expected["sha256"], str) or
                not re.fullmatch(r"[0-9a-f]{64}", expected["sha256"])):
            raise ValueError("Invalid runtime-adjacent bank file pin")
        path = root / name
        if path.is_symlink() or not path.is_file():
            raise ValueError("Runtime-adjacent bank payload is not a direct regular file")
        before = path.stat()
        raw = path.read_bytes()
        after = path.stat()
        if ((before.st_dev, before.st_ino, before.st_size, before.st_mtime_ns, before.st_ctime_ns) !=
                (after.st_dev, after.st_ino, after.st_size, after.st_mtime_ns, after.st_ctime_ns) or
                len(raw) != expected["bytes"] or hashlib.sha256(raw).hexdigest() != expected["sha256"]):
            raise ValueError("Runtime-adjacent bank payload changed or differs from its qualified pin")
    return json.loads(json.dumps(declaration))


def copy_runtime_adjacent_data(runtime, stage, qualified):
    declaration = runtime_adjacent_data(runtime, qualified["report"]) if qualified else None
    if declaration is None:
        return None
    for name, row in declaration["files"].items():
        target = stage / "bin" / name
        if target.exists() or target.is_symlink():
            raise ValueError("Runtime-adjacent package destination already exists")
        copy(Path(runtime).parent / name, target)
        if target.stat().st_size != row["bytes"] or sha(target) != row["sha256"]:
            raise ValueError("Runtime-adjacent package copy differs")
    if runtime_adjacent_data(runtime, qualified["report"]) != declaration:
        raise ValueError("Runtime-adjacent bank changed during packaging")
    return declaration


def packaged_frontend(stage, qualified, shader_relocation):
    """Rebind the passed proof to the existing fixed portable wrapper layout."""
    from packaged_shader_defaults import (PACKAGE_LOADER, PACKAGE_PAYLOAD,
                                          PACKAGE_RUNTIME, PACKAGE_SHADERS)
    report = json.loads(json.dumps(qualified["report"]))
    report["runtime"] = dict(path=PACKAGE_RUNTIME, sha256=sha(stage / PACKAGE_RUNTIME),
        payload=dict(path=PACKAGE_PAYLOAD, sha256=sha(stage / PACKAGE_PAYLOAD)),
        loader=dict(path=PACKAGE_LOADER, sha256=sha(stage / PACKAGE_LOADER)))
    # Preserve the compact AOT package pin as well as legacy inline inventories.
    # The module bank is revalidated against the relocated payload below.
    report["shaders"] = dict(report["shaders"], binding="packaged-compiled-default",
        directory=PACKAGE_SHADERS, compiled_default=shader_relocation["compiled_default"])
    report["packaging"] = dict(source_capability_sha256=qualified["source_capability_sha256"],
        source_runtime_sha256=qualified["report"]["runtime"]["sha256"],
        strip_debug_only=True, relocation=shader_relocation,
        scope="Preserve the qualified frontend scope and explicit gameplay/parity limits")
    encoded = (json.dumps(report, indent=2) + "\n").encode()
    if len(encoded) > 65536:
        raise ValueError("Packaged frontend capability exceeds its bounded owner")
    (stage / (PACKAGE_RUNTIME + ".frontend-test.json")).write_bytes(encoded)
    return dict(path=PACKAGE_RUNTIME + ".frontend-test.json", sha256=hashlib.sha256(encoded).hexdigest(),
                source_capability_sha256=qualified["source_capability_sha256"])


def dependencies(path):
    result = subprocess.run(["ldd", str(path)], capture_output=True, text=True, check=True)
    if "not found" in result.stdout:
        raise RuntimeError("Unresolved runtime dependency: " + result.stdout)
    return [Path(match) for match in re.findall(r"(?:=>\s+|^\s*)(/\S+)\s+\(", result.stdout, re.M)]


def merge_runtime_library(libraries, source):
    """One basename must retain one exact owner across all packaged programs."""
    previous = libraries.get(source.name)
    if previous is not None and (previous.stat().st_size != source.stat().st_size or sha(previous) != sha(source)):
        raise ValueError("Conflicting portable shared library owners: " + source.name)
    libraries.setdefault(source.name, source)


def verify_bundled_movies(root):
    """Authenticate the two approved movie sources required by authored runtimes."""
    from bundle_movies import read_manifest, verify_file
    from mcc_movies import ROLES, SOURCE_MANIFEST, _owned, source_manifest
    root = resource_root(root)
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
    """Explicit reference helper for two supplied roles and paired metadata."""
    from bundle_movies import bundle as bundle_movies, read_manifest
    from mcc_movies import SOURCE_MANIFEST, source_manifest
    root, stage = resource_root(root), Path(stage)
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
    """Authenticate every authored master and runtime bitmap for that family."""
    from mcc_prompts import verify_source as verify_prompt_source
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
    root, stage = resource_root(root), Path(stage)
    if verify_bundled_prompts(root) != verified:
        raise ValueError("Authored prompt source changed after package preflight")
    for name in sorted(verified["files"]):
        copy(root / name, stage / name)
    if verify_bundled_prompts(stage) != verified:
        raise ValueError("Packaged authored prompt source differs from its verified originals")
    return verified


def copy_bundled_loading_ambience(root, stage):
    """Ship the approved original tone; authenticate source and staged bytes."""
    root = resource_root(root)
    _, owner = bundled_ambience(root)
    for name in ("manifest.json", TONE):
        copy(Path(root) / AMBIENCE / name, Path(stage) / AMBIENCE / name)
    if bundled_ambience(stage)[1] != owner:
        raise ValueError("Packaged original loading ambience differs from its source")
    return owner


def presentation_sources(root, requirements):
    """Preflight only the finite source family required by the exact runtime."""
    if requirements['family'] == mcc_presentation.AUTHORED:
        return dict(bundled_movies=verify_bundled_movies(root),
                    authored_prompts=verify_bundled_prompts(root),
                    loading_ambience=bundled_ambience(root)[1])
    return dict(mcc_loading_converter=converter_dependencies(root))


def copy_presentation_sources(root, stage, sources):
    """Copy approved authored sources without converting or relabeling them."""
    return dict(bundled_movies=copy_bundled_movies(root, stage, sources['bundled_movies']),
                authored_prompts=copy_bundled_prompts(root, stage, sources['authored_prompts']),
                loading_ambience=copy_bundled_loading_ambience(root, stage))


def write_archive(stage, pending, movies=None):
    # Authored-family packages preserve the two explicitly assigned payloads.
    stored = set(movies["roles"].values()) if movies is not None else set()
    with zipfile.ZipFile(pending, "x", compression=zipfile.ZIP_DEFLATED, compresslevel=6,
                         allowZip64=True) as archive:
        for path in sorted(stage.rglob("*")):
            if path.is_file():
                relative = path.relative_to(stage).as_posix()
                # The exact assigned MP4s are already compressed. Store their
                # original bytes; retain the normal compression for all else.
                archive.write(path, stage.name + "/" + relative,
                              compress_type=zipfile.ZIP_STORED if relative in stored
                              else zipfile.ZIP_DEFLATED)


def _assemble(root, runtime, launchers, output, shader_directory, shaders, stage, pending,
              *, frontend=None, presentation=None):
    presentation = presentation if presentation is not None else dict(mcc_loading_converter=converter_dependencies(root))
    decoder_source = presentation.get("mcc_loading_converter")
    stage.mkdir(parents=True)
    custom_assets = install_custom_assets(root, stage)
    variants = custom_assets['groups']['native_variants']
    (stage / "assets/imported").mkdir(parents=True)
    (stage / "bin").mkdir()
    executable_sources = [runtime, launchers / "Halo2-Launcher", launchers / "Halo2-Title-Launcher"]
    copied_elf = []
    for source in executable_sources:
        destination = stage / "bin" / (source.name + ".real")
        copy(source, destination)
        if source == runtime and frontend and sha(destination) != frontend["report"]["runtime"]["sha256"]:
            raise ValueError("Qualified frontend binary changed before packaging")
        subprocess.run(["strip", "--strip-debug", str(destination)], check=True)
        copied_elf.append(destination)
    if decoder_source is not None:
        # Setup decodes the user's genuine MCC media; no original loading tone or
        # predecoded game media is shipped. Own the decoder's portable dependency
        # closure with the same loader used by the packaged runtime.
        source_decoder = root / (CONVERTER_PAYLOAD if "runtime" in decoder_source["converter"]
                                 else decoder_source["converter"]["path"])
        decoder_payload = stage / CONVERTER_PAYLOAD
        copy(source_decoder, decoder_payload)
        subprocess.run(["strip", "--strip-debug", str(decoder_payload)], check=True)
        copied_elf.append(decoder_payload)
        copy(root / CODEBOOK, stage / CODEBOOK)
    runtime_data = copy_runtime_adjacent_data(runtime, stage, frontend)
    # The existing renderer embeds its build-time directory as a fixed-length
    # path constant. Relocate that data constant without changing instructions,
    # field widths or any shader bytes. Redundant ./ segments retain its length.
    packaged_runtime = stage / "bin/halo2_native.real"
    payload = packaged_runtime.read_bytes()
    payload, shader_relocation = relocate_compiled_shader_default(payload, shader_directory)
    packaged_runtime.write_bytes(payload)
    shutil.copytree(shader_directory, stage / "bin/shaders")
    if (runtime.parent / "upscalers").is_dir():
        shutil.copytree(runtime.parent / "upscalers", stage / "bin/upscalers")
    if shader_inventory(stage / "bin/shaders", executable=packaged_runtime) != shaders:
        raise RuntimeError("Packaged shader bytes differ from the existing bank")
    for source in (root / "tools").glob("*.py"):
        if not source.is_symlink():
            copy(source, stage / "tools" / source.name)
    if decoder_source is not None:
        # Offline V3 receipts can retain their original macOS decoder provenance.
        for name in ("__init__.py", "loading_converter.py"):
            copy(root / "tools/macos" / name, stage / "tools/macos" / name)
    for source in (root / "tools/reference").glob("*.tsv"):
        if not source.is_symlink():
            copy(source, stage / "tools/reference" / source.name)
    shutil.copytree(root / "tools/data", stage / "tools/data", dirs_exist_ok=True,
                    ignore=shutil.ignore_patterns("__pycache__", ".git"))
    verify_ui_layout(root)
    copy(root / UI_LAYOUT_SOURCE, stage / UI_LAYOUT_SOURCE)
    copy(root / "resources/launcher.svg", stage / "resources/launcher.svg")
    shutil.copytree(root / "resources/licenses", stage / "resources/licenses")
    for name in ("COPYING", "PROVENANCE.json", "README.md"):
        source = root / "external/ww2ogg" / name
        if source.is_file():
            copy(source, stage / "external/ww2ogg" / name)
    for name in ("LICENSE", "LICENSE.md", "LICENSE.txt", "COPYING"):
        if (root / name).is_file():
            copy(root / name, stage / name)
    packaged_presentation = (copy_presentation_sources(root, stage, presentation)
                             if decoder_source is None else {})
    packaged_presentation['controller_prompts'] = custom_assets['groups']['controller_prompts']
    # Ship the interpreter, Tk and Pillow used by onboarding. No compiler,
    # development checkout, local cache or credentials. Loading media is
    # imported only from the user's MCC installation during setup.
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
            merge_runtime_library(library_sources, source)
    loader = Path("/lib64/ld-linux-x86-64.so.2")
    merge_runtime_library(library_sources, loader)
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
    if decoder_source is not None:
        (stage / CONVERTER_WRAPPER).write_bytes(converter_wrapper())
        (stage / CONVERTER_WRAPPER).chmod(0o755)
    wrapper(stage / "python/bin/python3",
            'root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)\n'
            'export PYTHONHOME="$root/python"\n'
            'export TCL_LIBRARY="$root/python/tcl8.6" TK_LIBRARY="$root/python/tk8.6"\n'
            'exec "$root/lib/ld-linux-x86-64.so.2" --library-path "$root/lib" "$root/python/bin/python3.real" "$@"\n')
    if decoder_source is not None:
        decoder_files = {CONVERTER_WRAPPER, CONVERTER_PAYLOAD, CONVERTER_LOADER, CODEBOOK}
        decoder_files.update("lib/" + name for name in library_sources)
        decoder_manifest = dict(schema=CONVERTER_SCHEMA,
            source_converter=decoder_source["converter"],
            files={name: dict(bytes=(stage / name).stat().st_size, sha256=sha(stage / name))
                   for name in sorted(decoder_files)})
        (stage / CONVERTER_MANIFEST).write_text(json.dumps(decoder_manifest, indent=2) + "\n")
        decoder = converter_dependencies(stage)
        if converter_dependencies(root) != decoder_source:
            raise ValueError("MCC loading converter source changed during packaging")
        packaged_presentation['mcc_loading_converter'] = decoder
    frontend_owner = packaged_frontend(stage, frontend, shader_relocation) if frontend else None
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
        **packaged_presentation,
        "bundled_native_variants": variants,
        "bundled_custom_assets": custom_assets,
        "runtime_adjacent_data": runtime_data,
    }
    if frontend_owner:
        manifest.update(mode="MCC title and menus; qualified native offline routes",
                        frontend_capability=frontend_owner)
    subprocess.run([str(stage / "bin/halo2_native"), "--mcc-preview", "--help"],
                   check=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, timeout=30)
    (stage / "PRECOMPILED_TEST_BUILD.json").write_text(json.dumps(manifest, indent=2) + "\n")
    presentation_text = (
        "The approved port-supplied intro and credits are bundled and copied without transcoding.\n"
        "Attract remains unassigned. Authored SVG/bitmap button prompts retain their existing artwork.\n"
        "Setup reconstructs the V2 loading mask from MCC and uses the approved bundled original tone.\n"
        "The retained four-voice loading animation/audio policy is unchanged.\n"
        if decoder_source is None else
        "Intro, credits and attract remain unavailable until authentic MCC movie roles are qualified.\n"
        "No separate supplied intro/credits movie bank is included or required by setup.\n"
        "Controller artwork files are included.\n"
        "Menus use imported MCC fonts.\n"
        "Setup imports the genuine MCC loading track into presentation/loading-v3 using the bundled decoder/codebook.\n"
        "Loading plays the full decoded track once, then silence; authored MCC loop/transition timing remains unqualified.\n")
    (stage / "README.txt").write_text(
        "Halo 2 MCC content test build — Linux x86-64\n\n"
        "Extract the entire ZIP into a writable folder. Run ./Halo2-Launcher.\n"
        "The game, shaders, Python, Tk and native libraries are already compiled/bundled.\n"
        "Keep all folders beside the launcher. A Linux desktop and working Vulkan driver are required.\n\n"
        "First launch detects your installed Halo MCC and copies Halo 2 content to assets/imported.\n"
        "If detection fails, select your MCC installation or its halo2 folder.\n"
        "Allow space for the Halo 2 data plus decoded resources. No Xbox ISO, TU patch or Vista bank is needed.\n"
        "Custom button maps, UI masters, celestial ranks, compiled custom maps and game modes are included.\n"
        + presentation_text
        + ("Play opens the qualified MCC title screen and menus after setup.\n"
           "Native offline routes retain the supplied validation's limits. Complete gameplay,\n"
           "all maps/modes and Xbox TU1.5 parity remain in progress.\n\n" if frontend else
           "Select a map, then Play. Play currently opens the MCC content preview.\n"
           "The preview lets testers inspect maps, textures, materials and rendering. Native gameplay\n"
           "and complete Xbox TU1.5 conversion remain in progress; this package does not claim parity.\n"
           "This contains the existing compiled runtime; newer source-only compatibility fixes are not included.\n\n") +
        "To capture a test: ./Halo2-Launcher --launch-game --map lockout.map --frames 3 --screenshot lockout.ppm\n"
        "Runtime output can be collected with: ./Halo2-Launcher --launch-game > test.log 2>&1\n"
        "To update this precompiled package, extract a newer test ZIP. No compilation is needed.\n")
    files = sorted(p for p in stage.rglob("*") if p.is_file())
    (stage / "SHA256SUMS").write_text("".join(f"{sha(p)}  {p.relative_to(stage).as_posix()}\n" for p in files))
    write_archive(stage, pending, presentation.get("bundled_movies"))
    return {"zip": str(output), "bytes": pending.stat().st_size, "sha256": sha(pending),
            "stage_retained": False,
            "zip64_enabled": True, "frontend_capability": frontend_owner,
            "mode": manifest["mode"] if frontend_owner else "MCC content preview; no new gameplay/frontend acceptance"}


def package(root, runtime, launchers, output, shader_directory=None, *, frontend_cache=None):
    root, runtime, launchers = Path(root).resolve(), Path(runtime).resolve(), Path(launchers).resolve()
    output = Path(output)
    if output.is_symlink() or output.exists():
        raise ValueError("Refusing to replace an existing ZIP")
    output = output.absolute()
    shader_directory = Path(shader_directory or runtime.parent / "shaders").resolve(strict=True)
    shaders = shader_inventory(shader_directory, executable=runtime)
    frontend = qualified_frontend(runtime, frontend_cache) if frontend_cache is not None else None
    if frontend:
        declaration = frontend['report']['shaders']
        from native_renderer_shaders import AOT_PROFILE, validate_native_package
        qualified_files = (validate_native_package(shader_directory, executable=runtime,
            expected_sha256=declaration['package']['sha256'])['files']
            if declaration.get('profile') == AOT_PROFILE else declaration['files'])
        if Path(declaration['directory']) != shader_directory or qualified_files != shaders:
            raise ValueError("Package shaders differ from the qualified frontend owner")
    requirements = mcc_presentation.requirements(root, runtime=runtime)
    if requirements['family'] == mcc_presentation.AUTHORED and frontend is None:
        # Without the qualified packaged sidecar, first setup would correctly
        # choose the unqualified V3 default, not this authored source family.
        raise ValueError('Authored frontend packaging requires its qualified --frontend-cache')
    presentation = presentation_sources(root, requirements)
    variants = variant_sources(root)
    custom_assets = custom_asset_inventory(root)
    output.parent.mkdir(parents=True, exist_ok=True)
    # Only the complete requested ZIP persists. The private stage is removed
    # on success, failure and cancellation.
    with _cancellation(), tempfile.TemporaryDirectory(prefix=".mcc-package-", dir=output.parent) as temporary:
        temporary = Path(temporary)
        stage, pending = temporary / output.stem, temporary / "complete.zip"
        arguments = (resource_root(root), runtime, launchers, output, shader_directory, shaders, stage, pending)
        result = _assemble(*arguments, frontend=frontend, presentation=presentation)
        if presentation_sources(root, requirements) != presentation:
            raise ValueError("Presentation sources changed during packaging")
        if variant_sources(root) != variants:
            raise ValueError("Bundled multiplayer definitions changed during packaging")
        if custom_asset_inventory(root) != custom_assets:
            raise ValueError("Bundled custom assets changed during packaging")
        mcc_presentation.recheck(requirements)
        if frontend:
            final = qualified_frontend(runtime, frontend_cache)
            if final != frontend:
                raise ValueError("Qualified frontend inputs changed during packaging")
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
    parser.add_argument("--frontend-cache", type=Path,
                        help="private MCC cache used by the adjacent passed raw-runtime frontend capability")
    args = parser.parse_args()
    print(json.dumps(package(root, args.runtime, args.launchers, args.output, args.shader_directory,
                             frontend_cache=args.frontend_cache)))


if __name__ == "__main__":
    main()
