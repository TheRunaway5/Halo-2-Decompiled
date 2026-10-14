#!/usr/bin/env python3
"""Lower whole TU1.5 AI frame bodies reached by an authenticated original replay.

Every included routine retains its complete CFG, including untaken branches.
The reference selects routine identities only. No captured result, actor state
or service return is substituted for source instructions. The frame and three
shared helpers are exported; full campaign behavior remains open.
"""
import argparse,json,re,struct
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--reference',type=Path,action='append',required=True)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--directory',type=Path,required=True);a=ap.parse_args()
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    assert sha(NEW_DEFAULT)==NEW_PIN
    references=[p.resolve() for p in a.reference];entries=set();reached=set()
    for reference in references:
        r=json.loads((reference/'report.json').read_text())
        hardware=json.loads((reference/'hardware/report.json').read_text())
        assert NEW_PIN==r['original_xbe_sha256'] and r['completed'] and r['entry']==0x1d0670
        assert not r['instruction_patches'] and not r['supplied_game_services']
        assert hardware['emulator_retained_state_matches_hardware'] and hardware['original_report_sha256']==sha(reference/'report.json')
        assert sha(reference/'original-trace.bin')==r['files_sha256']['original-trace.bin']
        trace=[p for p, in struct.iter_unpack('<I',(reference/'original-trace.bin').read_bytes())]
        entries.update({r['entry'],*(int(p,16) for p in r['original_functions'])});reached.update(trace)
        for pc,target in zip(trace,trace[1:]):
            i=code.at(pc);assert bytes(i.bytes).hex()==r['executed_instruction_bytes'][f'{pc:08x}']
            if i.mnemonic=='call':assert code.inside(target);entries.add(target)
    # A tail jump does not enter a CALL hook and the older function inventory
    # omits some real entries. Recover only actual reached tail owners, while
    # retaining every complete routine's untaken branches as required calls.
    while True:
        additional=set()
        for entry in entries:
            body=_trace(code,entry,entries)
            additional.update(target for target in body.tail_jumps if target in reached)
        if additional<=entries:break
        entries.update(additional)
    sites,instructions,addresses,routines={},{},{},[]
    for entry in sorted(entries):
        body=_trace(code,entry,entries);assert body.code
        assert all(width in (1,4) for _,width,_ in body.tables)
        sites[f'{entry:08x}']=sorted(body.code)
        routines.append(dict(entry=entry,tables=[dict(disc=p,count=n) for p,w,n in body.tables if w==4],
            byte_tables=[dict(address=p,width=w,count=n,raw_hex=image.read(p,w*n).hex()) for p,w,n in body.tables if w==1]))
        for pc in sorted(body.code):
            i=code.at(pc);assert image.read(pc,i.size)==bytes(i.bytes)
            instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
            for operand in i.operands:
                if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
                elif operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{p:08x}':p for p in entries},
        body_sites=sites,instruction_map=instructions,address_map={f'{p:08x}':q for p,q in addresses.items()},routines=routines)
    a.directory.mkdir(parents=True,exist_ok=True);correspondence=a.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    generate(SimpleNamespace(output=a.output,manifest=a.directory/'generation.json',correspondence=correspondence,additional_entries=Path(__file__)),paired)
    source=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_ai_frame_tu15.py').replace('halo2/physics_live_support_tu15.hpp','halo2/source_ai_frame_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_ai_frame_tu15').replace('source_physics_live_support_tu15_owns_entry','source_ai_frame_tu15_owns_entry')
    source,n=re.subn(r'bool source_ai_frame_tu15_owns_entry\(std::uint32_t p\) \{[^\n]+',
        'bool source_ai_frame_tu15_owns_entry(std::uint32_t p) {return p==0x1d0670u || p==0x20b620u || p==0x20a160u || p==0x272550u;}',source);assert n==1
    a.output.write_text(source)
    report=json.loads((a.directory/'generation.json').read_text())
    report.update(generated_cpp_sha256=sha(a.output),module_generator_sha256=sha(Path(__file__)),
        original_reference_report_sha256=sha(references[0]/'report.json'),hardware_report_sha256=sha(references[0]/'hardware/report.json'),
        references=[dict(report=str(p/'report.json'),report_sha256=sha(p/'report.json'),hardware_report_sha256=sha(p/'hardware/report.json')) for p in references],
        exported_entry=0x1d0670,exported_helpers=[0x20b620,0x20a160,0x272550],scope=__doc__)
    (a.directory/'generation.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print('Complete original AI frame bodies',len(entries),'instructions',len(instructions))

if __name__=='__main__':main()
