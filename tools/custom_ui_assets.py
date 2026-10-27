"""Package authored UI masters and install the selected celestial rank artwork.

The complete source collection remains available to builds. Runtime replacement
is deliberately source-bound: the seven celestial slots in both Classic rank
banks use the existing custom DDS, with original logical geometry unchanged.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import uuid

from audit_rendering import FormatError
from installation_paths import resource_root
from mcc_movies import _cancellation, _lock, _owned, _publish_new

SOURCE = 'resources/ui-menu-masters'
BANK = 'presentation/custom-ranks'
PROFILE = 'H2_PORT_CUSTOM_RANKS_V1'
TAGS = (r'ui\global_bitmaps\rank_icons', r'ui\global_bitmaps\rank_icons_sm')
SLOTS = set(range(43, 50))


def _record(path):
    with path.open('rb') as stream:
        digest = hashlib.file_digest(stream, 'sha256').hexdigest()
    return dict(bytes=path.stat().st_size, sha256=digest)


def _source(root):
    root = resource_root(root)
    path = _owned(root / SOURCE / 'masters.json', root)
    if not 0 < path.stat().st_size <= 4 * 1024**2:
        raise FormatError('Custom UI source manifest is missing or too large')
    raw = path.read_bytes()
    value = json.loads(raw)
    if (value.get('schema') != 'halo2-vista-menu-masters-v1' or
            not isinstance(value.get('pages'), dict) or not 14 <= len(value['pages']) <= 1024):
        raise FormatError('Unrecognized custom UI masters')
    return root, raw, value


def _payload(root, page, field):
    name = page.get(field)
    prefix, suffix = ('svg', 'svg') if field == 'vector' else ('bitmaps', 'dds')
    if (not isinstance(name, str) or re.fullmatch(prefix + r'/[0-9a-f]{64}\.' + suffix, name) is None or
            re.fullmatch(r'[0-9a-f]{64}', page.get(field + '_sha256', '')) is None):
        raise FormatError('Invalid custom UI payload identity')
    path = _owned(root / SOURCE / name, root)
    row = _record(path)
    if not 0 < row['bytes'] <= 128 * 1024**2 or row['sha256'] != page[field + '_sha256']:
        raise FormatError('Custom UI payload differs: ' + name)
    return path, row


def inventory(root):
    """Authenticate all authored SVG/DDS sources; never copy reference donors."""
    root, raw, value = _source(root)
    records = {SOURCE + '/masters.json': dict(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest())}
    for page in value['pages'].values():
        if page.get('family', '').startswith('emblem-'):
            raise FormatError('Paused emblem drafts cannot enter the custom UI package')
        for field in ('vector', 'dds'):
            path, row = _payload(root, page, field)
            name = path.relative_to(root).as_posix()
            if name in records and records[name] != row:
                raise FormatError('Ambiguous custom UI payload')
            records[name] = row
    if _source(root)[1] != raw:
        raise FormatError('Custom UI sources changed during verification')
    return records


def install(root, destination):
    """Copy into a fresh caller-owned build/package stage."""
    source = resource_root(root)
    destination = Path(destination)
    records = inventory(root)
    for name in records:
        path = _owned(destination / name, destination)
        if path.exists():
            raise FormatError('Custom UI destination already exists: ' + name)
    for name, row in records.items():
        path = destination / name
        path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source / name, path)
        if _record(path) != row:
            raise FormatError('Custom UI source changed while packaging')
    if _source(root)[1] != (destination / SOURCE / 'masters.json').read_bytes():
        raise FormatError('Custom UI manifest changed while packaging')
    return dict(files=len(records), bytes=sum(row['bytes'] for row in records.values()),
                source_pages=len(_source(root)[2]['pages']), celestial_runtime_pages=14,
                original_fonts_included=False)


def runtime_sources(root):
    root, raw, value = _source(root)
    payloads, files, pages = {}, {}, []
    seen = set()
    for page in value['pages'].values():
        if page.get('family') != 'celestial-rank-illustration':
            continue
        _payload(root, page, 'vector')
        source, row = _payload(root, page, 'dds')
        name = BANK + '/' + page['dds']
        files[name], payloads[name] = row, source
        for binding in page.get('bindings', []):
            if binding.get('cache') != 'mainmenu.map':
                continue
            key = (binding.get('name'), binding.get('frame'))
            if key[0] not in TAGS or key[1] not in SLOTS or key in seen:
                raise FormatError('Invalid custom celestial rank slot')
            seen.add(key)
            pages.append(dict(name=key[0], frame=key[1], logical=page['logical'],
                physical=page['physical'], registration=binding['registration'],
                source_dds_sha256=binding['dds_sha256'], path=page['dds'], **row))
    if seen != {(tag, frame) for tag in TAGS for frame in SLOTS} or len(pages) != 14:
        raise FormatError('The complete custom celestial rank bank is required')
    metadata = (json.dumps(dict(schema=PROFILE, complete=True,
        source_manifest_sha256=hashlib.sha256(raw).hexdigest(), pages=pages,
        numeric_rank_source='MCC', celestial_rank_source='port-authored masters',
        original_logical_geometry=True), indent=2) + '\n').encode()
    name = BANK + '/manifest.json'
    files[name] = dict(bytes=len(metadata), sha256=hashlib.sha256(metadata).hexdigest())
    payloads[name] = metadata
    return dict(schema=PROFILE, complete=True, files=files, pages=14,
                source_manifest_sha256=hashlib.sha256(raw).hexdigest()), payloads


def required_bytes(root):
    return sum(row['bytes'] for row in runtime_sources(root)[0]['files'].values())


def verify(cache, entry):
    """Authenticate a declared runtime bank during private-cache admission."""
    cache = Path(cache)
    if (not isinstance(entry, dict) or entry.get('schema') != PROFILE or
            entry.get('complete') is not True or not isinstance(entry.get('files'), dict) or
            len(entry['files']) != 15):
        raise FormatError('Invalid custom UI runtime provenance')
    for name, row in entry['files'].items():
        if (not isinstance(name, str) or not name.startswith(BANK + '/') or
                '..' in Path(name).parts or Path(name).is_absolute() or
                not isinstance(row, dict) or not _matches(_owned(cache / name, cache), (row,))):
            raise FormatError('Custom UI runtime payload differs')
    path = BANK + '/manifest.json'
    if path not in entry['files']:
        raise FormatError('Custom UI runtime manifest is missing')
    metadata = json.loads((cache / path).read_bytes())
    if (metadata.get('schema') != PROFILE or metadata.get('complete') is not True or
            metadata.get('source_manifest_sha256') != entry.get('source_manifest_sha256')):
        raise FormatError('Custom UI runtime manifest differs from provenance')
    return True


def _matches(path, records):
    return path.is_file() and not path.is_symlink() and _record(path) in records


def _prepare(root, cache, previous, progress):
    entry, payloads = runtime_sources(root)
    files = entry['files']
    if previous is not None and (not isinstance(previous, dict) or previous.get('schema') != PROFILE or
            previous.get('complete') is not True or not isinstance(previous.get('files'), dict)):
        raise FormatError('Customized custom-rank provenance; existing data retained')
    old = previous['files'] if previous is not None else files
    # Bind custom slots to the copied Classic source; no numbered MCC frontend
    # substitute or differently numbered bank can silently enter these slots.
    assets = json.loads(_owned(cache / 'ui/mcc-menu-assets.json', cache).read_bytes())
    if assets.get('schema') != 'H2_MCC_MENU_RESOURCE_CACHE_V1' or assets.get('complete') is not True:
        raise FormatError('Custom ranks require the verified MCC menu resource owners')
    metadata = json.loads(payloads[BANK + '/manifest.json'])
    for page in metadata['pages']:
        owners = [row for row in assets['pages'] if row.get('cache') == 'mainmenu.map' and
                  (row.get('name'), row.get('frame')) == (page['name'], page['frame'])]
        if len(owners) != 1 or owners[0].get('dds_sha256') != page['source_dds_sha256'] or [
                owners[0].get('width'), owners[0].get('height')] != page['logical']:
            raise FormatError('Custom celestial rank differs from its MCC frame owner')
    bank = _owned(cache / BANK, cache, directory=True)
    if bank.exists():
        for path in bank.rglob('*'):
            _owned(path, cache, directory=path.is_dir())
            if path.is_dir() and path == bank / 'bitmaps':
                continue
            name = path.relative_to(cache).as_posix()
            if name not in files or not _matches(path, (old.get(name), files[name])):
                raise FormatError('Customized celestial rank assets; existing files retained')
    if shutil.disk_usage(cache).free < sum(row['bytes'] for row in files.values()) + 1024**2:
        raise FormatError('Insufficient space for custom rank artwork')
    progress('Installing custom moon, eclipse, sun, comet and Halo-ring rank artwork…')
    for name, row in files.items():
        path = _owned(cache / name, cache)
        if _matches(path, (row,)):
            continue
        path.parent.mkdir(parents=True, exist_ok=True)
        pending = path.parent / ('.custom-rank-' + uuid.uuid4().hex)
        try:
            data = payloads[name]
            if isinstance(data, bytes):
                pending.write_bytes(data)
            else:
                shutil.copyfile(data, pending)
            if not _matches(pending, (row,)):
                raise FormatError('Custom rank source changed while copying')
            if path.exists():
                if not _matches(path, (old.get(name), row)):
                    raise FormatError('Custom rank destination changed during repair')
                os.replace(pending, path)
            else:
                _publish_new(pending, path)
        finally:
            pending.unlink(missing_ok=True)
    if runtime_sources(root)[0] != entry:
        raise FormatError('Custom rank source changed during preparation; retry setup')
    verify(cache, entry)
    return entry


def prepare(root, cache, *, progress=print):
    cache = Path(cache)
    with _cancellation(), _lock(cache):
        return _prepare(root, cache, None, progress)


def repair(root, cache, *, progress=print):
    cache = Path(cache)
    with _cancellation(), _lock(cache):
        path = _owned(cache / 'provenance.json', cache)
        before = path.read_bytes()
        report = json.loads(before)
        if (report.get('schema') != 'H2_MCC_TU15_PREPARATION_V1' or
                report.get('external_installation_requirement') != 'Halo MCC only'):
            raise FormatError('Custom rank repair requires an owned private MCC cache')
        entry = _prepare(root, cache, report.get('custom_ui_assets'), progress)
        updated = {**report, 'custom_ui_assets': entry}
        if updated != report:
            pending = cache / ('.custom-rank-provenance-' + uuid.uuid4().hex)
            try:
                pending.write_text(json.dumps(updated, indent=2) + '\n')
                if path.read_bytes() != before:
                    raise FormatError('Custom rank provenance changed; retry setup')
                os.replace(pending, path)
            finally:
                pending.unlink(missing_ok=True)
        return cache, updated
