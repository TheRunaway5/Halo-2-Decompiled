#!/usr/bin/env python3
"""Unchanged TU1.5 physics pool constructors/lifecycle over actual host pages.

Only the reached kernel allocation/release imports use native resources. The
standalone caller ABI, host token policy and kernel-preserving native ABI are
declared. No source call is replaced, original instruction patched, or captured
heap supplied. This is fresh storage evidence, not an occupied world/match.
"""
import argparse
import hashlib
import json
import mmap
import ctypes
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

GP = [getattr(R,'UC_X86_REG_'+s) for s in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
STACK, SP, STOP = 0x2000000,0x200f000,0x3000000
GLOBALS = (0x52d68c,0x52d690,0x52be5c)
REPORT = ROOT/'analysis/multiplayer/sandbox-source/physics-pool-startup-tu15-20261012/correspondence-v1/complete-report.json'
REPORT_PIN = '6d10227d9838a4e6e2e2db443df0c699bbb203a7843a2b6f756a569fd755b527'
def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True)
    a=ap.parse_args()
    assert sha(NEW_DEFAULT)==NEW_PIN and sha(REPORT)==REPORT_PIN
    paired=json.loads(REPORT.read_text())
    mapping={int(p,16):q for p,q in paired['entries'].items()}
    instructions={int(q) for q in paired['instruction_map'].values()}
    a.output.mkdir(parents=True,exist_ok=False);(a.output/'storage').mkdir()
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    sequences=[];executed=set();counts={}
    def persist(raw):
        name='storage/'+hashlib.sha256(raw).hexdigest()+'.bin'
        path=a.output/name
        if not path.exists():path.write_bytes(raw)
        return name
    for layout,first in enumerate((0x10000000,0x11003000,0x12007000)):
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
        for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
        g.mem_map(STACK,0x10000);g.mem_map(STOP,0x1000);g.mem_map(0x80000000,0x1000)
        backing={};resources=[];stores=[];rows=[];site=[0];next_token=[first]
        def u(p):return struct.unpack('<I',g.mem_read(p,4))[0]
        def put(p,v):g.mem_write(p,struct.pack('<I',v))
        def hook(uc,p,n,user):
            if p in (0x800000b8,0x800000c7):
                sp=uc.reg_read(R.UC_X86_REG_ESP)
                assert u(sp)==(0x33b3c5 if p==0x800000b8 else 0x33b401)
                frontier=abi()
                bp=u(sp+4)
                if p==0x800000b8:
                    zero,np,flags,protection=[u(sp+8+i*4)for i in range(4)]
                    request=[u(bp),zero,u(np),flags,protection]
                    assert request[0:2]==[0,0] and request[3:]==[0x101000,4]
                    size=(request[2]+4095)&~4095
                    token=(next_token[0]+4095)&~4095
                    raw=mmap.mmap(-1,size,prot=mmap.PROT_READ|mmap.PROT_WRITE)
                    address=ctypes.addressof(ctypes.c_ubyte.from_buffer(raw))
                    uc.mem_map_ptr(token,size,3,address)
                    backing[token]=raw;next_token[0]=token+size
                    resources.append(dict(allocation=True,request=request,result=[0,token,size],entry_abi=frontier,
                        initial=persist(raw[:])))
                    put(bp,token);put(np,size);pop=24
                else:
                    np,kind=u(sp+8),u(sp+12);token,size=u(bp),u(np)
                    assert size==0 and kind==0x8000 and token in backing
                    resources.append(dict(allocation=False,request=[token,size,kind,0,0],result=[0,token,size],entry_abi=frontier))
                    uc.mem_unmap(token,len(backing[token]));backing.pop(token).close();pop=16
                uc.reg_write(R.UC_X86_REG_EAX,0)
                uc.reg_write(R.UC_X86_REG_ESP,sp+pop)
                uc.reg_write(R.UC_X86_REG_EIP,u(sp));return
            assert p in instructions and bytes(uc.mem_read(p,n))==image.read(p,n),hex(p)
            executed.add(p);site[0]=p;counts[p]=counts.get(p,0)+1
        def write(uc,access,p,n,value,user):
            if STACK<=p<STACK+0x10000:return
            assert any(base<=p and p+n<=base+len(raw)for base,raw in backing.items()) or any(
                start<=p and p+n<=start+4 for start in GLOBALS),hex(p)
            stores.append(dict(instruction=site[0],address=p,before=bytes(uc.mem_read(p,n)).hex(),
                written=(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()))
        g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write)
        def abi():return dict(gp=[g.reg_read(r)for r in GP],eflags=g.reg_read(R.UC_X86_REG_EFLAGS),
            mxcsr=g.reg_read(R.UC_X86_REG_MXCSR),x87_control=g.reg_read(R.UC_X86_REG_FPCW),
            x87_status=g.reg_read(R.UC_X86_REG_FPSW),xmm=[f'{g.reg_read(getattr(R,f"UC_X86_REG_XMM{i}")):032x}'for i in range(8)])
        def run(entry,kind,regs=None):
            values=[0x11223344,0x22334455,0x33445566,0x44556677,SP,0x66778899,0x778899aa,0x8899aabb]
            for index,value in (regs or {}).items():values[index]=value
            for reg,value in zip(GP,values):g.reg_write(reg,value)
            g.reg_write(R.UC_X86_REG_EFLAGS,2);g.reg_write(R.UC_X86_REG_FPCW,0x27f)
            g.reg_write(R.UC_X86_REG_FPSW,0);g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
            g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
            for i in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{i}'),int('102030405060708090a0b0c0d0e0f000',16)+i)
            put(SP,STOP);before=abi();begin=len(stores);resource_begin=len(resources)
            g.emu_start(mapping[entry],STOP,count=2000000)
            assert g.reg_read(R.UC_X86_REG_EIP)==STOP,kind
            rows.append(dict(entry=entry,kind=kind,entry_abi=before,exit_abi=abi(),
                stores=stores[begin:],resource_begin=resource_begin,resource_end=len(resources),
                globals=[u(p)for p in GLOBALS],live_allocations=len(backing)))
            return g.reg_read(R.UC_X86_REG_EAX)
        try:
            initial_globals=[bytes(g.mem_read(p,4)).hex()for p in GLOBALS]
            run(0x2284c0,'create_impacts');run(0x1d2620,'create_components')
            pools=[u(p)for p in GLOBALS]
            for pool,capacity in zip(pools,(32,32,512)):
                assert u(pool+0x29)&255==0
                run(0x16d210,'make_valid',{0:pool})
                datums=[run(0x16d2c0,'allocate',{2:pool})for _ in range(capacity)]
                assert all(d!=0xffffffff for d in datums)
                assert run(0x16d2c0,'exhausted',{2:pool})==0xffffffff
                for datum in (datums[0],datums[-1]):
                    assert run(0x16d560,'lookup_live',{0:datum,6:pool})!=0
                for datum in datums[::2]:run(0x16d4c0,'release_datum',{0:datum,2:pool})
                assert run(0x16d560,'lookup_retired',{0:datums[0],6:pool})==0
                replacements=[run(0x16d2c0,'reuse',{2:pool})for _ in datums[::2]]
                assert all(d!=0xffffffff for d in replacements)
                assert run(0x16d560,'lookup_stale_salt',{0:datums[0],6:pool})==0
                assert run(0x16d2c0,'reused_exhausted',{2:pool})==0xffffffff
                run(0x16d220,'reset',{6:pool})
                assert run(0x16d2c0,'allocate_after_reset',{2:pool})!=0xffffffff
            run(0x228540,'dispose_impacts')
            assert u(GLOBALS[0])==u(GLOBALS[1])==0 and len(backing)==1
            run(0x2284c0,'recreate_impacts')
            run(0x228540,'dispose_recreated_impacts')
            assert len(backing)==1
            final=[dict(address=p,bytes=len(raw),storage=persist(bytes(g.mem_read(p,len(raw)))))for p,raw in backing.items()]
            sequences.append(dict(name=f'layout_{layout}',first_token=first,initial_globals=initial_globals,
                resources=resources,steps=rows,final_storage=final))
        finally:
            del g
            for raw in backing.values():raw.close()
    report=dict(schema=1,image_sha256=NEW_PIN,correspondence_sha256=REPORT_PIN,
        tool_sha256=sha(__file__),scope=__doc__,stack=STACK,stack_bytes=0x10000,sp=SP,stop=STOP,
        globals=GLOBALS,sequences=sequences,canonical_instruction_map=paired['instruction_map'],
        unique_original_instructions=len(executed),
        original_instruction_counts={f'{p:08x}':n for p,n in sorted(counts.items())},
        original_instruction_patches=[],substituted_game_calls=[])
    path=a.output/'fixture.json';path.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(sha256=sha(path),sequences=len(sequences),
        steps=sum(len(s['steps'])for s in sequences),stores=sum(len(r['stores'])for s in sequences for r in s['steps']),
        original_instructions=len(executed))))
if __name__=='__main__':main()
