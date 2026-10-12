#!/usr/bin/env python3
"""Read-only source/cache audit and immutable death integration evidence copy.

This does not execute a death, create a physics result or validate a native
ragdoll port. Shared source copies describe the observed integration version.
"""
import hashlib
import json
import struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT, Xbe

OUT = ROOT / 'analysis/multiplayer/integration/death-animation-20261010'
OLD = ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def encoded(value):
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode()


def write(path, raw):
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        assert path.read_bytes() == raw, path
    else:
        path.write_bytes(raw)


def main():
    measured = OUT / 'correspondence'
    repeat = OUT / 'correspondence-repeat'
    files = sorted(p.relative_to(measured) for p in measured.rglob('*') if p.is_file())
    assert files == sorted(p.relative_to(repeat) for p in repeat.rglob('*') if p.is_file())
    for path in files:
        assert (measured / path).read_bytes() == (repeat / path).read_bytes(), path
    report = json.loads((measured / 'report.json').read_text())
    later = Path(report['later_path'])
    assert sha(OLD) == report['disc_1_1_sha256']
    assert sha(later) == report['later_owned_xbe_sha256']
    assert sha(ROOT / 'tools/research_mp_death_update_functions.py') == report['owner_selection_tool_sha256']
    assert sha(ROOT / 'tools/research_mp_sandbox_update_functions.py') == report['instruction_matcher_tool_sha256']
    images = dict(disc=Xbe(OLD), later=Xbe(later))
    instruction_rows = 0
    for row in report['routines']:
        assert len(row['candidates']) == 1
        assert row['normalized_instruction_and_internal_branch_topology_equal']
        assert row['switch_table_topology_equal']
        for side in ('disc', 'later'):
            path = measured / 'instructions' / row[side + '_listing']['file']
            assert sha(path) == row[side + '_listing']['sha256']
            for line in path.read_text().splitlines():
                address, hexbytes, _ = line.split(' ', 2)
                raw = bytes.fromhex(hexbytes)
                assert images[side].read(int(address, 16), len(raw)) == raw, (path, address)
                instruction_rows += 1
        for operand in row['paired_image_memory_operands']:
            for side, key in (('disc', 'old'), ('later', 'later')):
                assert images[side].read(int(operand[key + '_address'], 16), operand['width']).hex() == operand[key + '_hex']

    metadata = OUT / 'physics-v3.json'
    assert metadata.read_bytes() == (OUT / 'physics-v3-repeat.json').read_bytes()
    physics = json.loads(metadata.read_text())
    cache = ROOT / 'assets/disc/maps/lockout.map'
    assert sha(cache) == physics['cache_sha256']
    assert sha(ROOT / 'tools/inspect_mp_death_physics.py') == physics['tool_sha256']
    schema = ROOT / 'tools/reference/assembly/src/Assembly/Plugins/Halo2/phmo.xml'
    assert sha(schema) == physics['navigation_schema_sha256']
    data = cache.read_bytes()
    literal_rows = 0
    for record in [physics[key] for key in ('biped_root', 'model_root', 'physics_root')]:
        raw = bytes.fromhex(record['raw_hex'])
        assert data[record['file_offset']:record['file_offset'] + len(raw)] == raw
        literal_rows += 1
    blocks = physics['blocks']
    for block in blocks.values():
        for row in block['records']:
            raw = bytes.fromhex(row['raw_hex'])
            assert len(raw) == block['stride']
            assert data[row['file_offset']:row['file_offset'] + len(raw)] == raw
            literal_rows += 1
    nodes = blocks['nodes']['records']
    for node in nodes:
        for key in ('parent', 'sibling', 'child'):
            assert -1 <= node['fields'][key] < len(nodes)
        seen = {node['index']}
        parent = node['fields']['parent']
        while parent != -1:
            assert parent not in seen
            seen.add(parent)
            parent = nodes[parent]['fields']['parent']
    for row in blocks['rigid']['records']:
        assert 0 <= row['fields']['node'] < len(nodes)
    constraints = {1: blocks['limited_hinge']['records'], 2: blocks['ragdoll']['records']}
    constraint_references = 0
    for edge in blocks['node_edges']['records']:
        fields = edge['fields']
        for row in fields['constraints']:
            raw = bytes.fromhex(row['raw_hex'])
            assert data[row['file_offset']:row['file_offset'] + len(raw)] == raw
            constraint = constraints[row['type']][row['constraint_index']]['fields']
            assert (constraint['node_a'], constraint['node_b']) == (fields['node_a'], fields['node_b'])
            assert constraint['edge'] == edge['index']
            constraint_references += 1
    for name in ('metadata-repeat.txt', 'metadata-u5.txt', 'metadata-u5-repeat.txt'):
        assert (OUT / 'metadata.txt').read_bytes() == (OUT / name).read_bytes()

    prior = {
        'analysis/multiplayer/integration/spawn-source/death-tail/report.json': '7a467658d889a562b9d353f8378a7aba0f3bedff67f6f72b26075793248a4384',
        'analysis/multiplayer/integration/spawn-source/live-body-world/report.json': '2bf6fd373182b3db29d7409012f9ae566dcf2d9d496bbf15700184c4c8858951',
        'analysis/multiplayer/integration/spawn-source/player-visibility/report.json': 'f58232e963b26bcb4a6471db8c40488f2bb195c4be5cf8e0617331ae4d5f78dd',
        'analysis/multiplayer/sandbox-source/live-prop/frozen/manifest.json': 'acf18ffbd47ae8c548195b7e8de8cd64edc9a6b0d259382187f26955fb4ebe60',
    }
    for path, expected in prior.items():
        assert sha(ROOT / path) == expected, path
    prior_copied_files = 0
    for scope in ('live-prop/frozen', 'power-body-grouping/final'):
        directory = ROOT / 'analysis/multiplayer/sandbox-source' / scope
        manifest = json.loads((directory / 'manifest.json').read_text())
        if isinstance(manifest['files'], list):
            for row in manifest['files']:
                assert sha(directory / 'files' / row['path']) == row['sha256'], row['path']
                prior_copied_files += 1
        else:
            for path, expected in manifest['files'].items():
                assert sha(ROOT / path) == expected, path
                prior_copied_files += 1
        prior[str((directory / 'manifest.json').relative_to(ROOT))] = sha(directory / 'manifest.json')

    # Literal pointer occurrence only. Its natural dispatch and packet producer
    # are not inferred from table adjacency or annotation labels.
    pointer_occurrences = []
    for side, image in images.items():
        target = 0xeff50 if side == 'disc' else 0xf0e70
        needle = struct.pack('<I', target)
        for section in image.sections:
            raw = image.section_bytes(section)
            at = raw.find(needle)
            while at != -1:
                pointer_occurrences.append(dict(image=side, section=section.name,
                    address=f'{section.va + at:08x}', target=f'{target:08x}',
                    surrounding_hex=raw[max(0, at - 16):at + 20].hex()))
                at = raw.find(needle, at + 1)

    audit = dict(schema=1,source_and_cache_pins=prior,
        complete_body_matches=len(report['routines']),paired_instruction_rows=instruction_rows,
        independently_repeated_correspondence_files=len(files),
        literal_cache_records_verified=literal_rows,constraint_references_verified=constraint_references,
        metadata_independently_repeated=True,graph_output_repeated_and_supported_providers_equal=True,
        graph_lookup_supplied_identifiers=True,earlier_frozen_files_verified=prior_copied_files,
        forced_death_callback_pointer_occurrences=pointer_occurrences,
        new_original_gameplay_execution=False,new_native_production_code=False,
        new_native_ragdoll_differential=False,positive_spartan_dead_body_node_bank_measured=False,
        boundaries=['Complete static body correspondence and literal metadata do not supply constraint/runtime body/current-node outputs.',
            'Earlier standing biped world and animated pose proof is retained; earlier equipped death has physical component NONE.',
            'A new genuine allocated-Spartan positive death transition must be measured separately after this freeze.',
            'No death clip, fall admission, constraint frame, physical result, source flag or successful callback is fabricated.',
            'Shared native copies are audit snapshots; production/runtime and earlier freezes are not changed.'])
    write(OUT / 'frozen/audit.json', encoded(audit))
    paths = ['tools/research_mp_death_update_functions.py', 'tools/research_mp_sandbox_update_functions.py',
        'tools/inspect_mp_death_physics.py', 'tools/inspect_mp_death_animation.cpp',
        'tools/freeze_mp_death_animation_audit.py', 'docs/MP_DEATH_RAGDOLL_INTEGRATION_RESEARCH.md',
        'src/multiplayer_runtime.cpp', 'src/multiplayer.cpp', 'src/simulation.cpp',
        'include/halo2/multiplayer.hpp', 'include/halo2/simulation.hpp',
        'include/halo2/physics_body_grouping.hpp', 'src/physics_body_grouping.cpp',
        'include/halo2/physics_component_rebuild.hpp', 'src/physics_component_rebuild.cpp',
        'docs/MP_EQUIPPED_DEATH_TAIL_RESEARCH.md', 'docs/MP_SPARTAN_OWNED_POSE_RESEARCH.md',
        'docs/MP_LIVE_BIPED_WORLD_RESEARCH.md', 'docs/MP_PLAYER_VISIBILITY_RESEARCH.md',
        'tools/reference/assembly/src/Assembly/Plugins/Halo2/phmo.xml']
    paths += list(prior)
    paths += [str((OUT / name).relative_to(ROOT)) for name in ('physics-v3.json', 'physics-v3-repeat.json',
        'metadata.txt', 'metadata-repeat.txt', 'metadata-u5.txt', 'metadata-u5-repeat.txt', 'inspect')]
    paths += [str(path.relative_to(ROOT)) for directory in (measured, repeat) for path in directory.rglob('*') if path.is_file()]
    entries = []
    for relative in sorted(set(paths)):
        path = ROOT / relative
        raw = path.read_bytes()
        write(OUT / 'frozen/files' / relative, raw)
        entries.append(dict(path=relative,sha256=hashlib.sha256(raw).hexdigest(),bytes=len(raw)))
    manifest = dict(schema=1,scope='Immutable death/physical-node source and metadata audit; no completed ragdoll implementation.',
        audit_sha256=sha(OUT / 'frozen/audit.json'),files=entries)
    write(OUT / 'frozen/manifest.json', encoded(manifest))
    print(json.dumps(dict(manifest=str((OUT / 'frozen/manifest.json').relative_to(ROOT)),
        sha256=sha(OUT / 'frozen/manifest.json'),**audit)))


if __name__ == '__main__':
    main()
