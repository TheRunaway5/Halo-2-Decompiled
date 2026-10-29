"""Read-only admission of an explicitly validated interim MCC gameplay build.

This capability is independent of import/conversion parity. Its sidecar is
published by a release validation step, never by onboarding or this reader.
Callers first validate the selected private cache through their existing source
selection logic. A sidecar authorizes only the exact executable, compiled
default shader directory and Lockout map named by a passed test receipt.
Absolute bindings intentionally expire on relocation. A separate packaged
binding authenticates the canonical project-CWD wrapper and its actual ELF
payload/default literal, so that complete validated package can be relocated.
"""
import argparse
import hashlib
import json
import mmap
import os
from pathlib import Path, PurePosixPath
import re
import sys

# The launcher may inherit PYTHONSAFEPATH or use -P; sibling helpers must still
# resolve from this installed tool directory rather than the caller's CWD.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from file_identity import descriptor_identity, path_identity

SCHEMA = "H2_NATIVE_GAMEPLAY_TEST_V1"
CAPABILITY = "mcc-lockout-source-object-world-test"
SCOPE = "mcc-lockout-native-gameplay-test"
MAP = "halo2/h2_maps_win64_dx11/lockout.map"
MESSAGE = ("Native Lockout gameplay test available. Xbox TU1.5 conversion "
           "and gameplay, visual and audio parity remain incomplete.")
FRONTEND_SCHEMA = "H2_NATIVE_FRONTEND_TEST_V1"
FRONTEND_CAPABILITY = "mcc-native-frontend"
FRONTEND_SCOPE = "mcc-native-title-menu"
FRONTEND_MAP = "halo2/h2_maps_win64_dx11/mainmenu.map"
FRONTEND_LOADING_PROFILE = "mcc-logo-glow-mcc-track-oneshot-v1"
FRONTEND_H2EK_LOADING_PROFILE = "h2ek-logo-sweep-four-voice-v2"
FRONTEND_H2EK_LOADING_FILES = {
    "presentation/original-loading-v2/" + name for name in
    ("manifest.json", "ambience.xbox-adpcm", "loading-00.dds", "loading-01.dds",
     "loading-03.dds", "loading-04.dds", "loading-05.dds", "loading-11.dds")}
FRONTEND_PROMPT_FONT_PROFILE = "mcc-native-control-fonts-v1"


def _digest(value):
    if not isinstance(value, str) or not re.fullmatch(r"[0-9a-f]{64}", value):
        raise ValueError("Invalid native-test SHA256")
    return value


class NativeTestOwnerChanged(ValueError):
    """A strict digest lost its exact file identity while reading it."""

    def __init__(self, path, before, after_fd, after_path):
        super().__init__("Native-test owner changed during validation")
        self.path = Path(path)
        self.before = before
        self.after_fd = after_fd
        self.after_path = after_path


def _file_digest(path, *, embedded=None, exact=None, readonly_literal=False):
    if path.is_symlink() or not path.is_file():
        raise ValueError("Native-test file has no direct owner")
    with path.open("rb") as stream:
        before = descriptor_identity(stream.fileno())
        digest = hashlib.file_digest(stream, "sha256").hexdigest()
        if exact is not None:
            stream.seek(0)
            if before[2] != len(exact) or stream.read() != exact:
                raise ValueError("Native-test executable wrapper differs")
        if embedded is not None:
            if not before[2]:
                raise ValueError("Empty native-test executable")
            with mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ) as data:
                if data.find(embedded) < 0:
                    raise ValueError("Compiled default shader path was not retained")
                if readonly_literal:
                    from packaged_shader_defaults import readonly_literal_location
                    readonly_literal_location(data, embedded)
        after_fd, after_path = descriptor_identity(stream.fileno()), path_identity(path)
        if before != after_fd or before != after_path:
            raise NativeTestOwnerChanged(path, before, after_fd, after_path)
    return digest


def eos_worker_descriptor(runtime):
    """Seal the dedicated SDK host shipped beside a newly built runtime."""
    runtime = Path(runtime)
    name = 'eos_sdk_worker.exe' if runtime.suffix.lower() == '.exe' else 'eos_sdk_worker'
    worker = runtime.with_name(name)
    if not os.access(worker, os.X_OK):
        raise ValueError('EOS SDK worker is missing or is not executable')
    return dict(schema='H2_EOS_SDK_WORKER_V1', name=name,
                sha256=_file_digest(worker), bytes=worker.stat().st_size)


def validate_eos_worker(runtime, declaration):
    """Legacy capabilities omit this declaration; new ones must retain it."""
    if (not isinstance(declaration, dict) or
            set(declaration) != {'schema', 'name', 'sha256', 'bytes'} or
            declaration.get('schema') != 'H2_EOS_SDK_WORKER_V1' or
            type(declaration.get('bytes')) is not int or declaration['bytes'] <= 0):
        raise ValueError('Invalid EOS SDK worker declaration')
    _digest(declaration['sha256'])
    if eos_worker_descriptor(runtime) != declaration:
        raise ValueError('EOS SDK worker changed after qualification')


def _runtime_owner(root, runtime, *, frontend=False):
    """Authenticate executable/capability/shaders without granting cache admission."""
    root, runtime = Path(root).resolve(), Path(runtime)
    if not runtime.is_absolute() or runtime.is_symlink() or not os.access(runtime, os.X_OK):
        raise ValueError("Native-test executable requires its validated absolute owner")
    runtime = runtime.resolve(strict=True)
    sidecar = runtime.with_name(runtime.name + (".frontend-test.json" if frontend else ".native-test.json"))
    if sidecar.is_symlink() or not sidecar.is_file() or sidecar.stat().st_size > 65536:
        raise ValueError("No bounded native-test validation sidecar")
    capability_sha = _file_digest(sidecar)
    report = json.loads(sidecar.read_text())
    if (report.get("schema") != (FRONTEND_SCHEMA if frontend else SCHEMA) or
            report.get("capability") != (FRONTEND_CAPABILITY if frontend else CAPABILITY) or
            report["validation"].get("passed") is not True or
            report["validation"].get("scope") != (FRONTEND_SCOPE if frontend else SCOPE)):
        raise ValueError("No passed validation for this native capability")
    _digest(report["validation"]["report_sha256"])
    owner = report["runtime"]
    shaders = report["shaders"]
    if shaders.get("binding") == "compiled-default":
        if owner.get("path") != str(runtime):
            raise ValueError("Native-test executable was relocated")
        directory = Path(shaders["directory"])
        if not directory.is_absolute():
            raise ValueError("Native test requires its absolute compiled default shader owner")
        payload, compiled, readonly = runtime, os.fsencode(directory), False
    elif shaders.get("binding") == "packaged-compiled-default":
        from packaged_shader_defaults import (PACKAGE_LOADER, PACKAGE_PAYLOAD, PACKAGE_RUNTIME,
                                               PACKAGE_SHADERS, native_runtime_wrapper,
                                               relative_compiled_default)
        if (owner.get("path") != PACKAGE_RUNTIME or runtime != root / PACKAGE_RUNTIME or
                shaders.get("directory") != PACKAGE_SHADERS or
                owner["payload"].get("path") != PACKAGE_PAYLOAD or
                owner["loader"].get("path") != PACKAGE_LOADER):
            raise ValueError("Native package requires its fixed owned wrapper/payload/default layout")
        if _file_digest(runtime, exact=native_runtime_wrapper()) != _digest(owner["sha256"]):
            raise ValueError("Native package wrapper changed")
        loader = root / PACKAGE_LOADER
        if (loader.resolve(strict=True) != loader or not os.access(loader, os.X_OK) or
                _file_digest(loader) != _digest(owner["loader"]["sha256"])):
            raise ValueError("Native package loader changed")
        directory, payload = root / PACKAGE_SHADERS, root / PACKAGE_PAYLOAD
        if payload.resolve(strict=True) != payload or not os.access(payload, os.X_OK):
            raise ValueError("Native package payload has no direct executable owner")
        compiled = os.fsencode(relative_compiled_default(shaders["compiled_default"]))
        readonly = True
    elif shaders.get("binding") == "release-slot-compiled-default":
        from update_runtime import SLOT_WRAPPER, selected_runtime
        from packaged_shader_defaults import relative_compiled_default
        if selected_runtime(root) != runtime or owner.get("path") != "halo2_native":
            raise ValueError("Release capability requires the verified selected slot")
        if (owner["payload"].get("path") != "bin/halo2_native.real" or
                owner["loader"].get("path") != "lib/ld-linux-x86-64.so.2" or
                shaders.get("directory") != "bin/shaders"):
            raise ValueError("Release capability layout changed")
        if _file_digest(runtime, exact=SLOT_WRAPPER) != _digest(owner["sha256"]):
            raise ValueError("Release runtime wrapper changed")
        slot = runtime.parent
        loader, payload = slot / owner["loader"]["path"], slot / owner["payload"]["path"]
        if (loader.resolve(strict=True) != loader or not os.access(loader, os.X_OK) or
                _file_digest(loader) != _digest(owner["loader"]["sha256"]) or
                payload.resolve(strict=True) != payload or not os.access(payload, os.X_OK)):
            raise ValueError("Release runtime loader or payload changed")
        directory = slot / "bin/shaders"
        compiled = os.fsencode(relative_compiled_default(shaders["compiled_default"]))
        readonly = True
    else:
        raise ValueError("Unsupported native-test shader binding")
    if directory.is_symlink() or directory.resolve(strict=True) != directory:
        raise ValueError("Native test requires the validated compiled default shader owner")
    # The original qualified family has 97 files. New renderer programs may
    # expand it; every added file still needs the passed inventory's exact hash,
    # and the complete directory must match that inventory below.
    from native_renderer_shaders import (PROFILE, REQUIRED_FILES, AOT_PROFILE,
                                         AOT_PACKAGE_NAME, validate_native_package)
    profile = shaders.get("profile")
    package = directory/AOT_PACKAGE_NAME
    if profile == AOT_PROFILE or package.exists() or package.is_symlink():
        declaration = shaders.get('package')
        if (profile != AOT_PROFILE or 'files' in shaders or
                not isinstance(declaration, dict) or set(declaration) != {'path', 'sha256'} or
                declaration.get('path') != AOT_PACKAGE_NAME):
            raise ValueError('Native AOT shader capability lacks its exact package declaration')
        files = validate_native_package(directory, expected_sha256=_digest(declaration['sha256']),
                                        executable=payload)['files']
        complete = True
    elif profile is None:
        files = shaders["files"]
        complete = (isinstance(files, dict) and len(files) >= 97 and
                    {"world.vert.spv", "world.frag.spv", "hud.frag.spv"} <= files.keys())
    else:
        files = shaders["files"]
        complete = (profile == PROFILE and isinstance(files, dict) and REQUIRED_FILES <= files.keys())
    if not complete:
        raise ValueError("Native test requires its complete declared shader family")
    actual = {p.relative_to(directory).as_posix() for p in directory.rglob("*") if p.is_file() or p.is_symlink()}
    if actual != set(files):
        raise ValueError("Native-test shader inventory changed")
    for name, expected in files.items():
        relative = PurePosixPath(name)
        if (relative.is_absolute() or any(p in ("", ".", "..") for p in relative.parts) or
                "\\" in name or str(relative) != name):
            raise ValueError("Invalid native-test shader path")
        path = directory / relative
        if not path.resolve(strict=True).is_relative_to(directory) or _file_digest(path) != _digest(expected):
            raise ValueError("Native-test shader bytes changed")
    # An exact executable hash carries the validation authority; this additional
    # path check prevents accidentally treating adjacent shaders as its actual
    # compiled default. No command-line override bypasses the runtime's guard.
    expected = owner["payload"]["sha256"] if readonly else owner["sha256"]
    if _file_digest(payload, embedded=compiled, readonly_literal=readonly) != _digest(expected):
        raise ValueError("Native-test executable changed")
    if 'eos_worker' in report:
        validate_eos_worker(payload, report['eos_worker'])
    if _file_digest(sidecar) != capability_sha:
        raise ValueError("Native-test capability changed during validation")
    return dict(runtime=runtime, report=report, capability_sha256=capability_sha,
                runtime_sha256=_digest(owner["sha256"]))


def _frontend_contract(report):
    """One finite resource contract shared by admission and import dispatch."""
    validation = report["validation"]
    native_fonts = ("mcc_loading_presentation_profile" in validation or
                    "mcc_prompt_font_profile" in validation)
    h2ek = validation.get("mcc_loading_presentation_profile") == FRONTEND_H2EK_LOADING_PROFILE
    if native_fonts:
        if h2ek:
            if validation.get("original_loading_ambience") is not True:
                raise ValueError("H2EK loading requires its separately passed original ambience profile")
        elif (validation.get("mcc_loading_presentation_profile") != FRONTEND_LOADING_PROFILE or
              validation.get("original_loading_ambience") is True):
            raise ValueError("Frontend requires a separately passed MCC V3 loading profile; legacy proofs cannot be upgraded")
        if validation.get("mcc_prompt_font_profile") != FRONTEND_PROMPT_FONT_PROFILE:
            raise ValueError("Frontend requires a separately passed native MCC prompt font profile; legacy proofs cannot be upgraded")
    loading_files = {"presentation/loading-v3/manifest.json", "presentation/loading-v3/glow-mask.dds",
                     "presentation/loading-v3/loading.wav"}
    if h2ek:
        loading_files = set(FRONTEND_H2EK_LOADING_FILES)
    allowed = set(loading_files) if native_fonts else {
        "presentation/loading/manifest.json", "presentation/loading/glow-mask.dds",
        "presentation/loading/loading.wav", "presentation/loading-v2/manifest.json",
        "presentation/loading-v2/glow-mask.dds", "presentation/loading-v2/loading-tone.xbox-adpcm"}
    return dict(native_fonts=native_fonts, loading_files=loading_files,
                allowed_presentation=allowed)


def _check(root, runtime, cache, *, frontend=False):
    root, cache = Path(root).resolve(), Path(cache)
    if cache.is_symlink():
        raise ValueError("Native-test cache cannot be a symlink")
    cache = cache.resolve(strict=True)
    if cache.parent != root / "assets/imported" or cache.name.startswith("."):
        raise ValueError("Native-test map requires the selected private cache")
    report = _runtime_owner(root, runtime, frontend=frontend)["report"]
    bitmap = report["map"]
    expected_map = FRONTEND_MAP if frontend else MAP
    if bitmap.get("path") != expected_map:
        raise ValueError("This validation covers a different MCC map")
    path = cache / expected_map
    if not path.resolve(strict=True).is_relative_to(cache) or _file_digest(path) != _digest(bitmap["sha256"]):
        raise ValueError("Native-test Lockout map changed")
    if frontend:
        contract = _frontend_contract(report)
        if not contract["native_fonts"]:
            raise ValueError("Update to an MCC-capable runtime with separately passed MCC V3 loading "
                             "and native MCC prompt font profiles; the legacy authored presentation "
                             "contract is reference-only")
        resources = report["resources"]
        loading_files = contract["loading_files"]
        allowed_presentation = contract["allowed_presentation"]
        required = {"halo2/h2_maps_win64_dx11/shared.map",
                    "halo2/h2_maps_win64_dx11/textures.dat",
                    "halo2/h2_maps_win64_dx11/sounds_neutral.dat",
                    "halo2/h2_fonts/font_table.txt"}
        required.update(loading_files)
        if (cache / 'frontend-map-bindings.json').exists() or (cache / 'frontend-map-bindings.json').is_symlink():
            required.add('frontend-map-bindings.json')
        # Every imported font is part of frontend admission, including languages
        # beyond the active selector. A receipt cannot silently omit a font.
        fonts = cache / "halo2/h2_fonts"
        required.update(p.relative_to(cache).as_posix() for p in fonts.rglob("*")
                        if p.is_file() or p.is_symlink())
        ui = cache / "ui"
        if (ui / "pack.tsv").is_file():
            required.update(p.relative_to(cache).as_posix() for p in ui.rglob("*")
                            if p.is_file() or p.is_symlink())
        if not isinstance(resources, dict) or not required <= resources.keys() or len(resources) > 1024:
            raise ValueError("Frontend validation omits MCC resources")
        for name, expected in resources.items():
            relative = PurePosixPath(name)
            if (relative.is_absolute() or str(relative) != name or "\\" in name or
                    not relative.parts or
                    (relative.parts[0] not in ("halo2", "ui") and
                     name not in allowed_presentation and
                     name not in ("presentation/ui/native-layout.json", "frontend-map-bindings.json")) or
                    any(part in (".", "..") for part in relative.parts)):
                raise ValueError("Frontend resource escaped its MCC cache")
            path = cache / name
            if not path.resolve(strict=True).is_relative_to(cache) or _file_digest(path) != _digest(expected):
                raise ValueError("Frontend MCC resource changed")


def native_gameplay_test_available(root, runtime, cache):
    """Fail closed without changing cache flags, files, or runtime selection."""
    if runtime is None or cache is None:
        return False
    try:
        _check(root, runtime, cache)
        return True
    except (OSError, ValueError, KeyError, TypeError, AttributeError, RuntimeError):
        return False


def frontend_runtime_available(root, runtime, cache):
    """Admit only the separately validated MCC title/menu capability."""
    if runtime is None or cache is None:
        return False
    try:
        _check(root, runtime, cache, frontend=True)
        return True
    except (OSError, ValueError, KeyError, TypeError, AttributeError, RuntimeError):
        return False


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--runtime", type=Path, required=True)
    parser.add_argument("--cache", type=Path, required=True)
    parser.add_argument("--frontend", action="store_true")
    parser.add_argument("--selected-frontend", action="store_true",
                        help="Check this exact owner against the optional independent frontend selection")
    args = parser.parse_args()
    if args.selected_frontend:
        if not args.frontend:
            parser.error("--selected-frontend requires --frontend")
        from frontend_runtime_selection import selected_frontend_runtime
        selected = selected_frontend_runtime(args.root, args.runtime, args.cache)
        return 0 if selected is not None and selected == args.runtime.resolve() else 1
    available = frontend_runtime_available if args.frontend else native_gameplay_test_available
    return 0 if available(args.root, args.runtime, args.cache) else 1


if __name__ == "__main__":
    raise SystemExit(main())
