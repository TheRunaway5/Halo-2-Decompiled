#!/usr/bin/env python3
"""Literal actual local SMG particle shader/atlas inventory, without draw admission.

The selected system definition is obtained from the actual original 176C50
return in the render replay. Names never choose a shader or effect.
"""
import argparse
import json
from pathlib import Path
from inspect_multiplayer import Cache, ROOT
from oracle_smg_effect_owner_1_1 import sha, save, encoded

CACHE = ROOT / 'assets/disc/maps/lockout.map'
PIN = '14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9'


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--render-report', required=True, type=Path)
    ap.add_argument('--output', required=True, type=Path)
    a = ap.parse_args()
    assert sha(CACHE) == PIN
    source = json.loads(a.render_report.read_text())
    c = Cache(CACHE)

    def raw(at, n):
        assert 0 <= at <= at + n <= len(c.data)
        return dict(file_offset=f'{at:08x}', original_va=f'{at+c.magic:08x}',
                    bytes=n, raw_hex=bytes(c.data[at:at+n]).hex())

    def block(at, stride):
        return dict(header=raw(at, 8), stride=stride,
                    rows=[raw(p, stride) for p in c.block(at, stride)])

    def tag(datum, group):
        t = c.tags[datum]
        assert t['group'] == group and t['offset'] is not None
        return t

    systems = sorted({int(q['return_cpu']['registers']['eax'], 16)
                      for q in source['calls'] if q['entry'] == '00176c50'})
    assert systems
    output = []
    for va in systems:
        at = va - c.magic
        pt = tag(c.u(at+4), 'prt3')
        p = pt['offset']
        particle = dict(tag=pt, root=raw(p, 0xbc), system_definition=raw(at, 0x38),
            authored_billboard_style=c.h(p+4), source_vertex_classification=c.h(p+4)+7,
            shader_parameters=block(p+0x14, 0x28), properties=[], shader_definitions=[])
        for i in range(5):
            q = p+0x1c+i*0x10
            n = c.u(q+8)
            particle['properties'].append(dict(index=i, header=raw(q, 0x10),
                curve=raw(c.u(q+12)-c.magic, n) if n else None))
        bitmaps = {}
        for q in c.block(p+0x14, 0x28):
            if c.h(q+4) == 0 and c.u(q+12) != 0xffffffff:
                bitmaps[c.u(q+12)] = None
        for q in c.block(p+0x8c, 124):
            st = tag(c.u(q), 'stem')
            s = st['offset']
            processed = dict(raw=raw(q, 124), bitmap_constants=block(q+4, 12),
                pixel_constants=block(q+12, 4), vertex_constants=block(q+20, 16),
                template=st, template_root=raw(s, 0x60), templates=[])
            for b in c.block(q+4, 12):
                if c.u(b) != 0xffffffff:
                    bitmaps[c.u(b)] = None
            for t in c.block(s+0x58, 40):
                templ = dict(raw=raw(t, 40), lods=block(t, 10), layers=block(t+8, 2),
                    passes=block(t+16, 10), implementations=block(t+24, 6), remaps=block(t+32, 4),
                    pass_definitions=[])
                for ps in c.block(t+16, 10):
                    sp = tag(c.u(ps+4), 'spas')
                    sd = sp['offset']
                    pas = dict(tag=sp, root=raw(sd, 0x24), processed=[])
                    for r in c.block(sd+0x1c, 88):
                        details = dict(raw=raw(r, 88), implementations=block(r, 306),
                            bindings=block(r+8, 4), render_states=block(r+16, 5),
                            sampler_states=block(r+24, 24), fragments=block(r+32, 4),
                            external_remaps=block(r+56, 4), vertex_shaders=[])
                        for impl in c.block(r, 306):
                            vt = tag(c.u(impl+256), 'vrtx')
                            vr = vt['offset']
                            v = dict(tag=vt, root=raw(vr, 12), classifications=[])
                            for cl in c.block(vr+4, 28):
                                n = c.u(cl+12)
                                v['classifications'].append(dict(raw=raw(cl, 28),
                                    program=raw(c.u(cl+16)-c.magic, n) if n else None))
                            details['vertex_shaders'].append(v)
                        pas['processed'].append(details)
                    templ['pass_definitions'].append(pas)
                processed['templates'].append(templ)
            particle['shader_definitions'].append(processed)
        particle['bitmaps'] = []
        for datum in sorted(bitmaps):
            bt = tag(datum, 'bitm')
            b = bt['offset']
            particle['bitmaps'].append(dict(tag=bt, root=raw(b, 0x44), sequences=[
                dict(raw=raw(q, 0x3c), sprites=block(q+0x34, 0x20)) for q in c.block(b+0x3c, 0x3c)]))
        output.append(particle)
    save(a.output, encoded(dict(schema='actual-smg-particle-render-tags-v1',
        tool_sha256=sha(__file__), cache=dict(path=str(CACHE), sha256=PIN),
        render_report=dict(path=str(a.render_report), sha256=sha(a.render_report)),
        particles=output, scope='Literal authored resident data only. No implementation/layer/camera/GPU success is selected or supplied.')))


if __name__ == '__main__':
    main()
