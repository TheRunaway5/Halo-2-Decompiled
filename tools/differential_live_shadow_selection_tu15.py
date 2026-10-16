#!/usr/bin/env python3
"""Execute unchanged TU1.5 shadow selection or secondary admission on live native inputs.

No instruction, callee, fade, priority, clock, selected root or history result is
replaced. Native after-state is comparison data only. This verifies CPU inputs
and continuation, not submitted shadow graphics or matched framebuffer parity.
"""
import argparse, hashlib, json, struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from differential_physics_native_frame_tu15_20261012 import regions, segment


def sha(path):
    with Path(path).open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--case',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    call=json.loads((a.case/'call.json').read_text());assert call['entry'] in (0x333f0,0x134c00,0x168900,0x44710,0x40e80,0x2c470,0x1be90,0x1e1a0,0x22690,0x3baa0) and call['authority']=='OG Xbox TU1.5'
    before=json.loads((a.case/'before-abi.json').read_text());after=json.loads((a.case/'after-abi.json').read_text())
    guest=Uc(UC_ARCH_X86,UC_MODE_32);mapped=set();initial=[]
    def mount(address,data):
        pages=[p for p in range(address&~4095,(address+len(data)+4095)&~4095,4096)if p not in mapped]
        start=0
        while start<len(pages):
            end=start+1
            while end<len(pages) and pages[end]==pages[end-1]+4096:end+=1
            guest.mem_map(pages[start],(end-start)*4096);start=end
        mapped.update(pages);guest.mem_write(address,data)
    for address,data in regions(a.case/'before.bin'):
        mount(address,data);initial.append((address,len(data)))
    # Reference CPU segment adapter only. It provides the captured actual FS
    # base without supplying any game return or changing retained input bytes.
    gdt=0x7fff0000;assert gdt not in mapped
    mount(gdt,bytes(4096));guest.mem_write(gdt+8,segment(0,0x9b)+segment(0,0x93)+segment(before['fs_base'],0x93))
    guest.reg_write(R.UC_X86_REG_GDTR,(0,gdt,0xfff,0))
    for register,value in ((R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)):
        guest.reg_write(register,value)
    assert guest.reg_read(R.UC_X86_REG_FS_BASE)==before['fs_base']
    regs=[R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI]
    for register,value in zip(regs,before['gp']):guest.reg_write(register,value)
    for n,lanes in enumerate(before['xmm']):guest.reg_write(R.UC_X86_REG_XMM0+n,sum(value<<(32*k)for k,value in enumerate(lanes)))
    assert before['x87_depth']==0
    for register,value in ((R.UC_X86_REG_EFLAGS,before['eflags']),(R.UC_X86_REG_FPCW,before['x87_control']),(R.UC_X86_REG_FPSW,before['x87_status']),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,before['mxcsr'])):
        guest.reg_write(register,value)
    original=Xbe(NEW_DEFAULT);trace=[]
    def code(cpu,pc,size,_):
        assert bytes(cpu.mem_read(pc,size))==original.read(pc,size),hex(pc)
        trace.append(pc)
    guest.hook_add(UC_HOOK_CODE,code)
    stop=int.from_bytes(guest.mem_read(before['gp'][4],4),'little')
    guest.emu_start(call['entry'],stop,count=5000000)
    assert guest.reg_read(R.UC_X86_REG_EIP)==stop,'Original selection did not return'
    gp=[guest.reg_read(r)for r in regs]
    xmm=[[(guest.reg_read(R.UC_X86_REG_XMM0+n)>>(32*k))&0xffffffff for k in range(4)]for n in range(8)]
    differences=[];compared=0
    for address,expected in regions(a.case/'after.bin'):
        actual=bytes(guest.mem_read(address,len(expected)));compared+=len(expected)
        if actual!=expected:
            mismatch=next(n for n,(x,y)in enumerate(zip(actual,expected))if x!=y)
            differences.append(dict(address=address+mismatch,native=expected[mismatch],original=actual[mismatch],region=address))
    state=dict(gp=gp,xmm=xmm,eflags=guest.reg_read(R.UC_X86_REG_EFLAGS),x87_status=guest.reg_read(R.UC_X86_REG_FPSW),
               mxcsr=guest.reg_read(R.UC_X86_REG_MXCSR),x87_tag=guest.reg_read(R.UC_X86_REG_FPTAG))
    checks=dict(gp=gp==after['gp'],xmm=xmm==after['xmm'],defined_flags=(state['eflags']&0xcd5)==(after['eflags']&0xcd5),
                x87_status=state['x87_status']==after['x87_status'],mxcsr=state['mxcsr']==after['mxcsr'],
                retained_memory=not differences)
    report=dict(scope=__doc__,checks=checks,passed=all(checks.values()),compared_bytes=compared,
                instructions=len(trace),unique_instructions=len(set(trace)),native_after_abi=after,original_after_abi=state,
                memory_differences=differences[:20],original_sha256=NEW_PIN,tool_sha256=sha(__file__),
                before_sha256=sha(a.case/'before.bin'),native_after_sha256=sha(a.case/'after.bin'),
                instruction_patches=[],replaced_services=[],native_expected_used_as_original_input=False,
                live_shadow_graphics=False,original_framebuffer_parity=False,full_goal_complete=False)
    a.output.mkdir(parents=True)
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    (a.output/'instructions.json').write_text(json.dumps(trace)+'\n')
    print(json.dumps({k:report[k]for k in ('passed','checks','instructions','compared_bytes')}))
    assert report['passed'],'Native shadow selection differs from unchanged original; retained report contains exact failures'


if __name__=='__main__':main()
