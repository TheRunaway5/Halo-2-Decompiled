#!/usr/bin/env python3
"""Run complete original13660 to its unmodified native world-pass boundary.

The view/mode/BL/rectangle controls are declared inputs. The real cold
postprocess pointer remains null; active motion-blur histories are not proved.
All reached original children execute unchanged. No callee return is supplied.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from generate_lens_query_tu15 import QueryCode
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    assert not a.fixture.exists()and not a.report.exists()
    x=Xbe(NEW_DEFAULT);assert hashlib.sha256(x.data).hexdigest()==NEW_PIN;code=QueryCode(x)
    assert code.at(0x2bc57).op_str=='0x13660' and code.at(0x132be).op_str=='edi, 0x491e20'
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
    for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
    initial_image=bytes(u.mem_read(0x10000,0x800000));stack=0x2600000;u.mem_map(stack,0x10000)
    regs=[R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI]
    trace,pages=[],set()
    def instruction(cpu,pc,n,_):
        if pc==0x2bcb0:cpu.emu_stop();return
        assert bytes(cpu.mem_read(pc,n))==x.read(pc,n);trace.append(pc)
    def access(cpu,kind,pc,n,v,_):
        if pc<0x800000:pages.update(range(pc&~4095,(pc+n+4095)&~4095,4096))
    u.hook_add(UC_HOOK_CODE,instruction);u.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,access)
    banks,ids,cases=[],{},[]
    def ref(p,data):
        key=p,data.hex()
        if key not in ids:ids[key]=len(banks);banks.append(dict(address=p,hex=key[1]))
        return ids[key]
    for view_type in (0,1,2):
      for mode in range(9):
       for flag in (0,1):
        for split in (False,True):
            u.mem_write(0x10000,initial_image)
            def put(p,value):u.mem_write(p,struct.pack('<I',value))
            u.mem_write(0x491e22,struct.pack('<H',view_type));u.mem_write(0x491e27,bytes([flag]));put(0x4920b8,mode)
            u.mem_write(0x491e68,struct.pack('<4h',*((240,320,480,640)if split else(0,0,480,640))))
            put(0x4c686c,4 if split else 1);put(0x515ec0,0)
            raw=bytearray([0xcd])*0x8000;struct.pack_into('<I',raw,0x4000,0x2800000);u.mem_write(stack,bytes(raw))
            gp=[0x72000000+n*4096 for n in range(8)];gp[3]=flag;gp[4]=stack+0x4000
            for reg,value in zip(regs,gp):u.reg_write(reg,value)
            for n in range(8):u.reg_write(R.UC_X86_REG_XMM0+n,0)
            for reg,value in ((R.UC_X86_REG_EFLAGS,0x202),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80)):u.reg_write(reg,value)
            before=bytes(u.mem_read(0x10000,0x800000));trace.clear();pages.clear();u.emu_start(0x13660,0x2800000,count=200000)
            assert u.reg_read(R.UC_X86_REG_EIP)==0x2bcb0
            sp=u.reg_read(R.UC_X86_REG_ESP);arguments=list(struct.unpack('<6I',u.mem_read(sp+4,24)))
            initial=[(p,before[p-0x10000:p-0x10000+4096])for p in sorted(pages)]+[(stack,bytes(raw))]
            final=[(p,bytes(u.mem_read(p,len(data))))for p,data in initial]
            cases.append(dict(entry=0x13660,boundary_target=0x2bcb0,variant=f'type-{view_type}:mode-{mode}:flag-{flag}:split-{split}',entry_gp=gp,exit_gp=[u.reg_read(r)for r in regs],
                exit_xmm=[f'{u.reg_read(R.UC_X86_REG_XMM0+n):032x}'for n in range(8)],eflags=u.reg_read(R.UC_X86_REG_EFLAGS),eflags_mask=0xcc5,
                x87_status=u.reg_read(R.UC_X86_REG_FPSW),mxcsr=u.reg_read(R.UC_X86_REG_MXCSR),instructions=trace.copy(),arguments=arguments,
                initial_banks=[ref(p,b)for p,b in initial],final_banks=[ref(p,b)for p,b in final]))
    a.fixture.write_text(json.dumps(dict(entry=0x13660,original_sha256=NEW_PIN,banks=banks,cases=cases),separators=(',',':'))+'\n')
    a.report.write_text(json.dumps(dict(passed=True,cases=len(cases),original_sha256=NEW_PIN,enabled=sum(bool(c['arguments'][2]&255)for c in cases),instruction_patches=[],replaced_services=[],boundary=__doc__,full_goal_complete=False),indent=2)+'\n')
    print('PASS unchanged original lens parent gate',len(cases),'cases')
if __name__=='__main__':main()
