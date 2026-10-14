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


def inventory(path, *, vehicles_only=False):
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
        return dict(map=path.stem, cache_sha256=file_sha(path), characters=characters, units=roots,
                    models=models, shaders=c.shaders, templates=c.templates, passes=c.passes,
                    vertex_shaders=c.vertex_shaders, bitmaps=c.bitmaps,
                    full_visual_parity_verified=False)
    finally:
        c.close()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--maps', type=Path, default=ROOT/'assets/disc/maps')
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    a.output.mkdir(parents=True, exist_ok=False)
    rows = []
    for name in MAPS:
        report = inventory(a.maps/(name+'.map'))
        destination = a.output/(name+'.json')
        destination.write_text(json.dumps(report, indent=2)+'\n')
        row = dict(map=name, cache_sha256=report['cache_sha256'], report_sha256=file_sha(destination),
                   **{k:len(report[k]) for k in ('characters', 'units', 'models', 'shaders', 'passes', 'vertex_shaders')})
        rows.append(row)
        print(json.dumps(row), flush=True)
    (a.output/'manifest.json').write_text(json.dumps(dict(scope=__doc__, maps=rows,
        tool_sha256=file_sha(Path(__file__)), full_visual_parity_verified=False), indent=2)+'\n')


if __name__ == '__main__':
    main()
