#!/usr/bin/env python3
"""Pair complete cache clock, alias and drain owners without platform results."""
import argparse,json
from pathlib import Path
import research_animation_selection_u5 as pairing
OWNERS=(0x1274b0,0x13e9a0,0x21aa60,0x21a800,0x13efa0,0x21aaf0,0x16d4c0)
BOUNDARIES=(0x127290,0x337a40,0x2dc9c8)
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args()
    pairing.OWNERS=OWNERS;pairing.BOUNDARIES=BOUNDARIES
    out=args.output.resolve();pairing.run(out)
    # The small clock leaf has three identical u5 bodies. Its actual whole
    # caller, rather than proximity or an annotation, identifies this copy.
    r=json.loads((out/'report.json').read_text())
    images=[pairing.Xbe(p)for p in(pairing.OLD,pairing.NEW)]
    codes=[pairing._Code(x,'.text')for x in images]
    controls=[]
    for old,parent in((0x21a920,0x1274b0),(0x2dc892,0x21a800)):
        caller=next(q for q in r['owners']if q['old']==f'{parent:08x}')
        calls=[q for q in caller['calls']if q['old_target']==f'{old:08x}']
        assert len(calls)==1,calls
        entries=[old,int(calls[0]['new_target'],16)];seq=[]
        for tag,code,image,entry in zip(('old','u5'),codes,images,entries):
            flow=pairing._trace(code,entry,set())
            ins=[code.at(a)for a in sorted(flow.code)];seq.append(ins)
            pairing.save(out/'instructions'/f'{old:08x}-{tag}.asm',''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode())
        normalized=[[pairing.norm(i,x,{q.address:k for k,q in enumerate(ins)})for i in ins]for x,ins in zip(images,seq)]
        assert normalized[0]==normalized[1]
        controls.append(dict(old=f'{old:08x}',u5=f'{entries[1]:08x}',rows=len(seq[0]),actual_paired_caller=caller['old'],actual_paired_call=calls[0],normalized_complete_body_and_internal_branch_equal=True,unique_leaf_match=False,selection_authority='Direct call in uniquely paired complete caller; no address-neighborhood inference.'))
    pairing.save(out/'caller-correspondence.json',pairing.encoded(dict(schema='sound-cache-lifecycle-caller-correspondence-v1',controls=controls)))
    fields=(('entry_pool_pointer',0x5098dc),('page_allocator_pointer',0x5098e4),('physical_buffer_pointer',0x5098e0),('work_mutex_slot',0x4e792c),('work_head',0x4e9ebc),('requests_added',0x4e9ec0),('sound_system_pointer',0x4ed950))
    operands={}
    for name,old in fields:
        matches={m['new_va']for owner in r['owners']for m in owner['memory']if m['old_va']==f'{old:08x}'}
        assert len(matches)==1,(name,matches)
        operands[name]=dict(old=f'{old:08x}',u5=next(iter(matches)),kind='mutable operand address; not an executable-initial value or active default')
    pairing.save(out/'providers.json',pairing.encoded(dict(schema='sound-cache-lifecycle-binding-providers-v1',fields=operands,immutable_scalar_defaults=[],source_availability_inputs=[],tool_sha256=pairing.sha(__file__))))
