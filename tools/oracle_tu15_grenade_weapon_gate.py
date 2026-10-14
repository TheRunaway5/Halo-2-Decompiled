#!/usr/bin/env python3
"""Execute unchanged TU1.5 106510 with explicit weapon/header/tag ABI inputs.

This leaf verifies the weapon gate, not readiness production or grenade
admission. No child services or gameplay returns are supplied.
"""
import argparse, hashlib, json, struct, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent/'reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *

PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--xbe',type=Path,required=True);p.add_argument('--fixture',type=Path,required=True)
    a=p.parse_args();x=Xbe(a.xbe);assert hashlib.sha256(x.data).hexdigest()==PIN
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
    for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
    u.mem_map(0x1000000,0x10000)
    def w(p,v):u.mem_write(p,struct.pack('<I',v&0xffffffff))
    # Declared input backing, not an original object allocator result.
    w(0x4ecbd0,0x1000000);w(0x4f0414,0x1001000)
    w(0x1000044,0x1002000);w(0x1002014,0x1003000)
    w(0x1003000,0x12340002);w(0x1001028,0x1004000)
    regs=[UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,
          UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI]
    trace=[];u.hook_add(UC_HOOK_CODE,lambda uc,p,n,_:trace.append(p))
    cases=[]
    states=[-2147483648,-2,-1,0,1,4,5,6,9,10,11,12,127,255,2147483647]
    for present in (False,True):
        for flags in (0,0x40,0x80000000,0xffffffff):
            for state in states:
                w(0x100412c,flags);w(0x1003178,state);w(0x100f000,0x11000000)
                gp=[0x98765432,0x12340001 if present else 0xffffffff,0x13572468,
                    0x89abcdef,0x100f000,0x55667788,0x66778899,0x778899aa]
                for r,v in zip(regs,gp):u.reg_write(r,v)
                u.reg_write(UC_X86_REG_EFLAGS,0x202);trace.clear()
                u.emu_start(0x106510,0x11000000,count=100)
                assert u.reg_read(UC_X86_REG_EIP)==0x11000000
                assert not any(p<0x106510 or p>0x10655e for p in trace)
                cases.append(dict(present=present,flags=flags,state=state,entry_gp=gp,
                    exit_gp=[u.reg_read(r) for r in regs],eflags=u.reg_read(UC_X86_REG_EFLAGS),instructions=trace.copy()))
    result=dict(scope=__doc__,image_sha256=PIN,entry=0x106510,
        code_hex=x.read(0x106510,0x4f).hex(),cases=cases)
    a.fixture.parent.mkdir(parents=True,exist_ok=True)
    a.fixture.write_text(json.dumps(result,indent=2)+'\n')
    print('PASS unchanged TU1.5 grenade weapon gate:',len(cases),'cases')
if __name__=='__main__':main()
