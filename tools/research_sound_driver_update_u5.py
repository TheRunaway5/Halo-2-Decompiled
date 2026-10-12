#!/usr/bin/env python3
"""Pair complete sound driver update and DSP section owner; no device results."""
import argparse,json
from pathlib import Path
import research_animation_selection_u5 as pairing
OWNERS=(0x220d10,0x193020)
BOUNDARIES=(0x550a0,0x53500,0x2b4bf0)
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();pairing.OWNERS=OWNERS;pairing.BOUNDARIES=BOUNDARIES;pairing.run(out)
    r=json.loads((out/'report.json').read_text());images=[pairing.Xbe(p)for p in(pairing.OLD,pairing.NEW)];codes=[pairing._Code(x,'.text')for x in images]
    selected=[]
    for entry,parent in((0x2dbda1,0x193020),(0x2dbdbc,0x193020),(0x381ead,0x193020),(0x381e84,0x220d10)):
        caller=next(q for q in r['owners']if q['old']==f'{parent:08x}')
        call=next(q for q in caller['calls']if q['old_target']==f'{entry:08x}');seq=[]
        for tag,image,e in zip(('old','u5'),images,(entry,int(call['new_target'],16))):
            section=next(s for s in image.sections if s.va<=e<s.va+s.vsize);code=pairing._Code(image,section)
            f=pairing._trace(code,e,set());ins=[code.at(q)for q in sorted(f.code)];seq.append(ins)
            pairing.save(out/'instructions'/f'{entry:08x}-{tag}.asm',''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode())
        normal=[[pairing.norm(i,x,{q.address:k for k,q in enumerate(ins)})for i in ins]for x,ins in zip(images,seq)];assert normal[0]==normal[1]
        selected.append(dict(old=f'{entry:08x}',u5=call['new_target'],rows=len(seq[0]),actual_paired_call=call,normalized_complete_body_and_internal_branch_equal=True,selection='Actual direct call in paired owner; no code-section or address-neighborhood assumption.'))
    fields=(('driver_pointer',0x526474),('effects_generation',0x38d118),('effects_context_pointer',0x518480),('optional_sound_flag',0x4d0ed8),('quiesce_flag',0x47e4bc),('quiesce_clear_word',0x4d0e86))
    bindings={}
    for name,old in fields:
        matches={m['new_va']for q in r['owners']for m in q['memory']if m['old_va']==f'{old:08x}'};assert len(matches)==1,(name,matches)
        bindings[name]=dict(old=f'{old:08x}',u5=next(iter(matches)),mutable_operand=True)
    lines=[(out/'instructions'/f'00193020-{tag}.asm').read_text().splitlines()for tag in('old','u5')]
    for name,oldpc,oldvalue,extent in(('section_name',0x193023,0x45aea0,9),('quiesce_record',0x193052,0x47e4b8,0)):
        index=next(k for k,q in enumerate(lines[0])if q.startswith(f'{oldpc:08x} '));addresses=[int(q[index].split()[-1],16)for q in lines];assert addresses[0]==oldvalue
        bindings[name]=dict(old=f'{addresses[0]:08x}',u5=f'{addresses[1]:08x}',mutable_operand=name=='quiesce_record')
        if extent:bindings[name]['raw_hex']={tag:x.read(at,extent).hex()for tag,x,at in zip(('old','u5'),images,addresses)};assert images[0].read(addresses[0],extent)==images[1].read(addresses[1],extent)==b'DSPImage\0'
    update=next(q for q in r['owners']if q['old']=='00220d10');oldlines=(out/'instructions/00220d10-old.asm').read_text().splitlines();newlines=(out/'instructions/00220d10-u5.asm').read_text().splitlines();idx=next(k for k,q in enumerate(oldlines)if q.startswith('00220dbd '));tables=[0x220ddc,codes[1].at(int(newlines[idx].split()[0],16)).operands[0].mem.disp]
    destination=int(newlines[next(k for k,q in enumerate(oldlines)if q.startswith('00220dc4 '))].split()[0],16)
    assert [int.from_bytes(images[0].read(tables[0]+k*4,4),'little')for k in range(4)]==[0x220dc4]*4
    assert [int.from_bytes(images[1].read(tables[1]+k*4,4),'little')for k in range(4)]==[destination]*4
    pairing.save(out/'providers.json',pairing.encoded(dict(schema='sound-driver-update-providers-v1',tool_sha256=pairing.sha(__file__),bindings=bindings,caller_selected_section_wrappers=selected,dispatch_tables=[dict(image=tag,address=f'{at:08x}',raw16_hex=x.read(at,16).hex())for tag,x,at in zip(('old','u5'),images,tables)],platform_results=[])))
