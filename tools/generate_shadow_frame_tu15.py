#!/usr/bin/env python3
"""Lower authenticated complete TU1.5 shadow frame and owner bodies."""
import argparse
import json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha

# Located by the complete earlier/TU1.5 CFG audit. Generation reads actual
# TU1.5 bytes; external renderer/cache/observer services remain required owners.
ENTRIES = (0x226b0,0x30c40,0x31f40,0x320a0,0x32340,0x333f0,0x335b0,0x335d0,
           0x44710,0x45a70,0x3d7b0,0x3d7e0,0x3dc80,0x3e9b0,0x48ed0,0x4a1d0,0x4a7b0,0x4ac20,
           0x4bae0,0x4b150,0xbbd10,0xc3dc0,0x168900)

# Complete ordinary main40E80 callees verified against the unchanged original
# on a published main-queue bank. 41630 is a tail target (40F2C/40F43), so the
# CALL-only trace collector cannot discover its required complete body.
ENTRIES += (0x1e2b0,0x40e00,0x41630,0x42090,0x42750,0xc0700,0x179a40,0x17cd10)



def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, required=True)
    parser.add_argument('--output', type=Path,
                        default=Path('src/source_shadow_frame_tu15_generated.cpp'))
    parser.add_argument('--additional-trace',type=Path,action='append',default=[],
                        help='Authenticated original instruction trace; lower complete observed callees')
    parser.add_argument('--external-entry',type=lambda value:int(value,0),action='append',default=[],
                        help='Entry retained by an actual native platform owner, not lowered here')
    parser.add_argument('--additional-entry',type=lambda value:int(value,0),action='append',default=[],
                        help='Authenticated complete entry invoked directly by the native caller')
    args = parser.parse_args()
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    # Ordinary44E80 marks selected subparts through this complete leaf.
    # Elongation reaches the call even though the earlier bounded bank did not.
    assert 0x4526f in _trace(code,0x44e80,set()).code
    assert code.at(0x4526f).mnemonic=='call' and code.at(0x4526f).operands[0].imm==0x45a70
    entries=set(ENTRIES)|set(args.additional_entry)
    for trace_path in args.additional_trace:
        trace=json.loads(trace_path.read_text())
        for n,pc in enumerate(trace[:-1]):
            instruction=code.at(pc)
            if instruction.mnemonic=='call':
                operand=instruction.operands[0]
                if operand.type==3 and not operand.mem.base and not operand.mem.index:
                    imported=int.from_bytes(image.read(operand.mem.disp&0xffffffff,4),'little')
                    if 0x80000000<=imported<0x80001000:continue # Real external kernel boundary, absent from XBE trace.
                target=trace[n+1]
                if operand.type==2:
                    assert target==operand.imm&0xffffffff
                entries.add(target)
    entries=tuple(sorted(entries-set(args.external_entry)))
    sites, instructions, addresses, routines = {}, {}, {}, []
    for entry in entries:
        body = _trace(code, entry, set(entries))
        assert body.code and all(width in (1, 4) for _, width, _ in body.tables)
        sites[f'{entry:08x}'] = sorted(body.code)
        routines.append(dict(entry=entry,
            tables=[dict(disc=p, count=n) for p, width, n in body.tables if width == 4],
            byte_tables=[dict(address=p, width=width, count=n, raw_hex=image.read(p, width*n).hex())
                         for p, width, n in body.tables if width == 1]))
        for pc in sorted(body.code):
            instruction = code.at(pc)
            assert image.read(pc, instruction.size) == bytes(instruction.bytes)
            instructions[f'{pc:08x}'] = pc
            addresses[pc] = pc
            addresses[pc + instruction.size] = pc + instruction.size
            for operand in instruction.operands:
                if operand.type == 2:
                    addresses[operand.imm & 0xffffffff] = operand.imm & 0xffffffff
                elif operand.type == 3:
                    addresses[operand.mem.disp & 0xffffffff] = operand.mem.disp & 0xffffffff
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
                  entries={f'{p:08x}': p for p in entries}, body_sites=sites,
                  instruction_map=instructions,
                  address_map={f'{p:08x}': q for p, q in addresses.items()}, routines=routines)
    args.directory.mkdir(parents=True, exist_ok=True)
    correspondence = args.directory / 'bodies.json'
    correspondence.write_text(json.dumps(paired, indent=2, sort_keys=True) + '\n')
    generate(SimpleNamespace(output=args.output, manifest=args.directory/'generation.json',
                             correspondence=correspondence, additional_entries=Path(__file__)), paired)
    source = args.output.read_text().replace('tools/generate_physics_live_support_tu15.py',
        'tools/generate_shadow_frame_tu15.py').replace('halo2/physics_live_support_tu15.hpp',
        'halo2/source_shadow_frame_tu15.hpp').replace('source_run_physics_live_support_tu15',
        'source_run_shadow_frame_tu15').replace('source_physics_live_support_tu15_owns_entry',
        'source_shadow_frame_tu15_owns_entry')
    args.output.write_text(source)
    report = json.loads((args.directory/'generation.json').read_text())
    report.update(generated_cpp_sha256=sha(args.output), module_generator_sha256=sha(Path(__file__)))
    if args.additional_trace:report['original_callee_trace_sha256']={str(p):sha(p)for p in args.additional_trace}
    report['actual_native_external_entries']=[f'{p:08x}'for p in args.external_entry]
    report['additional_direct_entries']=[f'{p:08x}'for p in args.additional_entry]
    (args.directory/'generation.json').write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print('Complete TU1.5 shadow frame owners', len(entries), 'instructions', len(instructions))


if __name__ == '__main__':
    main()
