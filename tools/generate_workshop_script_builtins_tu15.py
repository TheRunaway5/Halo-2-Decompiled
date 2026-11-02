"""Lower complete authenticated TU1.5 Workshop HS evaluator bodies.

This emits fixed native operations, not an instruction interpreter. Argument
parsing/completion and called engine features remain real dispatch boundaries.
Generate into HALO2_TEST_SCRATCH before explicitly publishing reviewed sources.
"""
import argparse
import hashlib
import json
import os
import re
import struct
from pathlib import Path
from types import SimpleNamespace

from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body, generate


def generated_bodies(source):
    """Extract complete emitted routines, excluding declarations/dispatch."""
    starts=list(re.finditer(r'^static void run_([0-9a-f]+)\(WorldState &s\) \{\n',source,re.M))
    result={}
    for start in starts:
        terminator='throw std::runtime_error("Original solver fell outside authenticated CFG");\n}\n'
        end=source.index(terminator,start.end())+len(terminator)
        entry=int(start[1],16)
        assert entry not in result
        result[entry]=source[start.start():end]
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--audit-report', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--directory', type=Path, required=True)
    parser.add_argument('--module', choices=['workshop_script_builtins','workshop_titles','workshop_unit_controls'],
                        default='workshop_script_builtins')
    parser.add_argument('--owned-callees', nargs='*',type=lambda value:int(value,0),default=[])
    parser.add_argument('--preserve-module',type=Path,
                        help='Retain every existing complete generated body exactly while adding authenticated evaluators')
    parser.add_argument('--lifecycle-call-sites', nargs='*',type=lambda value:int(value,0),default=[],
                        help='Literal TU1.5 call sites for complete companion lifecycle bodies; not script admission')
    parser.add_argument('--names', nargs='+', default=[
        'rasterizer_bloom_override_blur_amount', 'rasterizer_bloom_override_box_factor',
        'rasterizer_bloom_override_silver_bullet'])
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert args.output.resolve().is_relative_to(scratch) and args.directory.resolve().is_relative_to(scratch)
    assert not args.output.exists() and not args.directory.exists()
    audit = json.loads(args.audit_report.read_text())
    assert audit['xbox_tu15_sha256'] == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    code = _Code(image, '.text')
    candidates = [row for row in audit['declaration_candidates'] if row['name'] in args.names]
    assert {row['name'] for row in candidates} == set(args.names)
    entries = set();preserved={};preserved_hash=None
    if args.preserve_module:
        previous=args.preserve_module.read_text()
        assert f'void source_run_{args.module}_tu15('in previous
        preserved=generated_bodies(previous);assert preserved
        preserved_hash=hashlib.sha256(args.preserve_module.read_bytes()).hexdigest()
        entries.update(preserved)
    for row in candidates:
        assert len(row['matches']) == 1
        selected = row['matches'][0]
        assert selected['xbox_arguments'] == row['arguments'] and selected['xbox_return_type'] == row['return_type']
        descriptor=int.from_bytes(image.read(0x480c00+selected['xbox_opcode']*4,4),'little')
        result,flags,evaluator,_,arity=struct.unpack('<HHIIH',image.read(descriptor,14))
        assert descriptor==int(selected['xbox_descriptor'],16) and result==selected['xbox_return_type']
        assert flags==selected['xbox_flags'] and evaluator==int(selected['xbox_evaluator'],16)
        assert arity==len(row['arguments']) and list(struct.unpack('<'+'H'*arity,
            image.read(descriptor+14,arity*2)))==row['arguments']
        entries.add(int(selected['xbox_evaluator'],16))
    lifecycle_calls=[]
    for site in args.lifecycle_call_sites:
        instruction=code.at(site)
        assert instruction.mnemonic=='call' and instruction.operands[0].type==2
        assert bytes(instruction.bytes)==image.read(site,instruction.size)
        target=instruction.operands[0].imm
        assert target not in (0x212010,0x211da0), 'HS parsing/completion remain engine owners'
        entries.add(target)
        lifecycle_calls.append(dict(site=site,target=target,bytes=bytes(instruction.bytes).hex()))
    # Admit a helper only through an authenticated call or tail edge from a
    # selected evaluator or another admitted helper. In particular, the AI
    # reference iterator tail-calls the original depth-first group initializer.
    pending=set(args.owned_callees)
    assert not pending & {0x212010,0x211da0}, 'HS parsing/completion remain engine owners'
    while pending:
        direct=set()
        for entry in entries:
            body=complete_body(code,entry)
            for pc in body.code:
                instruction=code.at(pc)
                if instruction.mnemonic in ('call','jmp') and instruction.operands[0].type==2:
                    target=instruction.operands[0].imm
                    if target not in body.code:direct.add(target)
        reached=pending & direct
        assert reached, 'callee is not reachable from the selected feature effects'
        entries.update(reached);pending-=reached
    instructions, addresses, bodies, routines, calls = {}, {}, {}, [], []
    for entry in sorted(entries):
        body = complete_body(code, entry)
        assert not body.tables and not getattr(body,'byte_tables',[]), 'add complete literal switch ownership before publication'
        bodies[f'{entry:08x}'] = sorted(body.code)
        routines.append(dict(entry=entry,tables=[],byte_tables=[]))
        for pc in sorted(body.code):
            instruction = code.at(pc)
            assert bytes(instruction.bytes) == image.read(pc,instruction.size)
            instructions[f'{pc:08x}'] = pc
            addresses[pc] = pc
            addresses[pc+instruction.size] = pc+instruction.size
            for operand in instruction.operands:
                if operand.type == 2:
                    addresses[operand.imm & 0xffffffff] = operand.imm & 0xffffffff
                elif operand.type == 3:
                    addresses[operand.mem.disp & 0xffffffff] = operand.mem.disp & 0xffffffff
            if instruction.mnemonic == 'call' and instruction.operands[0].type == 2:
                calls.append(dict(site=pc,target=instruction.operands[0].imm))
    paired = dict(image='English TU1.5',original_sha256=NEW_PIN,
        entries={f'{entry:08x}':entry for entry in sorted(entries)}, body_sites=bodies,
        instruction_map=instructions,address_map={f'{address:08x}':value for address,value in addresses.items()},routines=routines)
    args.directory.mkdir(parents=True)
    correspondence = args.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired,sort_keys=True,indent=2)+'\n')
    manifest = args.directory/'generation.json'
    generate(SimpleNamespace(output=args.output,manifest=manifest,correspondence=correspondence,
        additional_entries=Path(__file__)),paired,code_override=code)
    source = args.output.read_text().replace('tools/generate_physics_live_support_tu15.py',
        'tools/generate_workshop_script_builtins_tu15.py').replace('halo2/physics_live_support_tu15.hpp',
        f'halo2/source_{args.module}_tu15.hpp').replace('source_run_physics_live_support_tu15',
        f'source_run_{args.module}_tu15').replace('source_physics_live_support_tu15_owns_entry',
        f'source_{args.module}_tu15_owns_entry')
    generated=generated_bodies(source)
    assert all(generated.get(entry)==body for entry,body in preserved.items()), 'Existing native routine changed during additive generation'
    if args.preserve_module:
        assert hashlib.sha256(args.preserve_module.read_bytes()).hexdigest()==preserved_hash
    args.output.write_text(source)
    report = json.loads(manifest.read_text())
    report.update(module_generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        generated_cpp_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(), candidates=candidates,
        module=args.module,actual_calls=calls,lifecycle_calls=lifecycle_calls,engine_effects_qualified=False)
    report.update(preserved_module_sha256=preserved_hash,
        preserved_body_sha256={f'{entry:08x}':hashlib.sha256(body.encode()).hexdigest()for entry,body in preserved.items()},
        all_existing_bodies_unchanged=True)
    manifest.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(entries=len(entries),instructions=len(instructions),calls=len(calls),
                         source_bytes=args.output.stat().st_size)))


if __name__ == '__main__':
    main()
