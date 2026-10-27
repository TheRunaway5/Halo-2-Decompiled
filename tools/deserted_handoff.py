#!/usr/bin/env python3
"""Make and install a portable Deserted content overlay without replacing assets.

Planning is read-only. Staging requires a separate final native acceptance
receipt. The installer is self-contained Python and never changes the existing
application or prepared MCC import. Run development probes in tools/run_test.py.
"""
from __future__ import annotations

import argparse
import errno
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import plistlib
import shutil
import stat
import struct
import subprocess
import sys
import zlib

if sys.version_info < (3, 9):
    raise RuntimeError('The Deserted installer requires Python 3.9 or newer')


SCHEMA = 'DESERTED_PORTABLE_HANDOFF_V1'
ACCEPTANCE = 'DESERTED_HANDOFF_ACCEPTANCE_V1'
MAP = 'halo2/Mods/deserted/deserted.map'
SHARED = 'halo2/h2_maps_win64_dx11'
ALIASES = {SHARED + '/' + name: MAP for name in (
    'textures.dat', 'sounds_en.dat', 'sounds_neutral.dat', 'sounds_remastered.dat')}
METADATA = ('provenance.json', 'tag-conversion.json', 'custom-owner-receipt.json',
            'native-shaders/map-owners.json')
README = '''# Deserted

This bundle contains the qualified custom map and its source. The existing
Halo 2 application and prepared MCC import supply the runtime and shared/font
dependencies. Neither is overwritten. The exact required base file hashes are
listed in deserted-handoff.json; a mismatched or missing import is rejected.

Choose an unused destination outside the app, import, and extracted bundle.
An existing Python 3.9 or newer interpreter is required. From this extracted
bundle directory, use its installed python3 command:

    python3 install_deserted.py install --prepared-base "/path/to/prepared-MCC-import" --destination "/Users/you/Documents/Deserted-play"

Then start an unlimited-time walkaround with the existing application binary:

    python3 install_deserted.py launch --destination "/Users/you/Documents/Deserted-play" --app "/path/to/Halo2Native.app"

Select the existing engine app. Its actual executable filename is read from
Contents/Info.plist; the installer does not assume a bundled Python runtime or
a fixed engine executable name.

The working directory is Deserted-play and map/shared paths are relative to
its content/ folder, matching the native review. Four .dat filenames are
reconstructed as regular hardlinks to the single shipped map (regular copies
only when hardlinks are unavailable). The original import is read-only.

Keep source/ and qualification/ in this extracted bundle; the packed Blender
file, preserving exports, collision authoring, static wreck source and review
receipts stay outside runtime content. No engine or base MCC installation is
included. Final native execution on macOS is a separate platform check from
the retained native review on the authoring machine.

If this bundle includes the reviewed map-selection presentation, open the
existing native menu with Deserted's screenshot and description:

    python3 install_deserted.py launch-menu --destination "/Users/you/Documents/Deserted-play" --app "/path/to/Halo2Native.app"

The private menu uses exact local MCC resources. Its small catalog patch and
preview tail are reconstructed separately from gameplay content. Immutable
resources and the scenario filename alias use regular hardlinks when available;
the menu texture bank is a fresh private copy. The launch command above remains
the unlimited-time Fiesta walkaround.
'''


def _require(value, message):
    if not value:
        raise ValueError(message)


def _launch_argv(mode):
    _require(mode in ('fiesta', 'slayer'), 'Unexpected accepted game mode')
    return ['--multiplayer', '--map', 'content/' + MAP,
            '--shared-maps', 'content/' + SHARED, '--local-players', '1',
            '--graphics-mode', 'Original', '--viewport', '1280', '720'] + (
                ['--custom-slayer', 'fiesta'] if mode == 'fiesta' else []) + [
            '--mp-rule', '10', '0']


def _relative(name):
    _require(isinstance(name, str) and name and '\\' not in name and ':' not in name
             and '\0' not in name and all(p not in ('', '.', '..') for p in name.split('/')),
             'Expected an exact relative path: ' + repr(name))
    path = PurePosixPath(name)
    _require(not path.is_absolute() and path.as_posix() == name, 'Aliased relative path')
    return Path(*path.parts)


def _exact(path, *, directory=True, missing=False):
    """Reject symlinks, including parent components, just as native admission does."""
    path = Path(os.path.abspath(path))
    current = Path(path.anchor)
    parts = path.parts[1:]
    for index, part in enumerate(parts):
        _require(part in {p.name for p in current.iterdir()} or
                 (missing and index == len(parts) - 1), 'Missing exact path: ' + str(path))
        current /= part
        _require(not current.is_symlink(), 'Symlink in private content path: ' + str(current))
    if not missing:
        _require(current.is_dir() if directory else current.is_file(),
                 'Wrong filesystem type: ' + str(path))
    return path


def _files(root):
    root = _exact(root)
    result = {}
    for current, dirs, names in os.walk(root, followlinks=False):
        for name in dirs:
            _require(not (Path(current) / name).is_symlink(), 'Directory symlink in source tree')
        for name in names:
            path = Path(current) / name
            _require(stat.S_ISREG(path.lstat().st_mode), 'Nonregular file in source tree')
            result[path.relative_to(root).as_posix()] = path
    return dict(sorted(result.items()))


def _pin(path):
    with Path(path).open('rb') as stream:
        before = os.fstat(stream.fileno())
        digest = hashlib.sha256()
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
        after = os.fstat(stream.fileno())
    _require((before.st_size, before.st_mtime_ns, before.st_ctime_ns) ==
             (after.st_size, after.st_mtime_ns, after.st_ctime_ns), 'File changed during hashing')
    return dict(bytes=after.st_size, sha256=digest.hexdigest())


def _check(path, pin):
    _require(_pin(_exact(path, directory=False)) == pin, 'Hash mismatch: ' + str(path))


def _json(path):
    def pairs(rows):
        result = {}
        for key, value in rows:
            _require(key not in result, 'Duplicate JSON key: ' + key)
            result[key] = value
        return result
    return json.loads(Path(path).read_text(), object_pairs_hook=pairs)


def _write(path, value):
    with Path(path).open('x', encoding='utf-8') as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write('\n')


def _copy(source, target, pin):
    _check(source, pin)
    target.parent.mkdir(parents=True, exist_ok=True)
    with Path(source).open('rb') as inp, target.open('xb') as out:
        shutil.copyfileobj(inp, out, 1024 * 1024)
    _check(target, pin)


def _base_file(name):
    return name.startswith(SHARED + '/') or name.startswith('halo2/h2_fonts/')


def _closure(content, pins, *, map_name=MAP, metadata=METADATA):
    """Check actual indexed map and complete bank payload files, not only names."""
    for name in (map_name, *metadata):
        _require(name in pins, 'Missing required runtime owner: ' + name)
    provenance = _json(content / 'provenance.json')
    index = _json(content / 'native-shaders/map-owners.json')
    pointer = provenance['native_shader_library']['owner_bindings']
    _require(pointer['path'] == 'native-shaders/map-owners.json' and
             pointer['sha256'] == pins[pointer['path']]['sha256'] and
             pointer['bytes'] == pins[pointer['path']]['bytes'], 'Owner-index pointer differs')
    _require(index['schema'] == 'H2_MCC_NATIVE_MAP_OWNERS_V1', 'Unexpected owner-index schema')
    owners = index['maps']
    selected = [row for row in owners if row['map'] == map_name]
    _require(len(selected) == 1 and selected[0]['input_sha256'] == pins[map_name]['sha256']
             and selected[0]['input_bytes'] == pins[map_name]['bytes']
             and selected[0]['shared_directory'] == SHARED, 'Custom map binding differs')
    for row in owners:
        name = str(_relative(row['map']))
        _require(name in pins and pins[name]['sha256'] == row['input_sha256']
                 and pins[name]['bytes'] == row['input_bytes'], 'Indexed map owner missing: ' + name)
    banks = []
    for name in pins:
        if name.startswith('native-shaders/') and name.endswith('/library.json'):
            bank = _json(content / name)
            directory = str(_relative(name)).removesuffix('/library.json')
            payloads = [bank['rasterizer'], *bank['shader_tags']]
            for row in payloads:
                leaf = row['payload']
                _require(Path(leaf).name == leaf, 'Invalid bank payload filename')
                payload = directory + '/payloads/' + leaf
                payload_sha = row.get('sha256', leaf.removesuffix('.bin'))
                _require(payload in pins and pins[payload]['bytes'] == row['bytes'] and
                         pins[payload]['sha256'] == payload_sha, 'Bank payload missing: ' + payload)
            _require(directory + '/function-tables.bin' in pins, 'Missing bank scalar tables')
            banks.append(dict(directory=directory, library_sha256=pins[name]['sha256'],
                              payload_records=len(payloads)))
    _require(banks, 'No retained native material bank')
    for row in owners:
        if row['native_mode'] == 'owned-bank':
            name = row['bank_directory'] + '/library.json'
            _require(name in pins and pins[name]['sha256'] == row['bank_library_sha256'],
                     'Owned shader library mismatch')
    return dict(index_sha256=pins['native-shaders/map-owners.json']['sha256'],
                owners=owners, banks=banks,
                scope='All content files are inventoried. No shader-only sidecar filter is used; '
                      'cache-embedded main HUD, native lighting and static scenery remain in the exact map.')


def plan(content, prepared_base, presentation=None):
    """Read and hash the full runtime closure. Do not create a bundle or install."""
    content, prepared_base = _exact(content), _exact(prepared_base)
    paths = _files(content)
    pins = {name: _pin(path) for name, path in paths.items()}
    closure = _closure(content, pins)
    for name, source in ALIASES.items():
        _require(name in pins and pins[name] == pins[source], 'Resource alias is not the map bytes')
    base = {name: pin for name, pin in pins.items() if _base_file(name) and name not in ALIASES}
    _require(SHARED + '/shared.map' in base and any(n.startswith('halo2/h2_fonts/') for n in base),
             'Missing shared-map/font runtime closure')
    for name, pin in base.items():
        _check(prepared_base / _relative(name), pin)
    shipped = {name: pin for name, pin in pins.items() if name not in base and name not in ALIASES}
    result = dict(schema=SCHEMA, state='planned-only', map=MAP, shared_directory=SHARED,
                content_files=shipped, base_files=base, resource_aliases=ALIASES,
                runtime_closure=closure, shipped_content_bytes=sum(p['bytes'] for p in shipped.values()),
                required_local_base_bytes=sum(p['bytes'] for p in base.values()),
                no_runtime_or_base_mcc_assets_bundled=True)
    if presentation is not None:
        result['presentation'] = _presentation_plan(presentation, prepared_base, pins)
    return result


def _appended_pin(base, tail):
    digest = hashlib.sha256()
    size = 0
    for path in (base, tail):
        with Path(path).open('rb') as stream:
            for block in iter(lambda: stream.read(1024 * 1024), b''):
                digest.update(block)
                size += len(block)
    return dict(bytes=size, sha256=digest.hexdigest())


def _patch_segments(row):
    _require(isinstance(row['segments'], list) and 0 < len(row['segments']) <= 16,
             'Unexpected menu patch segment count')
    result = []
    end = 0
    for segment in row['segments']:
        offset, data = segment['offset'], bytes.fromhex(segment['hex'])
        _require(type(offset) is int and offset >= end and data and len(data) <= 8192 and
                 offset + len(data) <= row['decoded_base']['bytes'], 'Invalid or overlapping menu patch')
        result.append((offset, data))
        end = offset + len(data)
    return result


def _patched_bytes(base, row):
    raw = Path(base).read_bytes()
    _require(len(raw) >= 896 and struct.unpack_from('<II', raw) == (0x68656164, 13),
             'Menu patch requires MCC version 13')
    extent = struct.unpack_from('<I', raw, 8)[0]
    _require(896 < extent <= 128 * 1024 * 1024 and row['decoded_base']['bytes'] == extent,
             'Menu patch exceeds its bounded decoded owner')
    if struct.unpack_from('<H', raw, 28)[0] & 1:
        chunk_size, _, table, count = struct.unpack_from('<4I', raw, 776)
        _require(0 < chunk_size <= extent and 0 < count <= 65536 and table >= 896 and
                 table + count * 8 <= len(raw), 'Invalid menu chunk table')
        data = bytearray(raw[:896])
        for i in range(count):
            size, address = struct.unpack_from('<ii', raw, table + i * 8)
            remaining = min(chunk_size, extent - len(data))
            _require(size and address >= 896 and address + abs(size) <= len(raw) and remaining > 0,
                     'Invalid menu chunk extent')
            encoded = raw[address:address + abs(size)]
            if size > 0:
                decoder = zlib.decompressobj()
                block = decoder.decompress(encoded, remaining + 1)
                _require(decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail,
                         'Invalid or trailing menu chunk data')
            else:
                block = encoded
            _require(0 < len(block) <= remaining, 'Menu chunk exceeds declared extent')
            data.extend(block)
    else:
        data = bytearray(raw)
    _require(dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest()) == row['decoded_base'],
             'Decoded menu base differs from authored patch input')
    for offset, value in _patch_segments(row):
        data[offset:offset + len(value)] = value
    return data


def _patched_pin(base, row):
    data = _patched_bytes(base, row)
    return dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest())


def _presentation_plan(root, prepared_base, content_pins):
    """Authenticate the small menu delta and all exact local dependencies."""
    root = _exact(root)
    value = _json(root / 'presentation.json')
    _require(value.get('schema') == 'DESERTED_MENU_PRESENTATION_V1' and
             value.get('state') == 'prepared-menu-overlay-native-pending' and
             value.get('title') == 'Deserted' and value.get('description') ==
             "ODSTs foiled a terrorist bombing in this New Mombasa district. The streets are deserted. The firing lanes aren't." and
             value.get('map_id') == 20261021 and value.get('scenario_path') ==
             'scenarios\\multi\\dust2_mombasa\\dust2_mombasa' and
             value.get('cache_pin') == content_pins[MAP] and
             value.get('original_base_unchanged') is True and
             value.get('gameplay_cache_unchanged') is True,
             'Presentation must match the actual unchanged gameplay cache')
    _require(value.get('menu_launch_argv') == _menu_argv(), 'Unexpected menu launch paths')
    selection = value.get('menu_selection', {})
    _require(type(selection.get('row')) is int and selection['row'] >= 0 and
             selection.get('new_profile_progress') == -1 and
             isinstance(selection.get('visible_map_ids'), list) and
             len(selection['visible_map_ids']) > selection['row'] and
             selection['visible_map_ids'][selection['row']] == value['map_id'],
             'Presentation requires its authored native map-selection row')
    expected = {'presentation.json', value['screenshot']['path']}
    _check(root / _relative(value['screenshot']['path']),
           {k: value['screenshot'][k] for k in ('bytes', 'sha256')})
    for name, pin in value['menu_files'].items():
        _check(root / 'menu-files' / _relative(name), pin)
        expected.add('menu-files/' + name)
    for name, row in value['base_files'].items():
        _relative(name)
        _check(prepared_base / _relative(row['path']), {k: row[k] for k in ('bytes', 'sha256')})
    for name, row in value['appends'].items():
        _relative(name)
        _check(prepared_base / _relative(row['base']['path']),
               {k: row['base'][k] for k in ('bytes', 'sha256')})
        _check(root / _relative(row['tail']['path']), {k: row['tail'][k] for k in ('bytes', 'sha256')})
        expected.add(row['tail']['path'])
        _require(_appended_pin(prepared_base / row['base']['path'], root / row['tail']['path']) == row['result'],
                 'Menu resource append does not match its final identity')
    _require(set(value['patches']) == {SHARED + '/mainmenu.map'} and
             set(value['appends']) == {SHARED + '/textures.dat'}, 'Unexpected menu cache/resource patch owners')
    for name, row in value['patches'].items():
        _relative(name)
        _check(prepared_base / _relative(row['base']['path']),
               {k: row['base'][k] for k in ('bytes', 'sha256')})
        _require(_patched_pin(prepared_base / row['base']['path'], row) == row['result'],
                 'Menu byte patch differs from its final identity')
    destinations = set().union(*(set(value[k]) for k in ('menu_files', 'base_files', 'appends', 'patches')))
    _require(len(destinations) == sum(len(value[k]) for k in ('menu_files', 'base_files', 'appends', 'patches')),
             'Presentation destinations overlap')
    for name, source in value['aliases'].items():
        _relative(name)
        _require(name.startswith('custom-maps/deserted/') and source.startswith('content/') and
                 source.removeprefix('content/') in content_pins and name not in destinations,
                 'Menu alias lacks its explicit installed content owner')
        destinations.add(name)
    _require('custom-maps/deserted/halo2/Mods/deserted/dust2_mombasa.map' in value['aliases'],
             'Menu lacks the exact scenario filename alias')
    # Review is added only after the private native menu has actually rendered.
    reviewed = 'native-menu-review.json' in _files(root)
    if reviewed:
        expected.add('native-menu-review.json')
        review = _json(root / 'native-menu-review.json')
        _require(review.get('schema') == 'DESERTED_NATIVE_MENU_REVIEW_V1' and review.get('passed') is True and
                 review.get('cache_pin') == value['cache_pin'] and review.get('title') == value['title'] and
                 review.get('description') == value['description'] and
                 review.get('menu_files') == value['menu_files'] and review.get('patches') == value['patches'] and
                 review.get('appends') == value['appends'] and
                 review.get('menu_selection') == value['menu_selection'] and
                 review.get('preview') == value['preview'] and
                 review.get('selected_custom_map') is True,
                 'Menu review does not match this actual presentation')
        captures = review.get('captures', [])
        _require(isinstance(captures, list) and captures, 'Native menu review requires actual screenshots')
        for row in captures:
            name = str(_relative(row['path']))
            _check(root / name, {k: row[k] for k in ('bytes', 'sha256')})
            expected.add(name)
    _require(set(_files(root)) == expected, 'Unexpected or missing presentation file')
    return value | {'native_menu_review_passed': reviewed,
                    'bundle_files': {name: _pin(root / name) for name in sorted(expected)}}


def _menu_argv():
    return ['--menu', '--skip-intro', '--menu-maps', 'menu/' + SHARED]


def _accepted(path, map_pin):
    value = _json(path)
    _require(value.get('schema') == ACCEPTANCE and value.get('passed') is True and
             value.get('full_map_acceptance') is True and value.get('cache_sha256') == map_pin['sha256'],
             'A matching final full-map native acceptance receipt is required')
    _require(value.get('mode') in ('fiesta', 'slayer'), 'Acceptance must retain the qualified game mode')
    for name in ('cache_sha256', 'runtime_sha256', 'source_glb_sha256', 'source_blend_sha256'):
        digest = value.get(name, '')
        _require(len(digest) == 64 and all(c in '0123456789abcdef' for c in digest),
                 'Missing final artifact fingerprint: ' + name)
    for field, count in (('routes', 8), ('native_views', 26)):
        rows = value.get(field, [])
        _require(isinstance(rows, list) and len(rows) == count and
                 len({row['name'] for row in rows}) == count and
                 all(row.get('passed') is True for row in rows), 'Incomplete final native cohort: ' + field)
    _require(value.get('manual_visual_review_passed') is True, 'Final manual visual review is missing')
    coverage = value.get('source_light_coverage', {})
    _require(coverage.get('passed') is True and coverage.get('light_count') == 27 and
             coverage.get('all_27_sources_implemented') is True, 'All 27 source lights must be qualified')
    reports = value.get('reports', {})
    _require(set(reports) == {'routes', 'native_views', 'manual_visual_review', 'source_light_coverage'},
             'Final acceptance must pin all four underlying review reports')
    actual_reports = {}
    for label, row in reports.items():
        report = Path(path).parent / _relative(row['path'])
        _check(report, dict(bytes=row['bytes'], sha256=row['sha256']))
        actual = _json(report)
        _require(actual.get('passed') is True, 'Underlying acceptance report did not pass')
        identities = ['cache_sha256', 'source_glb_sha256']
        identities += ['source_blend_sha256'] if label == 'source_light_coverage' else ['runtime_sha256']
        _require(all(actual.get(field) == value[field] for field in identities),
                 'Underlying report belongs to different accepted artifacts: ' + label)
        actual_reports[label] = actual

    def cohort(rows, expected, label):
        _require(isinstance(rows, list) and len(rows) == len(expected) and
                 all(isinstance(row, dict) and isinstance(row.get('name'), str) for row in rows) and
                 len({row['name'] for row in rows}) == len(rows) and
                 {row['name'] for row in rows} == {row['name'] for row in expected} and
                 all(row.get('passed') is True for row in rows),
                 'Underlying review cohort differs from acceptance: ' + label)

    cohort(actual_reports['routes'].get('cases'), value['routes'], 'routes')
    native = actual_reports['native_views']
    _require(native.get('mode') == value['mode'], 'Native review game mode differs')
    cases = native.get('cases', [])
    _require(isinstance(cases, list) and all(isinstance(row, dict) for row in cases),
             'Invalid underlying native cases')
    cohort([row for row in cases if row.get('scope') == 'native camera inspection'],
           value['native_views'], 'native views')
    manual = actual_reports['manual_visual_review']
    _require(manual.get('full_map_acceptance') is True and manual.get('view_count') == 26,
             'Manual review has not accepted the complete map')
    cohort(manual.get('views'), value['native_views'], 'manual visual review')
    _require(all(row.get('reviewed_full_resolution') is True for row in manual['views']),
             'Manual review is missing full-resolution inspection')
    lights = actual_reports['source_light_coverage']
    _require(lights.get('source_lights') == 27 and
             lights.get('actual_blend_source_names_transforms_colors_strengths_and_emissive_objects_reconciled') is True and
             lights.get('all_local_lights_and_emissive_shaders_reachable_in_final_cache') is True,
             'Underlying report does not qualify all 27 authored lights')
    return value


def stage(content, prepared_base, sources, output, acceptance, presentation=None):
    """Create a new handoff folder after acceptance; no zip and no installation."""
    content, prepared_base = _exact(content), _exact(prepared_base)
    sources = {label: _exact(root) for label, root in sources.items()}
    manifest = plan(content, prepared_base, presentation=presentation)
    accepted = _accepted(acceptance, manifest['content_files'][MAP])
    if presentation is not None:
        capture = manifest['presentation']['screenshot']['capture']
        _require(all(capture.get(key) == accepted[key] for key in
                     ('cache_sha256', 'runtime_sha256', 'source_glb_sha256', 'source_blend_sha256')),
                 'Menu hero capture must come from the final accepted artifacts')
        _require('native-menu-review.json' in manifest['presentation']['bundle_files'],
                 'Stage requires actual native menu-selection review and screenshots')
        review = _json(Path(presentation) / 'native-menu-review.json')
        _require(review.get('runtime_sha256') == accepted['runtime_sha256'],
                 'Menu must be reviewed with the actual qualified native runtime')
    source_paths = {}
    for label, root in sorted(sources.items()):
        _require(_relative(label).name == label, 'Use one path component for each source label')
        for name, path in _files(root).items():
            source_paths['source/' + label + '/' + name] = path
    _require(source_paths, 'A map source bundle is required')
    paths = {'content/' + name: content / _relative(name) for name in manifest['content_files']}
    paths.update(source_paths)
    if presentation is not None:
        for name in manifest['presentation']['bundle_files']:
            paths['presentation/' + name] = _exact(Path(presentation) / _relative(name), directory=False)
    source_pins = {_pin(path)['sha256'] for path in source_paths.values()}
    _require(accepted['source_glb_sha256'] in source_pins and accepted['source_blend_sha256'] in source_pins,
             'Source bundle does not contain the final accepted GLB and packed Blender file')
    paths['install_deserted.py'] = _exact(__file__, directory=False)
    paths['qualification/final-native-acceptance.json'] = _exact(acceptance, directory=False)
    for row in accepted['reports'].values():
        name = 'qualification/' + row['path']
        _require(name not in paths, 'Acceptance report name collides with another bundle owner')
        paths[name] = _exact(Path(acceptance).parent / _relative(row['path']), directory=False)
    output = _exact(output, missing=True)
    _require(not output.exists(), 'Choose a new handoff folder; no overwrite is allowed')
    for source in (content, prepared_base, *sources.values()):
        _require(not output.is_relative_to(source) and not source.is_relative_to(output),
                 'Handoff destination must be separate from input trees')
    bundle_pins = {name: _pin(path) for name, path in paths.items()}
    output.mkdir()
    for name, path in paths.items():
        _copy(path, output / _relative(name), bundle_pins[name])
    with (output / 'README.md').open('x', encoding='utf-8') as stream:
        stream.write(README)
    bundle_pins['README.md'] = _pin(output / 'README.md')
    manifest.update(state='staged-after-native-acceptance', bundle_files=bundle_pins,
                    accepted_mode=accepted['mode'], acceptance=accepted,
                    launch_argv=_launch_argv(accepted['mode']),
                    launch_cwd='installed overlay root',
                    installation_policy='Fresh destination only. Existing app/import are read-only. '
                        'Shared/font base bytes are copied only after exact hash match. '
                        'Four DAT names are regular hardlinks to the single map, with regular-copy '
                        'fallback only if the destination filesystem cannot hardlink.')
    _write(output / 'deserted-handoff.json', manifest)
    return manifest


def _validate_manifest(bundle):
    bundle = _exact(bundle)
    manifest = _json(bundle / 'deserted-handoff.json')
    _require(manifest.get('schema') == SCHEMA and manifest.get('state') == 'staged-after-native-acceptance'
             and manifest.get('map') == MAP and manifest.get('shared_directory') == SHARED
             and manifest.get('resource_aliases') == ALIASES, 'Unexpected handoff manifest')
    accepted = _accepted(bundle / 'qualification/final-native-acceptance.json', manifest['content_files'][MAP])
    _require(manifest.get('accepted_mode') == accepted['mode'] and
             manifest.get('launch_argv') == _launch_argv(accepted['mode']) and
             manifest.get('launch_cwd') == 'installed overlay root',
             'Launch must use the accepted map inside this installed overlay')
    for name, pin in manifest['bundle_files'].items():
        _check(bundle / _relative(name), pin)
    for name, pin in manifest['content_files'].items():
        _require(manifest['bundle_files'].get('content/' + name) == pin and not _base_file(name),
                 'Content inventory differs from the shipped bytes')
    _require(set(manifest['base_files']).isdisjoint(manifest['content_files']), 'Duplicate base/content owner')
    if 'presentation' in manifest:
        value = manifest['presentation']
        _require(value['cache_pin'] == manifest['content_files'][MAP] and
                 value.get('menu_launch_argv') == _menu_argv() and
                 value.get('native_menu_review_passed') is True and
                 'native-menu-review.json' in value['bundle_files'], 'Invalid staged presentation owner')
        for name, pin in value['bundle_files'].items():
            _require(manifest['bundle_files'].get('presentation/' + name) == pin,
                     'Presentation inventory differs from shipped bytes')
    _require(all(_base_file(name) and name not in ALIASES for name in manifest['base_files']),
             'Unexpected external base dependency')
    actual = set(_files(bundle)) - {'deserted-handoff.json'}
    _require(actual == set(manifest['bundle_files']), 'Uninventoried or missing package file')
    return bundle, manifest


def _link_or_copy(source, target, pin):
    _check(source, pin)
    target.parent.mkdir(parents=True, exist_ok=True)
    method = 'hardlink'
    try:
        os.link(source, target)
    except OSError as error:
        if error.errno not in (errno.EXDEV, errno.EPERM, errno.EOPNOTSUPP, errno.ENOSYS):
            raise
        _copy(source, target, pin)
        method = 'regular-copy'
    _check(target, pin)
    return method


def _install_presentation(presentation, prepared_base, destination, value, content_pins):
    """Reconstruct a private native menu; every aliased map stays byte-exact."""
    presentation, prepared_base, destination = map(_exact, (presentation, prepared_base, destination))
    menu = destination / 'menu'
    _require(not menu.exists(), 'Menu installation requires a fresh private root')
    for row in value['base_files'].values():
        _check(prepared_base / _relative(row['path']), {k: row[k] for k in ('bytes', 'sha256')})
    for row in (*value['appends'].values(), *value['patches'].values()):
        _check(prepared_base / _relative(row['base']['path']), {k: row['base'][k] for k in ('bytes', 'sha256')})
    menu.mkdir()
    methods = []
    for name, pin in value['menu_files'].items():
        _copy(presentation / 'menu-files' / _relative(name), menu / _relative(name), pin)
    for name, row in value['base_files'].items():
        pin = {k: row[k] for k in ('bytes', 'sha256')}
        source = (destination / 'content' / _relative(row['path']) if content_pins.get(row['path']) == pin
                  else prepared_base / _relative(row['path']))
        methods.append(dict(path=name, source='installed-content' if source.is_relative_to(destination) else 'exact-local-base',
                            method=_link_or_copy(source, menu / _relative(name), pin)))
    for name, row in value['appends'].items():
        target = menu / _relative(name)
        _copy(prepared_base / _relative(row['base']['path']), target, {k: row['base'][k] for k in ('bytes', 'sha256')})
        _check(presentation / _relative(row['tail']['path']), {k: row['tail'][k] for k in ('bytes', 'sha256')})
        with target.open('ab') as out, (presentation / _relative(row['tail']['path'])).open('rb') as inp:
            shutil.copyfileobj(inp, out, 1024 * 1024)
        _check(target, row['result'])
    for name, row in value['patches'].items():
        target = menu / _relative(name)
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open('xb') as stream:
            stream.write(_patched_bytes(prepared_base / _relative(row['base']['path']), row))
        _check(target, row['result'])
    for name, source in value['aliases'].items():
        key = source.removeprefix('content/')
        _require(source.startswith('content/') and key in content_pins, 'Menu alias is not installed content')
        methods.append(dict(path=name, source=source,
                            method=_link_or_copy(destination / _relative(source), menu / _relative(name), content_pins[key])))
    pins = {name: _pin(path) for name, path in _files(menu).items()}
    _closure(menu, pins, map_name=SHARED + '/mainmenu.map', metadata=tuple(n for n in METADATA if n != 'custom-owner-receipt.json'))
    custom = menu / 'custom-maps/deserted'
    custom_pins = {name: _pin(path) for name, path in _files(custom).items()}
    _closure(custom, custom_pins, map_name='halo2/Mods/deserted/dust2_mombasa.map')
    _require(custom_pins['halo2/Mods/deserted/dust2_mombasa.map'] == content_pins[MAP],
             'Menu-selected gameplay cache changed')
    return dict(schema='DESERTED_PRIVATE_MENU_INSTALLATION_V1', passed=True,
                menu_files=pins, methods=methods, launch_argv=_menu_argv(),
                authoring_native_menu_review_passed=value.get('native_menu_review_passed') is True,
                gameplay_cache_unchanged=True, native_execution_proved=False)


def install(bundle, prepared_base, destination):
    """Install into a new overlay, never into the app or its existing MCC import."""
    bundle, manifest = _validate_manifest(bundle)
    prepared_base = _exact(prepared_base)
    destination = _exact(destination, missing=True)
    _require(not destination.exists(), 'Choose a new destination; existing files are never replaced')
    for source in (bundle, prepared_base):
        _require(not destination.is_relative_to(source) and not source.is_relative_to(destination),
                 'Installation must be separate from the bundle and base import')
    for name, pin in manifest['base_files'].items():
        _check(prepared_base / _relative(name), pin)
    destination.mkdir()
    for name, pin in manifest['bundle_files'].items():
        if name.startswith('content/'):
            _copy(bundle / _relative(name), destination / _relative(name), pin)
    for name, pin in manifest['base_files'].items():
        _copy(prepared_base / _relative(name), destination / 'content' / _relative(name), pin)
    aliases = []
    for name, source in ALIASES.items():
        target = destination / 'content' / _relative(name)
        source = destination / 'content' / _relative(source)
        target.parent.mkdir(parents=True, exist_ok=True)
        method = 'hardlink'
        try:
            os.link(source, target)
        except OSError as error:
            if error.errno not in (errno.EXDEV, errno.EPERM, errno.EOPNOTSUPP, errno.ENOSYS):
                raise
            _copy(source, target, manifest['content_files'][MAP])
            method = 'regular-copy'
        _check(target, manifest['content_files'][MAP])
        aliases.append(dict(path=name, method=method))
    installed = {name: _pin(path) for name, path in _files(destination / 'content').items()}
    _closure(destination / 'content', installed)
    presentation_receipt = None
    if 'presentation' in manifest:
        presentation_receipt = _install_presentation(bundle / 'presentation', prepared_base, destination,
                                                     manifest['presentation'], installed)
    receipt = dict(schema='DESERTED_PORTABLE_INSTALLATION_V1', passed=True,
                   handoff_manifest_sha256=_pin(bundle / 'deserted-handoff.json')['sha256'],
                   map_sha256=manifest['content_files'][MAP]['sha256'],
                   content_files=installed, resource_aliases=aliases,
                   launch_argv=manifest['launch_argv'], launch_cwd='.',
                   accepted_mode=manifest['accepted_mode'],
                   accepted_artifacts={name: manifest['acceptance'][name] for name in
                       ('cache_sha256', 'runtime_sha256', 'source_glb_sha256', 'source_blend_sha256')},
                   runtime_included=False, source_bundle=str(bundle / 'source'),
                   scope='Installation byte/closure proof only; macOS native execution remains a separate gate.')
    if presentation_receipt is not None:
        receipt['presentation'] = presentation_receipt
    _write(destination / 'deserted-installation.json', receipt)
    return receipt


def app_runtime(app):
    """Resolve the exact engine entry point from the selected app's own plist."""
    app = _exact(app)
    _require(app.suffix == '.app', 'Choose the existing Halo 2 macOS application')
    info = _exact(app / 'Contents/Info.plist', directory=False)
    value = plistlib.loads(info.read_bytes())
    name = value.get('CFBundleExecutable')
    _require(isinstance(name, str) and name and name not in ('.', '..') and
             not any(c in name for c in '/\\:\0') and Path(name).name == name,
             'The application must declare one exact executable basename')
    runtime = _exact(app / 'Contents/MacOS' / name, directory=False)
    _require(os.access(runtime, os.X_OK), 'The application entry point is not executable')
    return runtime


def launch(destination, app):
    """Use the existing app binary and this overlay's exact review working dir."""
    destination = _exact(destination)
    runtime = app_runtime(app)
    receipt = _json(destination / 'deserted-installation.json')
    _require(receipt.get('schema') == 'DESERTED_PORTABLE_INSTALLATION_V1' and
             receipt.get('passed') is True and receipt.get('launch_cwd') == '.', 'Invalid install receipt')
    _require(receipt.get('launch_argv') == _launch_argv(receipt.get('accepted_mode')),
             'Launch must use this installation and its accepted mode')
    for name, pin in receipt['content_files'].items():
        _check(destination / 'content' / _relative(name), pin)
    return subprocess.call([str(runtime), *receipt['launch_argv']], cwd=destination)


def launch_menu(destination, app):
    destination = _exact(destination)
    runtime = app_runtime(app)
    receipt = _json(destination / 'deserted-installation.json')
    _require(receipt.get('schema') == 'DESERTED_PORTABLE_INSTALLATION_V1' and receipt.get('passed') is True,
             'Invalid installation receipt')
    menu = receipt.get('presentation', {})
    _require(menu.get('passed') is True and menu.get('launch_argv') == _menu_argv(),
             'This installation has no qualified Deserted map-selection overlay')
    for name, pin in receipt['content_files'].items():
        _check(destination / 'content' / _relative(name), pin)
    for name, pin in menu['menu_files'].items():
        _check(destination / 'menu' / _relative(name), pin)
    return subprocess.call([str(runtime), *menu['launch_argv']], cwd=destination)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    for name in ('plan', 'stage'):
        item = sub.add_parser(name)
        item.add_argument('--content', type=Path, required=True)
        item.add_argument('--prepared-base', type=Path, required=True)
        item.add_argument('--presentation', type=Path)
    item = sub.choices['stage']
    item.add_argument('--output', type=Path, required=True)
    item.add_argument('--acceptance', type=Path, required=True)
    item.add_argument('--source', action='append', default=[], metavar='LABEL=DIRECTORY')
    item = sub.add_parser('install')
    item.add_argument('--bundle', type=Path, default=Path(__file__).absolute().parent)
    item.add_argument('--prepared-base', type=Path, required=True)
    item.add_argument('--destination', type=Path, required=True)
    item = sub.add_parser('launch')
    item.add_argument('--destination', type=Path, required=True)
    item.add_argument('--app', type=Path, required=True)
    item = sub.add_parser('launch-menu')
    item.add_argument('--destination', type=Path, required=True)
    item.add_argument('--app', type=Path, required=True)
    args = vars(parser.parse_args())
    action = args.pop('command')
    if action == 'stage':
        sources = {}
        for row in args.pop('source'):
            label, separator, path = row.partition('=')
            _require(separator and label not in sources, 'Source labels must be unique LABEL=DIRECTORY entries')
            sources[label] = Path(path)
        args['sources'] = sources
    result = globals()[action.replace('-', '_')](**args)
    if action in ('launch', 'launch-menu'):
        return result
    if action == 'plan':
        print(json.dumps(result, indent=2, sort_keys=True))
    else:
        print(json.dumps(dict(command=action, passed=True, map_sha256=(
            result.get('content_files', {}).get(MAP, {}).get('sha256') or result.get('map_sha256')))))
    return 0


if __name__ == '__main__':
    sys.exit(main())
