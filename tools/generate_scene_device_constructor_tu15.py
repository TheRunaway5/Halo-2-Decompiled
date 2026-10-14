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
    p.add_argument('--output', type=Path, default=Path('src/source_scene_device_constructor_tu15_generated.cpp'))
    p.add_argument('--directory', type=Path, required=True)
    a = p.parse_args()
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    assert code.at(0x10db94).bytes.hex() == 'ffd1'
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
        0x10c790: 96, 0x10c180: 36, 0x10d790: 73, 0x10d870: 73, 0x1d3770: 34,
        0x16e6a0: 78, 0x10cae0: 40, 0x117700: 108, 0x11e0b0: 21,
        0x11e4f0: 26, 0x30dc10: 191, 0x30d3f0: 137, 0x312620: 190}
    calls = {0x10c0b0: {0x1cf210}, 0x10c790: {0x16e6a0, 0x10c180, 0x10d790, 0x10d870},
        0x10d790: {0x1d3770, 0x1e3f30, 0x1ce720, 0xb9390},
        0x10d870: {0x1d3770, 0x1e3f30, 0x1ce720, 0xb9390},
        0x1d3770: {0x1e5360, 0x1d36c0}, 0x10cae0: {0x16e6a0},
        0x117700: {0xbaba0, 0x202ab0, 0x2184a0}, 0x30dc10: {0x30d3f0, 0x312620}}
    update_spec = json.loads(Path(__file__).with_name('scene_device_tu15_entries.json').read_text())
    assert update_spec['xbe_sha256'] == NEW_PIN
    for name, row in update_spec['entries'].items():
        entry = int(name, 16)
        entries[entry] = row['count']
        calls[entry] = set(row['calls'])
    instructions, addresses, sites = {}, {}, {}
    for entry, count in entries.items():
        body = complete_body(code, entry)
        assert len(body.code) == count and not body.tables
        sites[f'{entry:08x}'] = sorted(body.code)
        for pc in sorted(body.code):
            i = code.at(pc)
            assert image.read(pc, i.size) == bytes(i.bytes)
            if i.mnemonic == 'call':
                if i.operands[0].type == 2:
                    assert i.operands[0].imm in calls.get(entry, set()), (hex(entry), hex(pc), i.op_str)
                else:
                    assert pc in update_spec['entries'][f'{entry:08x}']['indirect_call_sites']
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
        routines=[dict(entry=p, tables=[], byte_tables=[]) for p in entries])
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
    a.output.write_text(source)
    report = json.loads(manifest.read_text())
    report.update(generated_cpp_sha256=sha(a.output), wrapper_sha256=sha(Path(__file__)),
        update_spec_sha256=sha(Path(__file__).with_name('scene_device_tu15_entries.json')),
        reached_site='0010db94', reached_instruction_hex='ffd1')
    manifest.write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')


if __name__ == '__main__':
    main()
