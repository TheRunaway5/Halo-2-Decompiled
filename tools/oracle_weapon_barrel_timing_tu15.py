#!/usr/bin/env python3
"""Execute complete untouched TU1.1/TU1.5 barrel timing owners on authored rows.

Object storage, state controls, time and tag-table bindings are declared ABI
inputs. Root block pointers are relocated to the copied literal barrel rows.
Every reached instruction, including CRT floor/conversion, remains original.
No game service is substituted. This is not trigger/heat/projectile execution.
"""
import argparse
import hashlib
import json
import random
import struct
from pathlib import Path

from weapon_reference_versions import images
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[1]
WEAPON, OTHER, UNIT, DEF, BARRELS, POOL, HEADERS, TAGS, TIME, OUT = [
    0x1000000 + n * 0x2000 for n in range(10)]
STACK, STOP = 0x2010000, 0x3000000
WI, OI, UI = 0x12340000, 0x12340001, 0x12340002
CONFIG = {
    'TU1.1': ((0x1062b0, 0x108010, 0x108110, 0x107f80),
              (0x4eb114, 0x4e78d0, 0x518444), (0x465030, 0x46504c)),
    'TU1.5': ((0x1071d0, 0x108f30, 0x109030, 0x108ea0),
              (0x4f0414, 0x4ecbd0, 0x51df54), (0x46a0d0, 0x46a0ec)),
}


def u32(raw, at=0):
    return struct.unpack_from('<I', raw, at)[0]


def pack(words):
    return struct.pack('<' + 'I' * len(words), *(x & 0xffffffff for x in words))


def state(counter, shots, rate, recovery, flags):
    raw = bytearray(52)
    struct.pack_into('<bBhHH', raw, 0, counter, 1, 0, flags, shots)
    struct.pack_into('<III', raw, 0x10, rate, 0x3e800000, 0x3f000000)
    struct.pack_into('<II', raw, 0x24, 0x3e000000, recovery)
    struct.pack_into('<I', raw, 0x30, 0xffffffff)
    return bytes(raw)


class Original:
    def __init__(self, label, image):
        self.label, self.image = label, image
        self.entries, globals_, addresses = CONFIG[label]
        self.g = g = Uc(UC_ARCH_X86, UC_MODE_32)
        g.mem_map(0x10000, 0x800000)
        for section in image.sections:
            g.mem_write(section.va, image.section_bytes(section))
        g.mem_map(0x1000000, 0x100000)
        g.mem_map(0x2000000, 0x20000)
        g.mem_map(STOP, 4096)
        for at, value in zip(globals_, (TAGS, POOL, TIME)):
            g.mem_write(at, pack([value]))
        g.mem_write(POOL + 0x44, pack([HEADERS]))
        for index, pointer, kind in ((0, WEAPON, 2), (1, OTHER, 2), (2, UNIT, 0)):
            g.mem_write(HEADERS + index * 12, struct.pack('<HBBII', 0x1234, 3, kind, 0, pointer))
        g.mem_write(TAGS + 7 * 16 + 8, pack([DEF]))
        g.mem_write(WEAPON, pack([7]))
        g.mem_write(OTHER, pack([7]))
        g.mem_write(UNIT + 0x212, b'\x00\x01')
        g.mem_write(UNIT + 0x218, pack([WI, OI]))
        self.constants = [u32(image.read(at, 4)) for at in addresses]
        self.instructions = {}
        g.hook_add(UC_HOOK_CODE, self.audit)

    def audit(self, g, address, size, _):
        if address not in self.instructions:
            raw = bytes(g.mem_read(address, size))
            assert raw == self.image.read(address, size), (self.label, hex(address))
            self.instructions[address] = raw.hex()

    def definition(self, root, barrels):
        self.g.mem_write(DEF, root)
        self.g.mem_write(DEF + 0x2d4, pack([BARRELS]))
        for index, raw in enumerate(barrels):
            self.g.mem_write(BARRELS + index * 0xec, raw)

    def call(self, operation, index, raw_state, rate, spent, alternate, other):
        g = self.g
        g.mem_write(WEAPON + 0x1a4 + index * 0x34, raw_state)
        if alternate != 0xffffffff:
            g.mem_write(WEAPON + 0x1a5 + (not index) * 0x34, bytes([alternate]))
        g.mem_write(OTHER + 0x1a4 + index * 0x34,
                    struct.pack('<bB', other[1], other[0]) if other else b'\x7f\x00')
        g.mem_write(WEAPON + 0x12c, pack([1 if other else 0]))
        g.mem_write(WEAPON + 0x154, pack([UI if other else 0xffffffff]))
        g.mem_write(WEAPON + 0x184, pack([spent]))
        g.mem_write(TIME + 2, struct.pack('<h', rate))
        g.mem_write(OUT, bytes(8))
        args = ([WI, OUT, OUT + 4] if operation == 0 else
                [index] if operation in (1, 3) else [WI, index])
        g.mem_write(STACK, pack([STOP, *args]))
        g.reg_write(UC_X86_REG_ESP, STACK)
        g.reg_write(UC_X86_REG_EAX, index if operation == 0 else WI)
        g.reg_write(UC_X86_REG_FPCW, 0x027f)
        g.reg_write(UC_X86_REG_FPSW, 0)
        g.reg_write(UC_X86_REG_FPTAG, 0xffff)
        g.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        g.emu_start(self.entries[operation], STOP, count=10000)
        assert g.reg_read(UC_X86_REG_EIP) == STOP
        assert g.reg_read(UC_X86_REG_ESP) == STACK + 4 + len(args) * 4
        after = bytes(g.mem_read(WEAPON + 0x1a4 + index * 0x34, 52))
        outputs = [g.reg_read(UC_X86_REG_EAX) & 255,
                   g.mem_read(OUT, 1)[0], g.mem_read(OUT + 4, 1)[0]] if operation == 0 else [0, 0, 0]
        return outputs + list(struct.unpack('<13I', after))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--scope', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--fixture', type=Path, required=True)
    args = ap.parse_args()
    scope = json.loads(args.scope.read_text())
    originals = [Original(label, image) for label, image, _ in images() if label in CONFIG]
    assert len(originals) == 2 and originals[0].constants == originals[1].constants
    rng = random.Random(0x1071d0)
    seen, definitions, lines = set(), [], ['# Independent complete TU1.1/TU1.5 barrel timing; 29 input and 16 output words.']
    counts = [0] * 4
    for map_ in scope['maps']:
        for weapon in map_['weapons']:
            if not weapon['resident'] or not weapon['barrels']:
                continue
            root = bytes.fromhex(weapon['root_hex'])
            barrels = [bytes.fromhex(b['raw_hex']) for b in weapon['barrels']]
            for index, barrel in enumerate(barrels):
                fields = [u32(barrel, at) for at in (0, 4, 8, 0x20, 0x24)]
                fields += [u32(root, 0x258)]
                minimum, maximum = struct.unpack_from('<hh', barrel, 0x1c)
                fields += [minimum & 65535, maximum & 65535]
                key = (*fields, len(barrels), index)
                if key in seen:
                    continue
                seen.add(key)
                definitions.append(dict(map=map_['path'], map_sha256=map_['sha256'],
                    tag=weapon['tag'], barrel=index, fields=fields, root_hex=root.hex(),
                    barrel_hex=[b.hex() for b in barrels]))
                for original in originals:
                    original.definition(root, barrels)
                controls = [(counter, shots, firing_rate, 0, 0, 0, None)
                    for counter in (-128, -1, 0, 1, 127)
                    for shots in sorted({0, 1, max(0, minimum - 1), minimum & 65535,
                                         maximum & 65535, 65535})
                    for firing_rate in (0, 0x3e800000, 0x3f800000)]
                controls += [(rng.randrange(-128, 128), rng.randrange(0, 16),
                    rng.choice((0, 0x3e800000, 0x3f000000, 0x3f800000, 0x7fc12345)),
                    rng.choice((0, 0x3e800000, 0x3f400000, 0x3f800000, 0x3fe00000)),
                    rng.choice((0, 0x40, 0x43, 0x8041)), rng.choice((0, 0x3f000000)),
                    rng.choice((None, (1, 0), (1, 1), (2, 0)))) for _ in range(32)]
                for counter, shots, firing_rate, recovery, flags, spent, other in controls:
                    alternate = rng.randrange(4) if len(barrels) > 1 else 0xffffffff
                    raw = state(counter, shots, firing_rate, recovery, flags)
                    inputs = [0, *fields, 30, *originals[0].constants, spent, alternate,
                              other[0] if other else 0xffffffff, other[1] & 255 if other else 0,
                              *struct.unpack('<13I', raw)]
                    results = [o.call(0, index, raw, 30, spent, alternate, other) for o in originals]
                    assert results[0] == results[1], (weapon['tag'], 'gate')
                    assert len(inputs) == 29
                    lines.append(' '.join(f'{v:08x}' for v in inputs + results[0]))
                    counts[0] += 1
                for operation in (1, 2, 3):
                    for recovery in (0, 0x3e800000, 0x3f400000, 0x3f800000, 0x3fe00000):
                        for tick_rate in (30, 60):
                            raw = state(17, 7, 0x3f000000, recovery, 0x8043)
                            inputs = [operation, *fields, tick_rate, *originals[0].constants,
                                      0, 0xffffffff, 0xffffffff, 0, *struct.unpack('<13I', raw)]
                            results = [o.call(operation, index, raw, tick_rate, 0, 0xffffffff, None)
                                       for o in originals]
                            assert results[0] == results[1], (weapon['tag'], operation)
                            lines.append(' '.join(f'{v:08x}' for v in inputs + results[0]))
                            counts[operation] += 1
    args.fixture.parent.mkdir(parents=True, exist_ok=True)
    args.fixture.write_text('\n'.join(lines) + '\n')
    report = dict(schema='weapon-barrel-timing-original-v1', scope=__doc__,
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        scope_sha256=hashlib.sha256(args.scope.read_bytes()).hexdigest(),
        fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
        definitions=definitions, case_counts=counts, total_cases=sum(counts),
        supplied_services=[], instruction_patches=[],
        images=[dict(label=o.label, sha256=hashlib.sha256(o.image.data).hexdigest(),
                     entries=[f'{e:08x}' for e in o.entries],
                     instructions={f'{a:08x}': raw for a, raw in sorted(o.instructions.items())})
                for o in originals])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(json.dumps(dict(definitions=len(definitions), case_counts=counts,
                          total_cases=sum(counts), fixture=str(args.fixture))), flush=True)


if __name__ == '__main__':
    main()
