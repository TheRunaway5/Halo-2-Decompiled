#!/usr/bin/env python3
"""Inventory the real matg rasterizer camouflage override across owned maps.

Reads all21 Xbox caches, their literal rasterizer global shader, selected
shaderLOD layers/passes, compiled implementations, externs and resources.
This is an authored asset inventory, not shader/geometry/GPU execution.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from audit_rendering import Cache
from audit_dynamic_model_shader_assets import MAPS

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'analysis/multiplayer/game-variants/camouflage-overlay-20261011'


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, default=OUT / 'assets.json')
    a = ap.parse_args()
    maps = []
    for index, name in enumerate(MAPS):
        path = ROOT / ('assets/disc/maps' if index < 12 else 'assets/map-pack/maps') / (name + '.map')
        c = Cache(path)
        matg_tag = next(t for t in c.tags.values() if t['group'] == 'matg')
        _, matg = c.tag(matg_tag['datum'], 'matg')
        rasters = c.block(matg + 0x108, 0x108)
        rows = []
        for r in rasters:
            datum = c.u32(r + 0xd0)
            c.shader(datum)
            layers = []
            _, shader = c.tag(datum, 'shad')
            for post in c.block(shader + 32, 124):
                _, stem = c.tag(c.u32(post), 'stem')
                for t in c.block(stem + 88, 40):
                    layer_bank = [c.u16(at) for at in c.block(t + 8, 2)]
                    templates = c.block(t + 16, 10)
                    for lod_index, lod in enumerate(c.block(t, 10)):
                        for layer, packed in enumerate(c.packed_range(c.u16(lod), layer_bank)):
                            if not c.u32(lod + 2) & (1 << layer):
                                continue
                            for ordinal in range(packed & 511, (packed & 511) + (packed >> 9)):
                                template = templates[ordinal]
                                pass_datum = c.u32(template + 4)
                                tag, spas = c.tag(pass_datum, 'spas')
                                impls = []
                                for pp in c.block(spas + 28, 88):
                                    externs = c.block(pp + 56, 4)
                                    textures = c.block(pp + 8, 4)
                                    for impl_index, impl in enumerate(c.block(pp, 306)):
                                        def extern(field):
                                            return [dict(raw_hex=c.data[e:e + 4].hex(), destination=c.u16(e), component=c.data[e + 2], source=c.data[e + 3])
                                                    for e in c.packed_range(c.u16(impl + field), externs)]
                                        impls.append(dict(index=impl_index, raw_hex=c.data[impl:impl + 306].hex(),
                                                          textures=[dict(raw_hex=c.data[e:e + 4].hex(), parameter=c.data[e], source_extern=c.data[e + 1], stage=c.data[e + 2], flags=c.data[e + 3])
                                                                    for e in c.packed_range(c.u16(impl), textures)],
                                                          pixel_externs=extern(0x122), vertex_externs=extern(0x124),
                                                          render_state_externs=extern(0x11e), state_externs=extern(0x120)))
                                layers.append(dict(shader_lod=lod_index, available_mask=c.u32(lod + 2), layer=layer, template_pass=ordinal,
                                                   pass_datum=pass_datum, pass_name=tag['name'], declared_implementations=c.u16(template + 8) >> 9, implementations=impls))
            rows.append(dict(address=c.magic + r, row_hex=c.data[r:r + 0x108].hex(), global_shader=datum,
                             flags=c.u16(r + 0xd4), parameters_bits=list(struct.unpack_from('<10I', c.data, r + 0xd8)), layers=layers))
        maps.append(dict(map=name, cache_sha256=hashlib.sha256(c.data).hexdigest(), matg=matg_tag['datum'], rasterizer_rows=rows,
                         shaders=c.shaders, templates=c.templates, passes=c.passes, vertex_shaders=c.vertex_shaders, bitmaps=c.bitmaps))
        c.close()
    report = dict(schema='original-camouflage-override-assets-v1', boundary=__doc__, maps=maps,
                  oracle_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  dependencies={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in (ROOT / 'tools/audit_rendering.py', ROOT / 'tools/audit_dynamic_model_shader_assets.py')},
                  limits=['Raw source texture-extern indices remain numeric until their original consumers are identified.',
                          'Pass implementation choice, override admission and actual resource/target readiness are separate live inputs.',
                          'No framebuffer, refraction, stencil, final blending or GPU parity result is inferred from authored names.'])
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(maps=len(maps), shader_names=sorted({s['name'] for m in maps for s in m['shaders'].values()}),
                          pass_names=sorted({s['name'] for m in maps for s in m['passes'].values()}), sha256=hashlib.sha256(a.output.read_bytes()).hexdigest()), indent=2))


if __name__ == '__main__':
    main()
