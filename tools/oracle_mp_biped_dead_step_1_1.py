#!/usr/bin/env python3
"""Execute unchanged1F0080 on declared dead-state movement API records.

The original integrator runs with raw physical/component/contact/output
records in all four FP modes. These cases cover awake-body movement, damping,
desired motion, saturating counters and contact-bank support admission. The
parent/shape query branches are excluded here; they are not supplied fake
callee results. No instruction patches or native expected outputs are used.
These are component tests, not live contact/pose producers.
"""
import argparse, hashlib, itertools, json, struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, PIN
from export_mp_ragdoll_owners import spans, sha

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True)
    a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(image.data).hexdigest()==PIN
    base,stack,sp,stop=0x1000000,0x2000000,0x200f000,0x3000000
    pool,headers,body,cp,components,physical= [base+i*0x1000 for i in range(6)]
    tags,definition,clock,apool,actors,ppool,properties,output=[base+i*0x1000 for i in range(6,14)]
    datum=0x83000000
    records=[]
    velocities=((0,0,0),(.001,-.002,-.3),(.9,-.4,.7),(1.3,.8,-.4),(-.7,.2,1.1),(0,0,-2))
    targets=((0,0,0),(.2,.7,0),(-.7,.9,0),(.001,-.001,0))
    for variant in range(192):
        records.append(dict(owner=0x1f0080,state=3,variant=variant,
            velocity=velocities[variant%6],target=targets[(variant//3)%4],flags=(0,2,4,6)[(variant//5)%4],
            awake=(variant//7)%2,ground=(variant//11)%2,timer=(0,253,254,255)[(variant//13)%4],
            contacts=(0,1,5)[(variant//17)%3],support=(variant//19)%3))
    rows=[];audit=[]
    for rounding,record in itertools.product(range(4),records):
        name=f'round{rounding}_case{len(audit)}'
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
        for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
        g.mem_map(base,0x10000);g.mem_map(stack,0x10000);g.mem_map(stop,0x1000)
        def put(p,v,n=4):g.mem_write(p,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
        def pf(p,v):g.mem_write(p,struct.pack('<f',v))
        put(0x4e78d0,pool);put(pool+0x44,headers);put(headers+8,body)
        put(body,0x81000000);put(0x4eb114,tags);put(tags+8,definition)
        put(0x526248,cp);put(cp+0x44,components);put(components+0x70,physical)
        put(body+0xb4,0x82000000 if record.get('component',True) else 0xffffffff)
        put(physical+0x44,record.get('awake',0),1);put(components+4,record.get('cflags',0))
        put(body+0x34b,record.get('disabled',0),1);put(body+0x3dc,record['state'],1)
        for i,v in enumerate(record.get('angular',(0,0,0))):pf(body+0x1b0+i*4,v)
        put(body+0x14,0xffffffff);put(body+0x348,record.get('bflags',record.get('airborne',0)),2)
        put(body+0x399,100,1);put(0x518444,clock);put(clock+2,30,2);pf(clock+4,1/30)
        pf(definition+0x250,record.get('rate',0));pf(body+0x2c4,record.get('buoyancy',0))
        put(body+0x134,record.get('flags',0));put(body+0x3f8,record.get('contact',0),1)
        put(body+0x12c,0x84000000 if record.get('actor') else 0xffffffff)
        put(0x4fcdc8,apool);put(apool+0x44,actors)
        put(actors+0x858,0x85000000 if record.get('property') else 0xffffffff)
        put(actors+7,record.get('actor_disabled',0),1);put(0x509bec,ppool);put(ppool+0x44,properties)
        put(properties+0x30,record.get('property_kind',0),2);pf(properties+0x60,record.get('property_rate',0))
        put(body+0x34c,record.get('selector',-1),2)
        put(output,0x7000101);put(output+4,record.get('action',0x400000c));put(output+8,0x71,1)
        cw,mx=0x27f+(rounding<<10),0x1f80+(rounding<<13)
        put(sp,stop)
        for reg,val in ((R.UC_X86_REG_ESP,sp),(R.UC_X86_REG_EAX,datum),
            (R.UC_X86_REG_ESI,datum),(R.UC_X86_REG_EBX,output),(R.UC_X86_REG_EDI,output+4),
            (R.UC_X86_REG_FPCW,cw),(R.UC_X86_REG_MXCSR,mx),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_FPSW,0)):
            g.reg_write(reg,val)
        mode=record.get('mode',0);byte=0xe5
        if record['owner']==0xe44c0:
            for i,v in enumerate((mode,byte,output+8)):put(sp+4+i*4,v)
        dead=body+0x3ec;bodybank=base+0xe000;contacts=base+0xf000
        put(components+0x70,bodybank);put(bodybank+0x44,record['awake'],1)
        for i,v in enumerate((.3,-.15,.4)):pf(bodybank+0x30+i*4,v)
        put(physical+0x18,0x82000000);put(physical+0xc,0xffffffff)
        put(dead+0x14,0xffffffff);put(dead+0xc,record['ground'],1)
        for off in (0xe,0xf,0x10):put(dead+off,record['timer'],1)
        for i,v in enumerate(record['velocity']):pf(physical+0x124+i*4,v)
        for i,v in enumerate(record['target']):pf(physical+0x138+i*4,v)
        pf(physical+0xf4,.91);pf(physical+0xf8,-.37)
        pf(physical+0x144,1.3);pf(physical+0x148,4.2)
        put(physical+0x14,record['flags'],1);put(physical+0xb9,record['variant']%2,1)
        put(components+0x18,0xff if record['support']==2 else 0,1);put(components+0x74,1)
        put(components+0x88,contacts);put(components+0x8c,record['contacts'])
        for i in range(record['contacts']):
            put(contacts+i*0x48+0x14,0x86000000)
            put(contacts+i*0x48+0x18,0xffffffff if record['support']==0 else 0x87000000)
        put(0x526254,base+0xd800);pf(base+0xd800,9.8)
        g.mem_write(output,bytes([0x71])*0x70)
        put(sp+4,dead);put(sp+8,output);g.reg_write(R.UC_X86_REG_EDI,physical)
        initial={};current={};writes=[];instruction=0
        def touch(p,raw):
            for i,v in enumerate(raw):
                if p+i not in current:initial[p+i]=current[p+i]=v
                assert current[p+i]==v,(name,hex(p+i))
        def code(uc,p,size,user):
            nonlocal instruction
            instruction=p
        def read(uc,access,p,size,value,user):touch(p,bytes(uc.mem_read(p,size)))
        def write(uc,access,p,size,value,user):
            before=bytes(uc.mem_read(p,size));touch(p,before)
            raw=(value&((1<<(8*size))-1)).to_bytes(size,'little')
            for i,v in enumerate(raw):current[p+i]=v
            if not stack<=p<stack+0x10000:writes.append((instruction,p,before,raw))
        touch(output,bytes(g.mem_read(output,0x70)));touch(dead,bytes(g.mem_read(dead,0x60)))
        g.hook_add(UC_HOOK_CODE,code);g.hook_add(UC_HOOK_MEM_READ,read);g.hook_add(UC_HOOK_MEM_WRITE,write)
        g.emu_start(record['owner'],stop,count=100000)
        assert g.reg_read(R.UC_X86_REG_EIP)==stop,(name,hex(g.reg_read(R.UC_X86_REG_EIP)))
        result=(g.reg_read(R.UC_X86_REG_XMM0)&0xffffffff if record['owner']==0xdee00 else g.reg_read(R.UC_X86_REG_EAX)&255)
        rows.append(f'C {name} {record["owner"]:08x} {dead:08x} {output:08x} {physical:08x} {byte:02x} {sp:08x} {cw:04x} {mx:08x} {result:08x}')
        for p,raw in spans(initial):rows.append(f'M {p:08x} {raw.hex()}')
        for site,p,before,raw in writes:rows.append(f'W {site:08x} {p:08x} {before.hex()} {raw.hex()}')
        rows.append(f'A {output:08x} '+bytes(g.mem_read(output,0x70)).hex());rows.append(f'A {dead:08x} '+bytes(g.mem_read(dead,0x60)).hex())
        audit.append(dict(name=name,inputs=record,result=f'{result:08x}',stores=len(writes)))
    fixture=a.output/'original.tsv';fixture.write_text('\n'.join(rows)+'\n')
    (a.output/'report.json').write_text(json.dumps(dict(tool_sha256=sha(__file__),source_sha256=PIN,
        fixture_sha256=sha(fixture),cases=audit,boundaries=__doc__,instruction_patches=[],supplied_dependency_returns=[],native_expected_data_used=False,live_gameplay_proof=False),indent=2)+'\n')
    print(json.dumps(dict(cases=len(audit),stores=sum(c['stores']for c in audit),fixture_sha256=sha(fixture))))
if __name__=='__main__':main()
