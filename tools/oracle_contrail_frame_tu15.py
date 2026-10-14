#!/usr/bin/env python3
"""Unchanged17EAD0 contrail tick, including actual point/pool retirement.

Declared detached point banks cover every authored Lockout definition. No
instruction or service return is replaced; dead returned scratch is excluded.
"""
import hashlib,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from audit_rendering import Cache

def main():
    x=Xbe(NEW_DEFAULT);assert hashlib.sha256(x.data).hexdigest()==NEW_PIN
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
    for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
    u.mem_map(0x2400000,0x20000);u.mem_map(0x2800000,0x10000);u.mem_map(0x3000000,4096)
    regs=[R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI]
    trace=[];reads=set();u.hook_add(UC_HOOK_CODE,lambda g,p,n,_:trace.append(p))
    u.hook_add(UC_HOOK_MEM_READ,lambda g,access,p,n,v,_:reads.update(range(p&~4095,(p+n+4095)&~4095,4096)) if p<0x800000 else None)
    c=Cache('assets/disc/maps/lockout.map');rows=[]
    for tag in c.tags.values():
        if tag['group']!='cont' or not tag['address']:continue
        _,p=c.tag(tag['datum'],'cont');states=c.block(p+0xe8,64)
        if not states:continue
        for delta in (0.,1/30,.1):
            for age in (.25,.99,1.25):
                for expired in (False,True):
                    owner,definition,state_at,points,cpool,ppool,tags,seed=0x2400000,0x2401000,0x2402000,0x2404000,0x2406000,0x2406100,0x2410000,0x2409000
                    root=bytearray(c.data[p:p+0xf0]);struct.pack_into('<I',root,0xec,state_at)
                    state=b''.join(bytes(c.data[q:q+64])for q in states)
                    runtime=bytearray(72);struct.pack_into('<H',runtime,0,0x1234)
                    struct.pack_into('<I',runtime,4,0x56780000);struct.pack_into('<I',runtime,8,0xffffffff)
                    struct.pack_into('<f',runtime,0x14,1);struct.pack_into('<H',runtime,0x30,2)
                    for chain in range(4):struct.pack_into('<I',runtime,0x38+chain*4,0x23450000 if chain==0 else 0xffffffff)
                    pointbank=bytearray(112)
                    for n in range(2):
                        at=n*56;struct.pack_into('<H',pointbank,at,0x2345);pointbank[at+2]=4 if expired else 0;pointbank[at+3]=len(states)-1
                        struct.pack_into('<3f',pointbank,at+4,age,10.,.5)
                        struct.pack_into('<h',pointbank,at+0x18,-1) # Actual NONE cluster boundary, no invented BSP bank.
                        struct.pack_into('<3f',pointbank,at+0x1c,n,1,2)
                        struct.pack_into('<I',pointbank,at+0x34,0x23450001 if n==0 else 0xffffffff)
                    def pool(capacity,stride,count,data,bitmap):
                        h=bytearray(80)
                        for at,value in ((0x20,capacity),(0x24,stride),(0x34,count),(0x38,count),(0x3c,count),(0x44,data),(0x48,bitmap)):struct.pack_into('<I',h,at,value)
                        h[0x29]=1;return bytes(h)
                    headers=bytearray(0x10000);struct.pack_into('<4I',headers,0,0x636f6e74,0x56780000,definition,len(root))
                    physics=c.u32(states[-1]+0x14);physics_banks=[]
                    if physics!=0xffffffff:
                        t,q=c.tag(physics,'pphy');physics_data=bytes(c.data[q:q+t['size']]);physics_at=0x240a000
                        struct.pack_into('<4I',headers,(physics&65535)*16,0x70706879,physics,physics_at,len(physics_data));physics_banks=[(physics_at,physics_data),(tags+(physics&65535)*16,bytes(headers[(physics&65535)*16:(physics&65535)*16+16]))]
                    stack=bytearray(0x5000);struct.pack_into('<If',stack,0x4000,0x3000000,delta)
                    banks=[(owner,bytes(runtime)),(definition,bytes(root)),(state_at,state),(points,bytes(pointbank)),(cpool,pool(8,72,1,owner,0x2406200)),(ppool,pool(16,56,2,points,0x2406210)),(0x2406200,struct.pack('<I',1)),(0x2406210,struct.pack('<I',3)),(tags,bytes(headers[:16])),(seed,struct.pack('<II',0x12345678,0x12345678)),(0x2800000,bytes(stack)),(0x4f7214,struct.pack('<I',cpool)),(0x4f7210,struct.pack('<I',ppool)),(0x4f0414,struct.pack('<I',tags)),(0x4f3cd8,struct.pack('<I',seed))]
                    # Literal1F3864..1F38AE default physics-global stores.
                    physics_globals=x.read(0x450268,4)+x.read(0x46a0d0,4)+x.read(0x450270,4)+bytes(16)
                    banks+=physics_banks+[(0x240b000,physics_globals),(0x52be68,struct.pack('<I',0x240b000))]
                    for at,data in banks:u.mem_write(at,data)
                    gp=[0x72001000+n*0x1000 for n in range(8)];gp[4]=0x2804000
                    for reg,value in zip(regs,gp):u.reg_write(reg,value)
                    for n in range(8):u.reg_write(R.UC_X86_REG_XMM0+n,0)
                    for reg,value in ((R.UC_X86_REG_EFLAGS,0x202),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80)):u.reg_write(reg,value)
                    trace.clear();reads.clear()
                    try:u.emu_start(0x17ead0,0x3000000,count=100000)
                    except Exception:
                        print(tag['name'],delta,age,expired,hex(u.reg_read(R.UC_X86_REG_EIP)),[hex(p)for p in trace[-8:]]);raise
                    assert u.reg_read(R.UC_X86_REG_EIP)==0x3000000
                    globals=[]
                    for page in sorted(reads):
                        data=bytearray(u.mem_read(page,4096))
                        for at,value in banks:
                            low,high=max(at,page),min(at+len(value),page+4096)
                            if low<high:data[low-page:high-page]=value[low-at:high-at]
                        globals.append((page,bytes(data)))
                    highbanks=[(at,data)for at,data in banks if at>=0x800000]
                    rows.append(dict(name=tag['name'],delta=delta,age=age,expired=expired,entry_gp=gp,exit_gp=[u.reg_read(reg)for reg in regs],eflags=u.reg_read(R.UC_X86_REG_EFLAGS),exit_xmm=[f'{u.reg_read(R.UC_X86_REG_XMM0+n):032x}'for n in range(8)],instructions=trace.copy(),initial_banks=[dict(address=at,zero_bytes=len(data),patches=[dict(offset=0x4000,hex=data[0x4000:0x4008].hex())])if at==0x2800000 else dict(address=at,hex=data.hex())for at,data in globals+highbanks],final_banks=[dict(address=at,hex=bytes(u.mem_read(at,len(data))).hex())for at,data in highbanks if at!=0x2800000]+[dict(address=0x2804000,hex=bytes(u.mem_read(0x2804000,8)).hex())]))
    fixture=Path('tests/fixtures/contrail_frame_tu15_original.json');fixture.write_text(json.dumps(dict(entry=0x17ead0,original_sha256=NEW_PIN,cases=rows),separators=(',',':'))+'\n')
    report=dict(cases=len(rows),original_sha256=NEW_PIN,fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),instruction_patches=[],replaced_services=[],scope=__doc__)
    Path('analysis/weapon-effects-20261013/live-tu15/contrail-geometry/frame-oracle.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Whole original contrail tick',len(rows),'cases')
if __name__=='__main__':main()
