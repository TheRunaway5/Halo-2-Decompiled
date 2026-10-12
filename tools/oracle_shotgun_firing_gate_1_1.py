#!/usr/bin/env python3
"""Execute original shotgun interval/admission and recovery owners unchanged.

Explicit raw object/tag/time banks, inherited from the bounded SMG fixture,
are input controls. No complete equipped weapon, animation, contact or physics
is claimed. No service is replaced inside either tested complete function.
"""
import hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Trace,ROOT,WEAPON,DEF,BARREL,MAG,TRIGGER,WI,POOL,UC_X86_REG_EAX,UC_X86_REG_ECX
from inspect_multiplayer import Cache

def main():
    t=Trace();c=Cache(ROOT/'assets/disc/maps/lockout.map')
    tag=next(v for v in c.tags.values() if v['group']=='weap' and v['name'].endswith('\\shotgun'))
    p=tag['offset'];t.g.mem_write(DEF,bytes(c.data[p:p+0x310]))
    banks=[]
    for off,stride,target in ((0x2c0,0x5c,MAG),(0x2c8,0x40,TRIGGER),(0x2d0,0xec,BARREL)):
        rows=c.block(p+off,stride);assert len(rows)==1
        raw=bytes(c.data[rows[0]:rows[0]+stride]);t.g.mem_write(target,raw);t.w(DEF+off+4,target)
        banks.append(dict(offset=f'{off:x}',file_offset=rows[0],raw_hex=raw.hex()))
    cases=[];out=POOL+0x1800
    for count,counter in ((0,0),(0,127),(1,0),(1,127)):
        t.g.mem_write(WEAPON+0x1a4,bytes(0x34));t.sh(WEAPON+0x1aa,count)
        t.g.mem_write(WEAPON+0x1a4,bytes([counter]));t.w(out,0)
        before=len(t.events);t.active='shotgun_admission';t.run(0x1062b0,(WI,out,out+1),((UC_X86_REG_EAX,0),))
        events=t.events[before:];assert not any('service' in e for e in events)
        admitted=t.g.reg_read(UC_X86_REG_EAX)&255
        assert admitted==(1 if count==0 else 0)
        cases.append(dict(shot_count=count,counter=counter,admitted=admitted,
            minimum= t.u(out,1),maximum=t.u(out+1,1),instructions=t.count))
    t.g.mem_write(WEAPON+0x1a4,bytes(0x34));before=len(t.events)
    t.active='shotgun_recovery';t.run(0x108010,(0,),((UC_X86_REG_EAX,WI),))
    assert not any('service' in e for e in t.events[before:])
    recovery=dict(mode=t.u(WEAPON+0x1a5,1),ticks=t.u(WEAPON+0x1a6,2),instructions=t.count)
    assert recovery['mode']==2 and recovery['ticks']==30
    report=dict(scope=__doc__,xbe_sha256=hashlib.sha256(t.image.data).hexdigest(),
        cache_sha256=hashlib.sha256(c.data).hexdigest(),tag=tag,banks=banks,cases=cases,recovery=recovery,
        rate_bits=[f'{t.u(BARREL+at):08x}' for at in (4,8)],
        trigger_behavior=t.u(TRIGGER+6,2),barrel_flags=f'{t.u(BARREL):08x}',
        replaced_services=[],instruction_patches=[])
    path=ROOT/'analysis/multiplayer/integration/shotgun-firing-gate-20261011.json'
    path.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(cases=len(cases),recovery=recovery,output=str(path))))
if __name__=='__main__':main()
