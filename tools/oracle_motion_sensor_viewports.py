#!/usr/bin/env python3
"""Execute original Xbox radar placement and final quads in every local cell.

Retail video and split-grid constructors, 139614..139685 center placement and
2548F0 graphics constructors run unchanged. Device/texture residency services
are the explicit boundaries inherited from the graphics oracle. This proves
source vertex geometry, not final Xbox framebuffer equality. Native widescreen
expansion is deliberately outside this 640x480 logical-coordinate fixture.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from oracle_motion_sensor_graphics import Trace
from oracle_multiplayer_hud_positions import ROOT, PIN
from inspect_multiplayer_hud import HudCache
from unicorn.x86_const import UC_X86_REG_EBP, UC_X86_REG_EBX


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--tsv', type=Path, required=True)
    args = ap.parse_args()
    c, r = HudCache(ROOT/'assets/disc/maps/lockout.map'), Trace()
    gp = next(t for t in c.tags.values() if t['group'] == 'matg')['offset']
    interface = c.block(gp+0x110, 0x98)[0]
    globals_ = r.alloc(bytes(0x180))
    r.w(0x4e034c, globals_); r.w(globals_+0x110, 1)
    r.w(globals_+0x114, r.alloc(c.data[interface:interface+0x98]))
    for off in (0x38, 0x40, 0x60, 0x68):
        t = c.tags[c.u(interface+off+4)]; p = t['offset']
        header = bytearray(c.data[p:p+0x4c])
        images = c.block(p+0x44, 0x74)
        struct.pack_into('<I', header, 0x48,
                         r.alloc(b''.join(c.data[q:q+0x74] for q in images)))
        r.w(r.table+(int(t['datum'], 16)&65535)*16+8, r.alloc(header))
    r.run(0x254200)
    frame = r.alloc(bytes(0x280))+0x280
    center_ptr = r.alloc(bytes(8))
    rows = []
    tsv = ['# flags players view appearance left top right bottom']
    for flags in (0, 1, 8, 9):
        video = r.video(flags)
        for players in range(1, 5):
            grid_mode, cells = r.cells(players)
            for cell in cells:
                view = cell['view']
                r.g.mem_write(0x4b9dd0, struct.pack('<4h', *cell['outer']))
                r.g.mem_write(0x4b9dd8, struct.pack('<4h', *cell['safe']))
                r.run(0x139614, stop=0x139685,
                      registers=[(UC_X86_REG_EBP, frame), (UC_X86_REG_EBX, view)])
                x, y = r.read(frame-4, '<2h')
                # Original1A2836..1A2860 adds the outer origin again.
                x += cell['outer'][1]; y += cell['outer'][0]
                r.g.mem_write(center_ptr, struct.pack('<2f', x, y))
                for appearance in (0, 1):
                    r.appearance = appearance
                    r.w(0x4b9ed8, view); r.w(0x4ba04c, players)
                    r.w(0x4e69c0+view*4, 0x3f800000)
                    r.calls = []; r.programs = []; r.w(r.cursor, r.buffer)
                    r.run(0x2548f0, argv=[center_ptr, 0x3f800000])
                    calls = [q for q in r.calls if q['entry'] == '003fb2d0'][-4:]
                    quad = [list(struct.unpack('<2f', struct.pack('<2I', *q['args'][:2])))
                            for q in calls]
                    assert len(quad) == 4 and quad[0][0] == quad[3][0]
                    bounds = [quad[0][0], quad[0][1], quad[2][0], quad[2][1]]
                    rows.append(dict(video=video, players=players, cell=cell,
                        appearance=appearance, center=[x, y], quad=quad, bounds=bounds))
                    tsv.append(' '.join(map(str, [flags, players, view, appearance, *bounds])))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.tsv.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(schema=1, xbe_sha256=PIN,
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        boundary=__doc__, original_instruction_patches=[], cases=rows), indent=2)+'\n')
    args.tsv.write_text('\n'.join(tsv)+'\n')
    print(f'PASS {len(rows)} original radar quads: 1-4 players, standard/wide, interlaced/progressive, both species')


if __name__ == '__main__':
    main()
