#!/usr/bin/env python3
"""Stage latest local Linux binaries. This is not a relocatable release packager."""
from __future__ import annotations

import argparse
import errno
from contextlib import contextmanager
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import stat
import struct
import sys
import uuid

PLATFORM = "linux-x86_64"
SCHEMA = "H2_LOCAL_BUILD_OUTPUT_V1"
OUTPUTS = {
    "halo2_native": "halo2_native",
    "halo2_launcher": "Halo2-Launcher",
    "halo2_title_launcher": "Halo2-Title-Launcher",
    "halo2_onboarding": "onboarding/Halo2-Launcher",
    "halo2_mcc_loading_audio": "halo2_mcc_loading_audio",
}
MANIFEST = "build-output.json"
MAX_MANIFEST = 1024 * 1024
DIR_FLAGS = os.O_RDONLY | getattr(os, "O_DIRECTORY", 0) | getattr(os, "O_NOFOLLOW", 0)
FILE_FLAGS = os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0) | getattr(os, "O_NONBLOCK", 0)


class StageError(RuntimeError):
    pass


def _directory(path):
    """Open an absolute directory by descriptors, refusing every symlink."""
    path = Path(os.path.abspath(path))
    descriptor = os.open(path.anchor, DIR_FLAGS)
    try:
        for part in path.parts[1:]:
            child = os.open(part, DIR_FLAGS, dir_fd=descriptor)
            os.close(descriptor)
            descriptor = child
        return descriptor
    except BaseException:
        os.close(descriptor)
        raise


def _child(parent, name):
    try:
        os.mkdir(name, 0o755, dir_fd=parent)
    except FileExistsError:
        pass
    return os.open(name, DIR_FLAGS, dir_fd=parent)


def _open_regular(parent, name, *, missing=False):
    try:
        descriptor = os.open(name, FILE_FLAGS, dir_fd=parent)
    except FileNotFoundError:
        if missing:
            return None
        raise
    if not stat.S_ISREG(os.fstat(descriptor).st_mode):
        os.close(descriptor)
        raise StageError("Expected a regular owned file: " + name)
    return descriptor


def _digest(descriptor):
    os.lseek(descriptor, 0, os.SEEK_SET)
    result = hashlib.sha256()
    while data := os.read(descriptor, 1024 * 1024):
        result.update(data)
    return result.hexdigest()


def _stamp(value):
    return value.st_dev, value.st_ino, value.st_size, value.st_mtime_ns, value.st_ctime_ns, value.st_mode


def _elf(descriptor):
    info = os.fstat(descriptor)
    os.lseek(descriptor, 0, os.SEEK_SET)
    header = os.read(descriptor, 64)
    if (len(header) != 64 or header[:7] != b"\x7fELF\x02\x01\x01" or
            struct.unpack_from("<H", header, 16)[0] not in (2, 3) or
            struct.unpack_from("<H", header, 18)[0] != 62 or
            struct.unpack_from("<I", header, 20)[0] != 1 or
            struct.unpack_from("<H", header, 52)[0] != 64):
        raise StageError("Expected an ELF64 little-endian x86_64 executable")
    program_offset = struct.unpack_from("<Q", header, 32)[0]
    program_size, program_count = struct.unpack_from("<HH", header, 54)
    if (not info.st_mode & 0o111 or program_size != 56 or not 0 < program_count < 65535 or
            program_offset < 64 or program_offset + program_size * program_count > info.st_size):
        raise StageError("Invalid ELF64 executable program-header extent or permissions")
    return info


def _read_manifest(parent):
    descriptor = _open_regular(parent, MANIFEST, missing=True)
    if descriptor is None:
        return {"schema": SCHEMA, "platform": PLATFORM,
                "scope": "latest local binaries; original compiled shader bindings preserved; not a relocatable release",
                "packaged_release": False, "files": {}}
    try:
        if os.fstat(descriptor).st_size > MAX_MANIFEST:
            raise StageError("Excessive local build manifest")
        raw = os.read(descriptor, MAX_MANIFEST + 1)
        manifest = json.loads(raw)
    finally:
        os.close(descriptor)
    if (not isinstance(manifest, dict) or manifest.get("schema") != SCHEMA or
            manifest.get("platform") != PLATFORM or manifest.get("packaged_release") is not False or
            not isinstance(manifest.get("files"), dict)):
        raise StageError("Unrecognized existing local build manifest; retained unchanged")
    for target, row in manifest["files"].items():
        if (target not in OUTPUTS or not isinstance(row, dict) or row.get("path") != OUTPUTS[target] or
                not isinstance(row.get("source"), str) or not Path(row["source"]).is_absolute() or
                not isinstance(row.get("bytes"), int) or isinstance(row["bytes"], bool) or row["bytes"] < 64 or
                not isinstance(row.get("sha256"), str) or not re.fullmatch(r"[0-9a-f]{64}", row["sha256"])):
            raise StageError("Invalid existing local build record; retained unchanged")
    return manifest


@contextmanager
def _publication():
    # Deliver cancellation after a complete binary/manifest publication or its
    # rollback, rather than between rename and its Python bookkeeping.
    previous = signal.pthread_sigmask(signal.SIG_BLOCK, {signal.SIGINT, signal.SIGTERM})
    try:
        yield
    finally:
        signal.pthread_sigmask(signal.SIG_SETMASK, previous)


def _temporary(parent, prefix, owned):
    with _publication():
        name = prefix + uuid.uuid4().hex
        descriptor = os.open(name, os.O_WRONLY | os.O_CREAT | os.O_EXCL | getattr(os, "O_NOFOLLOW", 0),
                             0o600, dir_fd=parent)
        owned.append((parent, name))
    return name, descriptor


def _write_all(descriptor, data):
    view = memoryview(data)
    while view:
        count = os.write(descriptor, view)
        if count <= 0:
            raise StageError("Incomplete staged binary write")
        view = view[count:]


def _copy_binary(source, destination, expected):
    os.lseek(source, 0, os.SEEK_SET)
    digest = hashlib.sha256()
    while data := os.read(source, 1024 * 1024):
        _write_all(destination, data)
        digest.update(data)
    if digest.hexdigest() != expected:
        raise StageError("Build output changed while copying; existing staged binary retained")


def _backup_binary(parent, name, existing, temporary):
    backup = ".previous-" + uuid.uuid4().hex
    try:
        os.link(name, backup, src_dir_fd=parent, dst_dir_fd=parent, follow_symlinks=False)
    except OSError as error:
        if error.errno not in (errno.EPERM, errno.EOPNOTSUPP, errno.EXDEV, errno.ENOSYS):
            raise
        # Mounted game drives may support atomic rename without hard links.
        # Keep a verified private copy for the same manifest-failure rollback.
        before = os.fstat(existing)
        backup, descriptor = _temporary(parent, ".previous-", temporary)
        try:
            _copy_binary(existing, descriptor, _digest(existing))
            os.fchmod(descriptor, stat.S_IMODE(before.st_mode))
            os.fsync(descriptor)
            if (_stamp(os.fstat(existing)) != _stamp(before) or
                    _stamp(os.stat(name, dir_fd=parent, follow_symlinks=False)) != _stamp(before)):
                raise StageError("Previous build output changed before publication")
        finally:
            os.close(descriptor)
    else:
        temporary.append((parent, backup))
    return backup


def stage(root, binary, target, platform=PLATFORM):
    if platform != PLATFORM:
        return {"status": "skipped", "platform": platform, "reason": "Only Linux x86_64 local binaries are supported"}
    if target not in OUTPUTS:
        raise StageError("Only the game runtime and three launcher targets may be staged")
    import fcntl

    root = Path(os.path.abspath(root))
    binary = Path(os.path.abspath(binary))
    relative = Path(OUTPUTS[target])
    if binary.name != relative.name:
        raise StageError("Source basename does not match the selected runtime/launcher target")
    descriptors = []
    temporary = []
    destination_parent = None
    backup = None
    binary_published = False
    manifest_published = False
    try:
        source_parent = _directory(binary.parent)
        descriptors.append(source_parent)
        source = _open_regular(source_parent, binary.name)
        descriptors.append(source)
        before = _elf(source)
        digest = _digest(source)
        if _stamp(before) != _stamp(os.fstat(source)):
            raise StageError("Build output changed while hashing")
        parent = _directory(root)
        descriptors.append(parent)
        for name in ("dist", "builds", PLATFORM):
            parent = _child(parent, name)
            descriptors.append(parent)
        platform_parent = parent
        lock = os.open(".stage-build-output.lock", os.O_RDWR | os.O_CREAT | getattr(os, "O_NOFOLLOW", 0),
                       0o600, dir_fd=platform_parent)
        descriptors.append(lock)
        if not stat.S_ISREG(os.fstat(lock).st_mode):
            raise StageError("Build output lock is not a regular file")
        fcntl.flock(lock, fcntl.LOCK_EX)
        manifest = _read_manifest(platform_parent)
        destination_parent = platform_parent
        for name in relative.parts[:-1]:
            destination_parent = _child(destination_parent, name)
            descriptors.append(destination_parent)
        existing = _open_regular(destination_parent, relative.name, missing=True)
        mode = stat.S_IMODE(before.st_mode)
        binary_changed = True
        if existing is not None:
            descriptors.append(existing)
            binary_changed = os.fstat(existing).st_size != before.st_size or _digest(existing) != digest or stat.S_IMODE(os.fstat(existing).st_mode) != mode
        record = {"path": relative.as_posix(), "source": str(binary), "bytes": before.st_size,
                  "sha256": digest, "mode": format(mode, "04o")}
        manifest_changed = manifest["files"].get(target) != record
        if (_stamp(os.stat(binary.name, dir_fd=source_parent, follow_symlinks=False)) != _stamp(before) or
                _stamp(os.fstat(source)) != _stamp(before)):
            raise StageError("Build output changed before publication")
        if binary_changed or manifest_changed:
            manifest["files"][target] = record
            encoded = (json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode()
            if len(encoded) > MAX_MANIFEST:
                raise StageError("Excessive local build manifest")
            manifest_name, manifest_fd = _temporary(platform_parent, ".build-output-", temporary)
            try:
                _write_all(manifest_fd, encoded)
                os.fchmod(manifest_fd, 0o644)
                os.fsync(manifest_fd)
            finally:
                os.close(manifest_fd)
            binary_name = None
            if binary_changed:
                binary_name, binary_fd = _temporary(destination_parent, ".binary-", temporary)
                try:
                    _copy_binary(source, binary_fd, digest)
                    os.fchmod(binary_fd, mode)
                    os.fsync(binary_fd)
                finally:
                    os.close(binary_fd)
            current = os.stat(binary.name, dir_fd=source_parent, follow_symlinks=False)
            if _stamp(current) != _stamp(before) or _stamp(os.fstat(source)) != _stamp(before):
                raise StageError("Build output changed before publication")
            with _publication():
                try:
                    if binary_changed:
                        if existing is not None:
                            backup = _backup_binary(destination_parent, relative.name, existing, temporary)
                        os.replace(binary_name, relative.name, src_dir_fd=destination_parent, dst_dir_fd=destination_parent)
                        binary_published = True
                    os.replace(manifest_name, MANIFEST, src_dir_fd=platform_parent, dst_dir_fd=platform_parent)
                    manifest_published = True
                    os.fsync(destination_parent)
                    if destination_parent != platform_parent:
                        os.fsync(platform_parent)
                except BaseException:
                    if binary_published and not manifest_published:
                        if backup is not None:
                            os.replace(backup, relative.name, src_dir_fd=destination_parent, dst_dir_fd=destination_parent)
                        else:
                            os.unlink(relative.name, dir_fd=destination_parent)
                        binary_published = False
                    raise
        return {"status": "staged", "platform": PLATFORM, "target": target,
                "output": str(root / "dist/builds" / PLATFORM / relative),
                "manifest": str(root / "dist/builds" / PLATFORM / MANIFEST),
                "binary_changed": binary_changed, "manifest_changed": manifest_changed or binary_changed,
                "sha256": digest, "bytes": before.st_size, "packaged_release": False}
    except BaseException:
        if binary_published and not manifest_published:
            if backup is not None:
                os.replace(backup, relative.name, src_dir_fd=destination_parent, dst_dir_fd=destination_parent)
            else:
                os.unlink(relative.name, dir_fd=destination_parent)
        raise
    finally:
        for parent, name in temporary:
            try:
                os.unlink(name, dir_fd=parent)
            except FileNotFoundError:
                pass
        for descriptor in reversed(descriptors):
            os.close(descriptor)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", required=True, type=Path)
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--target", required=True, choices=tuple(OUTPUTS))
    parser.add_argument("--platform", required=True)
    args = parser.parse_args(argv)
    previous = signal.signal(signal.SIGTERM, lambda number, frame: sys.exit(128 + number))
    try:
        result = stage(args.root, args.binary, args.target, args.platform)
    except (OSError, ValueError, StageError) as error:
        parser.exit(1, "Build output staging failed: " + str(error) + "\n")
    finally:
        signal.signal(signal.SIGTERM, previous)
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    main()
