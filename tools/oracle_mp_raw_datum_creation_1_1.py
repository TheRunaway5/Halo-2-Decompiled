#!/usr/bin/env python3
"""Execute original pool creation, validity and first allocation without stubs.

The caller explicitly owns a poisoned arena, allocator object and initial
arena/checksum state.16CFF0 uses actual original125E50 or20DA30 method tables;
their bodies and1656A0/165700/16D070/16D210/16D2C0 execute unchanged. No
allocator return is supplied. This proves these component owners, not full
startup, a constructed world or live gameplay. Names/method tables retain
authenticated original bytes and native tests preserve their read-only access.
"""
import argparse,itertools,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,PIN
from export_mp_ragdoll_owners import spans,sha

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
    a.output.mkdir(parents=True,exist_ok=False)
    path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(path)==PIN
    image=Xbe(path);code=_Code(image,'.text')
    context,arena,sp,stop=0x1000000,0x1100000,0x200f000,0x3000000
    rows=[];audit=[];executed=set()
    for capacity,stride,alignment,failure in itertools.product((0,1,31,33,512,2048),(2,12,160),(0,4,5),(False,True)):
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
        for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
        g.mem_map(context,4096);g.mem_map(arena,0x100000);g.mem_map(0x2000000,0x10000);g.mem_map(stop,4096)
        g.mem_write(arena,b'\x71'*0x100000)
        def put(p,v):g.mem_write(p,struct.pack('<I',v))
        table=0x45a478 if failure else 0x45a610
        method=struct.unpack('<I',image.read(table,4))[0]
        assert method==(0x20da30 if failure else 0x125e50)
        put(context,table);put(0x4ed650,arena);put(0x4ed654,(capacity%3)*4);put(0x4ed65c,0xffffffff)
        name=0x45a3a4 if stride==12 else 0x45cc3c
        initial={};current={};stores=[];site=[0];allocation=[None]
        def retained(p):return not 0x2000000<=p<0x2010000
        def touch(p,raw):
            if not retained(p):return
            for i,v in enumerate(raw):
                if p+i not in current:initial[p+i]=current[p+i]=v
                assert current[p+i]==v
        def hook(uc,p,size,user):
            assert bytes(uc.mem_read(p,size))==image.read(p,size)
            site[0]=p;executed.add(p)
            if p==0x16d01d:
                result=uc.reg_read(R.UC_X86_REG_EAX)
                if result:
                    end=arena+struct.unpack('<I',uc.mem_read(0x4ed654,4))[0]
                    allocation[0]=(result,end-result)
                    touch(result,bytes(uc.mem_read(result,end-result)))
        def read(uc,access,p,size,value,user):touch(p,bytes(uc.mem_read(p,size)))
        def write(uc,access,p,size,value,user):
            if not retained(p):return
            before=bytes(uc.mem_read(p,size));touch(p,before)
            raw=(value&((1<<(size*8))-1)).to_bytes(size,'little')
            for i,v in enumerate(raw):current[p+i]=v
            stores.append((site[0],p,before,raw))
        g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_READ,read);g.hook_add(UC_HOOK_MEM_WRITE,write)
        def run(entry,args=(),regs=()):
            put(sp,stop)
            for i,v in enumerate(args):put(sp+4+4*i,v)
            g.reg_write(R.UC_X86_REG_ESP,sp);g.reg_write(R.UC_X86_REG_EFLAGS,2)
            for reg,value in regs:g.reg_write(reg,value)
            g.emu_start(entry,stop,count=1000000)
            assert g.reg_read(R.UC_X86_REG_EIP)==stop
            return g.reg_read(R.UC_X86_REG_EAX)
        pool=run(0x16cff0,(name,stride,alignment),((R.UC_X86_REG_EAX,capacity),(R.UC_X86_REG_EDI,context)))
        result=0xffffffff
        if pool:
            run(0x16d210,regs=((R.UC_X86_REG_EAX,pool),))
            result=run(0x16d2c0,regs=((R.UC_X86_REG_EDX,pool),))
        label=str(len(audit))
        rows.append(f'C {label} {capacity:08x} {name:08x} {stride:08x} {alignment:08x} {context:08x} {pool:08x} {result:08x}')
        for p,raw in spans(initial):rows.append(f'M {p:08x} {raw.hex()}')
        for pc,p,before,raw in stores:rows.append(f'W {pc:08x} {p:08x} {before.hex()} {raw.hex()}')
        for p,raw in spans(current):rows.append(f'A {p:08x} {raw.hex()}')
        audit.append(dict(case=label,capacity=capacity,stride=stride,alignment=alignment,method=f'{method:08x}',allocation=allocation[0],stores=len(stores),pool=f'{pool:08x}',first_datum=f'{result:08x}'))
    fixture=a.output/'original.tsv';fixture.write_text('\n'.join(rows)+'\n')
    listing=a.output/'executed-original.asm'
    listing.write_text(''.join(f'{p:08x} {code.at(p).bytes.hex()} {code.at(p).mnemonic} {code.at(p).op_str}\n'for p in sorted(executed)))
    (a.output/'report.json').write_text(json.dumps(dict(source_sha256=PIN,oracle_sha256=sha(__file__),fixture_sha256=sha(fixture),instruction_listing_sha256=sha(listing),cases=audit,boundaries=__doc__,instruction_patches=[],supplied_dependency_returns=[],native_expected_data_used=False,live_gameplay_proof=False),indent=2)+'\n')
    print(json.dumps(dict(cases=len(audit),stores=sum(c['stores']for c in audit),fixture_sha256=sha(fixture))))
if __name__=='__main__':main()
