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

SCHEMA = "H2_NATIVE_GAMEPLAY_TEST_V1"
CAPABILITY = "mcc-lockout-source-object-world-test"
SCOPE = "mcc-lockout-native-gameplay-test"
MAP = "halo2/h2_maps_win64_dx11/lockout.map"
MESSAGE = ("Native Lockout gameplay test available. Xbox TU1.5 conversion "
           "and gameplay, visual and audio parity remain incomplete.")


def _digest(value):
    if not isinstance(value, str) or not re.fullmatch(r"[0-9a-f]{64}", value):
        raise ValueError("Invalid native-test SHA256")
    return value


def _identity(value):
    return (value.st_dev, value.st_ino, value.st_size,
            value.st_mtime_ns, value.st_ctime_ns)


def _file_digest(path, *, embedded=None, exact=None, readonly_literal=False):
    if path.is_symlink() or not path.is_file():
        raise ValueError("Native-test file has no direct owner")
    with path.open("rb") as stream:
        before = os.fstat(stream.fileno())
        digest = hashlib.file_digest(stream, "sha256").hexdigest()
        if exact is not None:
            stream.seek(0)
            if before.st_size != len(exact) or stream.read() != exact:
                raise ValueError("Native-test executable wrapper differs")
        if embedded is not None:
            if not before.st_size:
                raise ValueError("Empty native-test executable")
            with mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ) as data:
                if data.find(embedded) < 0:
                    raise ValueError("Compiled default shader path was not retained")
                if readonly_literal:
                    from packaged_shader_defaults import readonly_literal_location
                    readonly_literal_location(data, embedded)
        if (_identity(before) != _identity(os.fstat(stream.fileno())) or
                _identity(before) != _identity(path.stat())):
            raise ValueError("Native-test owner changed during validation")
    return digest


def _check(root, runtime, cache):
    root, runtime, cache = Path(root).resolve(), Path(runtime), Path(cache)
    if not runtime.is_absolute() or runtime.is_symlink() or not os.access(runtime, os.X_OK):
        raise ValueError("Native-test executable requires its validated absolute owner")
    runtime = runtime.resolve(strict=True)
    if cache.is_symlink():
        raise ValueError("Native-test cache cannot be a symlink")
    cache = cache.resolve(strict=True)
    if cache.parent != root / "assets/imported" or cache.name.startswith("."):
        raise ValueError("Native-test map requires the selected private cache")
    sidecar = runtime.with_name(runtime.name + ".native-test.json")
    if sidecar.is_symlink() or not sidecar.is_file() or sidecar.stat().st_size > 65536:
        raise ValueError("No bounded native-test validation sidecar")
    report = json.loads(sidecar.read_text())
    if (report.get("schema") != SCHEMA or report.get("capability") != CAPABILITY or
            report["validation"].get("passed") is not True or
            report["validation"].get("scope") != SCOPE):
        raise ValueError("No passed native Lockout test validation")
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
    else:
        raise ValueError("Unsupported native-test shader binding")
    if directory.is_symlink() or directory.resolve(strict=True) != directory:
        raise ValueError("Native test requires the validated compiled default shader owner")
    files = shaders["files"]
    # The original qualified family has 97 files. New renderer programs may
    # expand it; every added file still needs the passed inventory's exact hash,
    # and the complete directory must match that inventory below.
    if (not isinstance(files, dict) or len(files) < 97 or
            not {"world.vert.spv", "world.frag.spv", "hud.frag.spv"} <= files.keys()):
        raise ValueError("Native test requires the complete baseline shader inventory")
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
    bitmap = report["map"]
    if bitmap.get("path") != MAP:
        raise ValueError("This validation covers only MCC Lockout")
    path = cache / MAP
    if not path.resolve(strict=True).is_relative_to(cache) or _file_digest(path) != _digest(bitmap["sha256"]):
        raise ValueError("Native-test Lockout map changed")


def native_gameplay_test_available(root, runtime, cache):
    """Fail closed without changing cache flags, files, or runtime selection."""
    if runtime is None or cache is None:
        return False
    try:
        _check(root, runtime, cache)
        return True
    except (OSError, ValueError, KeyError, TypeError, AttributeError):
        return False


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--runtime", type=Path, required=True)
    parser.add_argument("--cache", type=Path, required=True)
    args = parser.parse_args()
    return 0 if native_gameplay_test_available(args.root, args.runtime, args.cache) else 1


if __name__ == "__main__":
    raise SystemExit(main())
