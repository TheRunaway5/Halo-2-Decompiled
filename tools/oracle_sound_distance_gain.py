#!/usr/bin/env python3
"""Execute unchanged 1.1/u5 distance-gain arithmetic with declared numeric ABI inputs.

The slice starts after original metadata/override selection and minimum flooring.
Actual resident Lockout/Triplicate distance bounds supply dense interval controls.
No instruction, callee, admission, listener, RNG or platform result is replaced.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from oracle_sound_control import ROOT, Cache, Xbe, OLD, NEW
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn import x86_const as R


def word(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def real(value):
    return struct.unpack('<f', struct.pack('<I', value))[0]


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def controls():
    pairs = set()
    maps = []
    for path in (ROOT / 'assets/disc/maps/lockout.map', Path(
            '/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/'
            'Halo 2 DLC/content/c/4d53006400000003/triplicate.map')):
        cache = Cache(path)
        classes = next(t['offset'] for t in cache.tags.values() if t['group'] == 'sncl')
        gestalt = next(t['offset'] for t in cache.tags.values() if t['group'] == 'ugh!')
        count = 0
        for tag in cache.tags.values():
            if tag['group'] != 'snd!' or tag['offset'] is None:
                continue
            at = tag['offset']
            flags, = struct.unpack_from('<H', cache.data, at)
            cls = cache.data[at + 2]
            playback, = struct.unpack_from('<h', cache.data, at + 6)
            assert cls < cache.u(classes) and 0 <= playback < cache.u(gestalt)
            cl = cache.u(classes + 4) - cache.magic + cls * 92
            pb = cache.u(gestalt + 4) - cache.magic + playback * 56
            minimum = cache.f(cl + 0x18) if flags & 0x400 else cache.f(pb)
            maximum = cache.f(cl + 0x1c) if flags & 0x800 else cache.f(pb + 4)
            pairs.add((word(max(minimum, real(0x3a83126f))), word(maximum)))
            count += 1
        maps.append(dict(path=str(path), sha256=sha(path), sounds=count))
    rows = set()
    for minimum, maximum in sorted(pairs):
        lower, upper = real(minimum), real(maximum)
        distances = [0., lower / 2, lower, upper, max(lower, upper) * 2]
        distances += [lower + (upper - lower) * n / 64 for n in range(65)]
        for boundary in (minimum, maximum):
            if 0 < boundary < 0x7f7fffff:
                distances += [real(boundary - 1), real(boundary + 1)]
        for distance in distances:
            rows.add((minimum, maximum, word(distance)))
    # Explicit corner controls, separate from authored metadata observations.
    for minimum, maximum in ((0, 0), (0x3f800000, 0x3f800000),
                             (0x41200000, 0x3f800000)):
        for distance in (0, 0x80000000, 0x3f800000, 0x41200000, 0x7f800000):
            rows.add((minimum, maximum, distance))
    return sorted(rows), maps


def execute(path, entry, inputs, epsilon, one):
    image = Xbe(path)
    machine = Uc(UC_ARCH_X86, UC_MODE_32)
    machine.mem_map(0x10000, 0x800000)
    for section in image.sections:
        machine.mem_write(section.va, image.section_bytes(section))
    stack, stop = 0x1008000, 0x100f000
    machine.mem_map(0x1000000, 0x10000)
    output = []
    for minimum, maximum, distance in inputs:
        # At12C434, ESP points at saved ESI/EBP/EBX and the two locals.
        # The source later restores those registers and returns with RET4.
        machine.mem_write(stack, struct.pack('<7I', 0, 0, 0, 0, minimum, stop, distance))
        machine.reg_write(R.UC_X86_REG_ESP, stack)
        machine.reg_write(R.UC_X86_REG_XMM0, maximum)
        machine.reg_write(R.UC_X86_REG_FPCW, 0x027f)
        machine.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
        machine.emu_start(entry, stop, count=1000)
        assert machine.reg_read(R.UC_X86_REG_EIP) == stop
        assert machine.reg_read(R.UC_X86_REG_ESP) == stack + 28
        output.append(machine.reg_read(R.UC_X86_REG_XMM0) & 0xffffffff)
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    pair = json.loads((ROOT / 'analysis/multiplayer/integration/sound-admission-native/'
                       'paired-v2/report.json').read_text())
    owner = next(r for r in pair['owners'] if r['old'] == '0012c370')
    entries = [0x12c434, int(owner['u5'], 16) + 0xc4]
    assert Xbe(OLD).read(entries[0], 8) == Xbe(NEW).read(entries[1], 8)
    pins = ['aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377',
            '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd']
    assert [sha(p) for p in (OLD, NEW)] == pins
    image = Xbe(OLD)
    epsilon, = struct.unpack('<I', image.read(0x46504c, 4))
    one, = struct.unpack('<I', image.read(0x465030, 4))
    assert image.read(0x4650e0, 4) == struct.pack('<I', 0x3a83126f)
    inputs, maps = controls()
    outputs = [execute(p, e, inputs, epsilon, one) for p, e in zip((OLD, NEW), entries)]
    assert outputs[0] == outputs[1]
    fixture = f'K {epsilon:08x} {one:08x}\n' + ''.join(
        ' '.join(f'{v:08x}' for v in (*row, answer)) + '\n'
        for row, answer in zip(inputs, outputs[0]))
    (args.output / 'fixtures.tsv').write_text(fixture)
    report = dict(boundary=__doc__, image_sha256=pins, entries=[hex(e) for e in entries],
                  cases=len(inputs), authored_maps=maps, paired_outputs_equal=True,
                  instruction_patches=[], intercepted_callees=[],
                  fixture_sha256=sha(args.output / 'fixtures.tsv'), tool_sha256=sha(__file__))
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
