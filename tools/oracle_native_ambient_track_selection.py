#!/usr/bin/env python3
"""Execute the unchanged normal/alternate track-selection slice, not admission.

The authored track is literal owned cache data. Initial phase0 and the alternate
request are declared raw ABI controls. No source device, pool, voice, readiness,
weather activation or successful callback is supplied. Execution stops before
the first optional sound creation call.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / 'tools'), str(ROOT / 'tools/reference/halo2-decompiled/tools')]
from inspect_multiplayer import Cache, MAPS
from inventory_mp_weapon_effects import DLC
from xbe import Xbe
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import *


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--image', choices=['base', '1.1', '1.5'], default='base')
    a = p.parse_args()
    a.output.mkdir(parents=True, exist_ok=True)
    bank = Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content')
    images = {
        'base': (ROOT / 'assets/disc/default.xbe',
                 '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'),
        '1.1': (ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',
                'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'),
        '1.5': (bank / 'u5English/default.xbe',
                '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'),
    }
    x = Xbe(images[a.image][0])
    assert hashlib.sha256(x.data).hexdigest() == images[a.image][1]
    paired = json.loads((ROOT / 'analysis/multiplayer/integration/live-authored-audio-20261010/ambient-correspondence/report.json').read_text())
    owner = next(r for r in paired['owners'] if r['base'] == 0x21a250)
    pairs = {} if a.image == 'base' else {
        r['old']: r['new'] for r in owner['images'][0 if a.image == '1.1' else 1]['pair']}
    entry, stop = (pairs.get(v, v) for v in (0x21a712, 0x21a80b))
    paths = [ROOT / 'assets/disc/maps' / (n + '.map') for n in MAPS]
    paths += [ROOT / 'assets/multiplayer-map-pack/demos/dlc' / b / (n + '.map')
              for b, ns in DLC.items() for n in ns]
    paths += [bank / 'c/4d53006400000004' / (n + '.map') for n in ('derelict', 'highplains2')]
    rows, maps = [], []
    for path in paths:
        c = Cache(path)
        u = Uc(UC_ARCH_X86, UC_MODE_32)
        u.mem_map(0x10000, 0x800000)
        for section in x.sections:
            u.mem_write(section.va, x.section_bytes(section))
        page = c.magic & ~4095
        u.mem_map(page, ((len(c.data) + c.magic - page + 4095) // 4096) * 4096)
        u.mem_write(c.magic, bytes(c.data))
        private = 0x1000000
        u.mem_map(private, 0x10000)
        stack, phase = private + 0xf000, private + 0x100
        # All resident lsnd tracks are tested, including unused palette tags.
        for datum, tag in c.tags.items():
            if tag['group'] != 'lsnd' or tag['offset'] is None:
                continue
            for index, at in enumerate(c.block(tag['offset'] + 0x1c, 0x58)):
                for alternate in (0, 1):
                    u.mem_write(stack, bytes(0x100))
                    u.mem_write(stack + 0x60, struct.pack('<I', alternate))
                    u.mem_write(phase, bytes(0x20))
                    u.mem_write(phase, struct.pack('<I', 0xffffffff))
                    u.reg_write(UC_X86_REG_ESP, stack)
                    u.reg_write(UC_X86_REG_EDX, at + c.magic)
                    u.reg_write(UC_X86_REG_EBP, phase)
                    u.reg_write(UC_X86_REG_ESI, 0xffffffff)
                    u.emu_start(entry, stop, count=10000)
                    assert u.reg_read(UC_X86_REG_EIP) == stop
                    rows.append(dict(map=path.stem, tag=f'{datum:08x}', track=index,
                                     flags=f'{c.u(at + 4):08x}', raw_track=c.data[at:at + 0x58].hex(),
                                     alternate=alternate, initial_phase=0,
                                     selected=f'{u.reg_read(UC_X86_REG_EBX):08x}',
                                     desired_phase=u.mem_read(phase + 13, 1)[0]))
        maps.append(dict(path=str(path), sha256=hashlib.sha256(c.data).hexdigest()))
        c.data.close()
        c.stream.close()
    report = dict(scope=__doc__, image=a.image, entry=f'{entry:08x}', stop=f'{stop:08x}',
                  xbe_sha256=images[a.image][1], tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  maps=maps, cases=rows)
    (a.output / 'report.json').write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    (a.output / 'fixtures.tsv').write_text(''.join(
        f"{r['map']}\t{r['tag']}\t{r['track']}\t{r['alternate']}\t{r['selected']}\t{r['desired_phase']}\n"
        for r in rows))
    print(f'PASS {len(rows)} unchanged original track-selection slices across {len(maps)} owned maps ({a.image})')


if __name__ == '__main__':
    main()
