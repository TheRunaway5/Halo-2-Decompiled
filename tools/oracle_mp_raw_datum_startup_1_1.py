#!/usr/bin/env python3
"""Original16D0D0/16D070/16D130/16D210/16D220 raw pool startup.

Declared caller allocations retain poisoned payload, real name-derived salt,
existing occupied elements and bitmap tails. No runtime object, pose, mode or
allocation result is supplied. Complete bodies execute unchanged without
callee stubs. This proves raw storage ownership, not live match binding.
"""
import argparse,hashlib,itertools,json
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,PIN
from export_mp_ragdoll_owners import spans,sha

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');assert hashlib.sha256(x.data).hexdigest()==PIN
    pool,name,elements,bitmap=0x1000000,0x1004000,0x1008000,0x100c000;sp,stop=0x200f000,0x3000000
    rows=[];audit=[]
    names=(b'object\0',b'p\0',b'\0',b'physical components\0',bytes(range(65,97))+b'\0')
    for entry,capacity,stride,variant in itertools.product((0x16d0d0,0x16d070,0x16d130,0x16d210,0x16d220),(0,1,31,33),(2,7,12),range(5)):
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
        for section in x.sections:g.mem_write(section.va,x.section_bytes(section))
        g.mem_map(pool,0x10000);g.mem_map(0x2000000,0x10000);g.mem_map(stop,0x1000)
        def put(p,v,n=4):g.mem_write(p,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
        for p in (pool,name,elements,bitmap):g.mem_write(p,bytes([0x71])*0x1000)
        name+=variant%4;g.mem_write(name,names[variant]);alignment=(0,1,2,4,5)[variant];context=0x12340000+variant
        put(pool+0x20,capacity);put(pool+0x24,stride);put(pool+0x2a,8 if variant%2 else 0,1)
        put(pool+0x29,0x65,1);put(pool+0x44,elements);put(pool+0x48,bitmap)
        g.mem_write(pool,names[variant]);
        for i in range(capacity):put(elements+i*stride,0 if (i+variant)%3==0 else 0x8000+i,2)
        put(sp,stop);args=[];regs={R.UC_X86_REG_ESI:pool}
        if entry==0x16d0d0:args=[name,capacity,stride,alignment,context,bitmap]
        elif entry==0x16d070:args=[pool,name,context];regs.update({R.UC_X86_REG_EBX:capacity,R.UC_X86_REG_EDX:stride,R.UC_X86_REG_ECX:alignment})
        elif entry==0x16d130:regs.update({R.UC_X86_REG_EAX:capacity,R.UC_X86_REG_ECX:elements})
        elif entry==0x16d210:regs={R.UC_X86_REG_EAX:pool}
        for i,v in enumerate(args):put(sp+4+i*4,v)
        regs[R.UC_X86_REG_ESP]=sp
        for reg,v in regs.items():g.reg_write(reg,v)
        initial={};current={};writes=[];pc=0
        def touch(p,raw):
            for i,v in enumerate(raw):
                if p+i not in current:initial[p+i]=current[p+i]=v
                assert current[p+i]==v
        def code(uc,p,size,user):
            nonlocal pc
            pc=p
        def read(uc,access,p,size,value,user):touch(p,bytes(uc.mem_read(p,size)))
        def write(uc,access,p,size,value,user):
            before=bytes(uc.mem_read(p,size));touch(p,before);raw=(value&((1<<(8*size))-1)).to_bytes(size,'little')
            for i,v in enumerate(raw):current[p+i]=v
            if not 0x2000000<=p<0x2010000:writes.append((pc,p,before,raw))
        extents=((pool,0x800),(elements,0x800),(bitmap,0x40))
        for p,n in extents:touch(p,bytes(g.mem_read(p,n)))
        g.hook_add(UC_HOOK_CODE,code);g.hook_add(UC_HOOK_MEM_READ,read);g.hook_add(UC_HOOK_MEM_WRITE,write)
        g.emu_start(entry,stop,count=100000);assert g.reg_read(R.UC_X86_REG_EIP)==stop
        label=str(len(audit));rows.append(f'C {label} {entry:08x} {pool:08x} {name:08x} {capacity:08x} {stride:08x} {alignment:02x} {context:08x} {bitmap:08x} {elements:08x}')
        for p,raw in spans(initial):rows.append(f'M {p:08x} {raw.hex()}')
        for site,p,before,raw in writes:rows.append(f'W {site:08x} {p:08x} {before.hex()} {raw.hex()}')
        for p,n in extents:rows.append(f'A {p:08x} '+bytes(g.mem_read(p,n)).hex())
        audit.append(dict(case=label,entry=f'{entry:08x}',capacity=capacity,stride=stride,name_variant=variant,stores=len(writes)))
        name-=variant%4
    fixture=a.output/'original.tsv';fixture.write_text('\n'.join(rows)+'\n')
    (a.output/'report.json').write_text(json.dumps(dict(source_sha256=PIN,tool_sha256=sha(__file__),fixture_sha256=sha(fixture),cases=audit,boundaries=__doc__,instruction_patches=[],supplied_dependency_returns=[],native_expected_data_used=False,live_gameplay_proof=False),indent=2)+'\n')
    print(json.dumps(dict(cases=len(audit),stores=sum(c['stores']for c in audit),fixture_sha256=sha(fixture))))
if __name__=='__main__':main()
