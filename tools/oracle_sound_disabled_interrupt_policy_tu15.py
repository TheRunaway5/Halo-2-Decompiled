#!/usr/bin/env python3
"""Authentic original one-store IEN prefix and global-disable helper.

The offline destination is an explicitly declared hardware reference fixture,
never a runtime MMIO bank or proof of missing interrupt producers.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R

STACK,STOP,APU,FS=0x2008000,0x2100000,0x19000000,0x2001000
GP=[getattr(R,'UC_X86_REG_'+s) for s in ['EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI']]

def run(image,case,helper):
    cpu=Uc(UC_ARCH_X86,UC_MODE_32)
    for base,size in [(0x10000,0x600000),(0x2000000,0x20000),(STOP,4096),(0x2200000,4096),
                      (APU,0x1000),(0x80000000,4096),(0xfe800000,0x60000)]:cpu.mem_map(base,size)
    for section in image.sections:
        raw=image.section_bytes(section)
        if section.va+len(raw)<=0x610000:cpu.mem_write(section.va,raw)
    fill=case['case']['fill'];cpu.mem_write(0x2000000,bytes([fill])*0x20000)
    def put(p,v):cpu.mem_write(p,struct.pack('<I',v&0xffffffff))
    def word(p):return struct.unpack('<I',cpu.mem_read(p,4))[0]
    def segment(base,access):return struct.pack('<Q',(base&65535)<<16|(base&0xff0000)<<16|(base&0xff000000)<<32|65535|0xf<<48|access<<40|0xc<<52)
    cpu.mem_write(0x2200008,segment(0,0x9b)+segment(0,0x93)+segment(FS,0x93))
    cpu.reg_write(R.UC_X86_REG_GDTR,(0,0x2200000,4095,0))
    for r,v in [(R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),
                (R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)]:cpu.reg_write(r,v)
    initial=case['reset']['constructor_input_gp'].copy();initial[1]=APU+8;initial[4]=STACK
    for r,v in zip(GP,initial):cpu.reg_write(r,v)
    for r,v in [(R.UC_X86_REG_EFLAGS,0x202),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_MXCSR,0x1f80)]:cpu.reg_write(r,v)
    put(FS+0x24,0);put(STACK,STOP);put(STACK+4,0)
    put(0xfe801004,0xd8 if helper else 0)
    instructions=[];stores=[];original={};level=0
    def hook(m,pc,size,_):
        nonlocal level
        if not helper and pc==0x38e8d3:m.emu_stop();return
        if pc in [0x800000a0,0x800000a1]:
            sp=m.reg_read(R.UC_X86_REG_ESP);ret=word(sp)
            if pc==0x800000a0:
                assert ret==0x383d8e and level==0 and m.reg_read(R.UC_X86_REG_ECX)&255==2
                level=2;old=m.reg_read(R.UC_X86_REG_EAX);m.reg_write(R.UC_X86_REG_EAX,old&0xffffff00)
            else:assert ret==0x383da3 and level==2 and m.reg_read(R.UC_X86_REG_ECX)&255==0;level=0
            put(FS+0x24,level);m.reg_write(R.UC_X86_REG_ESP,sp+4);m.reg_write(R.UC_X86_REG_EIP,ret);return
        raw=bytes(m.mem_read(pc,size));assert raw==image.read(pc,size)
        original[f'{pc:08x}']=raw.hex();instructions.append(pc)
    def store(m,access,p,n,value,_):stores.append([m.reg_read(R.UC_X86_REG_EIP),p,n,value&((1<<(8*n))-1)])
    cpu.hook_add(UC_HOOK_CODE,hook);cpu.hook_add(UC_HOOK_MEM_WRITE,store)
    cpu.emu_start(0x38e53d if helper else 0x38e8c9,STOP,count=10000)
    assert level==0 and word(0xfe801004)==0xd8
    assert cpu.reg_read(R.UC_X86_REG_EIP)==(STOP if helper else 0x38e8d3)
    return dict(fill=fill,initial_gp=initial,instructions=instructions,stores=stores,
        returned_gp=[cpu.reg_read(r)&0xffffffff for r in GP],returned_eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS),
        stack_hex=bytes(cpu.mem_read(STACK-0x100,0x200)).hex(),original_instruction_bytes=original)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--registered-reference',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    registered=json.loads(a.registered_reference.read_text());cases=[]
    for case in registered['cases']:cases.append(dict(fill=case['case']['fill'],initial_prefix=run(image,case,False),disable_helper=run(image,case,True)))
    report=dict(passed=True,original_xbe_sha256=NEW_PIN,instruction_patches=[],cases=cases,
        disabled_pending_mask=0xd8,unowned_next_pc=0x38e8d3,missing_routes_or_global_enable_implemented=False)
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(cases=len(cases),prefix_instructions=len(cases[0]['initial_prefix']['instructions']),helper_instructions=len(cases[0]['disable_helper']['instructions']))))
if __name__=='__main__':main()
