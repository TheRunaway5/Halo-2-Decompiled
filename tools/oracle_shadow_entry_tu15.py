#!/usr/bin/env python3
"""Execute unchanged TU1.5 shadow entry gates over declared input controls.

Includes complete disabled335D0 calls and standalone positive/negative gates.
No original instruction, callee or return is replaced. These are CPU admission
controls, not live caster selection, admitted GPU frames or framebuffer parity.
"""
import argparse, hashlib, itertools, json, struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--fixture',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True)
    a=p.parse_args();assert not a.fixture.exists() and not a.report.exists()
    x=Xbe(NEW_DEFAULT);assert hashlib.sha256(x.data).hexdigest()==NEW_PIN
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
    for section in x.sections:u.mem_write(section.va,x.section_bytes(section))
    u.mem_map(0x2400000,0x300000);stop=0x2800000;u.mem_map(stop,4096)
    regs=[R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI]
    trace=[]
    def observe(cpu,pc,size,_):
        assert bytes(cpu.mem_read(pc,size))==x.read(pc,size)
        trace.append(pc)
    u.hook_add(UC_HOOK_CODE,observe)
    cases=[];banks=[];ids={}
    def retain(address,data):
        key=(address,data)
        if key not in ids:ids[key]=len(banks);banks.append(dict(address=address,hex=data.hex()))
        return ids[key]
    controls=[]
    for present,count,bsp,bitmap,match in itertools.product((False,True),(-1,0,1,3),(False,True),(False,True),(False,True)):
        valid=present and count>0 and bsp and bitmap and match
        controls.append((0x226b0,present,count,bsp,bitmap,match,True,1,0))
        if not valid:controls.append((0x335d0,present,count,bsp,bitmap,match,True,1,0))
    for present,mode,allowed in itertools.product((False,True),(0,1,255),(0,1,255)):
        controls.append((0x335b0,True,1,True,True,True,present,mode,allowed))
        if present and mode and not allowed:
            controls.append((0x335d0,True,1,True,True,True,present,mode,allowed))
    for ordinal,(entry,present,count,bsp,bitmap,match,cinematic,mode,allowed) in enumerate(controls):
        lightmap,group,structure,cin,stack=0x2400000,0x2401000,0x2402000,0x2403000,0x2600000
        root=bytearray(0x100);struct.pack_into('<iI',root,0x80,count,group)
        record=bytearray(0x40);struct.pack_into('<I',record,4,0x12345678);struct.pack_into('<I',record,0x1c,1 if bitmap else 0xffffffff)
        sbsp=bytearray(0x40);struct.pack_into('<I',sbsp,8,0x12345678 if match else 0x12345679)
        cinematic_bank=bytearray(16);cinematic_bank[5]=mode
        rawstack=bytearray([0xcd])*0x8000;struct.pack_into('<II',rawstack,0x4000,stop,0)
        initial=[(0x4ecc14,struct.pack('<II',lightmap if present else 0,structure if bsp else 0)),
                 (0x51df50,struct.pack('<I',cin if cinematic else 0)),(0x515ef5,bytes([allowed])),
                 (lightmap,bytes(root)),(group,bytes(record)),(structure,bytes(sbsp)),(cin,bytes(cinematic_bank)),(stack,bytes(rawstack))]
        for address,data in initial:u.mem_write(address,data)
        gp=[0x72000000+n*0x1000+ordinal*16 for n in range(8)];gp[4]=stack+0x4000
        for register,value in zip(regs,gp):u.reg_write(register,value)
        for n in range(8):u.reg_write(R.UC_X86_REG_XMM0+n,0)
        for register,value in [(R.UC_X86_REG_EFLAGS,0x202),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80)]:u.reg_write(register,value)
        trace.clear();u.emu_start(entry,stop,count=10000);assert u.reg_read(R.UC_X86_REG_EIP)==stop
        cases.append(dict(entry=entry,entry_gp=gp,exit_gp=[u.reg_read(r)for r in regs],eflags=u.reg_read(R.UC_X86_REG_EFLAGS),
                          exit_xmm=[f'{u.reg_read(R.UC_X86_REG_XMM0+n):032x}'for n in range(8)],x87_status=u.reg_read(R.UC_X86_REG_FPSW),mxcsr=u.reg_read(R.UC_X86_REG_MXCSR),
                          instructions=trace.copy(),initial_banks=[retain(address,data)for address,data in initial],
                          final_banks=[retain(address,bytes(u.mem_read(address,len(data))))for address,data in initial]))
    a.fixture.parent.mkdir(parents=True,exist_ok=True);a.report.parent.mkdir(parents=True,exist_ok=True)
    a.fixture.write_text(json.dumps(dict(entry=0x335d0,original_sha256=NEW_PIN,banks=banks,cases=cases),separators=(',',':'))+'\n')
    report=dict(scope=__doc__,cases=len(cases),entries=sorted({c['entry']for c in cases}),instruction_patches=[],replaced_services=[],
                original_sha256=NEW_PIN,fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                live_shadow_activation=False,original_framebuffer_parity=False,full_goal_complete=False)
    a.report.write_text(json.dumps(report,indent=2)+'\n');print('PASS unchanged TU1.5 shadow admission',len(cases),'cases')


if __name__=='__main__':main()
