#!/usr/bin/env python3
"""Read original player/weapon model camouflage extern and animation masks.

All matg multiplayer bodies/hands and every weap world/FP mode material table
are an asset superset. All resident shader LODs and available independent
layers are included; dynamic layer3 requests implementation2. This does not
infer geometry, pose, visibility, owner, live shader LOD, or rendered pixels.
"""
import argparse
import collections
import hashlib
import json
import struct
from pathlib import Path

from audit_rendering import Cache
from audit_dynamic_model_shader_assets import MAPS

ROOT = Path(__file__).resolve().parents[1]
MASKS = [0xff0000, 0xff00, 0xff, 0xff000000, 0xffffff, 0xff0000, 0xff00, 0xff]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, default=ROOT / 'analysis/multiplayer/game-variants/camouflage-render-context-20261011/animation-masks.json')
    a = ap.parse_args()
    totals = collections.Counter()
    maps, records, overlaps = [], [], []
    for index, name in enumerate(MAPS):
        path = ROOT / ('assets/disc/maps' if index < 12 else 'assets/map-pack/maps') / (name + '.map')
        c = Cache(path)
        roles, unresolved = [], []

        def add_object(role, datum, group):
            if datum == 0xffffffff:
                unresolved.append(dict(role=role, reason='object definition NONE'))
                return
            _, body = c.tag(datum, group)
            model = c.u32(body + 0x38)
            if model == 0xffffffff:
                unresolved.append(dict(role=role, reason='model definition NONE'))
                return
            _, hlmt = c.tag(model, 'hlmt')
            mode = c.u32(hlmt + 4)
            if mode == 0xffffffff:
                unresolved.append(dict(role=role, reason='render model NONE'))
                return
            roles.append(dict(role=role, definition=datum, mode=mode))

        _, matg = c.tag(next(t['datum'] for t in c.tags.values() if t['group'] == 'matg'), 'matg')
        representations = c.block(matg + 0x138, 0xbc)
        for rep, race in ((2, 'chief'), (3, 'elite')):
            at = representations[rep]
            add_object(race + '_body', c.u32(at + 0xb4), 'bipd')
            roles.append(dict(role=race + '_hands', definition=c.u32(at + 4), mode=c.u32(at + 4)))
        for tag in c.tags.values():
            if tag['group'] != 'weap':
                continue
            add_object(tag['name'] + '_world', tag['datum'], 'weap')
            _, body = c.tag(tag['datum'], 'weap')
            for fp_index, at in enumerate(c.block(body + 0x2a8, 16)):
                mode = c.u32(at + 4)
                if mode == 0xffffffff:
                    unresolved.append(dict(role=tag['name'] + '_fp' + str(fp_index), reason='render model NONE'))
                else:
                    roles.append(dict(role=tag['name'] + '_fp' + str(fp_index), definition=tag['datum'], mode=mode))
        shaders = collections.defaultdict(list)
        for role in roles:
            _, model = c.tag(role['mode'], 'mode')
            materials = [c.u32(at + 12) for at in c.block(model + 0x60, 32)]
            role['materials'] = materials
            for datum in set(materials) - {0xffffffff}:
                shaders[datum].append(role['role'])
        totals['maps'] += 1
        totals['model_roles'] += len(roles)
        totals['unique_shader_map_contexts'] += len(shaders)
        for datum, shader_roles in sorted(shaders.items()):
            tag, s = c.tag(datum, 'shad')
            for d in c.block(s + 32, 124):
                _, stem = c.tag(c.u32(d), 'stem')
                t = c.block(stem + 88, 40)[0]
                templates = c.block(t + 16, 10)
                layers = [c.u16(at) for at in c.block(t + 8, 2)]
                animation_passes = c.block(d + 44, 2)
                animation_impls = c.block(d + 52, 10)
                animation_maps = c.block(d + 84, 4)
                overlays = c.block(d + 60, 20)
                switches = [0] * 8
                switches[4:6] = [c.u16(s + 0x3e), c.u16(s + 0x40)]
                for lod_index, lod in enumerate(c.block(t, 10)):
                    active_layers = c.packed_range(c.u16(lod), layers)
                    for layer, packed in enumerate(active_layers):
                        if not c.u32(lod + 2) & (1 << layer):
                            continue
                        implementation = 2 if layer == 3 else 0
                        for ordinal in range(packed & 511, (packed & 511) + (packed >> 9)):
                            template = templates[ordinal]
                            if implementation >= c.u16(template + 8) >> 9:
                                totals['unavailable_implementation_contexts'] += 1
                                continue
                            _, spas = c.tag(c.u32(template + 4), 'spas')
                            pp = c.block(spas + 28, 88)[0]
                            source = c.block(pp, 306)[implementation]
                            externs = c.block(pp + 56, 4)
                            fragments = c.packed_range(c.u16(source + 246), c.block(pp + 32, 4))
                            permutations = c.block(pp + 40, 6)
                            offsets, stage = [], 0
                            for fragment in fragments:
                                switch = struct.unpack_from('<h', c.data, fragment)[0]
                                choice = 0 if switch == -1 else switches[switch]
                                permutation = c.packed_range(c.u16(fragment + 2), permutations)[choice]
                                offsets.append(stage)
                                stage += (c.u16(permutation + 4) >> 9) - bool(c.u16(permutation + 2) & 1)

                            def write(at):
                                dest, component, param = struct.unpack_from('<HBB', c.data, at)
                                slot = (dest >> 6) & 15
                                if fragments:
                                    fragment = dest & 7
                                    switch = struct.unpack_from('<h', c.data, fragments[fragment])[0]
                                    if switch != -1 and ((dest >> 3) & 7) != switches[switch]:
                                        return None
                                    if slot != 8:
                                        slot += offsets[fragment]
                                assert 0 <= slot <= 8
                                register = 43 + ((dest >> 10) & 1) if slot == 8 else 10 + ((dest >> 10) & 1) * 8 + slot
                                return dict(destination=dest, component=component, source=param, register=register, mask=MASKS[component & 7])

                            totals['pass_contexts'] += 1
                            pe = [write(at) for at in c.packed_range(c.u16(source + 0x122), externs)]
                            pe = [e for e in pe if e and e['source'] == 37 and (e['component'] & 7) < 4]
                            if not pe:
                                continue
                            totals['camouflage_pass_contexts'] += 1
                            animated = []
                            if overlays and ordinal < len(animation_passes):
                                candidates = c.packed_range(c.u16(animation_passes[ordinal]), animation_impls)
                                if implementation < len(candidates):
                                    ai = candidates[implementation]
                                    animated = [write(at) for at in c.packed_range(c.u16(ai + 6), animation_maps)]
                                    animated = [e for e in animated if e]
                            totals['camouflage_pass_contexts_with_pixel_animation'] += bool(animated)
                            matches = [dict(extern=e, animation=b, mask=e['mask'] & b['mask']) for e in pe for b in animated if e['register'] == b['register'] and e['mask'] & b['mask']]
                            record = dict(map=name, shader=datum, shader_name=tag['name'], roles=sorted(shader_roles), shader_lod=lod_index,
                                          layer=layer, template_pass=ordinal, implementation=implementation, switches=switches,
                                          fragment_offsets=offsets, camouflage_externs=pe, pixel_animation=animated, overlaps=matches)
                            records.append(record)
                            if matches:
                                totals['overlap_pass_contexts'] += 1
                                overlaps.append(record)
        maps.append(dict(map=name, cache_sha256=hashlib.sha256(c.data).hexdigest(), roles=roles, unresolved_roles=unresolved))
        c.close()
    totals.setdefault('overlap_pass_contexts', 0)
    report = dict(schema='original-camouflage-animation-masks-v1', boundary=__doc__, summary=dict(totals), records=records, overlaps=overlaps,
                  maps=maps, oracle_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  dependencies={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in (ROOT / 'tools/audit_rendering.py', ROOT / 'tools/audit_dynamic_model_shader_assets.py')},
                  limits=['Authored shaderLOD/layer/default-switch asset contexts, not live draw counts.',
                          'Only scalar37 pixel extern component0..3 is compared with authored pixel animation masks; other input/producer semantics remain external.',
                          'All available independent layers are conservatively included; dynamic layer3 implementation2 only.',
                          'Model definition/render model NONE roles are enumerated, not replaced.',
                          'No GPU, distortion clone, invisibility, shadow or complete render-submission proof.'])
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report['summary'], indent=2))


if __name__ == '__main__':
    main()
