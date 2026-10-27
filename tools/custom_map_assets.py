"""Deliver registered compiled custom maps with their own resources and menu cards.

Source assets and installed caches are read-only inputs. ``prepare`` writes only
an unpublished preparation stage; ``check`` inspects existing caches without
repairing them. A model/authoring directory is never inferred to be a game map.
"""
import copy
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import struct
import uuid
import zlib

from audit_rendering import FormatError
from installation_paths import resource_root
from mcc_movies import _owned

MANIFEST = 'resources/custom-maps.json'
SCHEMA = 'H2_BUNDLED_CUSTOM_MAPS_V1'
MENU = 'halo2/h2_maps_win64_dx11/mainmenu.map'
TEXTURES = 'halo2/h2_maps_win64_dx11/textures.dat'
BINDINGS = 'frontend-map-bindings.json'
RECEIPT = 'bundled-custom-maps.json'


def _require(value, message):
    if not value:
        raise FormatError('Custom map assets: ' + message)


def _relative(value):
    _require(isinstance(value, str) and value and
             not any(c in value for c in '\\\0:\r\n\t') and
             all(p not in ('', '.', '..') for p in value.split('/')),
             'expected an exact relative filename')
    path = PurePosixPath(value)
    _require(not path.is_absolute() and path.as_posix() == value,
             'expected an exact relative filename')
    return Path(*path.parts)


def _json(path, root, *, limit=1024 * 1024):
    path = _owned(path, root)
    _require(path.is_file() and path.stat().st_size <= limit,
             'missing or oversized metadata: ' + str(path))
    try:
        result = json.loads(path.read_bytes())
    except (ValueError, UnicodeError) as error:
        raise FormatError('Custom map assets: invalid JSON: ' + str(path)) from error
    _require(isinstance(result, dict), 'metadata must be an object')
    return result


def _pin(path):
    before = path.stat()
    with path.open('rb') as stream:
        digest = hashlib.file_digest(stream, 'sha256').hexdigest()
    after = path.stat()
    identity = lambda value: (value.st_dev, value.st_ino, value.st_size,
                              value.st_mtime_ns, value.st_ctime_ns)
    _require(identity(before) == identity(after), 'source changed while hashing')
    return dict(bytes=after.st_size, sha256=digest)


def _files(root):
    result = {}
    _owned(root, root, directory=True)
    _require(root.is_dir(), 'missing compiled content: ' + str(root))
    for current, directories, names in os.walk(root, followlinks=False):
        for name in directories:
            _owned(Path(current) / name, root, directory=True)
        for name in names:
            path = _owned(Path(current) / name, root)
            _require(path.is_file(), 'nonregular content file')
            result[path.relative_to(root).as_posix()] = path
    return result


def source_maps(root):
    """Read the explicit delivery catalog; all registered payloads are required."""
    root = resource_root(root)
    manifest = _json(root / MANIFEST, root)
    _require(manifest.get('schema') == SCHEMA and isinstance(manifest.get('maps'), list)
             and len(manifest['maps']) <= 64, 'invalid delivery catalog')
    result, ids, map_ids = [], set(), set()
    for original in manifest['maps']:
        _require(isinstance(original, dict), 'invalid map entry')
        row = copy.deepcopy(original)
        identifier = row.get('id')
        _require(isinstance(identifier, str) and re.fullmatch('[a-z][a-z0-9_]{0,63}', identifier)
                 and identifier not in ids, 'invalid or duplicate map identity')
        ids.add(identifier)
        source = _relative(row.get('source'))
        _require(source.parts[:2] == ('assets', 'custom-maps'), 'map source is outside custom assets')
        map_name, profile_name = _relative(row.get('map')), _relative(row.get('profile'))
        _require(map_name.parts[:2] == ('halo2', 'Mods') and map_name.suffix == '.map',
                 'custom map must retain its separate Mods owner')
        package = _owned(root / source, root, directory=True)
        profile = _json(package / profile_name, root)
        map_id = profile.get('map_id')
        _require(type(map_id) is int and -(2**31) <= map_id < 2**31 and map_id != -1
                 and map_id not in map_ids, 'invalid or duplicate menu map id')
        map_ids.add(map_id)
        for field in ('name', 'description', 'scenario_path', 'preview'):
            _require(isinstance(profile.get(field), str) and profile[field], 'missing menu ' + field)
        _require(profile['scenario_path'].split('\\')[-1] + '.map' == map_name.name,
                 'map filename differs from its menu scenario')
        preview = profile_name.parent / _relative(profile['preview'])
        _owned(package / preview, root)
        _require((package / preview).is_file(), 'missing map preview')
        aliases = row.get('resource_aliases')
        _require(isinstance(aliases, list) and len(aliases) <= 16 and len(set(aliases)) == len(aliases),
                 'invalid custom resource aliases')
        for name in aliases:
            path = _relative(name)
            _require(path.parent.as_posix() == 'halo2/h2_maps_win64_dx11' and path.suffix == '.dat',
                     'resource alias is outside custom shared directory')
        content = _files(package / 'content')
        for name in (row['map'], 'provenance.json', 'custom-owner-receipt.json',
                     'tag-conversion.json', 'native-shaders/map-owners.json', *aliases):
            _require(name in content, 'missing prepared custom owner: ' + name)
        row.update(profile_data=profile, preview=preview.as_posix())
        result.append(row)
    return result


def source_bundle(root):
    """Hash runtime sources, excluding authoring, evidence and engine snapshots."""
    root = resource_root(root)
    files = {MANIFEST: root / MANIFEST}
    rows = source_maps(root)
    for row in rows:
        source = row['source']
        files[source + '/' + row['profile']] = root / source / row['profile']
        files[source + '/' + row['preview']] = root / source / row['preview']
        for name, path in _files(root / source / 'content').items():
            files[source + '/content/' + name] = path
    pins, inodes = {}, {}
    for name, path in sorted(files.items()):
        _owned(path, root)
        stat = path.stat()
        key = stat.st_dev, stat.st_ino, stat.st_size, stat.st_mtime_ns, stat.st_ctime_ns
        if key not in inodes:
            inodes[key] = _pin(path)
        pins[name] = dict(inodes[key])
    for row in rows:
        prefix = row['source'] + '/content/'
        pin = pins[prefix + row['map']]
        receipt = _json(root / prefix / 'custom-owner-receipt.json', root)
        _require(receipt.get('map') == row['map'] and receipt.get('input_bytes') == pin['bytes']
                 and receipt.get('input_sha256') == pin['sha256'], 'custom map differs from its owner receipt')
        _require(all(pins[prefix + alias] == pin for alias in row['resource_aliases']),
                 'custom resource alias differs from its compiled map')
        content = root / row['source'] / 'content'
        provenance = _json(content / 'provenance.json', root, limit=16 * 1024 * 1024)
        _verify_source_index(content, provenance)
    return pins


def required_bytes(root):
    # Conservative across filesystems that cannot retain hardlink aliases.
    return sum(row['bytes'] for row in source_bundle(root).values())


def copy_sources(root, destination, verified=None):
    """Copy deliverable sources to a package, retaining relative source layout."""
    root, destination = resource_root(root), Path(destination).absolute()
    pins = source_bundle(root)
    _require(verified is None or verified == pins, 'source inventory changed after preflight')
    pending = []
    # Verify every existing target before publishing any missing file.
    for name, pin in pins.items():
        path = _owned(destination / name, destination)
        if path.exists():
            _require(_pin(path) == pin, 'existing packaged custom asset retained: ' + name)
        else:
            pending.append(name)
    copied = {}
    for name in pending:
        path = destination / name
        for parent in reversed(path.parents):
            if parent == destination or parent.is_relative_to(destination):
                _owned(parent, destination, directory=True).mkdir(exist_ok=True)
        key = pins[name]['bytes'], pins[name]['sha256']
        temporary = path.with_name('.custom-map-' + uuid.uuid4().hex)
        try:
            if key in copied:
                try:
                    os.link(copied[key], temporary)
                except OSError:
                    shutil.copyfile(root / name, temporary)
            else:
                shutil.copyfile(root / name, temporary)
            _require(_pin(temporary) == pins[name], 'packaged custom asset changed during copy')
            os.replace(temporary, path)
            copied[key] = path
        finally:
            temporary.unlink(missing_ok=True)
    _require(source_bundle(root) == pins, 'custom sources changed during packaging')
    return dict(schema=SCHEMA, files=pins, maps=[row['id'] for row in source_maps(root)])


def _save(path, value):
    temporary = path.with_name('.custom-map-' + uuid.uuid4().hex)
    try:
        temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def _binding(row):
    base = 'custom-maps/' + row['id']
    return dict(map_id=row['profile_data']['map_id'],
        scenario_path=row['profile_data']['scenario_path'], cache_root=base,
        cache_file=base + '/' + row['map'], shared_maps=base + '/halo2/h2_maps_win64_dx11')


def _bindings(cache):
    path = _owned(cache / BINDINGS, cache)
    if not path.exists():
        return dict(schema='H2_FRONTEND_MAP_BINDINGS_V1', maps=[])
    result = _json(path, cache, limit=65536)
    _require(result.get('schema') == 'H2_FRONTEND_MAP_BINDINGS_V1' and
             isinstance(result.get('maps'), list) and len(result['maps']) <= 64,
             'invalid existing frontend map bindings')
    return result


def _normalize_source_index(previous, expected):
    """Only the policy-derived stock receipt may change in historical inputs."""
    normalized = copy.deepcopy(previous)
    expected_maps = {row['map']: row for row in expected['maps']}
    changes = []
    for row in normalized['maps']:
        authority = row['compatibility_authority']
        current = expected_maps.get(row['map'], {}).get('compatibility_authority', {})
        if (authority.get('scope') == 'stock-conversion-owner' and
                authority.get('producer') == current.get('producer') == 'mcc_native_map_owner_publication' and
                authority.get('profile') == current.get('profile') and
                authority.get('receipt_sha256') != current.get('receipt_sha256')):
            changes.append(dict(map=row['map'], old_sha256=authority['receipt_sha256'],
                                new_sha256=current['receipt_sha256']))
            authority['receipt_sha256'] = current['receipt_sha256']
    from mcc_native_map_owners import validate_index
    validate_index(normalized, expected)
    return changes


def _verify_source_index(source, provenance):
    # Historical packages retain the policy fingerprint from publication time.
    # _prepare authenticates their pointer, all source/final map bytes, native
    # banks, custom receipt and dependencies before this exact comparison. Only
    # the deterministic stock policy receipt is allowed to acquire today's pin.
    from mcc_native_map_owner_publication import _prepare, POLICIES
    _, expected, _, _, previous, _ = _prepare(source, provenance, (), POLICIES)
    _require(previous is not None, 'source custom map has no authenticated owner index')
    old = json.loads(previous)
    changes = _normalize_source_index(old, expected)
    from preserve_native_shaders import read_selected_library
    for row in expected['maps']:
        if row['native_mode'] == 'owned-bank':
            read_selected_library(source / row['bank_directory'], row['bank_namespace'])
    return dict(source_index=_pin(source / 'native-shaders/map-owners.json'),
                derived_stock_policy_receipts=changes)


def check(root, cache):
    """Return missing delivery IDs without changing an already installed cache."""
    cache = Path(cache).absolute()
    bindings = _bindings(cache)
    missing = []
    for row in source_maps(root):
        expected = _binding(row)
        matches = [value for value in bindings['maps'] if value.get('map_id') == expected['map_id']]
        if not matches:
            missing.append(row['id'])
            continue
        _require(matches == [expected], 'existing map binding differs; authored content retained')
        nested = _owned(cache / expected['cache_root'], cache, directory=True)
        provenance = _json(nested / 'provenance.json', cache)
        from mcc_native_map_owner_publication import verify_owner_index
        _require(verify_owner_index(nested, provenance) is not None,
                 'installed custom map has no authenticated owner index')
    return missing


def compatible(root, cache, report=None):
    """Whether a reused cache contains the current registered source cohort."""
    cache = Path(cache).absolute()
    rows = source_maps(root)
    if not rows:
        return True
    if check(root, cache):
        return False
    path = _owned(cache / RECEIPT, cache)
    if not path.exists():
        return False
    receipt = _json(path, cache, limit=16 * 1024 * 1024)
    _require(receipt.get('schema') == SCHEMA and receipt.get('complete') is True,
             'invalid installed delivery receipt')
    return receipt.get('source_files') == source_bundle(root)


def verify(cache, report):
    """Authenticate an installed delivery without requiring its source cohort."""
    cache = Path(cache).absolute()
    declaration = report.get('bundled_custom_maps')
    if declaration is None:
        return None
    _require(isinstance(declaration, dict) and declaration.get('schema') == SCHEMA
             and declaration.get('complete') is True and declaration.get('path') == RECEIPT,
             'invalid prepared delivery declaration')
    receipt_path = _owned(cache / RECEIPT, cache)
    _require(receipt_path.is_file() and _pin(receipt_path) == declaration.get('receipt'),
             'prepared custom map receipt changed')
    bindings_path = _owned(cache / BINDINGS, cache)
    _require(bindings_path.is_file() and _pin(bindings_path) == declaration.get('bindings'),
             'prepared custom map bindings changed')
    receipt = _json(receipt_path, cache, limit=16 * 1024 * 1024)
    _require(receipt.get('schema') == SCHEMA and receipt.get('complete') is True
             and receipt.get('maps') == declaration.get('maps')
             and isinstance(receipt.get('map_bindings'), list), 'prepared custom map receipt differs')
    bindings = _bindings(cache)
    from mcc_content import MccCache
    from mcc_multiplayer_catalog import catalog, summary
    from mcc_native_map_owner_publication import verify_owner_index
    menu = MccCache(_owned(cache / MENU, cache))
    try:
        tag, at = menu.tag(menu.u32(menu.index + 16), 'matg')
        cards = [summary(bytes(menu.data[at:at + 0xc64]))
                 for at in catalog(menu, tag, at + 0x178)]
        for binding in receipt['map_bindings']:
            _require(binding in bindings['maps'], 'prepared custom map binding is missing')
            nested = _owned(cache / _relative(binding['cache_root']), cache, directory=True)
            _require(binding['cache_root'].startswith('custom-maps/')
                     and binding['cache_file'].startswith(binding['cache_root'] + '/halo2/Mods/')
                     and binding['shared_maps'] == binding['cache_root'] + '/halo2/h2_maps_win64_dx11',
                     'prepared custom map lost its separate resource owner')
            map_path = _owned(cache / _relative(binding['cache_file']), cache)
            provenance = _json(nested / 'provenance.json', cache, limit=16 * 1024 * 1024)
            _require(verify_owner_index(nested, provenance) is not None,
                     'prepared custom map lost its authenticated owner index')
            matches = [card for card in cards if card['map_id'] == binding['map_id']]
            _require(len(matches) == 1 and matches[0]['scenario'] == binding['scenario_path'],
                     'prepared custom map differs from its menu card')
            custom = MccCache(map_path)
            try:
                scenario, at = custom.tag(custom.u32(custom.index + 12), 'scnr')
                records = catalog(custom, scenario, at + 0x398)
                _require(len(records) == 1, 'prepared custom map has ambiguous scenario card')
                card = summary(bytes(custom.data[records[0]:records[0] + 0xc64]))
                _require(card['map_id'] == binding['map_id'] and card['scenario'] == binding['scenario_path'],
                         'prepared custom scenario differs from its binding')
            finally:
                custom.close()
    finally:
        menu.close()
    return receipt


def prepare(root, cache, *, progress=lambda _: None):
    """Install maps/cards into an unpublished stage, preserving existing entries.

    Never call this to mutate a selected cache. For reuse call ``check`` first;
    a missing map requires a fresh preparation stage and ordinary publication.
    """
    from PIL import Image
    from author_dire_valley_menu import AppendTags, record_text, write_cache
    from mcc_content import MccCache
    from mcc_multiplayer_catalog import bitmap, catalog, summary
    from mcc_native_map_owners import bank_identity
    from mcc_native_map_owner_publication import publish_staged_owner_index, verify_owner_index

    root, cache = resource_root(root), Path(cache).absolute()
    pins = source_bundle(root)
    rows = source_maps(root)
    missing = set(check(root, cache))
    bindings = _bindings(cache)
    if not missing:
        return dict(schema=SCHEMA, complete=True, maps=[row['id'] for row in rows])
    provenance = _json(cache / 'provenance.json', cache, limit=16 * 1024 * 1024)
    verify_owner_index(cache, provenance)
    source_migrations = {}
    for row in rows:
        if row['id'] not in missing:
            continue
        progress('Installing custom map: ' + row['profile_data']['name'] + '…')
        binding = _binding(row)
        nested = _owned(cache / binding['cache_root'], cache, directory=True)
        _require(not nested.exists(), 'unregistered custom map directory retained: ' + str(nested))
        source = root / row['source'] / 'content'
        source_provenance = _json(source / 'provenance.json', root, limit=16 * 1024 * 1024)
        source_migrations[row['id']] = _verify_source_index(source, source_provenance)
        with Image.open(root / row['source'] / row['preview']) as image:
            image = image.convert('RGBA')
            _require(image.size == (220, 207), 'map preview must be220x207 pixels')
            pixels = image.tobytes('raw', 'BGRA')
        encoded = zlib.compress(pixels, 9)
        partition = struct.pack('<ii', 1, len(encoded)) + encoded
        menu = MccCache(cache / MENU)
        tag, at = menu.tag(menu.u32(menu.index + 16), 'matg')
        entries = catalog(menu, tag, at + 0x178)
        _require(not any(menu.u32(at) == row['profile_data']['map_id'] for at in entries),
                 'menu already contains an unregistered custom map; retained')
        free = next((at for at in entries if menu.u32(at) == 0xffffffff), None)
        _require(free is not None, 'menu has no free authored map card')
        menu_datum = menu.u32(free + 8)
        slot, frame_at, frame = bitmap(menu, menu_datum)
        _require(slot['name'].startswith('ui\\code_global_bitmaps\\multiplayer'),
                 'free menu preview has no reserved bitmap owner')
        template = bytes(menu.data[slot['offset']:slot['offset'] + 76]) + frame
        # Copy only immutable source inputs. Aliases in the destination share
        # its own map inode; no mutable cache file links to a resource source.
        nested.parent.mkdir(parents=True, exist_ok=True)
        shutil.copytree(source, nested, ignore=shutil.ignore_patterns(*[Path(p).name for p in row['resource_aliases']]))
        custom_path = nested / row['map']
        custom = MccCache(custom_path)
        scenario, at = custom.tag(custom.u32(custom.index + 12), 'scnr')
        cards = catalog(custom, scenario, at + 0x398)
        _require(len(cards) == 1 and custom.u32(cards[0]) == row['profile_data']['map_id'],
                 'custom scenario has a different map identity')
        card = bytearray(custom.data[cards[0]:cards[0] + 0xc64])
        record_text(card, row['profile_data'])
        _require(summary(card)['scenario'] == row['profile_data']['scenario_path'],
                 'custom scenario differs from its authored menu profile')
        # AppendTags uses this module-level name to create the new tag.
        # Set it locally through a subclass to preserve the existing helper's
        # fixed Dire Valley public interface and avoid global mutation.
        class MapTags(AppendTags):
            def image(self, template, partition):
                datum = super().image(template, partition)
                name = ('ui\\global_bitmaps\\map_shots\\map_mp_' + row['id']).encode() + b'\0'
                # The helper appends exactly one final name and offset.
                offset = struct.unpack_from('<I', self.offsets, len(self.offsets) - 4)[0]
                self.names[offset:] = name
                return datum
        builder = MapTags(custom)
        datum = builder.image(template, partition)
        struct.pack_into('<I', card, 8, datum)
        custom.data[cards[0]:cards[0] + len(card)] = card
        builder.finish(custom_path)
        custom.close()
        for name in row['resource_aliases']:
            alias = nested / name
            try:
                os.link(custom_path, alias)
            except OSError:
                shutil.copyfile(custom_path, alias)
        before = {name: _pin(cache / name) for name in (MENU, TEXTURES)}
        textures = cache / TEXTURES
        temporary = textures.with_name('.custom-map-' + uuid.uuid4().hex)
        try:
            shutil.copyfile(textures, temporary)
            with temporary.open('ab') as stream:
                stream.write(bytes((-stream.tell()) % 16))
                pixel_at = stream.tell()
                _require(pixel_at + len(partition) < 0x40000000, 'preview exceeds bitmap bank range')
                stream.write(partition)
            os.replace(temporary, textures)
        finally:
            temporary.unlink(missing_ok=True)
        struct.pack_into('<I', menu.data, frame_at + 56, pixel_at)
        struct.pack_into('<I', menu.data, frame_at + 80, len(partition))
        struct.pack_into('<I', card, 8, menu_datum)
        card[4:12] = menu.data[free + 4:free + 12]
        menu.data[free:free + len(card)] = card
        write_cache(menu, cache / MENU)
        menu.close()
        nested_provenance = copy.deepcopy(source_provenance)
        pin = _pin(custom_path)
        nested_provenance['source_files'][row['map'].removeprefix('halo2/')] = pin
        nested_provenance['native_shader_library'].pop('owner_bindings', None)
        (nested / 'native-shaders/map-owners.json').unlink()
        receipt = _json(nested / 'custom-owner-receipt.json', nested)
        bank = bank_identity(receipt['bank_directory'], (nested / receipt['bank_directory'] / 'library.json').read_bytes())
        receipt.update(input_bytes=pin['bytes'], input_sha256=pin['sha256'], source_sha256=pin['sha256'],
                       producer='custom_map_assets.py', profile=SCHEMA,
                       **{key: bank[key] for key in ('bank_directory', 'bank_namespace', 'bank_library_sha256')})
        raw = (json.dumps(receipt, sort_keys=True) + '\n').encode()
        (nested / 'custom-owner-receipt.json').write_bytes(raw)
        nested_provenance = publish_staged_owner_index(nested, nested_provenance, custom_receipts=[raw])
        _save(nested / 'provenance.json', nested_provenance)
        for name in (MENU, TEXTURES):
            provenance.setdefault('graphics_corrections', {}).setdefault('changes', []).append(dict(
                path=name, donor_sha256=before[name]['sha256'], converted_sha256=_pin(cache / name)['sha256'],
                producer='custom_map_assets.py', profile=SCHEMA))
        bindings['maps'].append(binding)
    _require(source_bundle(root) == pins, 'custom map sources changed during preparation')
    # This stage's old index was authenticated before writing. The fresh-stage
    # publisher derives the final map hashes and chain from actual output bytes.
    provenance.setdefault('native_shader_library', {}).pop('owner_bindings', None)
    (cache / 'native-shaders/map-owners.json').unlink(missing_ok=True)
    provenance = publish_staged_owner_index(cache, provenance)
    entry = dict(schema=SCHEMA, complete=True, maps=[row['id'] for row in rows],
                 map_bindings=[_binding(row) for row in rows],
                 source_index_migrations=source_migrations,
                 source_files=pins, ownership_only=True, full_gameplay_parity=False)
    _save(cache / BINDINGS, bindings)
    _save(cache / RECEIPT, entry)
    provenance['bundled_custom_maps'] = dict(schema=SCHEMA, complete=True, path=RECEIPT,
        receipt=_pin(cache / RECEIPT), bindings=_pin(cache / BINDINGS),
        maps=entry['maps'], ownership_only=True)
    _save(cache / 'provenance.json', provenance)
    _require(not check(root, cache), 'custom map delivery did not complete')
    verify(cache, provenance)
    return entry
