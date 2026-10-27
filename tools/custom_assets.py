#!/usr/bin/env python3
"""Authenticate and stage the complete port-owned custom asset source set.

Builds and packages carry the same sources consumed by MCC onboarding. The
receipt records asset delivery only; it does not admit gameplay or an engine.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import shutil
import uuid

from audit_rendering import FormatError
from installation_paths import resource_root
from mcc_movies import _cancellation, _lock, _owned, _publish_new

SCHEMA = 'H2_CUSTOM_ASSET_SOURCES_V1'
MANIFEST = 'custom-assets.json'


def _record(path):
    before = path.stat()
    with path.open('rb') as stream:
        digest = hashlib.file_digest(stream, 'sha256').hexdigest()
    after = path.stat()
    if (before.st_size, before.st_mtime_ns, before.st_ino) != (
            after.st_size, after.st_mtime_ns, after.st_ino):
        raise FormatError('Custom asset changed during verification: ' + str(path))
    return dict(bytes=after.st_size, sha256=digest)


def _groups(root):
    from controller_prompt_assets import inventory as prompt_inventory
    from custom_map_assets import source_bundle as map_inventory
    from custom_loading_assets import inventory as loading_inventory
    from h2ek_loading_ambience import inventory as source_policy_inventory
    from custom_ui_assets import inventory as ui_inventory
    from native_variant_assets import source_bundle as variant_sources
    variants = {name: dict(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest())
                for name, raw in variant_sources(root).items()}
    return dict(native_variants=variants, controller_prompts=prompt_inventory(root),
                custom_ui=ui_inventory(root), custom_maps=map_inventory(root),
                custom_loading=loading_inventory(root),
                permitted_source_policies=source_policy_inventory(root))


def _inventory(groups):
    result = {}
    for files in groups.values():
        for name, row in files.items():
            relative = PurePosixPath(name)
            if (relative.is_absolute() or relative.as_posix() != name or
                    any(part in ('', '.', '..') for part in relative.parts) or '\\' in name or
                    name == MANIFEST):
                raise FormatError('Invalid custom asset source path: ' + str(name))
            if name in result and result[name] != row:
                raise FormatError('Ambiguous custom asset source: ' + name)
            result[name] = dict(row)
    return dict(sorted(result.items()))


def inventory(root):
    """Return authenticated root-relative source hashes and logical extents."""
    return _inventory(_groups(root))


def required_bytes(root):
    """Conservative space requirement, including duplicate logical map owners."""
    return sum(row['bytes'] for row in inventory(root).values())


def _receipt(groups, files):
    summaries = {name: dict(files=rows, bytes=sum(row['bytes'] for row in rows.values()))
                 for name, rows in groups.items()}
    summaries['controller_prompts']['families'] = ['xbox', 'playstation']
    return dict(schema=SCHEMA, files=files, groups=summaries,
                bytes=sum(row['bytes'] for row in files.values()),
                scope='Custom source delivery; runtime admission remains separate')


def install(root, destination, *, progress=lambda _: None):
    """Refresh only managed bytes; preserve customized and unrelated files.

    Identical payloads share a destination-local inode. No build/package file
    is linked to its source, so editing a staged asset cannot alter originals.
    Interrupted copies resume using the last receipt and current source hashes.
    """
    source = resource_root(root)
    groups = _groups(root)
    files = _inventory(groups)
    entry = _receipt(groups, files)
    destination = Path(destination).absolute()
    if destination.resolve() == source:
        return entry
    _owned(destination, destination, directory=True).mkdir(parents=True, exist_ok=True)
    with _cancellation(), _lock(destination):
        receipt = _owned(destination / MANIFEST, destination)
        previous = json.loads(receipt.read_bytes()) if receipt.exists() else {}
        if previous and (not isinstance(previous, dict) or previous.get('schema') != SCHEMA or
                         not isinstance(previous.get('files'), dict)):
            raise FormatError('Invalid custom asset delivery receipt; existing files retained')
        old = previous.get('files', {})
        pending_files, local_payloads = {}, {}
        # Authenticate every affected destination before publishing any file.
        for name, row in files.items():
            path = _owned(destination / name, destination)
            if path.exists():
                current = _record(path)
                if current == row:
                    local_payloads[(row['bytes'], row['sha256'])] = path
                    continue
                if current != old.get(name):
                    raise FormatError('Customized custom asset retained: ' + name)
            pending_files[name] = row
        if pending_files:
            progress('Staging custom textures, controller buttons, maps and game modes…')
        for name, row in pending_files.items():
            path = _owned(destination / name, destination)
            path.parent.mkdir(parents=True, exist_ok=True)
            pending = path.with_name('.custom-asset-' + uuid.uuid4().hex)
            key = (row['bytes'], row['sha256'])
            try:
                existing_payload = local_payloads.get(key)
                if existing_payload is not None:
                    try:
                        os.link(existing_payload, pending)
                    except OSError:
                        shutil.copyfile(existing_payload, pending)
                else:
                    with _owned(source / name, source).open('rb') as src, pending.open('xb') as dst:
                        shutil.copyfileobj(src, dst, 1024**2)
                        dst.flush()
                        os.fsync(dst.fileno())
                if _record(pending) != row:
                    raise FormatError('Custom asset source changed during copying: ' + name)
                if path.exists():
                    if _record(path) != old.get(name):
                        raise FormatError('Custom asset destination changed during staging: ' + name)
                    os.replace(pending, path)
                else:
                    _publish_new(pending, path)
                local_payloads[key] = path
            finally:
                pending.unlink(missing_ok=True)
        if _inventory(_groups(root)) != files:
            raise FormatError('Custom asset sources changed while staging; retry the build')
        for name, row in files.items():
            if _record(_owned(destination / name, destination)) != row:
                raise FormatError('Staged custom asset changed before publication: ' + name)
        encoded = (json.dumps(entry, indent=2, sort_keys=True) + '\n').encode()
        if not receipt.exists() or receipt.read_bytes() != encoded:
            pending = destination / ('.custom-assets-' + uuid.uuid4().hex)
            try:
                pending.write_bytes(encoded)
                os.replace(pending, receipt)
            finally:
                pending.unlink(missing_ok=True)
    return entry


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--destination', type=Path, required=True)
    args = parser.parse_args()
    result = install(args.root, args.destination, progress=print)
    print(f"Verified {len(result['files'])} custom asset source files")


if __name__ == '__main__':
    main()
