"""Authenticate and package the port-owned Xbox and PlayStation prompt banks."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import uuid

from installation_paths import resource_root
from mcc_movies import _cancellation, _lock, _owned, _publish_new

FAMILIES = {'xbox': 'H2_PORT_XBOX_PROMPTS_V1', 'playstation': 'H2_PORT_PLAYSTATION_PROMPTS_V1'}
PROFILE = 'H2_LOCAL_PLAYER_PROMPTS_V1'
RUNTIME_FILES = {'prompts.tsv', 'prompt-source.tsv', *(f'atlases/{i}.dds' for i in range(6))}


def inventory(root):
    root = resource_root(root)
    result = {}
    for family, schema in FAMILIES.items():
        bank = root / 'resources/presentation' / ('prompts-' + family)
        path = bank / 'manifest.json'
        if path.is_symlink() or not 0 < path.stat().st_size <= 128 * 1024:
            raise ValueError('Invalid controller prompt manifest')
        raw = path.read_bytes()
        manifest = json.loads(raw)
        if manifest.get('schema') != schema or not isinstance(manifest.get('files'), dict):
            raise ValueError('Unrecognized controller prompt bank')
        files = manifest['files']
        required = {'prompts.tsv', 'reconstruction.json', 'prompt-source.tsv',
                    *(f'atlases/{i}.dds' for i in range(6))}
        if not required <= files.keys() or len(files) > 200:
            raise ValueError('Incomplete controller prompt bank')
        for name, record in files.items():
            relative = Path(name)
            source = bank / relative
            if (relative.is_absolute() or '..' in relative.parts or source.is_symlink() or
                    not source.resolve(strict=True).is_relative_to(bank.resolve(strict=True)) or
                    not 0 < record['bytes'] <= 32 * 1024**2 or source.stat().st_size != record['bytes'] or
                    hashlib.sha256(source.read_bytes()).hexdigest() != record['sha256']):
                raise ValueError('Controller prompt payload differs: ' + name)
            result[source.relative_to(root).as_posix()] = dict(record)
        result[path.relative_to(root).as_posix()] = dict(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest())
    return result


def install(root, destination):
    """Caller owns the staging directory and its cancellation/cleanup lifecycle."""
    source = resource_root(root)
    destination = Path(destination)
    records = inventory(root)
    for name in sorted(records):
        path = destination / name
        if path.exists() or path.is_symlink():
            raise ValueError('Controller prompt destination already exists: ' + name)
        path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source / name, path)
    if inventory(destination) != records or inventory(root) != records:
        raise ValueError('Controller prompts changed while packaging')
    return dict(families=list(FAMILIES), files=len(records), bytes=sum(r['bytes'] for r in records.values()),
                independent_font_owner=True)


def runtime_sources(root):
    """Keep the SVG masters in the port; vanilla caches need only the rasters."""
    source = resource_root(root)
    records = inventory(root)
    payloads, files = {}, {}
    for family, schema in FAMILIES.items():
        prefix = 'resources/presentation/prompts-' + family + '/'
        runtime = {name: records[prefix + name] for name in sorted(RUNTIME_FILES)}
        metadata = (json.dumps(dict(schema=schema, files=runtime,
            source_manifest_sha256=records[prefix + 'manifest.json']['sha256']), indent=2) + '\n').encode()
        for name, row in runtime.items():
            path = 'presentation/prompts-' + family + '/' + name
            files[path] = dict(row)
            payloads[path] = source / (prefix + name)
        path = 'presentation/prompts-' + family + '/manifest.json'
        files[path] = dict(bytes=len(metadata), sha256=hashlib.sha256(metadata).hexdigest())
        payloads[path] = metadata
    entry = dict(schema=PROFILE, complete=True, files=files,
                 families=list(FAMILIES), font_files_replaced=False)
    return entry, payloads


def required_bytes(root):
    return sum(row['bytes'] for row in runtime_sources(root)[0]['files'].values())


def _matches(path, records):
    before = path.stat()
    if path.is_symlink() or not path.is_file():
        raise ValueError('Controller prompt cache contains a non-file')
    with path.open('rb') as stream:
        digest = hashlib.file_digest(stream, 'sha256').hexdigest()
    after = path.stat()
    return ((before.st_size, before.st_mtime_ns) == (after.st_size, after.st_mtime_ns) and
            any(row['bytes'] == after.st_size and row['sha256'] == digest for row in records))


def _prepare(root, cache, previous, progress):
    entry, payloads = runtime_sources(root)
    files = entry['files']
    if previous is not None and (not isinstance(previous, dict) or previous.get('schema') != PROFILE or
            previous.get('complete') is not True or not isinstance(previous.get('files'), dict) or
            previous['files'].keys() != files.keys()):
        raise ValueError('Customized controller prompt provenance; cache retained')
    old = previous['files'] if previous is not None else files
    # Preflight both families before publishing anything. Known prior default
    # hashes permit upgrades; modified or extra files remain untouched. A
    # cancelled update can resume from a mix of known old/new default bytes.
    for family in FAMILIES:
        bank = _owned(cache / ('presentation/prompts-' + family), cache, directory=True)
        if not bank.exists():
            continue
        for path in bank.rglob('*'):
            _owned(path, cache, directory=path.is_dir())
            relative = path.relative_to(cache).as_posix()
            if path.is_dir() and path.name == 'atlases' and path.parent == bank:
                continue
            if relative not in files or not _matches(path, (old[relative], files[relative])):
                raise ValueError('Customized controller prompt assets; existing files retained')
    missing = sum(row['bytes'] for name, row in files.items() if not (cache / name).exists())
    if shutil.disk_usage(cache).free < max(missing, required_bytes(root)) + 1024**2:
        raise ValueError('Insufficient space for controller prompts')
    progress('Installing Xbox and PlayStation button prompts…')
    for name, row in files.items():
        path = _owned(cache / name, cache)
        if path.exists() and _matches(path, (row,)):
            continue
        path.parent.mkdir(parents=True, exist_ok=True)
        pending = path.parent / ('.controller-prompt-' + uuid.uuid4().hex)
        try:
            payload = payloads[name]
            if isinstance(payload, bytes):
                pending.write_bytes(payload)
            else:
                with payload.open('rb') as src, pending.open('xb') as dst:
                    shutil.copyfileobj(src, dst, 1024**2)
                    dst.flush();os.fsync(dst.fileno())
            if not _matches(pending, (row,)):
                raise ValueError('Controller prompt source changed while copying')
            if path.exists():
                if not _matches(path, (old[name], row)):
                    raise ValueError('Controller prompt cache changed during repair')
                os.replace(pending, path)
            else:
                _publish_new(pending, path)
        finally:
            pending.unlink(missing_ok=True)
    if runtime_sources(root)[0] != entry:
        raise ValueError('Controller prompt source changed during onboarding; retry setup')
    for name, row in files.items():
        if not _matches(_owned(cache / name, cache), (row,)):
            raise ValueError('Installed controller prompts failed verification')
    return entry


def prepare(root, cache, *, progress=print):
    """Install into the fresh importer-owned vanilla stage."""
    cache = Path(cache)
    with _cancellation(), _lock(cache):
        return _prepare(root, cache, None, progress)


def repair(root, cache, *, progress=print):
    """Repair/reuse only authenticated defaults after private-cache admission."""
    cache = Path(cache)
    with _cancellation(), _lock(cache):
        path = _owned(cache / 'provenance.json', cache)
        before = path.read_bytes()
        report = json.loads(before)
        if report.get('schema') != 'H2_MCC_TU15_PREPARATION_V1' or report.get('external_installation_requirement') != 'Halo MCC only':
            raise ValueError('Controller prompts require an owned vanilla MCC cache')
        entry = _prepare(root, cache, report.get('controller_prompts'), progress)
        updated = {**report, 'controller_prompts': entry}
        if updated == report:
            return cache, report
        pending = cache / ('.controller-provenance-' + uuid.uuid4().hex)
        try:
            pending.write_text(json.dumps(updated, indent=2) + '\n')
            if path.read_bytes() != before:
                raise ValueError('Vanilla cache provenance changed; retry onboarding')
            os.replace(pending, path)
        finally:
            pending.unlink(missing_ok=True)
        return cache, updated
