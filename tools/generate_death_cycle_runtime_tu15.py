#!/usr/bin/env python3
"""Lower complete authenticated TU1.5 death, detachment and observer owners."""
import argparse
import json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body, generate, sha

# Actual178F9F's effect call starts this recovery. Subsequent entries are the
# complete missing owners reached by fresh fatal cleanup and observer probes.
ENTRIES = (
    0xd9ab2,
    0xd9acc,
    0xbd020,
    0xd9a00,
    0xb98b0,
    0x186780,
    0x1cd7f0,
    0x2e9050,
    0x186ad0,
    0x1d9410,
    0x1d9360,
    0xe24f0,
    0x31e690,
    0xe5e60,
    0xccbe0,
    0x1d1f30,
    0xdbd64,
    0xe0b70,
    0x30bd0,
    0x28ada0,
    0x2e8d70,
    0x17b0d0,
    0x11e7b0,
    0xbc820,
    0xbc720,
    0x1704c0,
    0xccf40,
    0xe6f70,
    0x1f4850,
    0x2e8d90,
    0xd3340,
    0xd11d0,
    0xd0e10,
    0xd1020,
    0xd0b90,
    0xbb8c0,
    0xc0680,
    0xbb910,
    0xb95d0,
    0x10e340,
    0xfa8c0,
    0x148e20,
    0xfbe10,
    0xbe200,
    0x16b940,
    0x1e68e0,
    0x1e6980,
    0x24dc90,
    0x2e9750,
    0x2e96a0,
    0x30b890,
    0x30aa70,
    0x2e96b0,
    0x30aa50,
    0x1f3600,
    0xeb360,
    0xeb190,
    0xea9b0,
    0x105200,
    0x108f30,
    0x16b31b,
    0xf0d50,
    0x16b736,
    0xeab70,
    0x116920,
    0x16e360,
    0xba970,
    0xba9e0,
    0x171750,
    0x1715d0,
    0x1e6cb0,
    0x2a0230,
    0x2f7190,
    0x2f6ca0,
    0x1eea50,
    0x1f5230,
    0x1f4970,
    0x2e8fc0,
    0x1cddb0,
    0x315a00,
    0x304bd0,
    0x2f4de0,
    0x186ca0,
    0x186c90,
    0x176a10,
    0x2f3210,
    0x186c00,
    0x3222f0,
    0x315c60,
    0x315be0,
    0x31faa0,
    0x2f6c80,
    0x3235c0,
    0x1cb060,
    0x2e87d0,
    0x2e6000,
    0x2e5cb0,
    0x1f51d0,
    0x112a80,
    0x1f5780,
    0x10eab0,
    0x1030f0,
    0x106760,
    0xd39d0,
    0x107080,
    0x10af40,
    0x10a8d0,
    0x102d50,
    0x12fcb0,
    0x4057f0,
    0xcc1e0,
    0x108040,
    0x1074a0,
    0x1d9f10,
    0xc1600,
    0x1d6490,
    0x1d9b20,
    0x1d9bc0,
    0xe7790,
    0x1fa0b0,
    0x1ce4d0,
    0xe7e30,
    0x20650,
    0x1e27f0,
    0x1e2c90,
    0x17d210,
    0x17dd50,
    0x17d730,
    0xc2380,
    0xc3a20,
    0xc3960,
    0xc4c80,
    0x17dc60,
    0x17beb0,
    0x17bd40,
    0x17e100,
    0x17aeb0,
    0x176e70,
    0x13a6b0,
    0x22b790,
    0x17b980,
    0x17b9c0,
    0x177f10,
    0x17bb10,
    0x178020,
    0x178070,
    0x22ad50,
    0x22ad40,
    0x178920,
    0x17ba50,
    0x17ddc0,
    0x178110,
    0x251930,
    0x2520a0,
    0x176a40,
    0x252100,
    0x251c80,
    0x250780,
    0x250400,
    0x176a30,
    0x2503f0,
    0x145b70,
    0x1c180,
    0x176a20,
    0x250410,
    0x24ffe0,
    0x17ce40,
    0x1ce7b0,
    0x17dce0,
    0x17aba0,
    0x17adf0,
    0x250440,
    0x250430,
    0x250c60,
    0x17aae0,
    0x252480,
    0x250170,
    0x250310,
    0x250380,
    0x2502b0,
    0x2501c0,
    0x250220,
    0x1798e0,
    0x178e30,
    0x179230,
    0x1790a0,
    0x17afb0,
    0x178de0,
    0x17e5e0,
    0x17b1f0,
    0x17b880,
    0x17e460,
    0x17a1a0,
    0x17c6e0,
    0x17aef0,
    0x17b960,
    0x17b130,
    0x17a230,
    0x17b580,
    0x17b450,
    0x17c370,
    0x17c5c0,
    0x17c6a0,
    0x17c140,
    0x17a380,
    0x17aab0,
    0x17c330,
    0x17a0f0,
    0x17a4a0,
    0x17c020,
    0x17a420,
    0x17a030,
    # Actual170663 initializes the complete96-byte effect creation request
    # before the original projectile-state event calls17A030.
    0x178d80,
    # ActualDB862 prepares damage-owner identity from the current source
    # object table; keep the complete15-instruction D2AD0 body.
    0xd2ad0,
    # ActualDB397 dispatches this complete attached-effect request owner;
    # BAD60/1791C0/178E30 retain their existing external ownership.
    0x179700,
    # ActualDBC7A accumulates matching authored damage-material bits.
    0xdcfa0,
    # ActualDE17E finds the first authored signed-byte damage-state row.
    0x1706f0,
    # ActualDBA2E during live constructor delivery selects/marks the original
    # indexed object state record through1054B0, including its low-AL return.
    0x1054b0,
    # ActualD72D6 reaches this original light-receiver instance callback.
    # Mode1 returns without reading its pool; mode0 keeps47860 external.
    0x118760,
    # Actual311522 cleanup callback reached after the live grenade replay.
    # Complete source body clears its selected physical record flag.
    0x282780,
    0x17a2f0,
    0x177fa0,
    0x177020,
    0x178290,
    0x17ba10,
    0x2519f0,
    0x2516c0,
    0x160780,
    0x15b470,
    0x160570,
    0x15dd10,
    0x151bf0,
    0x109030,
    0x108ea0,
    0x13ba80,
    0x1a8e50,
    0x111e00,
    0x1e30d0,
    0x1e3100,
    0xe3c00,
    0x1e01a0,
    0x31d420,
    0x325a60,
    0x3233a0,
    0x1d65a0,
    0x31d660,
    0x325ce0,
    0x31d530,
    0x31d710,
    0x1d8400,
    0x31c9d0,
    0x1d69b0,
    0x31ce10,
    0x31caf0,
    0xe40b0,
    0xe4190,
    0x16bbc0,
    0x16b810,
    0xe4930,
    0x31e430,
    0x325d70,
    0x31e560,
    0x31e590,
    0x31d630,
    0x31cbf0,
    0xe3350,
    0x1f48d0,
    0x1db880,
    0x31cc40,
    0x1f5710,
    0xb87d0,
    0x31d930,
    0x3288b0,
    0x37c6b0,
    0x329100,
    0x3294e0,
    0x328a00,
    0x31d0d0,
    0x31cef0,
    0x1f4920,
    0xe3fd0,
    0x1d94e0,
    0x1e10f0,
    0x1442b0,
    0x145c20,
    0xaa200,
    0xaa270,
    0x120570,
    0x1201e0,
    0x30caa0,
    0x28a7d0,
    0x1d6590,
    0xe0900,
    0x30d1a0,
    0x1d8230,
    0x31d5b0,
    0x325a80,
    0x325aa0,
    0x31cea0,
    0x31ec50,
    0x1d3e00,
    0x1e4a40,
    0x1ce670,
    0x1d3f60,
    0x1d4f00,
    0x106310,
    0x111ed0,
    0x1cf190,
    0x31cb70,
)

def instruction_extensions(**helpers):
    def emit(instruction):
        if instruction.address in (0xdbfcc,0xd9ca4):
            # Explicit native composition boundary: the physical-delivery
            # block of DBB90 ends before attribution/CB410. No original RET
            # or successful downstream service is supplied here.
            return 'return;'
        if instruction.mnemonic == 'fimul':
            operand, = instruction.operands
            assert operand.type == 3 and operand.size == 4
            value = helpers['get'](instruction, operand)
            # The same signed-integer x87 owner used by the native vehicle
            # math module. Keep its extended intermediate and FP controls.
            return f's.fp.st(0)=mul(s.fp.st(0),static_cast<long double>(si({value})));'
        return None
    return emit

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=Path('src/gameplay/units/death_cycle_runtime_tu15_generated.cpp'))
    parser.add_argument('--directory', type=Path, default=Path('analysis/multiplayer/death-cycle-20261013/effect-owners'))
    args = parser.parse_args()
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    banks = [_Code(image, section) for section in image.sections if section.name in ('.text', 'D3D')]
    class Code:
        def owner(self, pc):
            return next(bank for bank in banks if bank.inside(pc))
        def at(self, pc):
            return self.owner(pc).at(pc)
    code = Code()
    assert code.at(0x178f9f).bytes.hex() == 'e82c210000'
    instructions, addresses, sites, routines = {}, {}, {}, []
    for entry in ENTRIES:
        body = complete_body(code.owner(entry), entry)
        if entry == 0xdbd64:
            # This entry is an authenticated block, not a complete function.
            # Every physical branch converges at DBFCC; the caller supplies
            # the actual DBB90 frame and consumes the retained state there.
            body.code = {pc for pc in body.code if pc <= 0xdbfcc}
        if entry in (0xd9ab2,0xd9acc):
            # D9A30 radius geometry, after admission and before cone/falloff.
            body.code = {pc for pc in body.code if pc <= 0xd9ca4}
        assert body.code, f'No authenticated code recovered at {entry:08x}'
        assert all(width in (1,4) for _,width,_ in body.tables)
        routines.append(dict(entry=entry,
            tables=[dict(disc=p,count=n) for p,width,n in body.tables if width==4],
            byte_tables=[dict(address=p,width=width,count=n,raw_hex=image.read(p,width*n).hex())
                for p,width,n in body.tables if width==1]))
        sites[f'{entry:08x}'] = sorted(body.code)
        for pc in sorted(body.code):
            i = code.at(pc)
            assert image.read(pc, i.size) == bytes(i.bytes)
            instructions[f'{pc:08x}'] = pc
            addresses[pc] = pc
            addresses[pc + i.size] = pc + i.size
            for operand in i.operands:
                if operand.type == 2:
                    addresses[operand.imm & 0xffffffff] = operand.imm & 0xffffffff
                elif operand.type == 3:
                    addresses[operand.mem.disp & 0xffffffff] = operand.mem.disp & 0xffffffff
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
        entries={f'{p:08x}': p for p in ENTRIES}, body_sites=sites,
        instruction_map=instructions,
        address_map={f'{p:08x}': q for p, q in addresses.items()}, routines=routines)
    args.directory.mkdir(parents=True, exist_ok=True)
    correspondence = args.directory / 'bodies.json'
    correspondence.write_text(json.dumps(paired, indent=2, sort_keys=True) + '\n')
    generate(SimpleNamespace(output=args.output, manifest=args.directory / 'generation.json',
        correspondence=correspondence, additional_entries=Path(__file__)), paired, code_override=code,
        extra_instruction_factory=instruction_extensions)
    source = args.output.read_text().replace('tools/generate_physics_live_support_tu15.py',
        'tools/generate_death_cycle_runtime_tu15.py').replace('halo2/physics_live_support_tu15.hpp',
        'halo2/death_cycle_runtime_tu15.hpp').replace('source_run_physics_live_support_tu15',
        'source_run_death_cycle_runtime_tu15').replace('source_physics_live_support_tu15_owns_entry',
        'source_death_cycle_runtime_tu15_owns_entry')
    # Keep a frozen component composition independent of optional application
    # observers when none of its authenticated instruction sites use them.
    if 'SourceProjectileUpdateScope' not in source and 'source_begin_projectile_contact_effect_tu15' not in source:
        source = source.replace('#include "halo2/source_projectile_effect_observer.hpp"\n', '')
    args.output.write_text(source)
    report = json.loads((args.directory / 'generation.json').read_text())
    report['bounded_blocks'] = {'000dbd64': {'stop_before': '000dbfcc',
        'scope': 'DBB90 physical delivery only; native caller frame'},
        '000d9ab2': {'stop_before':'000d9ca4',
        'scope':'D9A30 current source closest-point and physical radius direction; native caller frame'},
        '000d9acc': {'stop_before':'000d9ca4',
        'scope':'D9A30 radius direction with caller closest-point inputs; diagnostic entry'}}
    report['complete_native_entries'] = [entry for entry in report['complete_native_entries']
        if entry not in ('000dbd64','000d9ab2','000d9acc')]
    report['generated_cpp_sha256'] = sha(args.output)
    (args.directory / 'generation.json').write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')

if __name__ == '__main__':
    main()
