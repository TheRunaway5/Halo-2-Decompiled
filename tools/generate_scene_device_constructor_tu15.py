#!/usr/bin/env python3
"""Lower complete owned TU1.5 scene-device callbacks and reached vehicle-world support."""
import argparse
import json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body, generate, sha


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, default=Path('src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp'))
    p.add_argument('--directory', type=Path, required=True)
    a = p.parse_args()
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    assert code.at(0x10db94).bytes.hex() == 'ffd1'
    # Actual live source pickup/control activation dispatch.
    assert code.at(0x10c911).bytes.hex() == 'e8ea170100'
    # Complete device field/group APIs.10C1F0 starts after the actual
    # named-group getter's return/padding;10C210 is an interior offset.
    assert image.read(0x10c1e6,10).hex()=='c3cccccccccccccccccc'
    assert code.at(0x10c220).bytes.hex()=='e86b000000'
    assert code.at(0x10c449).bytes.hex()=='e892000000'
    assert code.at(0x117a8c).bytes.hex()=='e8bf2c0c00'

    # DEVI's inherited birth callback, used by the authored MACH descriptor.
    assert image.read(0x4747ac, 4) == b'ived'
    assert image.read(0x4747d4, 4) == bytes.fromhex('c0b51000')
    assert image.read(0x4748f8, 4) == bytes.fromhex('a8474700')
    assert image.read(0x47489c, 4) == bytes.fromhex('70781100')
    assert code.at(0x10e155).bytes.hex() == 'ffd0'
    assert code.at(0x10de23).bytes.hex() == 'ffd1'
    # Complete collision-type getter reached by this same C1120 construction
    # graph. Its literal MOV EAX,19 / RET body has no omitted owner or work.
    assert image.read(0xa9be0, 6) == bytes.fromhex('b813000000c3')
    assert code.at(0x10dc0b).bytes.hex() == 'ffd0'
    # DEVI's real post-birth callback and its complete group/channel owners.
    # The pool is constructed by B87F0 -> 10D990's descriptor initialization;
    # allocating a second pool here would discard the authored map groups.
    entries = {0x10b5c0: 56, 0x117870: 34, 0x10c0b0: 63, 0x118120: 69, 0xa9be0: 2,
        0xde9c0: 5, 0xc6310: 80, 0x112010: 36, 0x1d4df0: 18, 0xf0e70: 48, 0x102a00: 65, 0x106a50: 41, 0xfa5f0: 36,
        0x10c790: 96, 0x10c180: 36, 0x10d790: 73, 0x10d870: 73, 0x1d3770: 34,
        0x16e6a0: 78, 0x10cae0: 40, 0x117700: 108, 0x11e0b0: 21,
        0x11e4f0: 26, 0x30dc10: 191, 0x30d3f0: 137, 0x312620: 190,
        # MACH post-birth reaches the original BSP machine bank and publishes
        # its authored collision references (Cairo tram is a live consumer).
        0x202ab0: 7, 0x2184a0: 66,
        0x2fbb40: 11, 0x2fbba0: 241, 0x2e46e0: 37, 0x2fbb10: 16,
        0x10f2f0: 1,
        0x6a8e0: 57, 0x88470: 39,
        0x1cd100: 11, 0x2335f0: 36, 0x233650: 37, 0x233520: 34, 0x232d90: 27, 0x1dbb30: 54,
        0x1ccad0: 92,
        0x1cb6b0: 139,
        0x1cc040: 54,
        0x316420: 25,
        0x282850: 11,
        0x32b479: 25,
        0x2dee89: 157, 0x2dd524: 230, 0x2de040: 225, 0x2df06a: 5,
        0x31b170: 15, 0x31ab10: 530,
        0x3225f0: 44,
        0x2e4a70: 40, 0x3164b0: 25, 0x316460: 25,
        0x2e4e10: 15, 0x2e4ae0: 34,
        0x2827f0: 27,
        0x2ef960: 15, 0x2eeff0: 73,
        0x32ce01: 20, 0x32ce49: 8, 0x2e15c0: 16,
        0x2f0160: 15, 0x2f0080: 67,
        0x322b70: 15,
        0x3167a0: 15, 0x3167d0: 3,
        0x37c950: 2,
        0x316680: 14, 0x3166b0: 2,
        0x1cbfb0: 15, 0x1cbff0: 27,
        0x2826c0: 21,
        0x1cb420: 47,
        0x1f3660: 15,
        0x24dc20: 24,
        0x1ce5a0: 15, 0x1ce540: 33,
        0x1f67f0: 15, 0x1f6820: 25,
        0x1f72a0: 25,
        0x1d6f30: 50,
        0x2e1f80: 15, 0x2e1e50: 86,
        0x2e1320: 15, 0x2e1350: 55, 0x2e32c0: 13, 0x2e3320: 14, 0x2e3530: 13,
        0x2e29e0: 15, 0x2e2920: 29,
        0x2e6200: 15, 0x2e6230: 28,
        0x37c750: 29, 0x37c8a0: 59, 0x2e2ed0: 12,
        0x2e2560: 15, 0x2e2590: 44,
        0x2e16f0: 15, 0x2e1720: 21, 0x2e2ef0: 36, 0x233120: 11,
        0x2e17a0: 15,
        0x1495c0: 32, 0x1498e0: 24,
        0x1e7d90: 46, 0x140c80: 36, 0x140cf0: 34}
    calls = {0x10c0b0: {0x1cf210}, 0x10c790: {0x16e6a0, 0x10c180, 0x10d790, 0x10d870},
        0xde9c0: {0xc6310},
        0xc6310: {0xcbd80, 0x112010, 0xd8d50, 0xd8fc0, 0xcf4d0, 0xba570, 0xccf40, 0xbf050},
        0x112010: {0x1d4df0, 0x1ce810},
        0x1d4df0: {0x1e3150},
        0x2fbba0: {0x2e3250, 0x2e46e0},
        0x6a8e0: {0x158240, 0x88470}, 0x88470: {0x14fb60},
        0x1cd100: {0x32ba80},
        0x1ccad0: {0xb1870},
        0x1cb6b0: {0x282010,0x1496f0,0xbcee0,0x1d7ca0,0x16e8a0,0x282c10,0x32b3d6,0x31b1a0,0x32b3c5},
        0x1cc040: {0x316420,0x283450},
        0x282850: {0x32b479},
        0x32b479: {0x32a6e4,0x330c90,0x32ede0,0x2db419,0x2db402,0x32a71f},
        0x2dee89: {0x32a6e4,0x2dd524,0x2de040,0x2dd7c9,0x2df06a,0x32a71f},
        0x2de040: {0x2dd7c9,0x2dd193,0x2dd2b3,0x2dd2d4},
        0x31b170: {0x31ab10},
        0x31ab10: {0x319750,0x318680,0x3198a0,0x283450,0x3174a0,0x3225f0,0x37c950,0x2e12a0},
        0x2e4a70: {0x3164b0,0x316460,0x283450},
        0x2e4e10: {0x2e4ae0},0x2e4ae0: {0x3164b0,0x316460},
        0x2827f0: {0x3164b0,0x283450},
        0x2ef960: {0x2eeff0},0x2eeff0: {0x32ce01},
        0x32ce01: {0x32a6e4,0x32ce49,0x32a71f},0x32ce49: {0x32cda3},
        0x2f0160: {0x2f0080},
        0x322b70: {0x2f7a10},
        0x3167a0: {0x3167d0},
        0x316680: {0x3166b0},
        0x1cbfb0: {0x1cbff0},0x1cbff0: {0x2e12a0},
        0x1cb420: {0x283450},
        0x1f3660: {0x2e96b0},
        0x24dc20: {0x2e12a0},
        0x1ce5a0: {0x1ce540},0x1ce540: {0x283450,0x2e12a0},
        0x1f67f0: {0x1f6820},0x1f6820: {0x283450},
        0x1f72a0: {0x2e12a0},
        0x1d6f30: {0x1d7ca0},
        0x2e1f80: {0x2e1e50,0x2e1450,0x2e14e0,0x2e1550,0x2e1060},
        0x2e1e50: {0x283450,0x2e31e0,0x2e3250},
        0x2e1320: {0x2e1350},
        0x2e1350: {0x2e32c0,0x2e32e0,0x2e3320,0x2e3350,0x2e3530},
        0x2e29e0: {0x2e2920},
        0x2e6200: {0x2e6230},
        0x37c8a0: {0x2e2ed0},
        0x2e2560: {0x2e2590},
        0x2e16f0: {0x2e1720},0x2e1720: {0x2e2ef0},0x2e2ef0: {0x283450},
        0x233120: {0x32b479},
        0x2e17a0: {0x2f7a10},
        0x1495c0: {0x130030,0x1498e0},0x1498e0: {0x130030},
        0xf0e70: {0xc6310, 0x111b10, 0x1d6200, 0xbf050},
        0x102a00: {0x106a50, 0xbbbc0},
        0xfa5f0: {0xbbbc0},
        0x10d790: {0x1d3770, 0x1e3f30, 0x1ce720, 0xb9390},
        0x10d870: {0x1d3770, 0x1e3f30, 0x1ce720, 0xb9390},
        0x1d3770: {0x1e5360, 0x1d36c0}, 0x10cae0: {0x16e6a0},
        0x117700: {0xbaba0, 0x202ab0, 0x2184a0}, 0x30dc10: {0x30d3f0, 0x312620},
        0x1e7d90: {0x16e450, 0x166a80, 0x140c80},
        0x140c80: {0x32a4e0, 0x140cf0}}
    indirect_sites = {0x1e7d90: {0x1e7da3}, 0x140c80: {0x140c99}, 0x2fbb40: {0x2fbb4c},
        0x2fbba0: {0x2fbe3c,0x2fbe82}, 0x2fbb10: {0x2fbb25,0x2fbb2e},
        0x1cc040: {0x1cc067,0x1cc09a,0x1cc0d4},
        0x2dee89: {0x2deed4,0x2deeee,0x2df027,0x2df042},
        0x2dd524: {0x2dd5c3,0x2dd632,0x2dd70f,0x2dd793},
        0x2de040: {0x2de150},0x2df06a: {0x2df076},
        0x31b170: {0x31b18f},0x3225f0: {0x32260d},0x2e4a70: {0x2e4acf},
        0x2e4e10: {0x2e4e2f},
        0x2827f0: {0x282830},
        0x2ef960: {0x2ef97f},0x2eeff0: {0x2ef053,0x2ef06f,0x2ef0ac},
        0x32ce01: {0x32ce2c},0x2e15c0: {0x2e15e0},
        0x2f0160: {0x2f017f},0x2f0080: {0x2f00d5,0x2f0109,0x2f0135},
        0x322b70: {0x322b8f},
        0x3167a0: {0x3167bf},
        0x316680: {0x31669c},
        0x1cbfb0: {0x1cbfcf},
        0x2826c0: {0x2826f1},
        0x1cb420: {0x1cb454,0x1cb491},
        0x1f3660: {0x1f367f},
        0x24dc20: {0x24dc60},
        0x1ce5a0: {0x1ce5bf},0x1ce540: {0x1ce572},
        0x1f67f0: {0x1f680f},0x1f6820: {0x1f6852},
        0x1f72a0: {0x1f72ea},
        0x2e1e50: {0x2e1f22,0x2e1f62},
        0x2e1320: {0x2e133f},0x2e1350: {0x2e13b1},0x2e3530: {0x2e354b},
        0x2e29e0: {0x2e29ff},0x2e2920: {0x2e2959},
        0x2e6200: {0x2e621f},0x2e6230: {0x2e6273},
        0x37c750: {0x37c799},0x37c8a0: {0x37c928},0x2e2ed0: {0x2e2ee5},
        0x2e2560: {0x2e257f},0x2e2590: {0x2e25d4,0x2e25fe},
        0x2e16f0: {0x2e170f},0x2e2ef0: {0x2e2f49},
        0x2e17a0: {0x2e17bf},
        0x1495c0: {0x1495df,0x149603,0x149615},0x1498e0: {0x1498f0,0x14990d},
        0x31ab10: {0x31abfe,0x31ac49,0x31ac7e,0x31accc,0x31ad15,0x31ad4c,0x31ad7c,
            0x31ad9f,0x31adaf,0x31add9,0x31adf1,0x31ae07,0x31ae17,0x31ae42,0x31ae6e,
            0x31ae9a,0x31aec6,0x31aef2,0x31af1e,0x31af4a,0x31af76,0x31afa2,0x31afce,
            0x31aff7,0x31b01d,0x31b043,0x31b069,0x31b08f,0x31b0b5,0x31b0db,0x31b101,0x31b126}}
    # Literal original AI storage order: PVS data precedes machine-reference
    # backing. Both constructors use the actual retained allocator vtable.
    assert image.read(0x1cfab4, 5) == bytes.fromhex('e8d7820100')
    update_spec = json.loads(Path(__file__).with_name('scene_device_tu15_entries.json').read_text())
    assert update_spec['xbe_sha256'] == NEW_PIN
    for name, row in update_spec['entries'].items():
        entry = int(name, 16)
        entries[entry] = row['count']
        calls[entry] = set(row['calls'])
    instructions, addresses, sites, routines = {}, {}, {}, []
    for entry, count in entries.items():
        body = complete_body(code, entry)
        assert len(body.code) == count
        expected_tables=update_spec['entries'].get(f'{entry:08x}', {}).get('tables', [])
        if entry==0x6a8e0:
            expected_tables=[dict(address=0x6a984,width=4,count=5,
                raw_hex='1da9060021a9060021a906002ea9060040a90600')]
        actual_tables=[dict(address=p,width=w,count=n,raw_hex=image.read(p,w*n).hex()) for p,w,n in body.tables]
        assert actual_tables==expected_tables and all(w in (1,4) for _,w,_ in body.tables)
        routines.append(dict(entry=entry,
            tables=[dict(disc=p,count=n) for p,w,n in body.tables if w==4],
            byte_tables=[dict(address=p,width=w,count=n,raw_hex=image.read(p,w*n).hex()) for p,w,n in body.tables if w==1]))
        sites[f'{entry:08x}'] = sorted(body.code)
        for pc in sorted(body.code):
            i = code.at(pc)
            assert image.read(pc, i.size) == bytes(i.bytes)
            if i.mnemonic == 'call':
                if i.operands[0].type == 2:
                    assert i.operands[0].imm in calls.get(entry, set()), (hex(entry), hex(pc), i.op_str)
                else:
                    allowed = indirect_sites.get(entry, set(update_spec['entries'].get(
                        f'{entry:08x}', {}).get('indirect_call_sites', [])))
                    assert pc in allowed
            instructions[f'{pc:08x}'] = pc
            addresses[pc] = pc
            addresses[pc + i.size] = pc + i.size
            for operand in i.operands:
                if operand.type == 2:
                    addresses[operand.imm & 0xffffffff] = operand.imm & 0xffffffff
                elif operand.type == 3:
                    addresses[operand.mem.disp & 0xffffffff] = operand.mem.disp & 0xffffffff
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
        entries={f'{p:08x}': p for p in entries}, body_sites=sites,
        instruction_map=instructions, address_map={f'{p:08x}': q for p, q in addresses.items()},
        routines=routines)
    a.directory.mkdir(parents=True, exist_ok=False)
    correspondence = a.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired, indent=2, sort_keys=True)+'\n')
    manifest = a.directory/'generation.json'
    generate(SimpleNamespace(output=a.output, manifest=manifest, correspondence=correspondence,
        additional_entries=Path(__file__)), paired, code_override=code)
    source = a.output.read_text().replace('tools/generate_physics_live_support_tu15.py',
        'tools/generate_scene_device_constructor_tu15.py').replace('halo2/physics_live_support_tu15.hpp',
        'halo2/source_scene_device_constructor_tu15.hpp').replace('source_run_physics_live_support_tu15',
        'source_run_scene_device_constructor_tu15').replace('source_physics_live_support_tu15_owns_entry',
        'source_scene_device_constructor_tu15_owns_entry')
    # MACH117A8C calls this owned body internally, so it does not cross
    # the live external-owner observation boundary. Read-only diagnostics
    # retain the same actual ABI/memory and detach with their native owner.
    source=source.replace('#include "halo2/source_projectile_effect_observer.hpp"',
        '#include "halo2/source_projectile_effect_observer.hpp"\n#include "halo2/physics_internal_phantom_observation.hpp"')
    phase='static void run_1da750(WorldState &s) {\n'
    assert source.count(phase)==1
    source=source.replace(phase,phase+'SourceInternalPhantomObservation phase(s.memory,[&]{return s.save();});\n',1)
    a.output.write_text(source)
    report = json.loads(manifest.read_text())
    report.update(generated_cpp_sha256=sha(a.output), wrapper_sha256=sha(Path(__file__)),
        update_spec_sha256=sha(Path(__file__).with_name('scene_device_tu15_entries.json')),
        reached_site='0010db94', reached_instruction_hex='ffd1')
    manifest.write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')


if __name__ == '__main__':
    main()
