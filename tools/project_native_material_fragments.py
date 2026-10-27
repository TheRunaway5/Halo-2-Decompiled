#!/usr/bin/env python3
"""Project the complete offline material census into source-owned FS roles.

World tuples prove the world ABI. Existing raw-world vertex catalogs prove the
menu ABI. Typed HUD and particle references prove those procedural roles.
Native constructor/adaptation aliases carry their explicit family separately.
The projection does not promote every raw spas record into every shader family.
"""
import argparse
from collections import Counter
import hashlib
import json
import lzma
from pathlib import Path
import struct

from compile_native_world_kernels import atomic, validate_entry

SCHEMA = 'HALO2_NATIVE_WORLD_KERNEL_PROGRAMS_V1'
COLORS = frozenset((*range(10, 26), 43, 44))
EXPLICIT = frozenset(('world', 'world-fixed', 'world-emblem', 'menu', 'hud', 'particle',
    'bloom', 'pstencil-normal', 'pstencil-normal-utility', 'pstencil-texture',
    'pstencil-light-utility', 'pstencil-dependent-light'))


def controls(value):
    if (not isinstance(value, list) or len(value) != 60 or
            any(type(word) is not int or not 0 <= word <= 0xffffffff for word in value)):
        raise ValueError('Material projection requires complete literal 60-word controls')
    return [0 if i in COLORS else word for i, word in enumerate(value)]


def key(value):
    return hashlib.sha256(struct.pack('<60I', *value)).hexdigest()


def project(corpus, corpus_sha256, menu_program_hashes=(), source_path=None):
    if (corpus.get('schema') != SCHEMA or corpus.get('abi') != 1 or
            corpus.get('target') != 'vulkan1.3' or corpus.get('all_materials') is not True):
        raise ValueError('Material projection needs the explicit complete offline corpus')
    if not isinstance(corpus.get('entries'), list) or not corpus['entries']:
        raise ValueError('Material projection has no world program identities')
    inventories = corpus.get('prepared_inventories', [])
    if any(item.get('complete') is not True for item in inventories):
        raise ValueError('Material projection has an incomplete declared map inventory')
    selected, counts = {}, Counter()
    menu_hashes = set(menu_program_hashes)
    raw_menu_names = set()
    for row in corpus.get('cpu_vertex_programs', []):
        digest = hashlib.sha256(bytes.fromhex(row['vertex_program_hex'])).hexdigest()
        if digest in menu_hashes:
            for alias in row.get('aliases', []):
                if alias.get('name'):
                    raw_menu_names.add(alias['name'])
    texture_controls = set()
    world_controls = set()

    def emit(family, words, role, evidence, source_identity):
        if family not in EXPLICIT:
            raise ValueError('Native material source declares an unknown FS role: ' + family)
        normalized = controls(words)
        identity = (family, key(normalized))
        if identity not in selected:
            selected[identity] = {'family': family, 'controls': normalized, 'provenance': {
                'source_corpus_sha256': corpus_sha256, 'source_corpus': source_path,
                'source_control_sha256': identity[1], 'roles': [], 'representative_source_owners': [],
                'source_evidence_count': 0, 'complete_source_evidence_sha256': None,
                'complete_source_identity_evidence': {},
                'source_evidence_scope': 'Concrete routing representatives and complete original owner-count/digest pins; identity owner counts can include other family roles'},
                '_hash': hashlib.sha256()}
        row = selected[identity]
        provenance = row['provenance']
        if role not in provenance['roles']:
            provenance['roles'].append(role)
        if len(provenance['representative_source_owners']) < 8 and evidence not in provenance['representative_source_owners']:
            provenance['representative_source_owners'].append(evidence)
        provenance['source_evidence_count'] += 1
        origin_key, origin_evidence = source_identity
        provenance['complete_source_identity_evidence'][origin_key] = origin_evidence
        row['_hash'].update(json.dumps({'role': role, 'owner': evidence,
            'complete_source_identity': {origin_key: origin_evidence}}, sort_keys=True, separators=(',', ':')).encode()+b'\n')

    def source_evidence(row, identity):
        complete = row.get('source_owner_evidence')
        if complete is None:
            raw = b''.join(json.dumps(alias, sort_keys=True, separators=(',', ':')).encode()+b'\n'
                for alias in row.get('aliases', []))
            complete = {'count': len(row.get('aliases', [])), 'sha256': hashlib.sha256(raw).hexdigest(),
                'retention': 'Every original source alias is present'}
        if (type(complete.get('count')) is not int or complete['count'] < len(row.get('aliases', [])) or
                not isinstance(complete.get('sha256'), str) or len(complete['sha256']) != 64 or
                any(char not in '0123456789abcdef' for char in complete['sha256'])):
            raise ValueError('Native source owner evidence has incomplete count or identity')
        return identity, {key: complete[key] for key in ('count', 'sha256', 'retention') if key in complete}

    for row in corpus['entries']:
        words = controls(row['controls']);control_key = key(words);world_controls.add(control_key)
        origin = source_evidence(row, 'world:'+validate_entry(row)['key'])
        aliases = row.get('aliases', [])
        if not aliases:
            raise ValueError('Native world tuple has no actual source owner')
        for alias in aliases:
            emit('world', words, 'Complete authored world VS/declaration/control tuple', alias, origin)
            program_hash = hashlib.sha256(bytes.fromhex(row['vertex_program_hex'])).hexdigest()
            if program_hash in menu_hashes:
                emit('menu', words, 'Same authored VS is admitted by the raw-world native catalog', alias, origin)
            if alias.get('template_name', '').endswith('\\emblem_overlay_simple'):
                emit('world-emblem', words, 'Source armor emblem template selects the W-depth geometry wrapper', alias, origin)
            if (alias.get('vertex_shader', '').endswith('\\stencil_texture') and
                    alias.get('implementation') == 0):
                emit('pstencil-texture', words, 'Current source stencil_texture controls feed the independent texture-accumulation phase', alias, origin)
                texture_controls.add(control_key)

    unassigned = []
    for row in corpus.get('fragment_programs', []):
        words = controls(row['controls']);assigned = False
        origin = source_evidence(row, 'fragment:'+key(words))
        for alias in row.get('aliases', []):
            family = alias.get('family')
            if family:
                emit(family, words, 'Explicit source constructor, typed reference or adaptation role', alias, origin)
                assigned = True
                if alias.get('also_family'):
                    emit(alias['also_family'], words, 'Same source constructor has its separately declared wrapper role', alias, origin)
                if (alias.get('operation') == 'pstencil::neutral_detail_response' and
                        key(controls(alias['source_controls'])) in texture_controls):
                    emit('pstencil-texture', words, 'Completed neutral-detail controls retain their original texture-accumulation owner', alias, origin)
            if alias.get('kind') == 'particle-pass':
                emit('particle', words, 'Typed prt3/PRTM template-to-spas implementation reference', alias, origin)
                assigned = True
            if alias.get('kind') == 'hud-material-pass':
                emit('hud', words, 'Typed nhdt/hudg/mulg shader reference consumed by native HUD quads', alias, origin)
                assigned = True
            if alias.get('vertex_shader') in raw_menu_names:
                emit('menu', words, 'Source pass names a VS present in the compiled raw-world catalog', alias, origin)
                assigned = True
        if key(words) in world_controls:
            assigned = True
        if not assigned:
            unassigned.append(key(words))
    output = []
    for identity in sorted(selected):
        row = selected[identity]
        row['provenance']['complete_source_evidence_sha256'] = row.pop('_hash').hexdigest()
        counts[row['family']] += 1
        output.append(row)
    return {'schema': 'HALO2_NATIVE_FRAGMENT_PROGRAMS_V1', 'entries': output,
        'source_census': {'source_corpus_sha256': corpus_sha256, 'source_corpus': source_path,
            'map_entries': len(corpus.get('maps', [])), 'prepared_inventories': inventories,
            'world_tuple_count': len(corpus['entries']), 'fragment_control_count': len(corpus.get('fragment_programs', [])),
            'metadata_rejection_count': len(corpus.get('metadata_rejected', [])),
            'raw_source_programs_without_a_proved_runtime_family': sorted(set(unassigned)),
            'families': dict(sorted(counts.items())),
            'scope': 'Source-owned compilation roles; this projection alone is not runtime or map acceptance'}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--menu-catalog', type=Path, action='append', default=[])
    args = parser.parse_args()
    if args.input.resolve() == args.output.resolve():
        parser.error('The source corpus cannot be overwritten by its projection')
    encoded = args.input.read_bytes()
    compressed = args.input.suffix == '.xz'
    raw = lzma.decompress(encoded) if compressed else encoded
    catalogs = args.menu_catalog or [Path(__file__).resolve().parents[1]/'shaders/native-programs/menu/sources.json',
        Path(__file__).resolve().parents[1]/'shaders/pstencil/native-programs/sources.json',
        Path(__file__).resolve().parents[1]/'shaders/native-programs/renderer-raw/sources.json']
    hashes = set()
    for path in catalogs:
        catalog = json.loads(path.read_bytes())
        if catalog.get('schema') != 'halo2-native-menu-world-vertex-source-v2':
            raise ValueError('Raw-world source catalog has a different identity schema')
        hashes.update(row['program_sha256'] for row in catalog['entries'])
    result = project(json.loads(raw), hashlib.sha256(raw).hexdigest(), hashes, str(args.input.resolve()))
    if compressed:
        result['source_census']['source_archive'] = {'encoding': 'xz-compressed original JSON bytes',
            'sha256': hashlib.sha256(encoded).hexdigest(), 'decoded_bytes': len(raw)}
    result['source_census']['raw_world_vertex_catalogs'] = [
        {'path': str(path), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()} for path in catalogs]
    atomic(args.output, (json.dumps(result, indent=2, sort_keys=True)+'\n').encode())
    print(json.dumps(result['source_census']['families'], sort_keys=True))


if __name__ == '__main__':
    main()
