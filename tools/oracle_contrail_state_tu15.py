#!/usr/bin/env python3
"""Original52D30 CPU state with explicitly supplied native GPU service returns.

142D0 executes unchanged. SDK submissions and bitmap/vertex resource binding
are declared external boundaries; this does not validate those GPU services.
"""
import hashlib,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def main():
    x=Xbe(NEW_DEFAULT);assert hashlib.sha256(x.data).hexdigest()==NEW_PIN
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
    for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
    u.mem_map(0x2400000,0x10000);u.mem_map(0x2800000,0x10000);u.mem_map(0x3000000,4096)
    # Verified native submission ABI: each boundary returns without clobbering
    # integer/SSE registers. Source CPU state stores remain original instructions.
    services={0x143a0:4,0x3ff4b0:4,0x3ffdd0:0,0x3ff670:4,0x3ff5d0:4,0x3ffa90:4,0x1c570:4,0x401410:4,0x15160:0}
    calls=[]
    def code(g,p,n,_):
        if p not in services:return
        esp=g.reg_read(R.UC_X86_REG_ESP);ret=struct.unpack('<I',bytes(g.mem_read(esp,4)))[0]
        calls.append(dict(target=p,ecx=g.reg_read(R.UC_X86_REG_ECX),edx=g.reg_read(R.UC_X86_REG_EDX)))
        g.reg_write(R.UC_X86_REG_ESP,esp+4+services[p]);g.reg_write(R.UC_X86_REG_EIP,ret)
    u.hook_add(UC_HOOK_CODE,code);rows=[]
    for blend in range(11):
        for flags in range(8):
            for properties in (0,2):
                for declaration in (0,4):
                    data=bytearray(0x1000);struct.pack_into('<H',data,0x24,1);data[0x28]=properties
                    struct.pack_into('<H',data,0x2a,blend);data[0x2e]=flags
                    struct.pack_into('<I',data,0x5c,0x2400900);struct.pack_into('<I',data,0x924,0x12340000)
                    u.mem_write(0x2400000,bytes(data));u.mem_write(0x4922a0,struct.pack('<I',0x2400000))
                    u.mem_write(0x4c4cb4,struct.pack('<f',1));u.mem_write(0x40f0c8,bytes(4));u.mem_write(0x491788,bytes(240))
                    esp=0x280f000;u.mem_write(esp,struct.pack('<IIII',0x3000000,0x2400000,0x12340001,declaration))
                    for reg in (R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI):u.reg_write(reg,0)
                    u.reg_write(R.UC_X86_REG_ESP,esp)
                    for n in range(8):u.reg_write(R.UC_X86_REG_XMM0+n,0)
                    for reg,value in ((R.UC_X86_REG_EFLAGS,0x202),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80)):u.reg_write(reg,value)
                    calls.clear();u.emu_start(0x52d30,0x3000000,count=10000)
                    assert u.reg_read(R.UC_X86_REG_EIP)==0x3000000 and u.reg_read(R.UC_X86_REG_ESP)==esp+16
                    rows.append(dict(blend=blend,flags=flags,properties=properties,declaration=declaration,pixel=list(struct.unpack('<60I',u.mem_read(0x491788,240))),render={f'{p:08x}':struct.unpack('<I',u.mem_read(p,4))[0]for p in (0x4c4b00,0x4c4b08,0x4c4b0c,0x4c4b14,0x4c4b18,0x4c4b28,0x4c4b44)},services=calls.copy()))
    fixture=Path('tests/fixtures/contrail_state_tu15_original.json')
    fixture.write_text(json.dumps(dict(original_sha256=NEW_PIN,scope=__doc__,supplied_services={f'{k:08x}':v for k,v in services.items()},instruction_patches=[],blend_tables=[list(struct.unpack('<11I',x.read(p,44)))for p in (0x45c438,0x45c408,0x45c3d8)],cases=rows),separators=(',',':'))+'\n')
    print('Original contrail CPU state',len(rows),'cases; native GPU service boundaries explicit')
if __name__=='__main__':main()
