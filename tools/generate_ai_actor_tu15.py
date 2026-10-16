#!/usr/bin/env python3
"""Lower whole owned TU1.5 AI storage, actor and birth-state owners."""
import argparse,json,struct
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha
from validate_ai_hardware_step_reference_tu15 import validate as validate_hardware_step

# Existing scene-device storage owns1E7D90; live-support owns1E8410, CE440
# and the membership owners. Keep their actual external dispatch boundaries.
# State-context startup owns1E7710; its actual new-map owner1E7760 is here.
ENTRIES=(0x1cfa40,0x1cfc90,0x297640,0x265e80,0x208bf0,0x261a10,
    0x213c00,0x29be40,0x1af970,0x1d02c0,0x208df0,0x213c80,0x29f680,
    0x1e7e40,0x1ecc40,0x1b0cd0,0x1afa90,0x208ee0,0x208c90,0x20d540,
    0x1e9000,0x1ebe10,0x1ebcb0,0x275e30,0x271080,0x275b70,0x1ea1d0,0x1e7760,
    # Authored squad birth uses the real iterator/activation owners. The
    # campaign new-player continuation14E798 registers its full player datum.
    0x20cff0,0x1e9460,0x1e96a0,0x272520,0x1d03a0,
    # The actual NPC discharge aim path26DD14 queries its BSP cluster's
    # authored background-sound definition through this whole owner.
    0x11e660,
    # Actual public-placement Hunter object updateC7ED1 queries its retained
    # unit extension's89/8B flags through this complete source owner.
    0x1164d0,
    # Its adjacent real C7EE1 transition passes0.5f through116440 to the
    # complete1E14F0 retained four-byte transition owner.
    0x116440,0x1e14f0,
    # RealC7EC8 passes the same authored duration with BL=0.
    0x116400,
    # Actual object-updateE558D and AI-frame2635B4 query whether the
    # same object has an unfinished recording in the startup-owned5021FC pool.
    # Complete leaf2039B0 retains its bitmap iterator, AL result and RET4 ABI.
    0x2039b0,
    # Actual XAPILIB::_cinit table46DAD8..46DBF0 publishes these71
    # complete C++ constructors. Each copies an authored behavior mask into
    # its handler record; omitting them excludes combat behavior selection.
    *range(0x381a60,0x381ed0,0x10))

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,default=Path('src/source_ai_actor_tu15_generated.cpp'))
    ap.add_argument('--directory',type=Path,required=True)
    ap.add_argument('--reference',type=Path,action='append',default=[],help='Authenticated actual nested public Hunter unit-action original replay')
    a=ap.parse_args()
    assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    entries=set(ENTRIES);references=[]
    for path in a.reference:
        path=path.resolve();report=json.loads((path/'report.json').read_text());hardware=json.loads((path/'hardware/report.json').read_text())
        assert report['completed'] and report['entry']==0x11ddf0 and report['captured_unit_action']
        assert report['source_context']['public_campaign_placement'] and report['original_xbe_sha256']==NEW_PIN
        assert not report['instruction_patches'] and not report['supplied_game_services']
        assert hardware['original_report_sha256']==sha(path/'report.json')
        assert validate_hardware_step(path,report,hardware) is None, 'Unit-action estimate boundary requires explicit admission'
        assert sha(path/'original-trace.bin')==report['files_sha256']['original-trace.bin']
        trace=[pc for pc, in struct.iter_unpack('<I',(path/'original-trace.bin').read_bytes())];reached=set(trace);tails=set()
        entries.update({report['entry'],*(int(pc,16) for pc in report['original_functions'])})
        for pc,target in zip(trace,trace[1:]):
            instruction=code.at(pc);assert instruction.bytes.hex()==report['executed_instruction_bytes'][f'{pc:08x}']
            if instruction.mnemonic=='call':
                assert code.inside(target);entries.add(target)
            elif instruction.mnemonic=='jmp' and instruction.operands[0].type!=2:tails.add((pc,target))
        while True:
            additional=set()
            for entry in entries:
                body=_trace(code,entry,entries)
                additional.update(target for target in body.tail_jumps if target in reached)
                tables={p for p,width,count in body.tables if width==4}
                for pc,target in tails:
                    if pc in body.code and code.at(pc).operands[0].mem.disp not in tables:
                        assert code.inside(target);additional.add(target)
            if additional<=entries:break
            entries.update(additional)
        references.append(dict(report=str(path/'report.json'),report_sha256=sha(path/'report.json'),hardware_report_sha256=sha(path/'hardware/report.json')))
    assert struct.unpack('<3I',image.read(0x44b6f8,12))==(0x1cfa40,0x1cfc70,0x1cfc90)
    assert struct.unpack('<3I',image.read(0x44b158,12))==(0x1e7710,0x31d400,0x1e7760)
    constructors=[]
    handlers={struct.unpack('<I',image.read(0x47b478+i*4,4))[0]+4:i for i in range(0x83)}
    for slot in range(0x46dad8,0x46dbf4,4):
        entry=struct.unpack('<I',image.read(slot,4))[0]
        assert entry==0x381a60+(slot-0x46dad8)*4
        body=_trace(code,entry,set(ENTRIES));instructions_=[code.at(p) for p in sorted(body.code)]
        assert len(instructions_)==3
        load,store,ret=instructions_
        assert [i.mnemonic for i in instructions_]==['mov','mov','ret']
        assert load.op_str.startswith('eax, dword ptr [0x') and store.op_str.endswith('], eax')
        source=load.operands[1].mem.disp&0xffffffff
        destination=store.operands[0].mem.disp&0xffffffff
        assert destination in handlers
        constructors.append(dict(table_slot=slot,entry=entry,handler=handlers[destination],
            source=source,destination=destination,authored_mask=struct.unpack('<I',image.read(source,4))[0]))
    assert len(constructors)==71
    # Whole original _cinit walks this published table in increasing order.
    assert image.read(0x2dbbde,5)==bytes.fromhex('bef0d64600')
    assert image.read(0x2dbbe5,5)==bytes.fromhex('bf78dd4600')
    sites,instructions,addresses,routines={},{},{},[]
    for entry in sorted(entries):
        body=_trace(code,entry,set(ENTRIES));assert body.code
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
    source=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_ai_actor_tu15.py').replace('halo2/physics_live_support_tu15.hpp','halo2/source_ai_actor_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_ai_actor_tu15').replace('source_physics_live_support_tu15_owns_entry','source_ai_actor_tu15_owns_entry')
    a.output.write_text(source)
    report=json.loads((a.directory/'generation.json').read_text())
    report.update(generated_cpp_sha256=sha(a.output),module_generator_sha256=sha(Path(__file__)),
        references=references,behavior_mask_initializers=constructors,crt_dispatcher=0x2dbbad,
        crt_table_begin=0x46d6f0,crt_table_end=0x46dd78)
    (a.directory/'generation.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print('Complete AI storage/actor owners',len(entries),'instructions',len(instructions))

if __name__=='__main__':main()
