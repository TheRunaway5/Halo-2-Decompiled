#!/usr/bin/env python3
"""Read bounded authored character child/effect references from Xbox caches.

Roots come from two independently pinned censuses. Traversal follows only the
known object attachment table and hlmt variant-child table; model permutation
states retain their looping-effect references. Arbitrary effect graphs, scripted
spawns, equipped weapons, damage-created debris and first-person dependencies
remain separate work. No active state or attachment is selected by this tool.
"""
import argparse
import json
from collections import Counter
from pathlib import Path

from audit_campaign import MAPS
from audit_campaign_character_shaders import NONE, OBJECT_MODEL_GROUPS
from audit_rendering import Cache, FormatError, file_sha

ROOT = Path(__file__).resolve().parents[1]
READERS = (
    'src/source_map_effects.cpp', 'src/source_combat_attachments.cpp',
    'src/content.cpp', 'src/model_selection.cpp', 'include/halo2/model_selection.hpp',
)


def load_census(directory):
    manifest = json.loads((directory / 'manifest.json').read_text())
    rows = {row['map']: row for row in manifest['maps']}
    if len(rows) != len(MAPS) or set(rows) != set(MAPS):
        raise FormatError('Dependency census requires all 15 map reports')
    reports = {}
    for name in MAPS:
        path = directory / (name + '.json')
        if file_sha(path) != rows[name]['report_sha256']:
            raise FormatError(f'{name}: census report hash differs')
        reports[name] = json.loads(path.read_text())
        if reports[name]['cache_sha256'] != rows[name]['cache_sha256']:
            raise FormatError(f'{name}: census cache identity differs')
    return reports


def inventory(path, base, supplemental):
    if not base['cache_sha256'] == supplemental['cache_sha256'] == file_sha(path):
        raise FormatError('Dependency cache identity differs from root censuses')
    c = Cache(path)
    try:
        def reference(datum, encoded_group=None):
            if datum == NONE:
                return None
            if datum not in c.tags or not c.tags[datum]['address']:
                raise FormatError(f'Authored dependency has no resident tag: {datum:08x}')
            tag = c.tags[datum]
            if encoded_group is not None and encoded_group != tag['group']:
                raise FormatError(f'Authored reference group differs: {datum:08x}')
            return dict(datum=datum, group=tag['group'], name=tag['name'])

        def group(at):
            return c.u32(at).to_bytes(4, 'big').decode('ascii')

        roots = {u['datum']: 'base' for u in base['units']}
        for unit in supplemental['units']:
            if unit['datum'] in roots:
                raise FormatError('Supplemental and base root sets overlap')
            roots[unit['datum']] = 'supplemental'
        todo, seen, objects, models, edges = list(sorted(roots)), set(), [], {}, []

        def add_edge(parent, kind, row, target):
            edges.append(dict(parent=parent, kind=kind, row=row, target=target))
            if target and target['group'] in OBJECT_MODEL_GROUPS and target['datum'] not in seen:
                todo.append(target['datum'])

        while todo:
            datum = todo.pop(0)
            if datum in seen:
                continue
            seen.add(datum)
            tag = reference(datum)
            if tag['group'] not in OBJECT_MODEL_GROUPS:
                raise FormatError('Render dependency root is not a supported object group')
            _, at = c.tag(datum, tag['group'])
            model = c.u32(at + 0x38)
            row = dict(**tag, root_scope=roots.get(datum, 'authored_child_or_attachment'),
                       default_variant=c.u32(at + 0x30), model=model, attachments=[])
            # Existing readers: object+94 block, 18-byte attachment records.
            for index, attachment in enumerate(c.block(at + 0x94, 0x18)):
                child_datum = c.u32(attachment + 4)
                target = None if child_datum == NONE else reference(child_datum, group(attachment))
                record = dict(index=index, original_bytes=c.data[attachment:attachment + 0x18].hex(),
                              marker=c.u32(attachment + 8), target=target)
                row['attachments'].append(record)
                add_edge(datum, 'object_attachment', index, target)
            objects.append(row)
            if model == NONE or model in models:
                continue
            mt, at = c.tag(model, 'hlmt')
            definition = dict(name=mt['name'], render_model=c.u32(at + 4), variants=[])
            # Existing content/model_selection readers: hlmt+50 variants,
            # 38-byte rows; +1C children, 10-byte child rows.
            for variant_index, variant in enumerate(c.block(at + 0x50, 0x38)):
                record = dict(index=variant_index, name=c.u32(variant),
                              region_selection_bytes=c.data[variant + 4:variant + 0x14].hex(),
                              children=[], regions=[])
                for index, child in enumerate(c.block(variant + 0x1C, 0x10)):
                    child_datum = c.u32(child + 12)
                    target = None if child_datum == NONE else reference(child_datum, group(child + 8))
                    child_row = dict(index=index, parent_marker=c.u32(child),
                                     child_marker=c.u32(child + 4), target=target,
                                     original_bytes=c.data[child:child + 0x10].hex())
                    record['children'].append(child_row)
                    add_edge(model, 'hlmt_variant_child', [variant_index, index], target)
                # Retain every authored state without choosing RNG, damage or
                # looping-effect admission. Float values stay as original bits.
                for region_index, region in enumerate(c.block(variant + 0x14, 0x14)):
                    permutations = []
                    for permutation_index, permutation in enumerate(c.block(region + 8, 0x20)):
                        states = []
                        for state_index, state in enumerate(c.block(permutation + 0xC, 0x18)):
                            target = reference(c.u32(state + 0xC))
                            states.append(dict(index=state_index,
                                               original_bytes=c.data[state:state + 0x18].hex(),
                                               looping_effect=target,
                                               looping_marker=c.u32(state + 0x10)))
                            add_edge(model, 'hlmt_permutation_state_looping_effect',
                                     [variant_index, region_index, permutation_index, state_index], target)
                        permutations.append(dict(index=permutation_index,
                            original_bytes=c.data[permutation:permutation + 0x20].hex(), states=states))
                    record['regions'].append(dict(index=region_index, permutations=permutations))
                definition['variants'].append(record)
            models[model] = definition
        return dict(map=path.stem, cache_sha256=base['cache_sha256'],
                    roots=[dict(datum=d, scope=scope) for d, scope in sorted(roots.items())],
                    objects=objects, model_definitions=models, edges=edges,
                    discovered_render_roots=[o for o in objects if o['datum'] not in roots],
                    traversal_complete_for_declared_fields=True,
                    arbitrary_attachment_dependency_closure=False,
                    live_dependency_admission_verified=False,
                    full_visual_parity_verified=False)
    finally:
        c.close()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--maps', type=Path, default=ROOT / 'assets/disc/maps')
    p.add_argument('--base-census', type=Path, required=True)
    p.add_argument('--supplemental-census', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    reader_pins = {name: file_sha(ROOT / name) for name in READERS}
    base, supplemental = load_census(a.base_census), load_census(a.supplemental_census)
    a.output.mkdir(parents=True, exist_ok=False)
    rows = []
    for name in MAPS:
        report = inventory(a.maps / (name + '.map'), base[name], supplemental[name])
        path = a.output / (name + '.json')
        path.write_text(json.dumps(report, indent=2) + '\n')
        row = dict(map=name, cache_sha256=report['cache_sha256'], report_sha256=file_sha(path),
                   roots=len(report['roots']), objects=len(report['objects']),
                   hlmt=len(report['model_definitions']), edges=len(report['edges']),
                   nonnull_edges=sum(e['target'] is not None for e in report['edges']),
                   discovered_render_roots=len(report['discovered_render_roots']),
                   target_groups=dict(Counter(e['target']['group'] for e in report['edges'] if e['target'])))
        rows.append(row)
        print(json.dumps(row), flush=True)
    if reader_pins != {name: file_sha(ROOT / name) for name in READERS}:
        raise FormatError('Existing reader sources changed during dependency inventory')
    (a.output / 'manifest.json').write_text(json.dumps(dict(scope=__doc__, maps=rows,
        tool_sha256=file_sha(Path(__file__)), reader_source_sha256=reader_pins,
        base_census_manifest_sha256=file_sha(a.base_census / 'manifest.json'),
        supplemental_census_manifest_sha256=file_sha(a.supplemental_census / 'manifest.json'),
        arbitrary_attachment_dependency_closure=False, full_visual_parity_verified=False), indent=2) + '\n')


if __name__ == '__main__':
    main()
