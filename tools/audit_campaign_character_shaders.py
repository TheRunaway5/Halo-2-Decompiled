#!/usr/bin/env python3
"""Read every authored campaign character model and shader dependency.

This is an independent cache census. Character inheritance is followed from
char+8/+10; bipd/crea tags include authored units absent from the initial squads.
No expected prepared native material or rendered output supplies these records.
"""
import argparse
import hashlib
import json
from pathlib import Path

from audit_campaign import MAPS
from audit_rendering import Cache, FormatError, file_sha

ROOT = Path(__file__).resolve().parents[1]
NONE = 0xFFFFFFFF
OBJECT_MODEL_GROUPS = ('bipd', 'crea', 'vehi', 'scen', 'bloc', 'mach',
                       'ctrl', 'lifi', 'ssce', 'weap', 'eqip')


def inventory(path, *, vehicles_only=False, supplemental_character_roots=False,
              authored_dependency_roots=None):
    c = Cache(path)
    try:
        inherited = {}

        def unit(datum, ancestors=()):
            if datum in inherited:
                return inherited[datum]
            if datum in ancestors:
                raise FormatError('cyclic authored character inheritance')
            _, at = c.tag(datum, 'char')
            result = c.u32(at + 0x10)
            parent = c.u32(at + 8)
            if result == NONE and parent != NONE:
                result = unit(parent, (*ancestors, datum))
            inherited[datum] = result
            return result

        characters = []
        groups = ('vehi',) if vehicles_only else ('bipd', 'crea')
        units = {d for d, t in c.tags.items() if t['address'] and t['group'] in groups}
        for d, t in c.tags.items():
            if not vehicles_only and t['group'] == 'char' and t['address']:
                target = unit(d)
                characters.append(dict(datum=d, name=t['name'], unit=target))
                if target != NONE:
                    units.add(target)
        discovery = None
        if supplemental_character_roots:
            if vehicles_only:
                raise ValueError('Supplemental character roots and vehicles-only are separate scopes')
            base_units = set(units)
            base_models = set()
            for datum in base_units:
                _, at = c.tag(datum, c.tags[datum]['group'])
                model = c.u32(at + 0x38)
                if model != NONE:
                    _, at = c.tag(model, 'hlmt')
                    base_models.add(c.u32(at + 4))
            units, selected = set(), []
            for datum, tag in c.tags.items():
                if (not tag['address'] or datum in base_units or
                        tag['group'] not in OBJECT_MODEL_GROUPS):
                    continue
                _, at = c.tag(datum, tag['group'])
                model = c.u32(at + 0x38)
                if model == NONE:
                    continue
                _, at = c.tag(model, 'hlmt')
                mode = c.u32(at + 4)
                namespace = tag['name'].startswith('objects\\characters\\')
                alias = mode in base_models
                if namespace or alias:
                    units.add(datum)
                    selected.append(dict(datum=datum, name=tag['name'], group=tag['group'],
                                         character_namespace=namespace,
                                         shares_base_character_render_model=alias))
            characters = []
            discovery = dict(
                criterion='Resident non-base object with a render model, in objects\\characters\\ '
                          'or sharing a render model with a base bipd/crea/char target',
                object_groups=list(OBJECT_MODEL_GROUPS), base_units=len(base_units), roots=selected,
                arbitrary_attachment_dependency_closure=False,
                full_campaign_character_scope_verified=False)
        if authored_dependency_roots is not None:
            if vehicles_only or supplemental_character_roots:
                raise ValueError('Authored dependency roots require their own census scope')
            units = {row['datum'] for row in authored_dependency_roots}
            if len(units) != len(authored_dependency_roots):
                raise FormatError('Duplicate authored dependency root')
            for row in authored_dependency_roots:
                tag = c.tags.get(row['datum'])
                if (not tag or not tag['address'] or tag['group'] not in OBJECT_MODEL_GROUPS or
                        tag['group'] != row['group'] or tag['name'] != row['name']):
                    raise FormatError('Authored dependency root identity differs from cache')
            characters = []
            discovery = dict(criterion='New render-object roots reached by the pinned authored '
                            'object-attachment/hlmt-variant-child dependency graph',
                            roots=authored_dependency_roots,
                            arbitrary_attachment_dependency_closure=False,
                            full_campaign_character_scope_verified=False)
        models, roots = {}, []
        for d in sorted(units):
            t = c.tags[d]
            _, at = c.tag(d, t['group'])
            hlmt = c.u32(at + 0x38)
            if hlmt == NONE:
                roots.append(dict(datum=d, name=t['name'], group=t['group'], model=None))
                continue
            _, h = c.tag(hlmt, 'hlmt')
            mode = c.u32(h + 4)
            roots.append(dict(datum=d, name=t['name'], group=t['group'], model=mode,
                              default_variant=c.u32(at + 0x30)))
            if mode in models:
                continue
            mt, m = c.tag(mode, 'mode')
            shaders = [c.u32(p + 12) for p in c.block(m + 0x60, 32)]
            for shader in shaders:
                if shader != NONE:
                    c.shader(shader)
            # Keep every variant/region/permutation and all six geometry LODs.
            regions = []
            for region in c.block(m + 0x1C, 16):
                permutations = [dict(name=c.u32(p), sections=[c.u16(p + 4 + 2*k) for k in range(6)])
                                for p in c.block(region + 8, 16)]
                regions.append(dict(name=c.u32(region), permutations=permutations))
            sections = [dict(classification=c.u16(p), vertices=c.u16(p+4),
                             opaque_influences=c.data[p+0x14], transparent_influences=c.data[p+0x15],
                             resource_pointer=c.u32(p+0x38), resource_size=c.u32(p+0x3C))
                        for p in c.block(m+0x24, 0x5C)]
            models[mode] = dict(name=mt['name'], materials=shaders, regions=regions, sections=sections)
        report = dict(map=path.stem, cache_sha256=file_sha(path), characters=characters, units=roots,
                      models=models, shaders=c.shaders, templates=c.templates, passes=c.passes,
                      vertex_shaders=c.vertex_shaders, bitmaps=c.bitmaps,
                      full_visual_parity_verified=False)
        if discovery is not None:
            report['root_discovery'] = discovery
        return report
    finally:
        c.close()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--maps', type=Path, default=ROOT/'assets/disc/maps')
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--supplemental-character-roots', action='store_true',
                   help='Inventory explicitly discovered non-unit cinematic/proxy/attachment roots separately')
    p.add_argument('--authored-dependency-census', type=Path,
                   help='Inventory newly discovered render roots from a separately pinned dependency graph')
    a = p.parse_args()
    dependency_rows = None
    if a.authored_dependency_census:
        if a.supplemental_character_roots:
            p.error('Supplemental discovery and authored dependency roots are separate scopes')
        dependency_manifest = json.loads((a.authored_dependency_census/'manifest.json').read_text())
        dependency_rows = {row['map']: row for row in dependency_manifest['maps']}
        if len(dependency_rows) != len(MAPS) or set(dependency_rows) != set(MAPS):
            raise FormatError('Authored dependency census requires all 15 campaign maps')
    a.output.mkdir(parents=True, exist_ok=False)
    rows = []
    for name in MAPS:
        dependency_roots = None
        if dependency_rows is not None:
            dependency_path = a.authored_dependency_census/(name+'.json')
            if file_sha(dependency_path) != dependency_rows[name]['report_sha256']:
                raise FormatError('Authored dependency report hash differs')
            dependency = json.loads(dependency_path.read_text())
            if file_sha(a.maps/(name+'.map')) != dependency['cache_sha256']:
                raise FormatError('Authored dependency cache identity differs')
            dependency_roots = dependency['discovered_render_roots']
        report = inventory(a.maps/(name+'.map'),
                           supplemental_character_roots=a.supplemental_character_roots,
                           authored_dependency_roots=dependency_roots)
        destination = a.output/(name+'.json')
        destination.write_text(json.dumps(report, indent=2)+'\n')
        row = dict(map=name, cache_sha256=report['cache_sha256'], report_sha256=file_sha(destination),
                   **{k:len(report[k]) for k in ('characters', 'units', 'models', 'shaders', 'passes', 'vertex_shaders')})
        rows.append(row)
        print(json.dumps(row), flush=True)
    (a.output/'manifest.json').write_text(json.dumps(dict(scope=__doc__, maps=rows,
        supplemental_character_roots=a.supplemental_character_roots,
        authored_dependency_census_manifest_sha256=(file_sha(a.authored_dependency_census/'manifest.json')
                                                   if a.authored_dependency_census else None),
        tool_sha256=file_sha(Path(__file__)), full_visual_parity_verified=False), indent=2)+'\n')


if __name__ == '__main__':
    main()
