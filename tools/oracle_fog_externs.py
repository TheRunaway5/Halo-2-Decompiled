#!/usr/bin/env python3
"""Whole original scalar39/color41 dispatch calls in retail/TU1.1/TU1.5.

Finite fog datum/near/far/forward and addressed default-RGB records are
declared input contexts. All original dispatch/provider instructions execute;
no return, reciprocal, color, pointer producer or instruction is substituted.
Actual SSE intermediates and MXCSR nearest/gradual controls are observed.
This proves these consumers, not fog loading/visibility/layer admission,
default-pointer initialization, a complete pass or framebuffer output.
"""
import argparse
import hashlib
import itertools
import json
import re
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_game_over import SPECS
from oracle_mp_full_scoreboard_1_5 import _Code, literal
from inspect_render_primitives_title_updates import prefix
from xbe import Xbe
from capstone.x86 import X86_OP_MEM

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'analysis/multiplayer/game-variants/fog-externs-20261011'
FIXTURE = ROOT / 'tests/fixtures/fog_externs_original.json'
BASE, DEFAULT, OUTPUT, STACK, STOP = 0x20000000, 0x20000000, 0x20000100, 0x200fe000, 0x200ff000


def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def pack(values):
    return struct.pack('<' + 'I' * len(values), *values)


def inputs():
    intervals = [(bits(a), bits(b)) for a, b in ((10., 30.), (.3, 1.1), (-.3, 1.1), (0., 1.), (-0., 1.),
                 (1., 1.), (30., 10.), (-30., -10.), (-10., -30.), (0., 0.), (-0., 0.), (.5, 1.), (0., 32768.), (100., 100.25))]
    intervals.extend(((0x3f800000, 0x3f800001), (0x3f7fffff, 0x3f800000)))
    forwards = [[bits(v) for v in row] for row in ((1., -.5, .25), (.3, -1.1, 1.3), (0., -0., 0.), (-0., 0., -0.), (-3., 7., -.1))]
    forwards.append([0x00800000, 1, 0x80000001])
    defaults = [[bits(v) for v in row] for row in ((.25, .5, .75), (-.1, 1.1, -0.), (0., 0., 0.))]
    defaults.append([1, 0x80800000, bits(.3)])
    return [dict(datum=datum, near_bits=near, far_bits=far, forward_bits=forward, default_rgb_bits=default)
            for (near, far), forward, default, datum in itertools.product(intervals, forwards, defaults, (0xffffffff, 7))]


class Source:
    def __init__(self, name, retail):
        self.name, self.spec = name, SPECS[name]
        self.image = Xbe(ROOT / self.spec['path'])
        assert sha(self.image.data) == self.spec['pin']
        self.code = _Code(self.image, '.text')
        self.g = Uc(UC_ARCH_X86, UC_MODE_32)
        self.g.mem_map(0x10000, 0x800000)
        for section in self.image.sections:
            self.g.mem_write(section.va, self.image.section_bytes(section))
        self.g.mem_map(BASE, 0x100000)
        self.executed, self.observations = {}, []
        self.entries, self.branches, self.fields = {}, {}, {}
        sec = self.image.section('.text')
        raw = self.image.section_bytes(sec)
        for owner, base, index in (('scalar', 0x3385d, 39), ('color', 0x33ef9, 41)):
            pattern = prefix(retail, base, 32)
            matches = [sec.va + m.start() for m in re.finditer(pattern, raw, re.S)]
            assert len(matches) == 1, (name, owner, matches)
            branch = matches[0]
            dispatchers = []
            for match in re.finditer(re.escape(struct.pack('<I', branch)), raw):
                table = sec.va + match.start() - index * 4
                signature = b'\xff\x24\x85' + struct.pack('<I', table)
                dispatchers.extend(sec.va + m.start() for m in re.finditer(re.escape(signature), raw))
            assert len(dispatchers) == 1, (name, owner, dispatchers)
            self.entries[owner] = dispatchers[0]
            self.branches[owner] = branch
            f, ins = literal(self.code, branch, set())
            operands = [o.mem.disp for i in ins for o in i.operands if o.type == X86_OP_MEM and not o.mem.base and not o.mem.index]
            if owner == 'scalar':
                # Owner instruction positions bind actual globals, not reused
                # retail addresses or inferred title-update deltas.
                seq = [i for i in ins if i.address >= branch]
                self.fields['datum'] = seq[0].operands[0].mem.disp
                loads = [i for i in seq if i.mnemonic == 'movss' and i.operands[1].type == X86_OP_MEM]
                self.fields['far'], self.fields['near'] = (loads[n].operands[1].mem.disp for n in (0, 1))
            else:
                seq = [i for i in ins if i.address >= branch]
                loads = [i for i in seq if i.mnemonic == 'movss' and i.operands[1].type == X86_OP_MEM]
                self.fields['forward'] = loads[2].operands[1].mem.disp
                pointer = [i for i in seq if i.mnemonic == 'mov' and len(i.operands) == 2 and i.operands[1].type == X86_OP_MEM and not i.operands[1].mem.base]
                assert len(pointer) == 1
                self.fields['default_pointer'] = pointer[0].operands[1].mem.disp
        self.g.hook_add(UC_HOOK_CODE, self.hook)

    def hook(self, g, pc, size, unused):
        assert bytes(g.mem_read(pc, size)) == self.image.read(pc, size), f'{self.name}:changed{pc:08x}'
        self.executed[pc] = size
        ins = self.code.at(pc)
        if ins.mnemonic in ('movss', 'comiss', 'subss', 'divss', 'xorps', 'mulss', 'movaps', 'ret'):
            self.observations.append(dict(pc=pc, mnemonic=ins.mnemonic, operands=ins.op_str,
                                          fpcw=g.reg_read(UC_X86_REG_FPCW), mxcsr=g.reg_read(UC_X86_REG_MXCSR),
                                          xmm_low_bits=[g.reg_read(reg) & 0xffffffff for reg in (UC_X86_REG_XMM0, UC_X86_REG_XMM1, UC_X86_REG_XMM2, UC_X86_REG_XMM3)]))

    def w(self, address, values):
        self.g.mem_write(address, pack(values))

    def run(self, row):
        self.w(self.fields['datum'], [row['datum']])
        self.w(self.fields['near'], [row['near_bits']])
        self.w(self.fields['far'], [row['far_bits']])
        self.w(self.fields['forward'], row['forward_bits'])
        self.w(self.fields['default_pointer'], [DEFAULT])
        default = pack([0xd15ea5e5, *row['default_rgb_bits'], 0x5341f00d])
        self.g.mem_write(DEFAULT, default)
        self.g.mem_write(OUTPUT - 4, b'\xa5' * 20)
        outputs, observations = {}, {}
        for owner, index in (('scalar', 39), ('color', 41)):
            self.w(STACK, [STOP])
            self.g.reg_write(UC_X86_REG_ESP, STACK)
            self.g.reg_write(UC_X86_REG_EAX, index)
            self.g.reg_write(UC_X86_REG_ESI, OUTPUT)
            self.g.reg_write(UC_X86_REG_EFLAGS, 2)
            self.g.reg_write(UC_X86_REG_FPCW, 0x027f)
            self.g.reg_write(UC_X86_REG_MXCSR, 0x1f80)
            self.g.reg_write(UC_X86_REG_FPTAG, 0xffff)
            self.g.reg_write(UC_X86_REG_FPSW, 0)
            for reg in (UC_X86_REG_XMM0, UC_X86_REG_XMM1, UC_X86_REG_XMM2, UC_X86_REG_XMM3):
                self.g.reg_write(reg, 0x3e000000)
            self.observations = []
            self.g.emu_start(self.entries[owner], STOP, count=10000)
            assert self.g.reg_read(UC_X86_REG_EIP) == STOP and self.g.reg_read(UC_X86_REG_ESP) == STACK + 4
            assert all(q['fpcw'] == 0x027f and (q['mxcsr'] & ~63) == 0x1f80 for q in self.observations)
            outputs[owner] = self.g.reg_read(UC_X86_REG_XMM0) & 0xffffffff if owner == 'scalar' else list(struct.unpack('<3I', self.g.mem_read(OUTPUT, 12)))
            observations[owner] = self.observations.copy()
        assert bytes(self.g.mem_read(DEFAULT, len(default))) == default
        assert bytes(self.g.mem_read(OUTPUT - 4, 4)) == b'\xa5' * 4 and bytes(self.g.mem_read(OUTPUT + 12, 4)) == b'\xa5' * 4
        for key, values in (('datum', [row['datum']]), ('near', [row['near_bits']]), ('far', [row['far_bits']]), ('forward', row['forward_bits']), ('default_pointer', [DEFAULT])):
            assert bytes(self.g.mem_read(self.fields[key], 4 * len(values))) == pack(values)
        assert all((word & 0x7f800000) != 0x7f800000 for word in [outputs['scalar'], *outputs['color']])
        return dict(scalar39_bits=outputs['scalar'], color41_bits=outputs['color'],
                    default_before_hex=default.hex(), default_after_hex=bytes(self.g.mem_read(DEFAULT, len(default))).hex(),
                    scalar_observations=observations['scalar'], color_observations=observations['color'])


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, default=OUT)
    ap.add_argument('--fixture', type=Path, default=FIXTURE)
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    retail = Xbe(ROOT / SPECS['retail']['path'])
    cases, versions, listings = inputs(), {}, []
    for name in SPECS:
        source = Source(name, retail)
        results = [source.run(row) for row in cases]
        versions[name] = dict(image_sha256=source.spec['pin'], owners=source.entries, provider_branches=source.branches,
                              fields=source.fields, results=results)
        for owner, branch in source.branches.items():
            f, ins = literal(source.code, branch, set())
            path = args.output / f'{name}-{owner}-{branch:08x}.asm'
            path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins))
            listings.append(dict(image=name, owner=owner, branch=branch, listing=path.name, listing_sha256=sha(path.read_bytes())))
        path = args.output / f'{name}-executed.json'
        path.write_text(json.dumps([dict(address=pc, bytes_hex=source.image.read(pc, size).hex()) for pc, size in sorted(source.executed.items())], sort_keys=True, indent=2) + '\n')
        listings.append(dict(image=name, owner='executed', listing=path.name, listing_sha256=sha(path.read_bytes())))
    normalized = lambda version: [(q['scalar39_bits'], q['color41_bits']) for q in version['results']]
    assert normalized(versions['retail']) == normalized(versions['tu11']) == normalized(versions['u5'])
    fixture = dict(schema='original-fog-externs-v1', boundary=__doc__, inputs=cases, versions=versions,
                   instruction_patches=[], supplied_returns=[])
    args.fixture.write_text(json.dumps(fixture, sort_keys=True, separators=(',', ':')) + '\n')
    report = dict(schema='original-fog-externs-report-v1', boundary=__doc__, fixture_sha256=sha(args.fixture.read_bytes()),
                  cases=len(cases), complete_dispatch_calls=len(cases) * 6, versions={n: {k:v for k,v in version.items() if k!='results'} for n,version in versions.items()},
                  owners=listings, fpcw=0x027f, mxcsr=0x1f80,
                  dependencies={str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in (Path(__file__).resolve(), ROOT / 'tools/inspect_render_primitives_title_updates.py', ROOT / 'tools/oracle_game_over.py', ROOT / 'tools/oracle_mp_full_scoreboard_1_5.py')},
                  instruction_patches=[], supplied_returns=[],
                  limits=['Finite declared near/far/forward/default RGB, including signed zero and gradual subnormal products; all actual outputs finite.',
                          'Actual MXCSR control is nearest with all exceptions masked and DAZ/FTZ clear. Other rounding/flush modes, nonfinite inputs and architectural exception-status delivery are not admitted.',
                          'Default pointer/RGB storage is explicit caller input. The initialization/reset/provider of live default RGB remains separate.',
                          'Whole dispatcher calls execute only actual39/41 branches. Other extern indices, fog loading, camera producer, visibility/layer admission, pass preparation, GPU and framebuffer remain separate.'])
    path = args.output / 'report.json'
    path.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(cases=len(cases), calls=report['complete_dispatch_calls'], fixture_sha256=report['fixture_sha256'], report_sha256=sha(path.read_bytes())), indent=2))


if __name__ == '__main__':
    main()
