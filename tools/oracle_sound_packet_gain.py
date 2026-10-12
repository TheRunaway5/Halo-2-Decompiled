#!/usr/bin/env python3
"""Ordinary permutation gain and driver volume from unchanged Xbox instructions.

Actual grenade-bounce definitions have zero pitch and gain variance, range base
zero and one authored permutation. Prior original caller-control results feed
the ordinary 12A603..12A654 packet gain slice, then 221CF1..221D14, including its
unchanged normalization callee. This does not execute original admission/DSP.
"""
import argparse
import json
import struct
from pathlib import Path
from oracle_sound_packet_pitch import digest
from oracle_sound_control import ROOT, Cache, Xbe, OLD, NEW
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn import x86_const as R


def word(x):
    return struct.unpack('<I', struct.pack('<f', x))[0]


def controls():
    paths = [ROOT / 'assets/disc/maps/lockout.map', ROOT / 'assets/map-pack/maps/triplicate.map']
    caches = [Cache(p) for p in paths]
    rows = []
    for line in (ROOT / 'tests/fixtures/sound_control.original.tsv').read_text().splitlines():
        f = line.split()
        if f[0] != 'C' or f[4] != '00000000' or f[5] != '00000000' or f[6] != '00000000':
            continue
        bank, datum, control = int(f[1]), int(f[2], 16), int(f[3], 16)
        c = caches[bank]
        tag = c.tags[datum]
        if 'frag_bnc_' not in tag['name']:
            continue
        g = next(t['offset'] for t in c.tags.values() if t['group'] == 'ugh!')
        at = tag['offset']
        pb, first = struct.unpack_from('<hh', c.data, at + 6)
        p = c.u(g + 4) - c.magic + pb * 56
        assert c.f(p + 20) == 0 and struct.unpack_from('<hh', c.data, p + 24) == (0, 0)
        r = c.u(g + 36) - c.magic + first * 12
        start, count = struct.unpack_from('<hh', c.data, r + 8)
        assert count == 1
        perm = c.u(g + 44) - c.magic + start * 16
        gain, = struct.unpack_from('<b', c.data, perm + 4)
        control_gain, = struct.unpack('<f', struct.pack('<I', int(f[7], 16)))
        combined = word(c.f(p + 16) + control_gain)
        if struct.unpack('<f', struct.pack('<I', combined))[0] <= -64:
            continue
        rows.append((bank, datum, control, combined, gain))
    assert len(rows) >= 30
    return rows, [dict(path=str(p), sha256=digest(p)) for p in paths]


def execute(path, side, rows, pair):
    def sites(owner, values):
        old = (pair / f'instructions/{owner:08x}-old.asm').read_text().splitlines()
        new = (pair / f'instructions/{owner:08x}-{side}.asm').read_text().splitlines()
        return [int(new[next(i for i, l in enumerate(old) if l.startswith(f'{v:08x} '))].split()[0], 16) for v in values]
    start, end = sites(0x12a3f0, (0x12a603, 0x12a654))
    driver_start, driver_end = sites(0x221b90, (0x221cf1, 0x221d14))
    image = Xbe(path)
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    cpu.mem_map(0x10000, 0x800000)
    for section in image.sections:
        cpu.mem_write(section.va, image.section_bytes(section))
    stack, permutation = 0x1008000, 0x1001000
    cpu.mem_map(0x1000000, 0x10000)
    answers = []
    for _, _, _, combined, gain in rows:
        cpu.mem_write(stack + 0x0c, struct.pack('<I', combined))
        cpu.mem_write(permutation + 4, struct.pack('<b', gain))
        for reg, value in ((R.UC_X86_REG_ESP, stack), (R.UC_X86_REG_EDI, permutation),
                           (R.UC_X86_REG_FPCW, 0x027f), (R.UC_X86_REG_MXCSR, 0x1f80)):
            cpu.reg_write(reg, value)
        cpu.emu_start(start, end, count=100)
        assert cpu.reg_read(R.UC_X86_REG_EIP) == end
        packet_gain, = struct.unpack('<I', cpu.mem_read(stack + 0x34, 4))
        cpu.reg_write(R.UC_X86_REG_EBP, packet_gain)
        cpu.emu_start(driver_start, driver_end, count=1000)
        assert cpu.reg_read(R.UC_X86_REG_EIP) == driver_end
        volume = cpu.reg_read(R.UC_X86_REG_ECX) & 0xffffffff
        answers.append((packet_gain, volume))
    return answers


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    pair = ROOT / 'analysis/multiplayer/integration/sound-platform-voice-contract/paired'
    assert digest(pair / 'report.json') == '29bf3e2273704f6d2f3bf18ece740389febeda3b987f16e55c3fa9a29d2e9e4d'
    pins = ['aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377',
            '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd']
    assert [digest(p) for p in (OLD, NEW)] == pins
    source = ROOT / 'tests/fixtures/sound_control.original.tsv'
    assert digest(source) == 'fa4a8e6032b8706c4c4cd56941fde87855da5bb4048a2cb0887de72d109e5137'
    rows, maps = controls()
    answers = [execute(path, side, rows, pair) for path, side in ((OLD, 'old'), (NEW, 'u5'))]
    assert answers[0] == answers[1]
    fixture = ''.join(f'{bank} {datum:08x} {control:08x} {combined:08x} {gain:d} {packet:08x} {volume:08x}\n'
                      for (bank, datum, control, combined, gain), (packet, volume) in zip(rows, answers[0]))
    (args.output / 'fixtures.tsv').write_text(fixture)
    report = dict(boundary=__doc__, image_sha256=pins, cases=len(rows), maps=maps,
                  upstream_sha256=digest(source), instruction_patches=[], intercepted_callees=[],
                  paired_outputs_equal=True, fixture_sha256=digest(args.output / 'fixtures.tsv'),
                  tool_sha256=digest(__file__))
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'{len(rows)} original ordinary packet gain controls match both updates')


if __name__ == '__main__':
    main()
