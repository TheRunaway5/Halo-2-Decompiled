"""Import the port's authored SVG prompt rasters without replacing MCC fonts.

The portable source carries the original masters and provenance. Only the
seven runtime files, manifest and relative binding enter a private MCC cache.
Presentation repairs share the movie lock and cancellation/publication helpers.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import uuid

from audit_rendering import FormatError
from mcc_movies import _cancellation, _lock, _owned, _publish_new

SCHEMA = "H2_PORT_AUTHORED_PROMPTS_V1"
SOURCE_SCHEMA = "vista-prompt-svg-v2"
SOURCE = "resources/presentation/prompts"
BANK = "presentation/prompts"
MANIFEST = BANK + "/manifest.json"
BINDING = b"H2_VISTA_PROMPTS_V1\n.\n"
SELECTORS = ("fixedsys-9", "conduit-12", "handel_gothic-11", "handel_gothic-24",
             "conduit-13", "conduit-9", "conduit-13", "conduit-12", "MSLCD-14",
             "conduit-13", "handel_gothic-13", "conduit-9")
RUNTIME_FILES = {"prompts.tsv", *(f"atlases/{i}.dds" for i in range(6))}
MAX_METADATA = 64 * 1024


def _json(path, root):
    path = _owned(path, root)
    if not path.is_file() or not 0 < path.stat().st_size <= MAX_METADATA:
        raise FormatError("Authored prompt manifest is missing or too large")
    raw = path.read_bytes()
    try:
        def unique(pairs):
            value = {}
            for name, item in pairs:
                if name in value:
                    raise ValueError("Duplicate metadata key")
                value[name] = item
            return value
        metadata = json.loads(raw, object_pairs_hook=unique)
    except (ValueError, UnicodeError) as error:
        raise FormatError("Invalid authored prompt JSON") from error
    return raw, metadata


def _records(value, *, artwork=False):
    if not isinstance(value, dict):
        raise FormatError("Invalid authored prompt file inventory")
    if artwork:
        if (len(value) != 83 or "reconstruction.json" not in value or
                any(name != "reconstruction.json" and
                    re.fullmatch(r"vectors/[0-9a-f]{64}\.svg", name) is None for name in value)):
            raise FormatError("Authored prompt masters must be the exact 82 vectors and provenance")
    elif set(value) != RUNTIME_FILES:
        raise FormatError("Authored prompts require exactly the seven runtime files")
    for row in value.values():
        if (not isinstance(row, dict) or set(row) != {"bytes", "sha256"} or
                type(row.get("bytes")) is not int or not 0 < row["bytes"] <= 32 * 1024**2 or
                not isinstance(row.get("sha256"), str) or
                re.fullmatch(r"[0-9a-f]{64}", row["sha256"]) is None):
            raise FormatError("Invalid authored prompt size or SHA-256")


def source_manifest(root):
    """Read the bounded portable contract; payload verification is separate."""
    from installation_paths import resource_root
    root = resource_root(root)
    raw, value = _json(root / SOURCE / "manifest.json", root)
    if (not isinstance(value, dict) or value.get("schema") != SCHEMA or
            value.get("source_schema") != SOURCE_SCHEMA or type(value.get("density")) is not int or
            value.get("density") != 4 or
            value.get("selectors") != list(SELECTORS) or
            value.get("external_vista_or_xbox_required") is not False):
        raise FormatError("Unsupported authored prompt source manifest")
    _records(value.get("runtime_files"))
    _records(value.get("artwork_files"), artwork=True)
    provenance = value.get("source_provenance")
    if (not isinstance(provenance, dict) or set(provenance) !=
            {"delivery_sha256", "bundle_manifest_sha256", "reconstruction_sha256"} or
            any(not isinstance(v, str) or re.fullmatch(r"[0-9a-f]{64}", v) is None
                for v in provenance.values()) or
            provenance["reconstruction_sha256"] != value["artwork_files"]["reconstruction.json"]["sha256"]):
        raise FormatError("Invalid authored prompt source provenance")
    return raw, value


def required_bytes(root):
    return sum(row["bytes"] for row in source_manifest(root)[1]["runtime_files"].values())


def _verify(path, row, root):
    path = _owned(path, root)
    if not path.is_file() or path.stat().st_size != row["bytes"]:
        raise FormatError("Authored prompt extent differs; existing data was retained: " + str(path))
    before = path.stat()
    with path.open("rb") as stream:
        actual = hashlib.file_digest(stream, "sha256").hexdigest()
    after = path.stat()
    if (actual != row["sha256"] or
            (before.st_size, before.st_mtime_ns) != (after.st_size, after.st_mtime_ns)):
        raise FormatError("Authored prompt SHA-256 differs; existing data was retained: " + str(path))
    return before.st_size, before.st_mtime_ns


def _inventory(directory, root, allowed):
    if not directory.exists():
        return
    allowed_directories = {Path(name).parent.as_posix() for name in allowed} - {"."}
    for path in directory.rglob("*"):
        if path.is_symlink():
            raise FormatError("Authored prompt paths must not contain symlinks")
        name = path.relative_to(directory).as_posix()
        if ((path.is_file() and name not in allowed) or
                (path.is_dir() and name not in allowed_directories) or
                (not path.is_file() and not path.is_dir())):
            raise FormatError("Authored prompt bank contains custom data; existing files were retained")
        _owned(path, root, directory=path.is_dir())


def verify_source(root):
    """Authenticate masters and runtime originals without opening Vista fonts."""
    from installation_paths import resource_root
    root = resource_root(root)
    raw, value = source_manifest(root)
    records = {**value["runtime_files"], **value["artwork_files"]}
    source = root / SOURCE
    _inventory(source, root, {*records, "manifest.json"})
    stamps = {name: _verify(source / name, row, root) for name, row in records.items()}
    if source_manifest(root)[0] != raw:
        raise FormatError("Authored prompt manifest changed during verification")
    return raw, value, stamps


def _prepare(root, cache, progress, previous=None):
    from installation_paths import resource_root
    root = resource_root(root)
    progress("Verifying the port's authored button prompt masters and rasters…")
    raw, value, stamps = verify_source(root)
    files = value["runtime_files"]
    entry = dict(schema=SCHEMA, complete=True, path=MANIFEST,
                 sha256=hashlib.sha256(raw).hexdigest(),
                 binding=BANK + "/prompt-source.tsv", runtime_files=files)
    if previous is not None and previous != entry:
        raise FormatError("Cached authored prompt provenance is customized; existing data was retained")
    destination = _owned(cache / BANK, cache, directory=True)
    _inventory(destination, cache, {*files, "manifest.json", "prompt-source.tsv"})
    metadata = {"manifest.json": raw, "prompt-source.tsv": BINDING}
    for name, expected in metadata.items():
        path = _owned(destination / name, cache)
        if path.exists() and path.read_bytes() != expected:
            raise FormatError("Cached authored prompt metadata is customized; existing data was retained")
    missing = []
    for name, row in files.items():
        path = _owned(destination / name, cache)
        if path.exists():
            _verify(path, row, cache)
        else:
            missing.append(name)
    _owned(cache / "presentation", cache, directory=True).mkdir(exist_ok=True)
    destination.mkdir(exist_ok=True)
    _owned(destination / "atlases", cache, directory=True).mkdir(exist_ok=True)
    if shutil.disk_usage(destination).free < sum(files[n]["bytes"] for n in missing) + 1024**2:
        raise FormatError("Insufficient free space for authored button prompts")
    for name in sorted(missing):
        progress("Copying authored button prompt " + name + "…")
        pending = destination / (".prompt-copy-" + uuid.uuid4().hex)
        try:
            with _owned(root / SOURCE / name, root).open("rb") as source, pending.open("xb") as output:
                shutil.copyfileobj(source, output, 1024 * 1024)
                output.flush()
                os.fsync(output.fileno())
            _verify(pending, files[name], cache)
            _publish_new(pending, destination / name)
        finally:
            pending.unlink(missing_ok=True)
    if (any(((root / SOURCE / name).stat().st_size, (root / SOURCE / name).stat().st_mtime_ns) != stamp
            for name, stamp in stamps.items()) or source_manifest(root)[0] != raw):
        raise FormatError("Authored prompt sources changed during import; retry setup")
    for name, row in files.items():
        _verify(destination / name, row, cache)
    for name, data in metadata.items():
        if (destination / name).exists():
            if _owned(destination / name, cache).read_bytes() != data:
                raise FormatError("Cached authored prompt metadata changed during import")
            continue
        pending = destination / (".prompt-metadata-" + uuid.uuid4().hex)
        try:
            pending.write_bytes(data)
            _publish_new(pending, destination / name)
        finally:
            pending.unlink(missing_ok=True)
    progress("Authored button prompts are verified. MCC fonts and existing data were retained.")
    return entry


def prepare(root, cache, *, progress=print):
    """Prepare an importer-owned stage without publishing its provenance."""
    root, cache = Path(root).resolve(), Path(cache)
    _owned(cache, cache, directory=True)
    if not cache.is_dir():
        raise FormatError("Authored prompt cache directory is missing")
    with _cancellation(), _lock(cache):
        return _prepare(root, cache, progress)


def repair(root, cache, *, progress=print):
    """Verify/resume only this bank after onboarding validates cache ownership."""
    root, cache = Path(root).resolve(), Path(cache)
    _owned(cache, cache, directory=True)
    with _cancellation(), _lock(cache):
        path = _owned(cache / "provenance.json", cache)
        before = path.read_bytes()
        report = json.loads(before)
        if (not isinstance(report, dict) or report.get("schema") != "H2_MCC_TU15_PREPARATION_V1" or
                report.get("external_installation_requirement") != "Halo MCC only"):
            raise FormatError("Authored prompts require an owned MCC cache")
        entry = _prepare(root, cache, progress, report.get("port_authored_prompts"))
        updated = {**report, "port_authored_prompts": entry}
        if updated == report:
            return cache, report
        pending = cache / (".prompt-provenance-" + uuid.uuid4().hex)
        try:
            pending.write_text(json.dumps(updated, indent=2) + "\n")
            if path.read_bytes() != before:
                raise FormatError("MCC provenance changed during prompt repair; retry setup")
            os.replace(pending, path)
        finally:
            pending.unlink(missing_ok=True)
        return cache, updated


def main():
    import argparse
    import sys
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--cache", type=Path)
    parser.add_argument("--verify-source", action="store_true")
    args = parser.parse_args()
    try:
        if args.verify_source:
            verify_source(args.root)
        else:
            from mcc_onboarding import repair_prompts
            repair_prompts(args.root, cache=args.cache)
        return 0
    except (OSError, ValueError) as error:
        print("Authored prompt preparation failed: " + str(error), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
