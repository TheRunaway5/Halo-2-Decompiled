"""Discover installed MCC Halo 2 Workshop missions and prepare private owners.

Steam subscriptions are read-only. Each prepared mission owns its map, resource
directory and native template bank; no stock map is replaced or impersonated.
Preparation is compatibility plumbing, not a claim of complete mod parity.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import itertools
import json
import os
from pathlib import Path
import re
import shutil
import struct
import sys
import tempfile

from audit_rendering import FormatError
from steam_client import MCC, libraries, host_path
from mcc_movies import _cancellation, _publish_new

SCHEMA = 'H2_STEAM_WORKSHOP_V1'
PROFILE = 'H2_WORKSHOP_PREPARATION_V1'
RESOURCE_PROFILE = 'H2_WORKSHOP_RESOURCE_OWNERS_V1'
RESOURCE_BANKS = ('textures.dat', 'sounds_en.dat', 'sounds_neutral.dat', 'sounds_remastered.dat')
MAX_METADATA = 1024 * 1024


def require(value, message):
    if not value:
        raise FormatError('Workshop: ' + message)


def relative(value):
    require(isinstance(value, str) and value and len(value) < 1024 and
            not any(c in value for c in '\\:\0\r\n') and
            all(part not in ('', '.', '..') for part in value.split('/')),
            'invalid relative content path')
    path = Path(value)
    require(not path.is_absolute(), 'absolute content path')
    return path


def owned(root, value):
    path = root
    for part in relative(value).parts:
        path /= part
        require(not path.is_symlink(), 'symlink in mod content: ' + value)
    require(path.resolve().is_relative_to(root), 'content escapes mod directory')
    return path


def source_owned(root, value):
    """Resolve Windows-authored source spelling without ambiguous aliases."""
    path = root
    for part in relative(value).parts:
        if path.is_dir():
            matches = [entry for entry in path.iterdir() if entry.name.casefold() == part.casefold()]
            require(len(matches) <= 1, 'ambiguous source filename case: ' + value)
            path = matches[0] if matches else path / part
        else:
            path /= part
        require(not path.is_symlink(), 'symlink in mod content: ' + value)
    require(path.resolve().is_relative_to(root), 'content escapes mod directory')
    return path


def metadata(root, name, *, source=False):
    path = source_owned(root, name) if source else owned(root, name)
    require(path.is_file() and path.stat().st_size <= MAX_METADATA,
            'missing or oversized metadata: ' + name)
    def pairs(rows):
        result = {}
        for key, value in rows:
            require(key not in result, 'duplicate metadata key: ' + key)
            result[key] = value
        return result
    value = json.loads(path.read_text(encoding='utf-8-sig'), object_pairs_hook=pairs)
    require(isinstance(value, dict), 'metadata must be an object')
    return value


def localized(value, fallback=''):
    text = value.get('Neutral', fallback) if isinstance(value, dict) else fallback
    return text[:2048] if isinstance(text, str) else fallback


def roots(*, home=None, environment=None, extra_libraries=()):
    env = os.environ if environment is None else environment
    candidates = []
    if env.get('HALO2_WORKSHOP_ROOT'):
        candidates.append(host_path(env['HALO2_WORKSHOP_ROOT'], env))
    candidates.extend(library / f'steamapps/workshop/content/{MCC}'
                      for library in libraries(home=home, environment=env,
                                               extra_libraries=extra_libraries))
    seen = set()
    for path in candidates:
        try:
            path = path.expanduser().resolve(strict=True)
            if path.is_dir() and path not in seen:
                seen.add(path)
                yield path
        except OSError:
            pass


def read_mod(root):
    root = root.resolve(strict=True)
    info = metadata(root, 'ModInfo.json', source=True)
    if info.get('Engine') != 'Halo2':
        return None
    identifier = info.get('ModIdentifier', {})
    require(isinstance(identifier, dict) and isinstance(identifier.get('HostedModIds', {}), dict),
            'invalid Workshop identity metadata')
    identity = identifier.get('HostedModIds', {}).get('SteamWorkshopId')
    require(type(identity) is int and str(identity) == root.name and identity > 0,
            'Workshop item identity differs from its directory')
    result = dict(id=str(identity), directory=str(root),
                  title=localized(info.get('Title'), str(identity)),
                  description=localized(info.get('Description')), missions=[], error='')
    try:
        contents = info.get('GameModContents', {})
        require(isinstance(contents, dict) and
                all(type(contents.get(key, False)) is bool for key in ('HasCampaign', 'HasSharedFiles')),
                'invalid game content flags')
        result['shared_files'] = contents.get('HasSharedFiles', False)
        result['inherit_shared_files'] = info.get('InheritSharedFiles', 'FromMCC')
        require(info.get('InheritSharedFiles', 'FromMCC') in ('FromMCC', 'None'),
                'unsupported shared-file inheritance')
        rows = []
        if contents.get('HasCampaign'):
            campaign = metadata(root, 'CampaignInfo.json', source=True)
            entries = campaign.get('CampaignMaps')
            require(isinstance(entries, list) and len(entries) <= 256,
                    'invalid campaign mission list')
            rows.extend((name, True) for name in entries)
        multiplayer = contents.get('MultiplayerMaps')
        if multiplayer is None:
            directory = source_owned(root, 'multiplayer')
            multiplayer = [path.relative_to(root).as_posix() for path in sorted(directory.glob('*.json'))] if directory.is_dir() else []
        require(isinstance(multiplayer, list) and len(multiplayer) <= 256, 'invalid multiplayer map list')
        rows.extend((name, False) for name in multiplayer)
        seen = set()
        for name, campaign in rows:
            row = metadata(root, str(relative(name)), source=True)
            scenario = row.get('ScenarioFile')
            require(isinstance(scenario, str) and
                    re.fullmatch(r'[A-Za-z0-9_ -]{1,128}', scenario),
                    'invalid scenario filename')
            require(scenario not in seen, 'duplicate mission filename')
            seen.add(scenario)
            path = source_owned(root, 'maps/' + scenario + '.map')
            require(path.is_file() and 896 <= path.stat().st_size <= 1024**3,
                    'mission map is absent, incomplete or oversized: ' + scenario)
            with path.open('rb') as stream:
                require(stream.read(8) == struct.pack('<II', 0x68656164, 13),
                        'mission is not an MCC Halo 2 Classic map: ' + scenario)
            result['missions'].append(dict(item=str(root), scenario=scenario,
                title=localized(row.get('Title'), scenario),
                description=localized(row.get('Description')), campaign=campaign, metadata=str(relative(name))))
        require(result['missions'], 'no installed campaign or multiplayer missions')
    except (OSError, ValueError, TypeError, FormatError) as error:
        result['missions'] = []
        result['error'] = str(error)
    return result


def scan(**kwargs):
    found, errors = [], []
    directories = list(roots(**kwargs))
    for directory in directories:
        try:
            # Bound enumeration, including malformed/non-Halo-2 subscriptions.
            for index, entry in enumerate(sorted(itertools.islice(directory.iterdir(), 4097))):
                require(index < 4096, 'too many installed Workshop entries')
                if not entry.name.isdecimal() or entry.is_symlink() or not entry.is_dir():
                    continue
                try:
                    mod = read_mod(entry)
                    if mod is not None:
                        found.append(mod)
                except (OSError, ValueError, TypeError, FormatError) as error:
                    errors.append(entry.name + ': ' + str(error))
        except OSError as error:
            errors.append(str(error))
    found.sort(key=lambda row: (row['title'].casefold(), row['id'], row['directory']))
    return dict(schema=SCHEMA, roots=list(map(str, directories)), mods=found, errors=errors)


def pin(path):
    before = path.stat()
    with path.open('rb') as stream:
        digest = hashlib.file_digest(stream, 'sha256').hexdigest()
    after = path.stat()
    require((before.st_dev, before.st_ino, before.st_size, before.st_mtime_ns, before.st_ctime_ns) ==
            (after.st_dev, after.st_ino, after.st_size, after.st_mtime_ns, after.st_ctime_ns),
            'input changed while reading: ' + str(path))
    return dict(bytes=after.st_size, sha256=digest)


def save(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, sort_keys=True, indent=2) + '\n', encoding='utf-8')


def copy_input(source, target, expected):
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, target)
    require(pin(target) == expected and pin(source) == expected,
            'input changed during private preparation')


def resource_inputs(item, base, mod):
    """Select physical owners, never a bank that merely happens to decode.

    MCC's authored halo2/h2_maps_win64_dx11 bank takes precedence over auxiliary
    DATs beside maps (Digtacular ships both, with different offset namespaces).
    Without that bank, map-local DATs or the decoded map own local resources.
    A tag loaded from shared.map independently keeps its shared resource owner.
    """
    nested = 'halo2/h2_maps_win64_dx11/'
    maps = {}
    for name in ('shared.map', 'single_player_shared.map', 'mainmenu.map'):
        candidates = [source_owned(item, 'maps/' + name), source_owned(item, nested + name)]
        if mod['inherit_shared_files'] == 'FromMCC':
            candidates.append(base / 'halo2/h2_maps_win64_dx11' / name)
        maps[name] = next((p for p in candidates if p.is_file()), None)
        require(maps[name] is not None, 'prepared MCC resource is absent: ' + name)
    local, inherited = {}, {}
    for name in RESOURCE_BANKS:
        local[name] = next((p for p in (source_owned(item, nested + name),
                                       source_owned(item, 'maps/' + name)) if p.is_file()), None)
        shared = maps['shared.map']
        candidates = [shared.parent / name]
        if shared.is_relative_to(item):
            candidates.append(source_owned(item, nested + name))
        inherited[name] = next((p for p in candidates if p.is_file()), shared)
        for p in (local[name], inherited[name]):
            require(p is None or (not p.is_symlink() and 0 < p.stat().st_size <= 2 * 1024**3),
                    'missing, linked or oversized resource bank: ' + name)
    return dict(maps=maps, local=local, inherited=inherited)


def template_bank(base, destination, namespace, wanted):
    """Retain native templates; authored mod material instances remain intact."""
    from preserve_native_shaders import read_selected_library
    records, sources, first = {}, {}, None
    # Campaign templates first, with deterministic remaining-map closure.
    names = ['03b_newmombasa', '03a_oldmombasa', '01b_spacestation', 'coagulation']
    names += sorted(p.name for p in (base / 'native-shaders').iterdir()
                    if (p / 'library.json').is_file() and p.name not in names)
    dependencies = []
    for name in names:
        directory = base / 'native-shaders' / name
        if not (directory / 'library.json').is_file():
            continue
        library, _ = read_selected_library(directory, name)
        if first is None:
            first = copy.deepcopy(library)
        dependencies.append(dict(map=name, **pin(directory / 'library.json')))
        for record in library['shader_tags']:
            key = record['group'], record['name']
            if record['group'] == 'shad' or key in records:
                continue
            records[key] = copy.deepcopy(record)
            sources[record['payload']] = directory / 'payloads' / record['payload']
    require(first is not None, 'prepare MCC before launching Workshop missions')
    # Include the transitive template/pass/program closure but no stock shad
    # instance; a same-named mod may deliberately change its material values.
    pending = set(wanted)
    # Global consumers (notably the decal scene) obtain their programs from
    # the retained rasterizer bank, independently of any material template.
    # Relocate that bank's vertex closure too; leaving raw source handles here
    # can alias unrelated mod tags or fail on the first campaign frame.
    pending.update(tuple(ref[1]) for ref in first['rasterizer']['references']
                   if ref[1][0] in ('stem', 'spas', 'vrtx'))
    selected = {}
    while pending:
        key = pending.pop()
        if key in selected or key not in records:
            continue
        record = records[key]
        selected[key] = record
        pending.update(tuple(ref[1]) for ref in record['references']
                       if ref[1][0] in ('stem', 'spas', 'vrtx'))
    first.update(map=namespace, shader_tags=[selected[key] for key in sorted(selected)],
                 custom_authoring=dict(producer=PROFILE, material_instances='authored MCC mod'))
    for record in [*first['shader_tags'], first['rasterizer']]:
        source = sources.get(record['payload'])
        if source is None:
            source = base / 'native-shaders' / dependencies[0]['map'] / 'payloads' / record['payload']
        target = destination / 'payloads' / record['payload']
        if not target.exists():
            copy_input(source, target, pin(source))
    save(destination / 'library.json', first)
    tables = base / 'native-shaders/function-tables.bin'
    copy_input(tables, destination / 'function-tables.bin', pin(tables))
    return dependencies


def prepare(item, scenario, base, output, *, input_identity=None):
    """Publish one private, content-addressed map; interrupted stages disappear."""
    from mcc_content import MccCache
    from mcc_native_map_owners import bank_identity, BANK_FIELDS
    from mcc_native_map_owner_publication import publish_staged_owner_index, verify_owner_index
    item, base = Path(item).resolve(strict=True), Path(base).resolve(strict=True)
    mod = read_mod(item)
    require(mod is not None and not mod['error'], mod['error'] if mod else 'not a Halo 2 item')
    mission = next((row for row in mod['missions'] if row['scenario'] == scenario), None)
    require(mission is not None, 'mission is not declared by this Workshop item')
    source = source_owned(item, 'maps/' + scenario + '.map')
    source_pin = pin(source)
    output = Path(output).absolute()
    require(not output.exists(), 'output already exists; existing content was retained')
    resources = resource_inputs(item, base, mod)
    output.parent.mkdir(parents=True, exist_ok=True)
    with _cancellation(), tempfile.TemporaryDirectory(prefix='.workshop-', dir=output.parent) as temporary:
        stage = Path(temporary)
        shared = stage / 'halo2/h2_maps_win64_dx11'
        shared.mkdir(parents=True)
        relative_map = f'halo2/Mods/{mod["id"]}/{scenario}.map'
        target = stage / relative_map
        copy_input(source, target, source_pin)
        cache = MccCache(target)
        try:
            tag = cache.tags[cache.u32(cache.index + 12)]
            # ScenarioFile selects a physical cache through authenticated local
            # metadata. Its internal scnr path is separately authored: the E3
            # low-spec cache deliberately retains earthcity_e3's scenario name.
            # Keep that name and all tag bytes; never rename/impersonate a map.
            require(tag['group'] == 'scnr', 'selected map has no scenario tag')
            require(cache.u16(tag['offset'] + 0x10) == (0 if mission['campaign'] else 1),
                    'map scenario type differs from its mission metadata')
            wanted = {(t['group'], t['name']) for t in cache.tags.values()
                      if t['group'] in ('stem', 'spas', 'vrtx')}
            needs_inherited = any(not t['address'] for t in cache.tags.values())
            if needs_inherited:
                inherited_cache = MccCache(resources['maps']['shared.map'])
                try:
                    slots = {t['datum'] & 65535: t for t in inherited_cache.tags.values() if t['address']}
                    for tag in cache.tags.values():
                        inherited = slots.get(tag['datum'] & 65535)
                        if inherited is not None:
                            require(all(tag[k] == inherited[k] for k in ('datum', 'group', 'name')),
                                    'shared tag datum/group/path collision: ' + tag['name'])
                        if not tag['address']:
                            require(inherited is not None, 'shared tag is absent: ' + tag['name'])
                    wanted.update((t['group'], t['name']) for t in inherited_cache.tags.values()
                                  if t['group'] in ('stem', 'spas', 'vrtx'))
                finally:
                    inherited_cache.close()
            # Embedded locators address the decoded container. Materialize that
            # exact container when Steam supplied a compressed cache; dat aliases
            # must not point at its encoded chunk table instead of the resources.
            if cache.u16(28) & 1:
                struct.pack_into('<H', cache.data, 28, cache.u16(28) & ~1)
                target.write_bytes(cache.data)
            decoded_pin = dict(bytes=len(cache.data), sha256=hashlib.sha256(cache.data).hexdigest())
        finally:
            cache.close()
        output_pin = pin(target)
        inventory = {str(target.relative_to(stage / 'halo2')): source_pin}
        converted = []
        for name in ('shared.map', 'single_player_shared.map', 'mainmenu.map'):
            source_shared = resources['maps'][name]
            shared_pin = pin(source_shared)
            copy_input(source_shared, shared / name, shared_pin)
            inventory['h2_maps_win64_dx11/' + name] = shared_pin
            converted.append(dict(map=name, role='resources', source_sha256=shared_pin['sha256'],
                                  output_sha256=shared_pin['sha256']))
        copied = {}
        resource_records = {}
        for owner in (('local', 'inherited') if needs_inherited else ('local',)):
            for name, resource in resources[owner].items():
                destination = shared / (name if owner == 'local' else 'inherited-resources/' + name)
                destination.parent.mkdir(parents=True, exist_ok=True)
                if resource is None:
                    os.link(target, destination)
                    resource_records[owner + '/' + name] = dict(source=str(source), embedded=True, **output_pin)
                    continue
                resource_pin = pin(resource)
                if resource in copied:
                    os.link(copied[resource], destination)
                elif resource.suffix.lower() == '.map':
                    # Embedded resources address the decoded container, just
                    # as local scenario resources do. Keep the map copy intact.
                    data = MccCache(resource)
                    try:
                        struct.pack_into('<H', data.data, 28, data.u16(28) & ~1)
                        destination.write_bytes(data.data)
                    finally:
                        data.close()
                else:
                    copy_input(resource, destination, resource_pin)
                copied[resource] = destination
                require(pin(resource) == resource_pin, 'resource changed during private preparation')
                inventory[str(destination.relative_to(stage / 'halo2'))] = pin(destination)
                resource_records[owner + '/' + name] = dict(source=str(resource), input=resource_pin,
                                                            prepared=pin(destination))
        shutil.copytree(base / 'halo2/h2_fonts', stage / 'halo2/h2_fonts')
        bankdir = 'native-shaders/maps/workshop'
        dependencies = template_bank(base, stage / bankdir, 'workshop', wanted)
        tables = stage / bankdir / 'function-tables.bin'
        copy_input(tables, stage / 'native-shaders/function-tables.bin', pin(tables))
        save(stage / 'native-shaders/catalog.json', dict(schema='H2_PRESERVED_NATIVE_SHADER_CATALOG_V1', maps=[]))
        provenance = dict(schema='H2_MCC_TU15_PREPARATION_V1', external_installation_requirement='Halo MCC only',
                          source_files=inventory, native_shader_library=dict(maps={}),
                          workshop=dict(profile=PROFILE, item=str(item), id=mod['id'], scenario=scenario,
                                        source=source_pin, prepared_map=output_pin,
                                        decoded=decoded_pin, templates=dependencies,
                                        resource_profile=RESOURCE_PROFILE, resource_files=resource_records))
        if output_pin != source_pin:
            provenance['output_changes'] = [dict(path=relative_map, donor_sha256=source_pin['sha256'],
                converted_sha256=output_pin['sha256'], operation='decoded-embedded-resource-owner')]
        save(stage / 'tag-conversion.json', dict(profile=provenance['schema'], maps=converted))
        save(stage / 'provenance.json', provenance)
        bank = bank_identity(bankdir, (stage / bankdir / 'library.json').read_bytes())
        receipt = dict(map=relative_map, input_bytes=output_pin['bytes'], input_sha256=output_pin['sha256'],
                       source_sha256=source_pin['sha256'], cache_role='scenario',
                       shared_directory='halo2/h2_maps_win64_dx11', native_mode='owned-bank',
                       bank_owner_map=relative_map, producer='steam_workshop.py', profile=PROFILE,
                       **{key: bank[key] for key in BANK_FIELDS})
        raw = (json.dumps(receipt, sort_keys=True) + '\n').encode()
        (stage / 'custom-owner-receipt.json').write_bytes(raw)
        provenance = publish_staged_owner_index(stage, provenance, custom_receipts=[raw])
        save(stage / 'provenance.json', provenance)
        require(verify_owner_index(stage, provenance) is not None, 'prepared owner verification failed')
        require(pin(source) == source_pin, 'Steam changed the map during preparation')
        if input_identity is not None:
            save(stage / 'workshop-inputs.json', input_identity)
        _publish_new(stage, output)
    return dict(schema=SCHEMA, map=str(output / relative_map),
                shared_maps=str(output / 'halo2/h2_maps_win64_dx11'), prepared_root=str(output),
                source=source_pin, campaign=mission['campaign'])


def prepare_cached(item, scenario, base, storage):
    from mcc_native_map_owner_publication import verify_owner_index
    item, base = Path(item).resolve(strict=True), Path(base).resolve(strict=True)
    mod = read_mod(item)
    require(mod is not None and not mod['error'], 'invalid installed Halo 2 item')
    mission = next((row for row in mod['missions'] if row['scenario'] == scenario), None)
    require(mission is not None, 'mission is not declared by this Workshop item')
    source = pin(source_owned(item, 'maps/' + scenario + '.map'))
    resources = resource_inputs(item, base, mod)
    resource_pins = {str(p): pin(p) for group in resources.values() for p in group.values() if p is not None}
    identity = dict(profile=PROFILE, source=source, base=pin(base / 'provenance.json'),
                    producer=pin(Path(__file__)), resources=resource_pins,
                    mod_metadata=pin(source_owned(item, 'ModInfo.json')),
                    mission_metadata=pin(source_owned(item, mission['metadata'])))
    if mission['campaign']:
        identity['campaign_metadata'] = pin(source_owned(item, 'CampaignInfo.json'))
    key = hashlib.sha256(json.dumps(identity, sort_keys=True).encode()).hexdigest()
    storage = Path(storage).absolute()
    storage.mkdir(parents=True, exist_ok=True)
    require(storage.resolve() == storage, 'prepared storage must have an exact private path')
    output = storage / (mod['id'] + '-' + scenario + '-' + key[:24])
    if not output.exists():
        return prepare(item, scenario, base, output, input_identity=identity)
    require(not output.is_symlink(), 'prepared mod storage is a symlink')
    provenance = metadata(output, 'provenance.json')
    require(metadata(output, 'workshop-inputs.json') == identity and
            provenance['workshop']['source'] == source and
            verify_owner_index(output, provenance) is not None,
            'existing prepared mod changed; content was retained')
    path = owned(output, f'halo2/Mods/{mod["id"]}/{scenario}.map')
    require(pin(path) == provenance['workshop']['prepared_map'], 'prepared map changed; content was retained')
    return dict(schema=SCHEMA, map=str(path), shared_maps=str(output / 'halo2/h2_maps_win64_dx11'),
                prepared_root=str(output), source=source, campaign=mission['campaign'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--scan', action='store_true')
    parser.add_argument('--item', type=Path)
    parser.add_argument('--scenario')
    parser.add_argument('--base-cache', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--storage', type=Path)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    try:
        if args.scan:
            result = scan()
        else:
            require(all((args.item, args.scenario, args.base_cache, args.output or args.storage)),
                    'preparation needs item, scenario, base cache and output')
            result = (prepare_cached(args.item, args.scenario, args.base_cache, args.storage)
                      if args.storage else prepare(args.item, args.scenario, args.base_cache, args.output))
        save(args.report, result)
        return 0
    except (OSError, ValueError, KeyError, TypeError, FormatError) as error:
        save(args.report, dict(schema=SCHEMA, error=str(error)))
        print(str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
