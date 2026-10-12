#!/usr/bin/env python3
"""Execute unchanged1E94E0 and all six physical movement selection owners.

Declared physical packet, current state and output API records cover signed
movement, zero, infinities, NaNs, flags and all four FP rounding modes. No
callee effects, return values, instruction patches or native expected values
are supplied. These are component inputs, not a produced live physical pose.
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
    for state,flags,vector,contact in itertools.product(range(1,7),(0,0x20),
        ((0,0,0),(1,0,0),(-1,0,0),(0,1,0),(0,-1,0),(1,1,1),(-1,-1,-1),(.000001,-.000001,.1),
         (float('inf'),0,0),(0,float('inf'),0),(float('nan'),0,0),(0,float('nan'),0),(0,0,float('nan'))),(0,1)):
        records.append(dict(owner=0x1e94e0,state=state,flags=flags,vector=vector,contact=contact))
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
        put(physical+0x14,record['flags'],1)
        for i,v in enumerate(record['vector']):pf(physical+0xdc+i*4,v)
        put(body+0x414,record['contact'],1)
        put(physical+0xbc,record['contact'],1);pf(physical+0xd8,1)
        for reg,val in ((R.UC_X86_REG_EAX,physical),(R.UC_X86_REG_ECX,body+0x3dc),(R.UC_X86_REG_ESI,output)):
            g.reg_write(reg,val)
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
        touch(output,bytes(g.mem_read(output,9)))
        g.hook_add(UC_HOOK_CODE,code);g.hook_add(UC_HOOK_MEM_READ,read);g.hook_add(UC_HOOK_MEM_WRITE,write)
        g.emu_start(record['owner'],stop,count=100000)
        assert g.reg_read(R.UC_X86_REG_EIP)==stop,(name,hex(g.reg_read(R.UC_X86_REG_EIP)))
        result=(g.reg_read(R.UC_X86_REG_XMM0)&0xffffffff if record['owner']==0xdee00 else g.reg_read(R.UC_X86_REG_EAX)&255)
        rows.append(f'C {name} {record["owner"]:08x} {(body+0x3dc):08x} {output:08x} {physical:08x} {byte:02x} {sp:08x} {cw:04x} {mx:08x} {result:08x}')
        for p,raw in spans(initial):rows.append(f'M {p:08x} {raw.hex()}')
        for site,p,before,raw in writes:rows.append(f'W {site:08x} {p:08x} {before.hex()} {raw.hex()}')
        rows.append(f'A {output:08x} '+bytes(g.mem_read(output,9)).hex())
        audit.append(dict(name=name,inputs=record,result=f'{result:08x}',stores=len(writes)))
    fixture=a.output/'original.tsv';fixture.write_text('\n'.join(rows)+'\n')
    (a.output/'report.json').write_text(json.dumps(dict(tool_sha256=sha(__file__),source_sha256=PIN,
        fixture_sha256=sha(fixture),cases=audit,boundaries=__doc__,instruction_patches=[],supplied_dependency_returns=[],native_expected_data_used=False,live_gameplay_proof=False),indent=2)+'\n')
    print(json.dumps(dict(cases=len(audit),stores=sum(c['stores']for c in audit),fixture_sha256=sha(fixture))))
if __name__=='__main__':main()
