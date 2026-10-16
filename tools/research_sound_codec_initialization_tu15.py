#!/usr/bin/env python3
"""Pin codec initializer/callback CPU graph and every direct leaf consumer."""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

ENTRIES=[0x38ec8b,0x38e484,0x38f45f,0x38ed2c,0x38ecff,0x38ed90,0x38edad,
         0x38f378,0x38f389,0x38f443,0x38f24b,0x38f31f,0x38eee3,0x38ed7c,
         0x38f1f4,0x38f3d4,0x38f05c,0x38f395,0x38f1a6,0x38edf4]
WANTED={0x38ed2c,0x38ecff,0x38ed90,0x38edad,0x38f45f}
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert not a.output.exists()
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'DSOUND');bodies={entry:_trace(code,entry,set(ENTRIES))for entry in ENTRIES}
    a.output.mkdir(parents=True);sites={pc for body in bodies.values()for pc in body.code};instructions={}
    entries=[]
    for entry,body in bodies.items():
        rows=[]
        for pc in sorted(body.code):
            op=code.at(pc);assert bytes(op.bytes)==image.read(pc,op.size)
            instructions[f'{pc:08x}']=bytes(op.bytes).hex();rows.append(f'{pc:08x} {bytes(op.bytes).hex()} {op.mnemonic} {op.op_str}')
        name=f'original-{entry:08x}.asm';(a.output/name).write_text('\n'.join(rows)+'\n')
        entries.append(dict(entry=entry,listing=name,listing_sha256=hashlib.sha256((a.output/name).read_bytes()).hexdigest(),sites=sorted(body.code)))
    refs=[];literal_refs=[]
    for section in image.sections:
        raw=image.section_bytes(section)
        for off in range(len(raw)-4):
            if raw[off]!=0xe8:continue
            pc=section.va+off;target=(pc+5+struct.unpack_from('<i',raw,off+1)[0])&0xffffffff
            if target in WANTED:
                assert pc in sites,(hex(pc),hex(target))
                op=code.at(pc);assert op.mnemonic=='call'and len(op.bytes)==5
                refs.append(dict(section=section.name,instruction=pc,target=target,authenticated_body=True))
        for target in WANTED:
            needle=struct.pack('<I',target);at=raw.find(needle)
            while at>=0:
                literal_refs.append(dict(section=section.name,address=section.va+at,target=target));at=raw.find(needle,at+1)
    assert {(r['instruction'],r['target'])for r in refs}=={(0x38e497,0x38f45f),(0x38ed90,0x38ecff),(0x38edb3,0x38ed90),(0x38f46c,0x38ed2c),(0x38f481,0x38edad),(0x38f4fe,0x38ed90),(0x38f514,0x38ed90)}
    imports=[]
    for pc in sorted(sites):
        op=code.at(pc)
        if op.mnemonic not in ['call','jmp']or not op.operands or op.operands[0].type!=3:continue
        mem=op.operands[0].mem
        if not mem.base and not mem.index and 0x41b000<=mem.disp<0x41c000:
            value=struct.unpack('<I',image.read(mem.disp,4))[0];imports.append(dict(instruction=pc,slot=mem.disp,ordinal=value&0x7fffffff,tail=op.mnemonic=='jmp'))
    report=dict(passed=True,original_xbe_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),entries=entries,instruction_bytes=instructions,direct_leaf_consumers=refs,literal_leaf_references=literal_refs,imports=imports,
      hardware_only_leaf=dict(entry=0x38ed2c,scope='Admitted38F46C call only; register/stack operations, direct FEC0012C/FEC00130 hardware controls and optional KeStallExecutionProcessor. No ordinary game data store or allocation.',source_data_writes=[],native_adaptation_boundary='Actual native PCM route creation/configuration, never PCR/CAS/GLOB_CNT words.'),
      mixer_writes=[dict(parent=0x38edb3,index=0x13,byte_offset=0x26,value=0),dict(parent=0x38f4fe,index=1,byte_offset=2,value=0),dict(parent=0x38f514,index=0xc,byte_offset=0x18,value=0x0808)],
      unbound_channel_reset=dict(instruction=0x38f49e,channel_offsets=[0x10,0x70],byte_offset=0xb,value=2,source_channels_created_after_initializer=True),
      next_required_registration=dict(instruction=0x38f4c4,slot=0x41b7a8,ordinal=44,requested_bus_level=6,interrupt_object=0x391208,isr=0x38f378,source_context='actual controller ECX from38E497'),
      native_hardware_results_supplied=False,source_ready_words_supplied=False,full_constructor_returned=False)
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(bodies=len(entries),sites=len(sites),direct_leaf_consumers=len(refs),imports=len(imports))))
if __name__=='__main__':main()
