#!/usr/bin/env python3
"""Authenticate full motion/world bodies and operands against the supplied TU1.5."""
import argparse, json, sys
from pathlib import Path
import research_mp_sandbox_update_functions as base
from oracle_smg_firing_1_1 import Xbe, _Code, _trace, ROOT
ENTRIES=(0x317650,0x316200,0x316440,0x31cfe0,0x31d0f0,0x31ed30,0x31ec90,
    0x31ee50,0x31edc0,0x321600,0x321760,0x321530,0x31f8b0,0x322690,0x321320,
    0x1dc750,0x378ab0)
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True)
    a=ap.parse_args();base.ENTRIES=ENTRIES
    sys.argv=[sys.argv[0],'--output',str(a.output/'report.json')];base.main()
    r=json.loads((a.output/'report.json').read_text());rows=r['routines']
    old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');new=Xbe(base.NEW_DEFAULT)
    oc,nc=_Code(old,'.text'),_Code(new,'.text')
    mapped={int(q['disc_1_1_entry'],16):int(q['later_entry'],16)for q in rows if q.get('normalized_instruction_and_internal_branch_topology_equal')}
    for q in rows:
        addr=int(q['disc_1_1_entry'],16)
        if addr in mapped:continue
        oi=[oc.at(p)for p in sorted(_trace(oc,addr,set(ENTRIES)).code)]
        candidates=[]
        for candidate in q['candidates']:
            b=int(candidate,16);ni=[nc.at(p)for p in sorted(_trace(nc,b,set(mapped.values())).code)]
            if base.full_tokens(oi,old)!=base.full_tokens(ni,new):continue
            paired=[(x.operands[0].imm,y.operands[0].imm)for x,y in zip(oi,ni)
                if x.mnemonic=='call'and x.operands[0].type==y.operands[0].type==2]
            if all(x in mapped and mapped[x]==y for x,y in paired):candidates.append(b)
        assert len(candidates)==1,(q,candidates)
        mapped[addr]=candidates[0]
        q.update(later_entry=f'{candidates[0]:08x}',normalized_instruction_and_internal_branch_topology_equal=True,
            disambiguated_by_complete_body_and_verified_direct_callees=True)
    assert set(mapped)==set(ENTRIES)
    instruction_map={};operand_map={}
    for q in rows:
        addr=int(q['disc_1_1_entry'],16);target=mapped[addr]
        oi=[oc.at(p)for p in sorted(_trace(oc,addr,set(ENTRIES)).code)]
        ni=[nc.at(p)for p in sorted(_trace(nc,target,set(mapped.values())).code)]
        assert base.full_tokens(oi,old)==base.full_tokens(ni,new)
        q['old_instructions']=len(oi);q['later_instructions']=len(ni)
        for x,y in zip(oi,ni):
            instruction_map[f'{x.address:08x}']=y.address
            for xo,yo in zip(x.operands,y.operands):
                if xo.type==yo.type==3 and not xo.mem.base and not yo.mem.base and base.containing(old,xo.mem.disp) and base.containing(new,yo.mem.disp):
                    key=f'{xo.mem.disp:08x}';assert key not in operand_map or operand_map[key]['later']==yo.mem.disp
                    ob,nb=old.read(xo.mem.disp,xo.size),new.read(yo.mem.disp,yo.size)
                    operand_map[key]=dict(later=yo.mem.disp,width=xo.size,old_hex=ob.hex(),later_hex=nb.hex(),equal=ob==nb)
        # Every claimed complete body retains its exact source bytes.
        for label,ins in [('disc',oi),('later',ni)]:
            p=a.output/'instructions'/f'{addr:08x}-{label}-{addr if label=="disc"else target:08x}.asm'
            p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
            q[label+'_listing']=dict(file=p.name,sha256=base.sha(p))
    r.update(tool_sha256=base.sha(__file__),routines=rows,complete_bodies=len(mapped),
        entries={f'{k:08x}':v for k,v in mapped.items()},instruction_map=instruction_map,operand_map=operand_map)
    p=a.output/'complete-report.json';p.write_text(json.dumps(r,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(complete_bodies=len(mapped),report_sha256=base.sha(p))))
if __name__=='__main__':main()
