#!/usr/bin/env python3
"""Original1D2620→11E150→33785C→16D070 with actual host raw allocation.

Only the NtAllocateVirtualMemory resource boundary adapts to Linux mmap. The
original request, actual host page size/backing and explicit32-bit mirror are
retained. No allocator return, pool field, validity, salt or datum is supplied
by a gameplay service. This is storage-owner evidence, not whole map startup
or live corpse/solver evidence. The negative kernel/error branches are not
covered by this successful-resource experiment.
"""
import argparse,ctypes,json,mmap,os,struct
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
    image=Xbe(path);code=_Code(image,'.text');executed=set();rows=[];audit=[]
    stack,sp,stop,mirror=0x2000000,0x200f000,0x3000000,0x1000000
    for stages in (1,2,3):
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
        for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
        g.mem_map(stack,0x10000);g.mem_map(stop,4096)
        g.mem_map(0x80000000,4096)
        initial={};current={};stores=[];requests=[];backing=[];site=[0]
        def u(p):return struct.unpack('<I',g.mem_read(p,4))[0]
        def put(p,v):g.mem_write(p,struct.pack('<I',v))
        def retained(p):return not stack<=p<stack+0x10000
        def touch(p,raw):
            if not retained(p):return
            for i,v in enumerate(raw):
                if p+i not in current:initial[p+i]=current[p+i]=v
                assert current[p+i]==v
        def hook(uc,p,size,user):
            if p==0x800000b8:
                esp=uc.reg_read(R.UC_X86_REG_ESP)
                bp,zero,np,flags,protect=[u(esp+4+i*4)for i in range(5)]
                base,requested=u(bp),u(np)
                assert (base,zero,requested,flags,protect)==(0,0,0x1409b,0x101000,4)
                page=os.sysconf('SC_PAGE_SIZE');mapped=(requested+page-1)//page*page
                raw=mmap.mmap(-1,mapped,prot=mmap.PROT_READ|mmap.PROT_WRITE)
                address=ctypes.addressof(ctypes.c_ubyte.from_buffer(raw))
                uc.mem_map_ptr(mirror,mapped,3,address);backing.append(raw)
                touch(mirror,bytes(raw))
                requests.append(dict(base=base,zero_bits=zero,bytes=requested,
                    allocation_type=flags,protection=protect,status=0,
                    resulting_base=mirror,resulting_bytes=mapped,
                    actual_host_page_bytes=page,raw_initial_sha256=sha_bytes(raw[:])))
                put(bp,mirror);put(np,mapped)
                uc.reg_write(R.UC_X86_REG_EAX,0)
                uc.reg_write(R.UC_X86_REG_ESP,esp+24)
                uc.reg_write(R.UC_X86_REG_EIP,u(esp));return
            assert bytes(uc.mem_read(p,size))==image.read(p,size)
            site[0]=p;executed.add(p)
        def read(uc,access,p,size,value,user):touch(p,bytes(uc.mem_read(p,size)))
        def write(uc,access,p,size,value,user):
            if not retained(p):return
            before=bytes(uc.mem_read(p,size));touch(p,before)
            raw=(value&((1<<(size*8))-1)).to_bytes(size,'little')
            for i,v in enumerate(raw):current[p+i]=v
            stores.append((site[0],p,before,raw))
        g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_READ,read);g.hook_add(UC_HOOK_MEM_WRITE,write)
        def run(entry,regs=()):
            put(sp,stop);g.reg_write(R.UC_X86_REG_ESP,sp);g.reg_write(R.UC_X86_REG_EFLAGS,2)
            for reg,value in regs:g.reg_write(reg,value)
            g.emu_start(entry,stop,count=1000000)
            assert g.reg_read(R.UC_X86_REG_EIP)==stop
            return g.reg_read(R.UC_X86_REG_EAX)
        try:
            run(0x1d2620);pool=u(0x526248);datum=0xffffffff
            assert pool==mirror and len(requests)==1
            if stages>=2:run(0x16d210,((R.UC_X86_REG_EAX,pool),))
            if stages>=3:datum=run(0x16d2c0,((R.UC_X86_REG_EDX,pool),))
            rows.append(f'C {stages} {pool:08x} {datum:08x}')
            q=requests[0]
            rows.append('K '+' '.join(f'{q[k]:08x}'for k in ('base','zero_bits','bytes','allocation_type','protection','status','resulting_base','resulting_bytes')))
            for p,raw in spans(initial):rows.append(f'M {p:08x} {raw.hex()}')
            for pc,p,before,raw in stores:rows.append(f'W {pc:08x} {p:08x} {before.hex()} {raw.hex()}')
            for p,raw in spans(current):rows.append(f'A {p:08x} {raw.hex()}')
            audit.append(dict(stages=stages,pool=f'{pool:08x}',datum=f'{datum:08x}',stores=len(stores),kernel_requests=requests))
        finally:
            del g
            for raw in backing:raw.close()
    fixture=a.output/'original.tsv';fixture.write_text('\n'.join(rows)+'\n')
    listing=a.output/'executed-original.asm'
    listing.write_text(''.join(f'{p:08x} {code.at(p).bytes.hex()} {code.at(p).mnemonic} {code.at(p).op_str}\n'for p in sorted(executed)))
    (a.output/'report.json').write_text(json.dumps(dict(source_sha256=PIN,oracle_sha256=sha(__file__),fixture_sha256=sha(fixture),instruction_listing_sha256=sha(listing),cases=audit,boundaries=__doc__,instruction_patches=[],supplied_gameplay_returns=[],native_expected_data_used=False,live_gameplay_proof=False),indent=2)+'\n')
    print(json.dumps(dict(cases=len(audit),stores=sum(c['stores']for c in audit),fixture_sha256=sha(fixture))))

def sha_bytes(raw):
    import hashlib
    return hashlib.sha256(raw).hexdigest()
if __name__=='__main__':main()
