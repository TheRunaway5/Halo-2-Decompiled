#!/usr/bin/env python3
"""Original TU1.5 B4C40 (vehicle rider update) fixture for the native port.

Executes the untouched English TU1.5 B4C40 bytes, with its only callees
D2B60/D2BD0 (rider iterator), in Unicorn over declared synthetic object
trees: vehicles with riders, nested units with and without +4 bit 26,
non-unit children, parent chains the iterator climbs, header/data type
mismatches, and finite float fields including denormals. Each case keeps the
seeded words and every byte the original wrote outside its stack, so
tests/native_tu15_vehicle_child_update_tests.cpp replays the same inputs
through the native kernel. NaN/infinity operands are excluded: emulator NaN
propagation is not hardware evidence. Default MXCSR 0x1F80, x87 CW 0x027F.

Run with capstone/unicorn available, for example:
  python3 tools/run_test.py -- env PYTHONPATH=<site-packages> python3 \
    tools/oracle_vehicle_child_update_tu15.py --output tests/fixtures/tu15_vehicle_child_update.original.json
"""
import argparse, hashlib, json, random, struct, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / 'tools/reference/halo2-decompiled/tools')]
from xbe import Xbe  # noqa: E402
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_MEM_WRITE, UC_HOOK_CODE  # noqa: E402
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_ESP,  # noqa: E402
                               UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_EBP,
                               UC_X86_REG_MXCSR)

XBE = Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/'
           'Halo 2 DLC/content/u5English/default.xbe')
PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
ENTRY, ITERATOR_INIT, ITERATOR_NEXT = 0xb4c40, 0xd2b60, 0xd2bd0
OBJECTS_GLOBAL, TIME_GLOBAL = 0x4ecbd0, 0x51df54
ARRAY, HEADERS, TIME, DATA, INPUTS = 0x1000000, 0x1001000, 0x1002000, 0x1100000, 0x1200000
STACK, STOP = 0x2000000, 0x3000000
STRIDE = 0x400
NONE = 0xffffffff


def f2u(x):
    return struct.unpack('<I', struct.pack('<f', x))[0]


def finite_float(rng):
    choice = rng.random()
    if choice < .06:
        return struct.unpack('<f', struct.pack('<I', rng.randrange(1, 0x800000) | (rng.random() < .5) << 31))[0]
    if choice < .1:
        return 0.0 if rng.random() < .5 else -0.0
    return struct.unpack('<f', struct.pack('<f', rng.uniform(-300, 300) * (10 ** rng.randint(-3, 2))))[0]


class Case:
    def __init__(self, rng):
        self.words = {}
        self.rng = rng
        self.count = 0

    def word(self, at, value):
        self.words[at] = value & 0xffffffff

    def add(self, header_type, data_type, ride):
        index = self.count
        self.count += 1
        handle = (0x1234 + index) << 16 | index
        data = DATA + index * STRIDE
        self.word(HEADERS + index * 12, header_type << 24 | self.rng.randrange(0, 0x10000) << 8 & 0xff0000)
        self.word(HEADERS + index * 12 + 8, data)
        flags = self.rng.randrange(0, 1 << 32) & ~(1 << 26) | (ride << 26)
        self.word(data + 4, flags)
        self.word(data + 0xc, NONE)
        self.word(data + 0x10, NONE)
        self.word(data + 0x14, NONE)
        self.word(data + 0xa8, (self.rng.randrange(0, 0x10000)) | data_type << 16 | self.rng.randrange(0, 256) << 24)
        for offset in (0x64, 0x68, 0x6c, 0x88, 0x8c, 0x90, 0x270, 0x274, 0x278):
            self.word(data + offset, f2u(finite_float(self.rng)))
        self.word(data + 0x134, self.rng.randrange(0, 1 << 32))
        self.word(data + 0x1fc, self.rng.randrange(0, 1 << 32))
        return handle, data

    def link(self, parent, children):
        (_, pdata) = parent
        previous = None
        for child in children:
            (handle, cdata) = child
            self.word(cdata + 0x14, parent[0])
            if previous is None:
                self.word(pdata + 0x10, handle)
            else:
                self.word(previous[1] + 0xc, handle)
            previous = child


def build(rng, index):
    case = Case(rng)
    unit = lambda: rng.choice([0, 1])
    other = lambda: rng.choice([2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 0x21, 0x40])
    kind = index % 6
    # The vehicle B4C40 is called on (header type 1 unless the case tests the gate).
    gate = 1 if kind != 5 else rng.choice([0, 2, 3])
    vehicle = case.add(gate, 1 if rng.random() < .9 else unit(), rng.random() < .5)
    root = vehicle
    if kind == 4:
        # Parent chain: D2B60 climbs while the child has bit 26 and the
        # parent is a unit (data +AA in 0/1).
        top = case.add(1, unit() if rng.random() < .8 else other(), rng.random() < .3)
        case.word(vehicle[1] + 4, case.words[vehicle[1] + 4] | 1 << 26 if rng.random() < .8 else
                  case.words[vehicle[1] + 4])
        case.link(top, [vehicle] + [case.add(rng.choice([0, 1, 2]), rng.choice([0, 1, 2]), rng.random() < .5)
                                    for _ in range(rng.randint(0, 2))])
        root = top
    # Children of the vehicle: units (riders), turrets with their own riders,
    # and non-unit attachments.
    children = []
    for _ in range(rng.randint(0, 5)):
        data_type = unit() if rng.random() < .7 else other()
        child = case.add(data_type if rng.random() < .9 else other(), data_type, rng.random() < .5)
        grandchildren = []
        for _ in range(rng.randint(0, 3) if rng.random() < .6 else 0):
            gtype = unit() if rng.random() < .7 else other()
            grandchildren.append(case.add(gtype, gtype, rng.random() < .5))
        case.link(child, grandchildren)
        children.append(child)
    case.link(vehicle, children)
    case.word(ARRAY + 0x44, HEADERS)
    case.word(OBJECTS_GLOBAL, ARRAY)
    case.word(TIME_GLOBAL, TIME)
    case.word(TIME, rng.randrange(0, 1 << 32) & 0xffff0000 | 30)
    case.word(TIME + 4, f2u(rng.choice([1 / 30, 1 / 60, 1 / 15, 1 / 29.97, finite_float(rng)])))
    position = linear = 0
    if rng.random() < .55 or kind == 3:
        position = INPUTS
        for lane in range(3):
            case.word(INPUTS + lane * 4, f2u(finite_float(rng)))
    if rng.random() < .65 or kind in (0, 1):
        linear = INPUTS + 0x10
        for lane in range(3):
            case.word(INPUTS + 0x10 + lane * 4, f2u(finite_float(rng)))
    return case, vehicle[0], position, linear, root


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--cases', type=int, default=240)
    parser.add_argument('--seed', type=int, default=0xb4c40)
    args = parser.parse_args()
    data = XBE.read_bytes()
    assert hashlib.sha256(data).hexdigest() == PIN, 'TU1.5 default.xbe pin mismatch'
    image = Xbe(XBE)
    rng = random.Random(args.seed)
    rows = []
    executed = set()
    for index in range(args.cases):
        case, vehicle, position, linear, root = build(rng, index)
        emu = Uc(UC_ARCH_X86, UC_MODE_32)
        emu.mem_map(0x10000, 0x580000)
        for section in image.sections:
            body = image.section_bytes(section)
            if section.va + len(body) <= 0x590000:
                emu.mem_write(section.va, body)
        for base in (ARRAY, DATA, INPUTS, STACK - 0x10000, STOP):
            emu.mem_map(base, 0x100000 if base != STACK - 0x10000 else 0x20000)
        # Zero the globals this case owns so image .data never leaks in.
        for at, value in case.words.items():
            emu.mem_write(at, struct.pack('<I', value))
        writes = {}
        stack_low, stack_high = STACK - 0x10000, STACK + 0x10000

        def on_write(uc, access, address, size, value, user):
            if stack_low <= address < stack_high:
                return
            for i in range(size):
                writes[address + i] = None

        def on_code(uc, address, size, user):
            executed.add(address)

        emu.hook_add(UC_HOOK_MEM_WRITE, on_write)
        emu.hook_add(UC_HOOK_CODE, on_code, begin=0x12000, end=0x383d40)
        esp = STACK - 4
        emu.mem_write(esp, struct.pack('<I', STOP))
        for register, value in ((UC_X86_REG_EAX, vehicle), (UC_X86_REG_EDX, position), (UC_X86_REG_ESI, linear),
                                (UC_X86_REG_ESP, esp), (UC_X86_REG_EBX, 0x5a5a5a5a), (UC_X86_REG_ECX, 0x3c3c3c3c),
                                (UC_X86_REG_EDI, 0x7e7e7e7e), (UC_X86_REG_EBP, 0x6b6b6b6b)):
            emu.reg_write(register, value)
        emu.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        emu.emu_start(ENTRY, STOP, count=200000)
        # Compact form: seeded words as one hex string of (address, value)
        # little-endian pairs; written bytes as contiguous (address, bytes) runs.
        words = b''.join(struct.pack('<II', at, value) for at, value in sorted(case.words.items()))
        runs = []
        for at in sorted(writes):
            if runs and runs[-1][0] + len(runs[-1][1]) == at:
                runs[-1][1].append(at)
            else:
                runs.append([at, [at]])
        written = [[start, bytes(emu.mem_read(start, len(span))).hex()] for start, span in runs]
        rows.append(dict(vehicle=vehicle, position=position, linear=linear, iterator_root=root,
                         words=words.hex(), written=written))
    listing_sites = sorted(executed)
    report = dict(reference='English TU1.5 default.xbe', xbe_sha256=PIN, entry='B4C40', callees=['D2B60', 'D2BD0'],
                  executor='unicorn ' + __import__('unicorn').__version__, mxcsr=0x1f80, x87_control_word=0x27f,
                  seed=args.seed, cases=rows, executed_sites=len(listing_sites),
                  executed_ranges=dict(b4c40=len([a for a in listing_sites if 0xb4c40 <= a < 0xb4da0]),
                                       d2b60=len([a for a in listing_sites if 0xd2b60 <= a < 0xd2bc9]),
                                       d2bd0=len([a for a in listing_sites if 0xd2bd0 <= a < 0xd2db0])),
                  nan_or_infinity_operands=False,
                  source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, separators=(',', ':')) + '\n')
    print(json.dumps(dict(cases=len(rows), executed_sites=len(listing_sites), ranges=report['executed_ranges'],
                          bytes_written=sum(len(w[1]) // 2 for r in rows for w in r['written']))))


if __name__ == '__main__':
    main()
