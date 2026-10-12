#!/usr/bin/env python3
"""Unchanged transition clocks, animation weights and completion API matrix.

Raw caller records include boundary byte counters, signed decay weights,
target weights, actual source constants, RNG seeds and all four FP modes.
Complete1150D0/1DCD10,10F430,1102B0 and1105E0/E7E30 execute without patches,
dependency-return injection or native expected results. These component
records do not stand in for live animation or pose producers.
"""
import argparse,hashlib,itertools,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,PIN
from export_mp_ragdoll_owners import spans,sha

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True)
    a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');assert hashlib.sha256(image.data).hexdigest()==PIN
    base,stack,sp,stop=0x1000000,0x2000000,0x200f000,0x3000000
    pool,headers,body,definition,tags,rng=[base+i*0x1000 for i in range(6)]
    manager,extension=body+0x400,body+0x600;datum=0x81000000
    scalars=(-2.,-0.,0.,.00001,.01,.5,.99999,1.,2.,float('inf'),float('-inf'),float('nan'))
    rows=[];audit=[]
    for rounding,entry,variant in itertools.product(range(4),(0x1150d0,0x10f430,0x1102b0,0x1105e0),range(192)):
        name=f'round{rounding}_{entry:08x}_{variant}'
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
        for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
        g.mem_map(base,0x10000);g.mem_map(stack,0x10000);g.mem_map(stop,0x1000)
        def put(p,v,n=4):g.mem_write(p,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
        def pf(p,v):g.mem_write(p,struct.pack('<f',v))
        put(0x4e78d0,pool);put(pool+0x44,headers);put(headers+8,body)
        put(body,0x82000000);put(body+0x12a,0x400,2);put(body+0x33e,0x600,2)
        put(0x4eb114,tags);put(tags+8,definition);put(0x4ee9d8,rng)
        put(rng+4,(variant*0x91e10da5+0x24681357)&0xffffffff)
        for i in range(5):
            p=extension+0x80+i*4
            put(p,(0,1,126,127,128,254,255)[(variant+i)%7],1)
            put(p+1,(0,1,127,128,255)[(variant//3+i)%5],1)
            put(p+3,(0,1,2,3,0x80,0x81)[(variant//5+i)%6],1)
        put(extension+0x38,0x83000000 if variant%3==0 else -1)
        pf(extension+0x40,scalars[variant%len(scalars)])
        pf(definition+0x1e8,scalars[(variant//2)%len(scalars)])
        for i in range(12):put(extension+0x50+i,(0,1,127,128,129,254,255)[(variant+i)%7],1)
        put(extension+0x36,-1 if variant%5==0 else 0,2);put(extension+2,(0,1,2,254,255)[(variant//3)%5],1)
        pf(extension+0x60,scalars[(variant//3)%len(scalars)])
        pf(extension+0x64,scalars[(variant//5)%len(scalars)])
        pf(extension+0x68,scalars[(variant//7)%len(scalars)])
        put(extension+0x6c,(0,5,12,13,-1)[(variant//2)%5],2)
        for i in range(13):put(extension+0x6e+i,(0,1,127,128,254,255)[(variant+i)%6],1)
        put(manager+0x6c,variant%2,1)
        for i in (0,0x40):
            put(manager+i,-1 if (variant>>(2 if i else 1))&1 else 0x84000000)
            put(manager+i+6,-1 if (variant>>(4 if i else 3))&1 else 0,2)
            put(manager+i+0x12,variant//7,1);put(manager+i+0x11,(0,1,2,8,9)[(variant//9)%5],1)
        cw,mx=0x27f+(rounding<<10),0x1f80+(rounding<<13);put(sp,stop)
        for reg,v in ((R.UC_X86_REG_ESP,sp),(R.UC_X86_REG_EAX,datum),(R.UC_X86_REG_ECX,manager),
                      (R.UC_X86_REG_FPCW,cw),(R.UC_X86_REG_MXCSR,mx),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_FPSW,0)):
            g.reg_write(reg,v)
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
        touch(extension,bytes(g.mem_read(extension,0x100)));touch(manager,bytes(g.mem_read(manager,0x90)))
        g.hook_add(UC_HOOK_CODE,code);g.hook_add(UC_HOOK_MEM_READ,read);g.hook_add(UC_HOOK_MEM_WRITE,write)
        g.emu_start(entry,stop,count=100000);assert g.reg_read(R.UC_X86_REG_EIP)==stop
        result=g.reg_read(R.UC_X86_REG_EAX)&255
        rows.append(f'C {name} {entry:08x} {datum:08x} {manager:08x} {sp:08x} {cw:04x} {mx:08x} {result:02x}')
        for p,raw in spans(initial):rows.append(f'M {p:08x} {raw.hex()}')
        for site,p,before,raw in writes:rows.append(f'W {site:08x} {p:08x} {before.hex()} {raw.hex()}')
        for p,n in ((extension,0x100),(manager,0x90)):
            rows.append(f'A {p:08x} '+bytes(g.mem_read(p,n)).hex())
        audit.append(dict(name=name,entry=f'{entry:08x}',variant=variant,result=result,stores=len(writes)))
    fixture=a.output/'original.tsv';fixture.write_text('\n'.join(rows)+'\n')
    (a.output/'report.json').write_text(json.dumps(dict(tool_sha256=sha(__file__),source_sha256=PIN,fixture_sha256=sha(fixture),
        cases=audit,boundaries=__doc__,instruction_patches=[],supplied_dependency_returns=[],native_expected_data_used=False,live_gameplay_proof=False),indent=2)+'\n')
    print(json.dumps(dict(cases=len(audit),stores=sum(c['stores']for c in audit),fixture_sha256=sha(fixture))))
if __name__=='__main__':main()
