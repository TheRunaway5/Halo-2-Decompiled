#!/usr/bin/env python3
"""Original ordinary voice pitch composition, with authored metadata inputs.

The prior unchanged caller-control/range oracle supplies control pitch and range.
This executes 12A55B..12A569's conversion/subtraction/store in both owned images.
Zero authored random-pitch bounds make the same inputs reproducible through the
actual native packet adapter without replacing its RNG or permutation owner.
This is a numeric packet contract, not full original admission, Doppler or DSP.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from oracle_sound_control import ROOT, Cache, Xbe, OLD, NEW
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn import x86_const as R


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def controls():
    source = ROOT / 'tests/fixtures/sound_control.original.tsv'
    assert digest(source) == 'fa4a8e6032b8706c4c4cd56941fde87855da5bb4048a2cb0887de72d109e5137'
    paths = [ROOT / 'assets/disc/maps/lockout.map', ROOT / 'assets/map-pack/maps/triplicate.map']
    caches = [Cache(p) for p in paths]
    gestalts = [next(t['offset'] for t in c.tags.values() if t['group'] == 'ugh!') for c in caches]
    rows = []
    selected = [[set(), set()], [set(), set()]]
    for line in source.read_text().splitlines():
        fields = line.split()
        if fields[0] != 'C':
            continue
        _, bank, datum, control, base, pitch, ran, gain = fields
        bank = int(bank)
        datum, control, base, pitch, ran = [int(v, 16) for v in (datum, control, base, pitch, ran)]
        if base or ran == 0xffff:
            continue
        c, g = caches[bank], gestalts[bank]
        at = c.tags[datum]['offset']
        playback, first = struct.unpack_from('<hh', c.data, at + 6)
        p = c.u(g + 4) - c.magic + playback * 56
        if struct.unpack_from('<hh', c.data, p + 24) != (0, 0):
            continue
        volume, = struct.unpack('<f', struct.pack('<I', int(gain, 16)))
        if volume + c.f(p + 16) <= -64:
            continue
        r = c.u(g + 36) - c.magic + (first + ran) * 12
        interval, = struct.unpack_from('<h', c.data, r + 2)
        it = c.u(g + 28) - c.magic + interval * 10
        center, = struct.unpack_from('<h', c.data, it)
        # Keep bounded actual inputs, including nonzero range centers and
        # ordinary material controls. Never edit authored metadata or results.
        if not center and not pitch:
            continue
        group = selected[bank][bool(center)]
        if datum not in group and len(group) >= 4:
            continue
        group.add(datum)
        rows.append((bank, datum, control, ran, pitch, center))
    assert len(rows) >= 30
    return rows, [dict(path=str(p), sha256=digest(p)) for p in paths], source


def execute(path, entry, end, rows):
    image = Xbe(path)
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    cpu.mem_map(0x10000, 0x800000)
    for section in image.sections:
        cpu.mem_write(section.va, image.section_bytes(section))
    stack = 0x1008000
    cpu.mem_map(0x1000000, 0x10000)
    answers = []
    for _, _, _, _, pitch, center in rows:
        cpu.reg_write(R.UC_X86_REG_ESP, stack)
        cpu.reg_write(R.UC_X86_REG_ECX, center & 0xffffffff)
        cpu.reg_write(R.UC_X86_REG_XMM0, pitch)
        cpu.reg_write(R.UC_X86_REG_FPCW, 0x027f)
        cpu.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
        cpu.emu_start(entry, end, count=10)
        assert cpu.reg_read(R.UC_X86_REG_EIP) == end
        answers.append(struct.unpack('<I', cpu.mem_read(stack + 0x30, 4))[0])
    return answers


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    pair = ROOT / 'analysis/multiplayer/integration/sound-platform-voice-contract/paired'
    assert digest(pair / 'report.json') == '29bf3e2273704f6d2f3bf18ece740389febeda3b987f16e55c3fa9a29d2e9e4d'
    listings = [(pair / f'instructions/0012a3f0-{side}.asm').read_text().splitlines() for side in ('old', 'u5')]
    indices = [next(i for i, line in enumerate(listings[0]) if line.startswith(f'{pc:08x} ')) for pc in (0x12a55b, 0x12a569)]
    entries = [[int(lines[i].split()[0], 16) for i in indices] for lines in listings]
    pins = ['aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377',
            '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd']
    assert [digest(p) for p in (OLD, NEW)] == pins
    rows, maps, source = controls()
    outputs = [execute(path, *entry, rows) for path, entry in zip((OLD, NEW), entries)]
    assert outputs[0] == outputs[1]
    fixture = ''.join(f'{bank} {datum:08x} {control:08x} {ran:x} {answer:08x}\n'
                      for (bank, datum, control, ran, _, _), answer in zip(rows, outputs[0]))
    (args.output / 'fixtures.tsv').write_text(fixture)
    report = dict(boundary=__doc__, image_sha256=pins, entries=entries, cases=len(rows), maps=maps,
                  upstream_fixture=str(source), upstream_sha256=digest(source),
                  paired_instruction_report_sha256=digest(pair / 'report.json'),
                  paired_outputs_equal=True, instruction_patches=[], intercepted_callees=[],
                  fixture_sha256=digest(args.output / 'fixtures.tsv'), tool_sha256=digest(__file__))
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'{len(rows)} original ordinary packet pitch controls match both updates')


if __name__ == '__main__':
    main()
