#!/usr/bin/env python3
"""Lower complete unit feedback bodies from authenticated weapon impact callers."""
import argparse
import json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--reference',type=Path,required=True)
    p.add_argument('--unit-reaction-reference',type=Path)
    p.add_argument('--directory',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    references=[(0x1f5620,0xe0ddd,a.reference)]
    if a.unit_reaction_reference:references.append((0xe3e20,0xe0de6,a.unit_reaction_reference))
    instructions={};addresses={};bodies={};proofs=[];callees=set()
    for entry,caller,directory in references:
        reference=json.loads((directory/'report.json').read_text())
        hardware=json.loads((directory/'hardware/report.json').read_text())
        assert reference['completed'] and reference['entry']==entry
        assert reference['original_xbe_sha256']==NEW_PIN
        assert reference['source_context']['actual_source_weapon_sequence']
        assert reference['source_context']['actual_nested_caller']==caller
        assert not reference['instruction_patches'] and not reference['supplied_game_services']
        assert hardware['original_report_sha256']==sha(directory/'report.json')
        assert hardware['emulator_retained_state_matches_hardware'] and hardware['entire_exit_equal_except_platform_IF']
        if entry==0xe3e20:assert reference['source_context']['callee_stack_arguments']==[1]
        call=code.at(caller);assert call.mnemonic=='call' and call.operands[0].imm==entry
        body=_trace(code,entry,{entry});assert body.code and not body.tables
        bodies[entry]=sorted(body.code);callees.update(body.calls)
        proofs.append(dict(entry=entry,caller=caller,reference_sha256=sha(directory/'report.json'),
            hardware_sha256=sha(directory/'hardware/report.json')))
        for pc in sorted(body.code):
            op=code.at(pc);assert bytes(op.bytes)==image.read(pc,op.size)
            instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+op.size]=pc+op.size
            for operand in op.operands:
                if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
                elif operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{entry:08x}':entry for entry in bodies},
        body_sites={f'{entry:08x}':sites for entry,sites in bodies.items()},instruction_map=instructions,
        address_map={f'{p:08x}':v for p,v in addresses.items()},routines=[dict(entry=entry,tables=[],byte_tables=[]) for entry in bodies])
    a.directory.mkdir(parents=True,exist_ok=False);path=a.directory/'bodies.json'
    path.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    generate(SimpleNamespace(output=a.output,manifest=a.directory/'generation.json',correspondence=path,
        additional_entries=Path(__file__)),paired,complete_listings=True,code_override=code)
    s=a.output.read_text().replace('halo2/physics_live_support_tu15.hpp','halo2/source_damage_feedback_tu15.hpp')
    s=s.replace('source_run_physics_live_support_tu15','source_run_damage_feedback_tu15')
    s=s.replace('source_physics_live_support_tu15_owns_entry','source_damage_feedback_tu15_owns_entry')
    s=s.replace('tools/generate_physics_live_support_tu15.py','tools/generate_damage_feedback_tu15.py')
    a.output.write_text(s)
    report=json.loads((a.directory/'generation.json').read_text())
    report.update(generated_cpp_sha256=sha(a.output),generator_sha256=sha(Path(__file__)),
        reference_sha256=sha(a.reference/'report.json'),hardware_sha256=sha(a.reference/'hardware/report.json'),
        authenticated_callers=proofs,external_complete_callees_required=sorted(callees-set(bodies)),full_weapon_parity=False)
    (a.directory/'generation.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(whole_routines=len(bodies),static_instructions=len(instructions),external_callees=len(callees-set(bodies)))))
if __name__=='__main__':main()
