#!/usr/bin/env python3
"""Run unchanged42CE0 and all its callees on declared point/marker-chain banks.

Uses every authored Lockout contrail state bank, then explicit orientation,
endpoint, interpolation and view-fade boundary inputs. No code/service patches.
"""
import hashlib,json,random,struct
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
    u.mem_map(0x2400000,0x20000);u.mem_map(0x2800000,0x1000);u.mem_map(0x3000000,4096)
    regs=[R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI]
    trace=[];reads=set()
    u.hook_add(UC_HOOK_CODE,lambda g,p,n,_:trace.append(p))
    u.hook_add(UC_HOOK_MEM_READ,lambda g,access,p,n,v,_:reads.update(range(p&~4095,(p+n+4095)&~4095,4096)) if p<0x800000 else None)
    c=Cache('assets/disc/maps/lockout.map');rng=random.Random(0x42ce0)
    specifications=[]
    for t in c.tags.values():
        if t['group']!='cont' or not t['address']:continue
        _,p=c.tag(t['datum'],'cont');state=c.block(p+0xe8,64)
        if not state:continue
        definition=bytes(c.data[p:p+0xf0]);states=b''.join(bytes(c.data[q:q+64])for q in state)
        for chain in range(4):
            for count in (2,5):specifications.append((t['name'],definition,states,chain,count,None))
    first=specifications[0]
    for orientation in range(5):
        for fade in range(3):
            for flags in (0,3,0xc0):
                specifications.append((*first[:3],fade%4,5,(orientation,fade,flags)))
    rows=[]
    for name,raw,states,chain,count,boundary in specifications:
        owner,definition,state_at,points,pool,context,out=0x2400000,0x2401000,0x2402000,0x2404000,0x2406000,0x2407000,0x2408000
        tag=bytearray(raw);struct.pack_into('<I',tag,0xec,state_at)
        if boundary:
            orientation,fade,flags=boundary;struct.pack_into('<H',tag,0x18,orientation);struct.pack_into('<H',tag,0x68,fade);tag[0]=flags
        runtime=bytearray(72);struct.pack_into('<I',runtime,8,0xffffffff)
        for at in (0x14,0x1c,0x20):struct.pack_into('<f',runtime,at,rng.uniform(.1,2))
        struct.pack_into('<H',runtime,0x30+chain*2,count);struct.pack_into('<I',runtime,0x38+chain*4,0x12340000)
        bank=bytearray(56*count);nstates=len(states)//64
        for j in range(count):
            at=j*56;struct.pack_into('<H',bank,at,0x1234);bank[at+3]=min(j,nstates-1)
            # Interpolate only when the following authored state exists.
            bank[at+2]=2 if j<nstates-1 and j%2 else 0
            for off in (4,8,12,16,20,24):struct.pack_into('<f',bank,at+off,rng.uniform(.01,.95))
            struct.pack_into('<3f',bank,at+0x1c,1+j*.3,2+j*.2,3+j*.1)
            struct.pack_into('<3f',bank,at+0x28,0,0,1)
            struct.pack_into('<I',bank,at+0x34,(0x12340000|j+1) if j+1<count else 0xffffffff)
        poolbytes=bytearray(80);struct.pack_into('<I',poolbytes,0x44,points)
        ctx=struct.pack('<IIH2xIB3xIII',owner,definition,chain,definition+0x3c,1,count*2,0xffffffff,0)
        stack=bytearray(0x1000);struct.pack_into('<IIII',stack,0x800,0x3000000,out,0,context)
        camera=struct.pack('<3f',10,-7,4);axis=struct.pack('<3f',0,0,1)
        banks=[(owner,bytes(runtime)),(definition,bytes(tag)),(state_at,states),(points,bytes(bank)),(pool,bytes(poolbytes)),(context,ctx),(out,bytes([0xcd])*(count*2*24+64)),(0x2800000,bytes(stack)),(0x4f7210,struct.pack('<I',pool)),(0x4c65c0,camera),(0x474d10,struct.pack('<I',0x240a000)),(0x240a000,axis)]
        for at,data in banks:u.mem_write(at,data)
        gp=[0x72001000+n*0x1000 for n in range(8)];gp[4]=0x2800800
        for reg,value in zip(regs,gp):u.reg_write(reg,value)
        for n in range(8):u.reg_write(R.UC_X86_REG_XMM0+n,0)
        for reg,value in ((R.UC_X86_REG_EFLAGS,0x202),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80)):u.reg_write(reg,value)
        trace.clear();reads.clear()
        try:u.emu_start(0x42ce0,0x3000000,count=100000)
        except Exception:
            print(name,chain,count,boundary,"PC",hex(u.reg_read(R.UC_X86_REG_EIP)),"trace",[hex(p)for p in trace[-12:]])
            raise
        assert u.reg_read(R.UC_X86_REG_EIP)==0x3000000
        # Retain original immutable coefficient ranges read by this execution.
        # Globals supplied above override their overlapping original page bytes.
        globals=[]
        for page in sorted(reads):
            data=bytearray(u.mem_read(page,4096))
            for at,value in banks:
                low,high=max(at,page),min(at+len(value),page+4096)
                if low<high:data[low-page:high-page]=value[low-at:high-at]
            globals.append((page,bytes(data)))
        highbanks=[(at,data)for at,data in banks if at>=0x800000]
        rows.append(dict(name=name,chain=chain,count=count,boundary=boundary,entry_gp=gp,exit_gp=[u.reg_read(reg)for reg in regs],eflags=u.reg_read(R.UC_X86_REG_EFLAGS),exit_xmm=[f'{u.reg_read(R.UC_X86_REG_XMM0+n):032x}'for n in range(8)],instructions=trace.copy(),initial_banks=[dict(address=at,hex=data.hex())for at,data in globals+highbanks],final_banks=[dict(address=at,hex=bytes(u.mem_read(at,len(data))).hex())for at,data in highbanks if at!=0x2800000]+[dict(address=0x2800800,hex=bytes(u.mem_read(0x2800800,16)).hex())]))
    fixture=Path('tests/fixtures/contrail_geometry_tu15_original.json')
    fixture.write_text(json.dumps(dict(entry=0x42ce0,original_sha256=NEW_PIN,cases=rows),separators=(',',':'))+'\n')
    report=dict(cases=len(rows),authored_definitions=len({s[0]for s in specifications}),original_sha256=NEW_PIN,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),instruction_patches=[],replaced_services=[],scope=__doc__)
    Path('analysis/weapon-effects-20261013/live-tu15/contrail-geometry/oracle.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Whole original contrail geometry',len(rows),'cases')
if __name__=='__main__':main()
