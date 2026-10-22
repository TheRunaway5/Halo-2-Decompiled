"""Owned universal loading-converter inputs for macOS setup.

The manifest describes a native executable, its two private codec images and
the retained codebook. PCM admission remains with the shared V3 profile.
This module uses only Python's standard library in the packaged runtime.
"""
from __future__ import annotations

import json
import os
import struct

SCHEMA = 'H2_MACOS_MCC_LOADING_CONVERTER_RUNTIME_V1'
BASE = 'macos/loading-converter'
EXECUTABLE = BASE + '/bin/halo2_mcc_loading_audio'
LIBRARIES = (BASE + '/lib/libogg.0.dylib', BASE + '/lib/libvorbis.0.dylib')
LICENSES = tuple(BASE + '/licenses/' + name + '.txt' for name in ('Ogg', 'Vorbis', 'ww2ogg'))
ARCHITECTURES = {'arm64': (0x100000c, 0), 'x86_64': (0x1000007, 3)}


def _error(message):
    from audit_rendering import FormatError
    return FormatError('Mac loading converter ' + message)


def _arches(value):
    if (not isinstance(value, list) or not 1 <= len(value) <= 2 or
            any(not isinstance(v, str) or v not in ARCHITECTURES for v in value) or
            len(set(value)) != len(value)):
        raise _error('architectures differ from its CPU owners')
    return set(value)


def image_architectures(path, file_type):
    """Read bounded fat/thin 64-bit headers without requiring Xcode tools."""
    size = path.stat().st_size
    with path.open('rb') as stream:
        def thin(offset, length, expected=None):
            if offset < 0 or length < 32 or offset + length > size:
                raise _error('Mach-O slice exceeds its file')
            stream.seek(offset)
            header = stream.read(32)
            endian = {b'\xcf\xfa\xed\xfe': '<', b'\xfe\xed\xfa\xcf': '>'}.get(header[:4])
            if endian is None:
                raise _error('requires 64-bit Mach-O images')
            _, cpu, subtype, kind, count, commands, _, _ = struct.unpack(endian + '8I', header)
            identity = (cpu, subtype & 0xffffff)
            if (identity not in ARCHITECTURES.values() or kind != file_type or count > 4096 or
                    commands > length - 32 or (expected is not None and identity != expected)):
                raise _error('Mach-O header differs from its declared owner')
            return next(name for name, value in ARCHITECTURES.items() if value == identity)

        header = stream.read(8)
        if len(header) != 8:
            raise _error('Mach-O header is truncated')
        fat = {b'\xca\xfe\xba\xbe': ('>', False), b'\xbe\xba\xfe\xca': ('<', False),
               b'\xca\xfe\xba\xbf': ('>', True), b'\xbf\xba\xfe\xca': ('<', True)}.get(header[:4])
        if fat is None:
            return {thin(0, size)}
        endian, wide = fat
        count = struct.unpack(endian + 'I', header[4:])[0]
        if not 1 <= count <= 2:
            raise _error('fat image has unexpected CPU owners')
        stride = 32 if wide else 20
        table_end = 8 + count * stride
        raw = stream.read(count * stride)
        if len(raw) != count * stride:
            raise _error('fat image table is truncated')
        ranges, result = [], set()
        for i in range(count):
            fields = struct.unpack_from(endian + ('IIQQII' if wide else '5I'), raw, i * stride)
            cpu, subtype, offset, length, alignment = fields[:5]
            if (offset < table_end or alignment > 30 or offset % (1 << alignment) or
                    any(offset < end and start < offset + length for start, end in ranges)):
                raise _error('fat slices overlap or have invalid alignment')
            arch = thin(offset, length, (cpu, subtype & 0xffffff))
            if arch in result:
                raise _error('fat image repeats a CPU owner')
            result.add(arch)
            ranges.append((offset, offset + length))
        return result


def validate_metadata(decoder, codebook):
    """Validate portable receipt provenance, including offline cache reuse."""
    from mcc_loading_presentation import CODEBOOK, CODEBOOK_SHA, CONVERTER_MANIFEST, _hex
    runtime = decoder.get('runtime')
    if not isinstance(runtime, dict) or runtime.get('schema') != SCHEMA:
        raise _error('runtime provenance differs')
    _arches(runtime.get('architectures'))
    files = runtime.get('files')
    required = {EXECUTABLE, *LIBRARIES, *LICENSES, CODEBOOK}
    if not isinstance(files, dict) or set(files) != required:
        raise _error('file inventory differs from its private closure')
    for row in files.values():
        if (not isinstance(row, dict) or set(row) != {'bytes', 'sha256'} or
                type(row['bytes']) is not int or not 0 < row['bytes'] < 64 * 1024**2 or
                not _hex(row['sha256'], 64)):
            raise _error('file identity is invalid')
    if sum(row['bytes'] for row in files.values()) > 256 * 1024**2:
        raise _error('private closure is excessive')
    manifest = runtime.get('manifest')
    if (not isinstance(manifest, dict) or manifest.get('path') != CONVERTER_MANIFEST or
            type(manifest.get('bytes')) is not int or not 0 < manifest['bytes'] <= 65536 or
            not _hex(manifest.get('sha256'), 64)):
        raise _error('manifest identity is invalid')
    for row, name in ((decoder, EXECUTABLE), (codebook, CODEBOOK)):
        if row.get('path') != name or files[name] != {k: row.get(k) for k in ('bytes', 'sha256')}:
            raise _error('provenance owners disagree')
    if codebook['sha256'] != CODEBOOK_SHA:
        raise _error('codebook differs from the retained source')


def dependencies(root, manifest, value):
    from mcc_loading_presentation import CODEBOOK, _v3_dependency
    from mcc_movies import _owned
    before = manifest.read_bytes()
    if json.loads(before) != value:
        raise _error('manifest changed before admission')
    decoder = _v3_dependency(root / EXECUTABLE, root)
    codebook = _v3_dependency(root / CODEBOOK, root)
    decoder['runtime'] = dict(schema=SCHEMA, architectures=value.get('architectures'),
                              files=value.get('files'), manifest=_v3_dependency(manifest, root))
    validate_metadata(decoder, codebook)
    expected = set(decoder['runtime']['files']) - {CODEBOOK}
    actual = set()
    private = _owned(root / BASE, root, directory=True)
    for path in private.rglob('*'):
        _owned(path, root, directory=path.is_dir())
        if path.is_file():
            actual.add(path.relative_to(root).as_posix())
        elif not path.is_dir():
            raise _error('contains a nonregular entry')
    if actual != expected:
        raise _error('private directory inventory differs')
    for name, record in decoder['runtime']['files'].items():
        actual_record = _v3_dependency(root / name, root)
        if record != {k: actual_record[k] for k in ('bytes', 'sha256')}:
            raise _error('dependency changed during admission: ' + name)
    arches = _arches(value['architectures'])
    for name in (EXECUTABLE, *LIBRARIES):
        if image_architectures(root / name, 2 if name == EXECUTABLE else 6) != arches:
            raise _error('image CPU slices differ from the manifest')
    if not os.access(root / EXECUTABLE, os.X_OK):
        raise _error('executable lacks execution permission')
    if manifest.read_bytes() != before:
        raise _error('manifest changed during admission')
    return dict(converter=decoder, codebook=codebook)


def read_dependencies(root, manifest):
    """Return None for the shared Linux/development routes."""
    if not manifest.exists():
        return None
    if not manifest.is_file() or manifest.stat().st_size > 65536:
        raise _error('manifest is missing or excessive')
    value = json.loads(manifest.read_bytes())
    if isinstance(value, dict) and value.get('schema') == SCHEMA:
        return dependencies(root, manifest, value)
    return None
