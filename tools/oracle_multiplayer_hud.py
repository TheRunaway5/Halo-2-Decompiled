#!/usr/bin/env python3
"""Execute pinned Xbox HUD corner, layout, state, and input instructions.

This stops before graphics APIs. The center oracle disables projection by
reporting no active local player. It does not claim projected-reticle parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX,
                              UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI,
                              UC_X86_REG_XMM0, UC_X86_REG_XMM2, UC_X86_REG_XMM3,
                              UC_X86_REG_EBP, UC_X86_REG_FPCW, UC_X86_REG_MXCSR)

PIN = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
ARENA, STACK, STOP = 0x1000000, 0x200ff00, 0x3000000


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    x = Xbe(ROOT / 'assets/disc/default.xbe')
    assert hashlib.sha256(x.data).hexdigest() == PIN
    g = Uc(UC_ARCH_X86, UC_MODE_32)
    g.mem_map(0x10000, 0x600000)
    for section in x.sections:
        g.mem_write(section.va, x.section_bytes(section))
    g.mem_map(ARENA, 0x100000)
    g.mem_map(0x2000000, 0x10000)
    g.mem_map(STOP, 0x1000)

    def w(at, value): g.mem_write(at, struct.pack('<I', value & 0xffffffff))
    def r(at): return struct.unpack('<I', g.mem_read(at, 4))[0]
    def f(at, value): g.mem_write(at, struct.pack('<f', value))
    def h(at, value): g.mem_write(at, struct.pack('<h', value))
    def call(address, argv=(), registers=()):
        w(STACK, STOP)
        for i, value in enumerate(argv): w(STACK + 4 + i * 4, value)
        g.reg_write(UC_X86_REG_ESP, STACK)
        g.reg_write(UC_X86_REG_FPCW, 0x027f)
        g.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        for reg, value in registers: g.reg_write(reg, value)
        g.emu_start(address, STOP, count=200000)
        assert g.reg_read(UC_X86_REG_EIP) == STOP
        return g.reg_read(UC_X86_REG_EAX)

    shield_calls = []
    def hook(uc, address, size, _):
        if address == 0x14ddc0:  # no active player; suppress center projection
            sp = uc.reg_read(UC_X86_REG_ESP)
            uc.reg_write(UC_X86_REG_EAX, 0)
            uc.reg_write(UC_X86_REG_EIP, r(sp))
            uc.reg_write(UC_X86_REG_ESP, sp + 4)
        elif address == 0x22acb4:  # Spartan selector; no appearance services
            sp = uc.reg_read(UC_X86_REG_ESP)
            uc.reg_write(UC_X86_REG_EAX, 0)
            uc.reg_write(UC_X86_REG_EIP, r(sp))
            uc.reg_write(UC_X86_REG_ESP, sp + 4)
        elif address == 0x22a664:  # record authored draw inputs, before D3D
            sp = uc.reg_read(UC_X86_REG_ESP)
            shield_calls.append(dict(inputs=list(struct.unpack('<4f', g.mem_read(0x4e6980, 16))),
                                     custom_colors=[list(struct.unpack('<3f', g.mem_read(0x4e6990 + i * 12, 12))) for i in range(3)]))
            uc.reg_write(UC_X86_REG_EIP, r(sp))
            uc.reg_write(UC_X86_REG_ESP, sp + 16)
    g.hook_add(UC_HOOK_CODE, hook)
    w(0x4e6948, 0)
    layouts = []
    for width, height in ((640, 480), (640, 240), (320, 480), (320, 240)):
        g.mem_write(0x4b9dd0, struct.pack('<4h', 0, 0, height, width))
        for display_flag in (0, 1):
            g.mem_write(0x485ac2, bytes([display_flag]))
            layouts.append(dict(viewport=[width, height], display_flag=display_flag,
                                slots=[call(0x13a690, (anchor,)) for anchor in range(5)]))
    assert layouts[0]['slots'] == [0] * 5
    assert layouts[2]['slots'] == [1] * 5
    assert layouts[3]['slots'] == [1, 1, 1, 2, 1]
    assert layouts[6]['slots'] == [2] * 5

    # A supplied safe rectangle makes corner reads directly reproducible.
    g.mem_write(0x4b9dd8, struct.pack('<4h', 24, 32, 456, 608))
    anchors = []
    for a in range(5):
        call(0x1396c7, registers=[(UC_X86_REG_EAX, a), (UC_X86_REG_ESI, ARENA)])
        anchors.append(list(struct.unpack('<2f', g.mem_read(ARENA, 8))))
    assert anchors == [[608., 24.], [32., 24.], [32., 456.], [608., 456.], [320., 240.]]

    masks = []
    for required, excluded, states in (([0]*4, [0]*4, [1,0,0,0]),
                                        ([0x10,0,1,0], [0]*4, [0,0,1,0]),
                                        ([0x10,0,1,0], [0]*4, [0x10,0,0,0]),
                                        ([0x10,0,1,0], [0,0,4,0], [0x10,0,4,0])):
        g.mem_write(ARENA, struct.pack('<8H3B', *required, *excluded, 0, 0, 0))
        result = call(0x13ac87, (states[0], states[1], states[3]),
                      [(UC_X86_REG_EAX, ARENA), (UC_X86_REG_ECX, 0), (UC_X86_REG_EDX, states[2])]) & 255
        masks.append(dict(required=required, excluded=excluded, states=states, visible=bool(result)))
    assert [m['visible'] for m in masks] == [False, True, True, False]

    status, item, time, globals_, selectors, values = [ARENA + i * 0x1000 for i in range(6)]
    w(0x510c54, time); f(time + 4, 1 / 30); w(time + 8, 300)
    w(0x510c4c, globals_); f(globals_ + 0x28, .75)
    f(status + 0x1cc, .25); f(status + 0x218, .5); f(status + 0x220, .875)
    h(item + 6, 30); h(item + 8, 120); h(item + 10, 60)
    inputs = []
    for codes in ([0x30, 0x33, 0x36, 0x35], [0x40, 0x41, 0x42, 0x43]):
        g.mem_write(selectors, bytes(codes))
        call(0x13ad48, (0, selectors, values), [(UC_X86_REG_ECX, item), (UC_X86_REG_EDX, status)])
        inputs.append(dict(selectors=codes, values=list(struct.unpack('<4f', g.mem_read(values, 16)))))
    assert inputs[0]['values'] == [30., 120., .5, 0.]
    assert inputs[1]['values'] == [.25, .5, .75, .875]

    # Original scoreboard transition, entered after status construction and
    # stopped before local-player lookup. The supplied status has score HUD
    # enabled (+0x130) and a leading flag (+0x133).
    winning = []
    frame = ARENA + 0x9000
    local_status = frame - 0x274
    h(time + 2, 30)
    for enabled, leading, initial in ((True, True, 0.), (True, False, 1.),
                                      (True, True, .99), (True, False, .01),
                                      (False, True, .25)):
        g.mem_write(local_status + 0x130, bytes([enabled]))
        g.mem_write(local_status + 0x133, bytes([leading]))
        f(globals_ + 0x28, initial)
        g.reg_write(UC_X86_REG_ESP, STACK)
        g.reg_write(UC_X86_REG_EBP, frame)
        g.reg_write(UC_X86_REG_EDI, globals_)
        g.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        g.emu_start(0x13a874, 0x13a8ef, count=200000)
        assert g.reg_read(UC_X86_REG_EIP) == 0x13a8ef
        final = struct.unpack('<f', g.mem_read(globals_ + 0x28, 4))[0]
        winning.append(dict(enabled=enabled, leading=leading, initial=initial,
                            ticks_per_second=30, final=final))
    assert abs(winning[0]['final'] - 2 / 30) < 1e-7
    assert abs(winning[1]['final'] - (1 - 2 / 30)) < 1e-7
    assert [v['final'] for v in winning[2:]] == [1., 0., .25]

    # Enter the shield-history update after player/unit resolution. Every
    # remaining instruction through 13A9D8 is retail code.
    shield_history = []
    unit = ARENA + 0xa000
    f(globals_, 1.); f(globals_ + 4, -1.); w(globals_ + 8, 100)
    for tick, current in ((101,.5), (115,.5), (116,.5), (117,.5),
                          (118,.75), (119,.25), (120,.1), (134,.1)):
        f(unit + 0xf0, current); w(time + 8, tick)
        g.reg_write(UC_X86_REG_ESP, STACK)
        g.reg_write(UC_X86_REG_EAX, unit)
        g.reg_write(UC_X86_REG_EDI, globals_)
        g.reg_write(UC_X86_REG_EDX, time)
        g.reg_write(UC_X86_REG_XMM0, r(globals_))
        g.reg_write(UC_X86_REG_XMM2, 0)
        g.reg_write(UC_X86_REG_XMM3, 0x3f800000)
        g.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        g.emu_start(0x13a93d, 0x13a9d8, count=200000)
        assert g.reg_read(UC_X86_REG_EIP) == 0x13a9d8
        previous, pulse, last_tick = struct.unpack('<2fI', g.mem_read(globals_, 12))
        shield_history.append(dict(tick=tick, current=current, previous=previous,
                                   pulse=pulse, last_tick=last_tick))
    assert [(v['previous'],v['pulse'],v['last_tick']) for v in shield_history[:3]] == [
        (1.,0.,101), (1.,0.,101), (.5,.5,116)]
    assert shield_history[4]['previous'] == .75 and shield_history[4]['pulse'] == -1.
    assert shield_history[6]['last_tick'] == 119

    safe = []
    for mx, my in ((48,36), (32,24)):
        call(0x16a30, registers=[(UC_X86_REG_ECX, my), (UC_X86_REG_EDX, mx), (UC_X86_REG_XMM0, 0x3f800000)])
        base = list(struct.unpack('<8h', g.mem_read(0x485a8a, 16)))
        cells = []
        pos, grid, span, outer, inner = [ARENA + 0x8000 + i * 0x100 for i in range(5)]
        for gx, gy in ((1,1), (1,2), (2,1), (2,2)):
            g.mem_write(grid, struct.pack('<2h',gx,gy));g.mem_write(span,struct.pack('<2h',1,1))
            for y in range(gy):
                for x_ in range(gx):
                    g.mem_write(pos,struct.pack('<2h',x_,y))
                    call(0x2f6b0, (grid,span,outer), [(UC_X86_REG_ESI,pos),(UC_X86_REG_EBX,inner)])
                    cells.append(dict(grid=[gx,gy], position=[x_,y],
                                      outer=list(struct.unpack('<4h',g.mem_read(outer,8))),
                                      safe=list(struct.unpack('<4h',g.mem_read(inner,8)))))
        safe.append(dict(margins=[mx,my], initialized_rectangles=base, cells=cells))
    assert safe[0]['initialized_rectangles']==[0,0,480,640,36,48,444,592]
    assert safe[0]['cells'][1]['outer']==[0,0,240,640]
    assert safe[0]['cells'][1]['safe']==[36,48,236,592]

    # Execute the untouched special-5 branch, with its geometry already
    # prepared by the caller; capture shader inputs before the draw API.
    shield = []
    frame, widget = ARENA + 0x6000, ARENA + 0x7000
    for current, previous, pulse in ((1., 1., 0.), (.5, 1., .25), (0., 0., 0.), (2.5, 2.5, 0.)):
        shield_calls.clear()
        f(status + 0x2c, current); f(globals_, previous); f(globals_ + 4, pulse)
        w(frame + 0x70, 0); w(frame + 0x74, status); w(frame + 0x78, widget)
        w(frame + 0x44, 0); w(widget + 0x24, 1); w(widget + 0x2c, 2)
        g.reg_write(UC_X86_REG_ESP, STACK)
        g.reg_write(UC_X86_REG_EBP, frame)
        g.reg_write(UC_X86_REG_FPCW, 0x027f)
        g.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        g.emu_start(0x22b3cb, 0x22b5cb, count=200000)
        assert g.reg_read(UC_X86_REG_EIP) == 0x22b5cb
        shield.append(dict(current=current, previous=previous, pulse=pulse, draws=list(shield_calls)))
    assert shield[0]['draws'][0]['inputs'][:2] == [1., 1.]
    assert shield[0]['draws'][0]['custom_colors'][0] == [0.] * 3
    assert shield[1]['draws'][0]['inputs'][:2] == [1., .5]
    assert shield[1]['draws'][0]['custom_colors'][0] == [.75] * 3
    assert not shield[2]['draws'] and len(shield[3]['draws']) == 3
    report = dict(xbe_sha256=PIN, entries=['001396c7', '0013a690', '0013ac87', '0013ad48'],
                  layouts=layouts, supplied_safe_rectangle=[24,32,456,608], anchors=anchors,
                  masks=masks, inputs=inputs, winning_transition=winning,
                  shield_history=shield_history,
                  viewport_safe_bounds=safe, shield_special=shield,
                  scope='Original CPU instructions only. Supplied viewport/safe rectangle; no graphics, device, player projection, or visible-screen parity claim.')
    output = json.dumps(report, indent=2) + '\n'
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(output)
    else: print(output, end='')


if __name__ == '__main__':
    main()
