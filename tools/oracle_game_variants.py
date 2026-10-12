#!/usr/bin/env python3
"""Run original retail/TU1.1 variant defaults, descriptors, setters and getters.

Both executable images are unchanged. The globals pointer is NULL so the
initializer omits the localized name only. No gameplay, allocator, session or
objective callback is replaced or claimed. Native code is never imported.
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
from inspect_multiplayer import Cache
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_FPCW, UC_X86_REG_MXCSR)

VERSIONS = {
    'retail': dict(path='assets/disc/default.xbe',
        pin='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d',
        globals=0x4e034c, init=0x19d220, setter=0x2373be,
        getter=0x2374f0, normalize=0x19d650, table=0x470828),
    'tu11': dict(path='assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',
        pin='aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377',
        globals=0x4e791c, init=0x1a0a90, setter=0x2394ea,
        getter=0x23961c, normalize=0x1a0ec0, table=0x477d10),
}
MENU_PIN = '1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b'
VARIANT, RESULTS, STACK, STOP = 0x1000000, 0x1001000, 0x200ff00, 0x3000000
VALUES = (-2147483648, -65537, -32769, -1, 0, 1, 2, 255, 256,
          32768, 65535, 2147483647)


class Source:
    def __init__(self, spec):
        self.spec = spec
        self.image = Xbe(ROOT / spec['path'])
        assert hashlib.sha256(self.image.data).hexdigest() == spec['pin']
        self.g = Uc(UC_ARCH_X86, UC_MODE_32)
        self.g.mem_map(0x10000, 0x800000)
        for section in self.image.sections:
            self.g.mem_write(section.va, self.image.section_bytes(section))
        self.g.mem_map(VARIANT, 0x2000)
        self.g.mem_map(0x2000000, 0x10000)
        self.g.mem_map(STOP, 0x1000)
        self.g.mem_write(spec['globals'], bytes(4))
        self.executed = set()
        self.g.hook_add(UC_HOOK_CODE, lambda g, a, size, _: self.executed.add(a))

    def u(self, at):
        return struct.unpack('<I', self.g.mem_read(at, 4))[0]

    def call(self, address, argv=(), registers=()):
        self.g.mem_write(STACK, struct.pack('<I', STOP))
        for i, value in enumerate(argv):
            self.g.mem_write(STACK + 4 + i * 4, struct.pack('<I', value & 0xffffffff))
        self.g.reg_write(UC_X86_REG_ESP, STACK)
        self.g.reg_write(UC_X86_REG_FPCW, 0x027f)
        self.g.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        for reg, value in registers:
            self.g.reg_write(reg, value & 0xffffffff)
        self.g.emu_start(address, STOP, count=200000)
        assert self.g.reg_read(UC_X86_REG_EIP) == STOP
        value = self.g.reg_read(UC_X86_REG_EAX)
        return value if value < 0x80000000 else value - 0x100000000

    def init(self, engine):
        self.g.mem_write(VARIANT, b'\xa5' * 0x130)
        self.call(self.spec['init'], (VARIANT, engine))

    def set(self, parameter, value):
        self.call(self.spec['setter'], registers=(
            (UC_X86_REG_EAX, parameter), (UC_X86_REG_EBX, value),
            (UC_X86_REG_ESI, VARIANT)))

    def get(self, parameter):
        return self.call(self.spec['getter'], registers=(
            (UC_X86_REG_EAX, parameter), (UC_X86_REG_EDI, VARIANT)))

    def raw(self):
        return bytes(self.g.mem_read(VARIANT, 0x130)).hex()


def run(spec, variants):
    source = Source(spec)
    descriptors = []
    for parameter in range(112):
        entry = source.u(spec['table'] + parameter * 4)
        if entry:
            source.call(entry, (RESULTS, RESULTS + 4, RESULTS + 8))
            descriptor = dict(kind=source.u(RESULTS), offset=source.u(RESULTS + 4),
                              operand=source.u(RESULTS + 8))
        else:
            descriptor = dict(kind=-1, offset=0, operand=0)
        descriptors.append(descriptor)
    defaults = []
    for engine in range(7):
        source.init(engine)
        defaults.append(dict(engine=engine, raw=source.raw(),
                             values=[source.get(p) for p in range(112)]))
    builtins = []
    for index, variant in enumerate(variants):
        source.init(variant['engine'])
        for parameter, value in variant['overrides'].items():
            source.set(parameter, value)
        raw = source.raw()
        values = [source.get(p) for p in range(112)]
        # Key sorting coincidentally gives the same buffers for these39 rows.
        # The separate ordered-write oracle covers aliases and duplicate IDs.
        source.init(variant['engine'])
        for parameter, value in sorted(variant['overrides'].items()):
            source.set(parameter, value)
        assert source.raw() == raw, ('override order matters', index)
        accepted = source.call(spec['normalize'], registers=((UC_X86_REG_EBX, VARIANT),))
        assert source.raw() == raw, ('authored variant needs normalization', index)
        assert accepted & 255, ('authored variant failed original validation', index)
        builtins.append(dict(index=index, engine=variant['engine'], name=variant['name'],
            file_offset=variant['file_offset'], overrides=variant['overrides'], raw=raw,
            values=values, normalization_valid=bool(accepted & 255)))
    setters = []
    for parameter in range(112):
        engine = parameter % 7
        for value in VALUES:
            source.init(engine)
            source.set(parameter, value)
            setters.append(dict(engine=engine, parameter=parameter, input=value,
                                raw=source.raw(), value=source.get(parameter)))
    # Unsupported source parameter IDs have no descriptor and perform no write.
    for parameter in (-1, 112, 2147483647):
        source.init(0)
        before = source.raw()
        source.set(parameter, 1)
        assert source.raw() == before and source.get(parameter) == 0
        setters.append(dict(engine=0, parameter=parameter, input=1,
                            raw=before, value=0))
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    lines = []
    for address in sorted(source.executed):
        ins = next(decoder.disasm(source.image.read(address, 15), address))
        lines.append(f'{address:08x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}')
    return dict(descriptors=descriptors, defaults=defaults, builtins=builtins,
                setters=setters), '\n'.join(lines) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path,
        default=ROOT / 'analysis/multiplayer/game-variants/original.json')
    parser.add_argument('--fixture', type=Path)
    parser.add_argument('--descriptor-table', type=Path)
    args = parser.parse_args()
    cache = Cache(ROOT / 'assets/disc/maps/mainmenu.map')
    assert hashlib.sha256(cache.data).hexdigest() == MENU_PIN
    catalogue = cache.inspect_variants()
    assert len(catalogue['variants']) == 39
    reports = {}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    for name, spec in VERSIONS.items():
        reports[name], listing = run(spec, catalogue['variants'])
        (args.output.parent / f'{name}-instructions.txt').write_text(listing)
    assert reports['retail'] == reports['tu11'], 'retail/TU variant contracts differ'
    report = dict(scope=__doc__, tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        mainmenu_sha256=MENU_PIN, source_versions=VERSIONS, source=reports['retail'],
        parameters=catalogue['parameters'])
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    if args.fixture:
        lines = ['# Original retail/TU1.1 variant instructions; no native code imported.',
                 '# retail ' + VERSIONS['retail']['pin'],
                 '# tu11 ' + VERSIONS['tu11']['pin'], '# mainmenu ' + MENU_PIN]
        for row in reports['retail']['defaults']:
            lines.append(' '.join(map(str, ['D', row['engine'], row['raw'], *row['values']])))
        for row in reports['retail']['builtins']:
            overrides = [n for pair in row['overrides'].items() for n in pair]
            lines.append(' '.join(map(str, ['V', row['index'], row['engine'],
                json.dumps(row['name'], ensure_ascii=False), len(row['overrides']),
                *overrides, row['raw'], *row['values']])))
        for row in reports['retail']['setters']:
            lines.append(' '.join(map(str, ['S', row['engine'], row['parameter'],
                row['input'], row['raw'], row['value']])))
        args.fixture.parent.mkdir(parents=True, exist_ok=True)
        args.fixture.write_text('\n'.join(lines) + '\n')
    if args.descriptor_table:
        lines = ['// Generated from unchanged original descriptor instructions.',
                 '// Retail470828 and TU477D10 agree; see oracle_game_variants.py.']
        for parameter, d in enumerate(reports['retail']['descriptors']):
            lines.append(f'  {{{d["kind"]}, 0x{d["offset"]:x}, 0x{d["operand"]:08x}u}}, // {parameter}')
        args.descriptor_table.write_text('\n'.join(lines) + '\n')
    print(json.dumps(dict(versions=2, defaults=7, builtins=39,
        builtin_parameter_reads=39 * 112, setter_cases=len(reports['retail']['setters']),
        normalized_builtin_changes=0, report=str(args.output))))


if __name__ == '__main__':
    main()
