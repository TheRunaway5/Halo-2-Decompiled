#!/usr/bin/env python3
"""Lower whole TU1.5 AI frame bodies reached by an authenticated original replay.

Every included routine retains its complete CFG, including untaken branches.
The reference selects routine identities only. No captured result, actor state
or service return is substituted for source instructions. The frame and shared
placement/notification helpers are exported; full campaign behavior remains open.
"""
import argparse,json,re,struct
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha
from validate_ai_hardware_step_reference_tu15 import validate as validate_hardware_step
from mcc_script_builtin_hooks import add_hooks

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--reference',type=Path,action='append',required=True)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--directory',type=Path,required=True);a=ap.parse_args()
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    assert sha(NEW_DEFAULT)==NEW_PIN
    references=[p.resolve() for p in a.reference];entries=set();reached=set();indirect_tails=set();hardware_steps={}
    for reference in references:
        r=json.loads((reference/'report.json').read_text())
        hardware=json.loads((reference/'hardware/report.json').read_text())
        assert NEW_PIN==r['original_xbe_sha256'] and r['completed'] and r['entry'] in (0x1d0670,0x266d30,0x2658b0,0x1d1880,0x26f260,0x1d63c0,0x218020,0x2afa50,0x27fa20)
        if r['entry'] in (0x266d30,0x2658b0,0x1d1880,0x26f260,0x1d63c0,0x218020,0x2afa50,0x27fa20):
            assert r['source_context']['actual_nested_caller']=={0x266d30:0x204553,0x2658b0:0x204589,0x1d1880:0xcc9cd,0x26f260:0x26e5e4,0x1d63c0:0x1d5897,0x218020:0x11750d,0x2afa50:0x211c1b,0x27fa20:0x2afa89}[r['entry']]
            assert r['source_context']['public_campaign_placement']
        assert not r['instruction_patches'] and not r['supplied_game_services']
        assert hardware['original_report_sha256']==sha(reference/'report.json')
        step=validate_hardware_step(reference,r,hardware)
        if step:hardware_steps[str(reference)]=step
        assert sha(reference/'original-trace.bin')==r['files_sha256']['original-trace.bin']
        trace=[p for p, in struct.iter_unpack('<I',(reference/'original-trace.bin').read_bytes())]
        entries.update({r['entry'],*(int(p,16) for p in r['original_functions'])});reached.update(trace)
        for pc,target in zip(trace,trace[1:]):
            i=code.at(pc);assert bytes(i.bytes).hex()==r['executed_instruction_bytes'][f'{pc:08x}']
            if i.mnemonic=='call':assert code.inside(target);entries.add(target)
            elif i.mnemonic=='jmp' and i.operands[0].type!=2:
                assert code.inside(target);indirect_tails.add((pc,target))
    # Complete descriptor-table closure for the original 1B0020 indirect
    # call at 1B0195. Authored campaign actors reach row130 in 47B478;
    # the original descriptor, not a supplied service result, names 1C9C90.
    descriptor_entries=[]
    if 0x1b0020 in entries:
        assert image.read(0x47b478+130*4,4)==struct.pack('<I',0x48b770)
        assert image.read(0x48b770,0x20).hex()=='82000200f81f0000feffffff00000000909c1c00509d1c00e09c1c0000000000'
        assert image.read(0x1b0194,3).hex()=='53ffd0'
        entries.add(0x1c9c90)
        descriptor_entries.append(dict(table=0x47b478,index=130,descriptor=0x48b770,
            callback_offset=0x10,indirect_call=0x1b0195,entry=0x1c9c90))
    # Kind0 selector row48 is reached by actual1B0128. Its+10 member
    # receives both original arguments; neighboring bytes are not callbacks.
    if 0x1b0020 in entries:
        assert image.read(0x47b478+48*4,4)==struct.pack('<I',0x48af80)
        assert image.read(0x47b478+87*4,4)==struct.pack('<I',0x48af94)
        assert image.read(0x48af80,0x14).hex()=='3000000000000000feffffff0000000000fe1b00'
        assert image.read(0x1b011b,5).hex()=='66837f0200'
        assert image.read(0x1b0126,5).hex()=='5153ff5710'
        entries.add(0x1bfe00)
        descriptor_entries.append(dict(table=0x47b478,index=48,descriptor=0x48af80,
            descriptor_kind=0,callback_offset=0x10,indirect_call=0x1b0128,entry=0x1bfe00))
    # The actual 1B0128 selector call reaches row51's +10 member.
    # Keep its RET8 ABI and complete conditional paths inside this module.
    if 0x1b0020 in entries:
        assert image.read(0x47b478+51*4,4)==struct.pack('<I',0x48af58)
        assert image.read(0x48af58,0x14).hex()=='33000000ff070000feffffff00000000e0fc1b00'
        assert image.read(0x1b011b,5).hex()=='66837f0200'
        assert image.read(0x1b0126,5).hex()=='5153ff5710'
        entries.add(0x1bfce0)
        descriptor_entries.append(dict(table=0x47b478,index=51,descriptor=0x48af58,
            descriptor_kind=0,callback_offset=0x10,indirect_call=0x1b0128,entry=0x1bfce0))
    # A tail jump does not enter a CALL hook and the older function inventory
    # omits some real entries. Recover only actual reached tail owners, while
    # retaining every complete routine's untaken branches as required calls.
    while True:
        additional=set()
        for entry in entries:
            body=_trace(code,entry,entries)
            additional.update(target for target in body.tail_jumps if target in reached)
            # A CRT classifier can jump through a runtime function table
            # rather than a fixed switch table. Its real reached destination
            # has no CALL record. Keep known switch labels in their owning
            # CFG; admit only the original trace's other indirect tail owners.
            tables={p for p,width,count in body.tables if width==4}
            for pc,target in indirect_tails:
                if pc in body.code and code.at(pc).operands[0].mem.disp not in tables:
                    additional.add(target)
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
    exported_helpers=[0x20b620,0x20a160,0x272550,0x2063e0,0x1ed450]
    if 0x211da0 in entries:exported_helpers.append(0x211da0)
    if 0x266d30 in entries:exported_helpers.append(0x266d30)
    if 0x2658b0 in entries:exported_helpers.append(0x2658b0)
    if 0x1d1880 in entries:exported_helpers.append(0x1d1880)
    if 0x26f260 in entries:exported_helpers.append(0x26f260)
    if 0x1d63c0 in entries:exported_helpers.append(0x1d63c0)
    if 0x218020 in entries:exported_helpers.append(0x218020)
    if 0x2afa50 in entries:exported_helpers.append(0x2afa50)
    if 0x27fa20 in entries:exported_helpers.append(0x27fa20)
    source,n=re.subn(r'bool source_ai_frame_tu15_owns_entry\(std::uint32_t p\) \{[^\n]+',
        'bool source_ai_frame_tu15_owns_entry(std::uint32_t p) {return '+
        ' || '.join(f'p==0x{p:x}u' for p in [0x1d0670,*exported_helpers])+';}',source);assert n==1
    a.output.write_text(add_hooks(source))
    report=json.loads((a.directory/'generation.json').read_text())
    report.update(generated_cpp_sha256=sha(a.output),module_generator_sha256=sha(Path(__file__)),
        original_reference_report_sha256=sha(references[0]/'report.json'),hardware_report_sha256=sha(references[0]/'hardware/report.json'),
        references=[dict(report=str(p/'report.json'),report_sha256=sha(p/'report.json'),hardware_report_sha256=sha(p/'hardware/report.json')) for p in references],
        observed_indirect_tails=[dict(instruction=pc,target=target) for pc,target in sorted(indirect_tails)],
        observed_original_hardware_references=hardware_steps,
        authenticated_descriptor_entries=descriptor_entries,
        native_mcc_script_hook_generator_sha256=sha(Path(__file__).with_name('mcc_script_builtin_hooks.py')),
        exported_entry=0x1d0670,exported_helpers=exported_helpers,scope=__doc__)
    (a.directory/'generation.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print('Complete original AI frame bodies',len(entries),'instructions',len(instructions))

if __name__=='__main__':main()
