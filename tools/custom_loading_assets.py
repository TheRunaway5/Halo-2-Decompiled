"""Authenticate optional authored loading source art for builds and packages."""
import hashlib
import json
from pathlib import Path
import re

from audit_rendering import FormatError
from installation_paths import resource_root
from mcc_movies import _owned


def _record(path):
    before = path.stat()
    with path.open('rb') as stream:
        digest = hashlib.file_digest(stream, 'sha256').hexdigest()
    after = path.stat()
    if (before.st_ino, before.st_size, before.st_mtime_ns) != (
            after.st_ino, after.st_size, after.st_mtime_ns):
        raise FormatError('Authored loading source changed during verification')
    return dict(bytes=after.st_size, sha256=digest)


def inventory(root):
    """Carry the declared artwork, preserving its existing admission state."""
    from mcc_loading_assets import CUSTOM
    root = resource_root(root)
    source = _owned(root / CUSTOM, root, directory=True)
    if not source.exists():
        return {}
    profile = _owned(source / 'profile.json', root)
    if not profile.is_file() or not 0 < profile.stat().st_size <= 1024**2:
        raise FormatError('Missing or oversized authored loading source profile')
    raw = profile.read_bytes()
    metadata = json.loads(raw)
    if (not isinstance(metadata, dict) or metadata.get('schema') != 'H2_AUTHORED_LOADING_ASSETS_V1' or
            not isinstance(metadata.get('files'), dict) or not 0 < len(metadata['files']) <= 128):
        raise FormatError('Invalid authored loading source profile')
    records = {profile.relative_to(root).as_posix():
               dict(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest())}
    for name, row in metadata['files'].items():
        if (not isinstance(name, str) or Path(name).name != name or
                Path(name).suffix not in ('.svg', '.dds', '.png') or '\\' in name or
                not isinstance(row, dict) or type(row.get('bytes')) is not int or
                not 0 < row['bytes'] <= 256 * 1024**2 or
                re.fullmatch('[0-9a-f]{64}', row.get('sha256', '')) is None or
                row.get('origin') not in ('authored-vector', 'rasterized-authored-vector')):
            raise FormatError('Invalid authored loading source declaration')
        path = _owned(source / name, root)
        if not path.is_file():
            raise FormatError('Missing authored loading source: ' + name)
        record = _record(path)
        if record != {key: row[key] for key in ('bytes', 'sha256')}:
            raise FormatError('Authored loading source differs: ' + name)
        records[path.relative_to(root).as_posix()] = record
    if profile.read_bytes() != raw:
        raise FormatError('Authored loading source profile changed during verification')
    return records
