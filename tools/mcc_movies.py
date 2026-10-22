"""Copy the explicitly assigned bundled intro/credits into an MCC cache.

This bank is a user-authorized presentation exception, not an MCC asset or an
attract-movie assignment. Full payload hashes are checked during import/repair;
the runtime checks the pinned small manifest and file extents, then decodes.
"""
import contextlib
import ctypes
import errno
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import stat
import sys
import threading
import uuid

from audit_rendering import FormatError
from installation_paths import resource_root

SCHEMA = "H2_PORT_BUNDLED_MOVIES_V1"
SOURCE_SCHEMA = "halo2-upscaled-movies-v1"
SOURCE_MANIFEST = "resources/movie-manifest.json"
BANK = "presentation/movies"
MANIFEST = BANK + "/manifest.json"
ROLES = {"intro": "intro_60.mp4", "credits": "credits_60.mp4"}
MAX_METADATA = 64 * 1024


def _owned(path, root, *, directory=False):
    """Reject links at every owned path component, including dangling links."""
    path, root = Path(path), Path(root)
    try:
        relative = path.relative_to(root)
    except ValueError as error:
        raise FormatError("Movie path escapes its owner") from error
    for part in (root, *(root / Path(*relative.parts[:i]) for i in range(1, len(relative.parts) + 1))):
        if part.is_symlink():
            raise FormatError("Bundled movie paths must not contain symlinks: " + str(part))
    if path.exists() and (not path.is_dir() if directory else not path.is_file()):
        raise FormatError("Invalid bundled movie owner: " + str(path))
    return path


def _read(path, root):
    path = _owned(path, root)
    if not path.is_file() or path.stat().st_size > MAX_METADATA:
        raise FormatError("Bundled movie manifest is missing or too large: " + str(path))
    raw = path.read_bytes()
    try:
        return raw, json.loads(raw)
    except (ValueError, UnicodeError) as error:
        raise FormatError("Invalid bundled movie manifest") from error


def source_manifest(root):
    root = resource_root(root)
    raw, metadata = _read(root / SOURCE_MANIFEST, root)
    if not isinstance(metadata, dict) or metadata.get("schema") != SOURCE_SCHEMA:
        raise FormatError("Unsupported bundled movie source manifest")
    files = metadata.get("files")
    if not isinstance(files, dict) or set(files) != set(ROLES.values()):
        raise FormatError("Bundled movie roles must be exactly intro and credits")
    for row in files.values():
        if (not isinstance(row, dict) or set(row) != {"bytes", "sha256"} or
                type(row.get("bytes")) is not int or not 0 < row["bytes"] < 2**63 or
                not isinstance(row.get("sha256"), str) or
                re.fullmatch("[0-9a-f]{64}", row["sha256"]) is None):
            raise FormatError("Invalid bundled movie size or SHA-256")
    _, bundled = _read(root / "resources/movies/manifest.json", root)
    if bundled != metadata:
        raise FormatError("Bundled movie source manifests disagree")
    return hashlib.sha256(raw).hexdigest(), files


def required_bytes(root):
    return sum(row["bytes"] for row in source_manifest(root)[1].values())


def _metadata(root):
    source_sha, files = source_manifest(root)
    metadata = dict(schema=SCHEMA, complete=True, source_schema=SOURCE_SCHEMA,
                    source_manifest=dict(path=SOURCE_MANIFEST, sha256=source_sha),
                    roles={role: dict(path=name, **files[name]) for role, name in ROLES.items()},
                    copied_without_transcoding=True, attract_assigned=False,
                    external_vista_or_xbox_required=False)
    raw = (json.dumps(metadata, indent=2) + "\n").encode()
    entry = dict(schema=SCHEMA, complete=True, path=MANIFEST,
                 sha256=hashlib.sha256(raw).hexdigest(), roles=metadata["roles"])
    return files, raw, entry


def _stamp(path):
    value = path.stat()
    # Extents/timestamps qualify source stability without depending on exFAT's
    # unstable cross-process inode numbers. Payload hashes are authoritative.
    return value.st_size, value.st_mtime_ns


def _verify(path, row, root):
    path = _owned(path, root)
    if not path.is_file() or path.stat().st_size != row["bytes"]:
        raise FormatError("Bundled movie extent differs; existing files were retained: " + str(path))
    before = _stamp(path)
    with path.open("rb") as stream:
        digest = hashlib.file_digest(stream, "sha256").hexdigest()
    if digest != row["sha256"] or _stamp(path) != before:
        raise FormatError("Bundled movie SHA-256 differs; existing files were retained: " + str(path))
    return before


@contextlib.contextmanager
def _lock(cache):
    import fcntl
    path = _owned(cache / ".bundled-movies.lock", cache)
    fd = os.open(path, os.O_CREAT | os.O_RDWR | getattr(os, "O_NOFOLLOW", 0), 0o600)
    try:
        if not stat.S_ISREG(os.fstat(fd).st_mode):
            raise FormatError("Invalid bundled movie repair lock")
        fcntl.flock(fd, fcntl.LOCK_EX)
        yield
    finally:
        os.close(fd)


@contextlib.contextmanager
def _cancellation():
    # Setup may be terminated while writing outside test scratch. Let finally
    # remove an unfinished copy; already verified published files can resume.
    previous = None
    if threading.current_thread() is threading.main_thread():
        previous = signal.getsignal(signal.SIGTERM)
        def stopped(signum, frame):
            raise SystemExit(128 + signum)
        signal.signal(signal.SIGTERM, stopped)
    try:
        yield
    finally:
        if previous is not None:
            signal.signal(signal.SIGTERM, previous)


def _publish_new(source, destination):
    """Atomic no-replace rename, including filesystems without hard links."""
    if os.name == "nt":
        os.rename(source, destination)  # Windows rename refuses existing files.
        return
    if sys.platform == "darwin":
        from macos.atomic_files import rename_exclusive
        try:
            rename_exclusive(source, destination)
        except FileExistsError as error:
            raise FormatError("Bundled movie destination changed during import; existing files were retained") from error
        return
    library = ctypes.CDLL(None, use_errno=True)
    rename = getattr(library, "renameat2", None)
    if rename is None:
        raise FormatError("Atomic movie publication requires renameat2 on this platform")
    rename.argtypes = (ctypes.c_int, ctypes.c_char_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_uint)
    rename.restype = ctypes.c_int
    if rename(-100, os.fsencode(source), -100, os.fsencode(destination), 1) != 0:
        code = ctypes.get_errno()
        if code == errno.EEXIST:
            raise FormatError("Bundled movie destination changed during import; existing files were retained")
        raise OSError(code, os.strerror(code), str(destination))


def _prepare(root, cache, progress, previous=None):
    root = resource_root(root)
    files, manifest, entry = _metadata(root)
    if previous is not None and previous != entry:
        raise FormatError("Cached bundled movie provenance is customized; existing data was retained")
    destination = _owned(cache / BANK, cache, directory=True)
    manifest_path = destination / "manifest.json"
    if manifest_path.exists() or manifest_path.is_symlink():
        old, _ = _read(manifest_path, cache)
        if old != manifest:
            raise FormatError("Cached bundled movie manifest is customized; existing data was retained")
    # Validate both originals before copying either; a partial matching bank is
    # intentionally resumable after cancellation. Never overwrite custom data.
    progress("Verifying the bundled intro and credits movies…")
    stamps = {name: _verify(root / "resources/movies" / name, row, root)
              for name, row in files.items()}
    missing = []
    for name, row in files.items():
        path = _owned(destination / name, cache)
        if path.exists():
            _verify(path, row, cache)
        else:
            missing.append(name)
    _owned(cache / "presentation", cache, directory=True).mkdir(exist_ok=True)
    destination.mkdir(exist_ok=True)
    needed = sum(files[name]["bytes"] for name in missing)
    if needed and shutil.disk_usage(destination).free < needed + 16 * 1024**2:
        raise FormatError("Insufficient free space for bundled intro and credits")
    for name in missing:
        progress("Copying bundled " + name + " into the private presentation cache…")
        pending = destination / (".movie-copy-" + uuid.uuid4().hex)
        try:
            with (root / "resources/movies" / name).open("rb") as source, pending.open("xb") as output:
                shutil.copyfileobj(source, output, 1024 * 1024)
                output.flush()
                os.fsync(output.fileno())
            _verify(pending, files[name], cache)
            _publish_new(pending, destination / name)
        finally:
            pending.unlink(missing_ok=True)
    if any(_stamp(root / "resources/movies" / name) != stamp for name, stamp in stamps.items()):
        raise FormatError("Bundled movie sources changed during import; retry setup")
    if _metadata(root)[1] != manifest:
        raise FormatError("Bundled movie source manifests changed during import; retry setup")
    if not manifest_path.exists():
        pending = destination / (".movie-manifest-" + uuid.uuid4().hex)
        try:
            pending.write_bytes(manifest)
            _publish_new(pending, manifest_path)
        finally:
            pending.unlink(missing_ok=True)
    progress("Bundled intro and credits are verified. Existing MCC data and shaders were retained.")
    return entry


def prepare(root, cache, *, progress=print):
    """Assemble the bank in a fresh importer stage; caller owns provenance."""
    root, cache = Path(root).resolve(), Path(cache)
    _owned(cache, cache, directory=True)
    if not cache.is_dir():
        raise FormatError("Movie cache directory is missing")
    with _cancellation(), _lock(cache):
        return _prepare(root, cache, progress)


def repair(root, cache, *, progress=print):
    """Repair only this bank/provenance after onboarding authenticates a cache."""
    root, cache = Path(root).resolve(), Path(cache)
    _owned(cache, cache, directory=True)
    with _cancellation(), _lock(cache):
        path = _owned(cache / "provenance.json", cache)
        before = path.read_bytes()
        report = json.loads(before)
        if (not isinstance(report, dict) or report.get("schema") != "H2_MCC_TU15_PREPARATION_V1" or
                report.get("external_installation_requirement") != "Halo MCC only"):
            raise FormatError("Bundled movies require an owned MCC cache")
        entry = _prepare(root, cache, progress, report.get("port_bundled_movies"))
        updated = {**report, "port_bundled_movies": entry}
        if report == updated:
            return cache, report
        pending = cache / (".movie-provenance-" + uuid.uuid4().hex)
        try:
            pending.write_text(json.dumps(updated, indent=2) + "\n")
            if path.read_bytes() != before:
                raise FormatError("MCC provenance changed during movie repair; retry setup")
            os.replace(pending, path)
        finally:
            pending.unlink(missing_ok=True)
        return cache, updated


def main():
    import argparse
    import sys
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--cache", type=Path, help="Completed private MCC cache; defaults to the selected cache")
    args = parser.parse_args()
    try:
        from mcc_onboarding import repair_movies
        repair_movies(args.root, cache=args.cache)
        return 0
    except (OSError, ValueError) as error:
        print("Bundled movie preparation failed: " + str(error), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
