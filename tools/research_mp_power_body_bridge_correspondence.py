#!/usr/bin/env python3
"""Static original body correspondence; never seed/execute later gameplay."""
import sys,json,hashlib
from pathlib import Path
import research_mp_sandbox_update_functions as source
from oracle_mp_power_body_bridge_1_1 import ROOT,OUT,verify,ENTRIES
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write

if __name__=='__main__':
    verify()
    source.ENTRIES=tuple(sorted(ENTRIES))
    if len(sys.argv)==1:sys.argv+=['--output',str(OUT/'final/later-correspondence/report.json')]
    source.main()
    p=Path(sys.argv[sys.argv.index('--output')+1]);r=json.loads(p.read_text())
    old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');new=Xbe(source.NEW_DEFAULT)
    oc,nc=_Code(old,'.text'),_Code(new,'.text')
    f=_trace(oc,0x2e7610,set(ENTRIES));oi=[oc.at(a)for a in sorted(f.code)]
    assert len(oi)==7 and oi[-1].mnemonic=='jmp'
    prefix=b''.join(i.bytes for i in oi[:-1]);sec=new.section('.text');raw=new.section_bytes(sec)
    hits=[];at=0
    while True:
        at=raw.find(prefix,at)
        if at<0:break
        hits.append(sec.va+at);at+=1
    assert len(hits)==1,hits
    target=hits[0];nf=_trace(nc,target,{target,0x2ea050});ni=[nc.at(a)for a in sorted(nf.code)]
    assert source.full_tokens(oi,old)==source.full_tokens(ni,new)
    assert ni[-1].operands[0].imm==0x2ea050 and not f.tables and not nf.tables
    q=next(q for q in r['routines']if q['disc_1_1_entry']=='002e7610')
    q.update(later_entry=f'{target:08x}',candidates=[f'{target:08x}'],old_instructions=7,later_instructions=7,
        normalized_instruction_and_internal_branch_topology_equal=True,
        actual_tail_target_original='002e65a0',actual_tail_target_later='002ea050',
        exact_address_free_six_instruction_prefix=prefix.hex())
    for label,ins,image in (('disc',oi,old),('later',ni,new)):
        out=p.parent/'instructions'/f'002e7610-{label}-{0x2e7610 if label=="disc"else target:08x}.asm'
        for i in ins:assert image.read(i.address,len(i.bytes))==i.bytes
        frozen_write(out,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode())
        q[label+'_listing']={'file':out.name,'sha256':hashlib.sha256(out.read_bytes()).hexdigest()}
    r['wrapper_tool_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    r['native_later_gameplay_execution']=False
    frozen_write(p.parent/'complete-report.json',encoded(r))
    print(json.dumps(dict(complete_mapped=len(r['routines']),wrapper=f'{target:08x}')))
