#!/usr/bin/env python3
"""Private complete OG Xbox TU1.5 environment producers and lifecycle bodies.

Generation only: no game process, source-memory injection, or production edits.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
from types import SimpleNamespace

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha

ENTRIES = (
    0x1310f0, 0x131ac0, 0x1329b0, 0x132cd0, 0x1330e0, 0x1336c0,
    0x1338f0, 0x131470, 0x131610, 0x133a70, 0x133b40, 0x1342b0,
    0x1343b0, 0x137ac0, 0x14de90,
    0x137870, 0x1378b0, 0x1378c0, 0x1378e0, 0x137730,
    0x381010, 0x3811b0, 0x137800, 0x137bf0, 0x137c60,
)
# Required actual native callees remain explicit. This module does not replace
# their math, CRC, tag membership, BSP cluster, or function-curve owners.
ALLOWED_EXTERNALS = {0x32a660, 0x171070, 0x11f7e0, 0x13de70,
                     0x13e670, 0x14dd70, 0x166a80}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--directory',type=Path,required=True)
    args=parser.parse_args()
    if args.output.exists() or args.directory.exists():raise ValueError('Preserve generated evidence')
    args.directory.mkdir(parents=True)
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')

    def read_word(address):
        return struct.unpack('<I', image.read(address, 4))[0]

    callers = {0x2b863: 0x1310f0, 0x2b845: 0x14de90, 0x13a9e3: 0x1378e0, 0x2b5705: 0x137bf0, 0x2b5754: 0x137c60, 0x137bfb: 0x137800}
    for pc, target in callers.items():
        ins = code.at(pc)
        assert ins.mnemonic == 'call' and ins.operands[0].imm == target, (hex(pc), str(ins))

    # Authenticated subsystem row 64 and _cinit entries. Do not infer lifecycle
    # from an existing implementation or call reset on every view/checkpoint.
    table = {0x44b7d0: 0x137870, 0x44b7d4: 0x1378b0, 0x44b7d8: 0x1378c0,
             0x46d98c: 0x381010, 0x46d9c4: 0x3811b0}
    for address, value in table.items():
        assert read_word(address) == value, (hex(address), hex(read_word(address)))

    sites, instructions, addresses, routines, external_calls, body_pins = {}, {}, {}, [], {}, {}
    for entry in sorted(ENTRIES):
        body = _trace(code, entry, set(ENTRIES))
        assert body.code and not body.tables, hex(entry)
        sites[f'{entry:08x}'] = sorted(body.code)
        routines.append(dict(entry=entry, tables=[], byte_tables=[]))
        literal = []
        for pc in sorted(body.code):
            ins = code.at(pc)
            assert image.read(pc, ins.size) == bytes(ins.bytes)
            literal.append(bytes(ins.bytes))
            instructions[f'{pc:08x}'] = pc
            addresses[pc] = pc
            addresses[pc+ins.size] = pc+ins.size
            for operand in ins.operands:
                if operand.type == 2:
                    addresses[operand.imm & 0xffffffff] = operand.imm & 0xffffffff
                elif operand.type == 3:
                    addresses[operand.mem.disp & 0xffffffff] = operand.mem.disp & 0xffffffff
            if ins.mnemonic == 'call':
                assert ins.operands[0].type == 2, (hex(pc), ins.op_str)
                target = ins.operands[0].imm & 0xffffffff
                if target not in ENTRIES:
                    assert target in ALLOWED_EXTERNALS, (hex(pc), hex(target))
                    external_calls[f'{pc:08x}'] = f'{target:08x}'
        body_pins[f'{entry:08x}'] = dict(instructions=len(body.code),
            reachable_instruction_bytes_sha256=hashlib.sha256(b''.join(literal)).hexdigest())

    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
        entries={f'{entry:08x}': entry for entry in sorted(ENTRIES)}, body_sites=sites,
        instruction_map=instructions,
        address_map={f'{address:08x}': value for address, value in addresses.items()},
        routines=routines)
    correspondence = args.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired, indent=2, sort_keys=True)+'\n')
    output = args.output
    manifest = args.directory/'generation.json'
    generate(SimpleNamespace(output=output, manifest=manifest, correspondence=correspondence,
                             additional_entries=Path(__file__)), paired)
    source = output.read_text().replace('tools/generate_physics_live_support_tu15.py',
        'tools/generate_campaign_environment_tu15.py')
    source = source.replace('halo2/physics_live_support_tu15.hpp', 'halo2/source_campaign_environment_tu15.hpp')
    source = source.replace('source_run_physics_live_support_tu15', 'source_run_campaign_environment_tu15')
    source = source.replace('source_physics_live_support_tu15_owns_entry', 'source_campaign_environment_tu15_owns_entry')
    output.write_text(source)
    report = json.loads(manifest.read_text())
    report.update(generated_cpp_sha256=sha(output), module_generator_sha256=sha(Path(__file__)),
        body_pins=body_pins, exact_source_callers={f'{pc:08x}': f'{target:08x}' for pc,target in callers.items()},
        exact_lifecycle_pointer_cells={f'{address:08x}': f'{value:08x}' for address,value in table.items()},
        verified_external_calls=external_calls,
        scope='Complete authenticated environment producer/helper/sky/mixer lifecycle bodies. '
              'No production composition, native cold-memory admission, replay, frame consumer or visual claim.')
    manifest.write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')
    print(json.dumps(dict(entries=len(ENTRIES), instructions=len(instructions),
        generated_bytes=output.stat().st_size, generated_sha256=sha(output))))

if __name__=='__main__':main()
