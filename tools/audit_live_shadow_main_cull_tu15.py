#!/usr/bin/env python3
"""Run unchanged TU1.5 ordinary44710 on the actual collected native main bank.

No geometry, mask, culling return or resource completion is supplied. Optional
kernel synchronization uses actual captured native ownership, declared separately
from original Xbox OS proof. This reference probe does not claim native culling
or submitted/matched shadow graphics.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_INVALID
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from differential_physics_native_frame_tu15_20261012 import regions,segment
from shadow_native_synchronization_adapter import NativeShadowSynchronization
REGS=[R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,
      R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI]


def sha(p):
    with Path(p).open('rb')as f:return hashlib.file_digest(f,'sha256').hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--synchronization-library',type=Path)
    a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    a.output.mkdir(parents=True);snapshot=a.capture/'shadow-selection'
    abi=json.loads((snapshot/'before-abi.json').read_text());assert abi['x87_depth']==0
    guest=Uc(UC_ARCH_X86,UC_MODE_32);mapped=set();banks=[]
    def mount(address,data):
        pages=[p for p in range(address&~4095,(address+len(data)+4095)&~4095,4096)if p not in mapped]
        begin=0
        while begin<len(pages):
            end=begin+1
            while end<len(pages)and pages[end]==pages[end-1]+4096:end+=1
            guest.mem_map(pages[begin],(end-begin)*4096);begin=end
        mapped.update(pages);guest.mem_write(address,bytes(data))
    for address,data in regions(snapshot/'before.bin'):mount(address,data);banks.append((address,len(data)))
    gdt=0x7fff0000;assert gdt not in mapped;mount(gdt,bytes(4096))
    guest.mem_write(gdt+8,segment(0,0x9b)+segment(0,0x93)+segment(abi['fs_base'],0x93))
    guest.reg_write(R.UC_X86_REG_GDTR,(0,gdt,0xfff,0))
    for r,v in ((R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)):guest.reg_write(r,v)
    for r,v in zip(REGS,abi['gp']):guest.reg_write(r,v)
    for n,lanes in enumerate(abi['xmm']):guest.reg_write(R.UC_X86_REG_XMM0+n,sum(v<<(32*k)for k,v in enumerate(lanes)))
    for r,v in ((R.UC_X86_REG_EFLAGS,abi['eflags']),(R.UC_X86_REG_FPCW,abi['x87_control']),
                (R.UC_X86_REG_FPSW,abi['x87_status']),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,abi['mxcsr'])):guest.reg_write(r,v)
    u=lambda p:int.from_bytes(guest.mem_read(p,4),'little');context=0x556e18
    assert u(0x4cd3a0)==context and u(context+0x2a78)==0 and u(context+0x2a7c)==0
    prefix=u(0x4cd39c);assert 0<prefix==u(0x4cd398)<=850
    before=[list(struct.unpack('<8I',guest.mem_read(0x4c6958+n*32,32)))for n in range(prefix)]
    before_masks=list(struct.unpack('<512I',guest.mem_read(context+0x25c,2048)))
    stop=u(abi['gp'][4]);image=Xbe(NEW_DEFAULT);trace=[];synchronization=None
    if a.synchronization_library:
        synchronization=NativeShadowSynchronization(snapshot/'native-synchronization.json',a.synchronization_library)
        assert 0x80000000 not in mapped;mount(0x80000000,b'\x0f\x0b'*2048)
    def code(cpu,pc,size,_):
        if synchronization and 0x80000000<=pc<0x80001000:synchronization.kernel(cpu,pc);return
        assert bytes(cpu.mem_read(pc,size))==image.read(pc,size),hex(pc)
        trace.append(pc)
    faults=[]
    def invalid(cpu,access,address,size,value,_):
        faults.append(dict(pc=cpu.reg_read(R.UC_X86_REG_EIP),access=access,address=address,size=size,
            gp=[cpu.reg_read(r)for r in REGS]));return False
    guest.hook_add(UC_HOOK_MEM_INVALID,invalid)
    guest.hook_add(UC_HOOK_CODE,code);error=None
    try:
        guest.emu_start(0x44710,stop,count=10000000)
        assert guest.reg_read(R.UC_X86_REG_EIP)==stop,'Original main culler did not return'
    except (UcError,AssertionError,RuntimeError)as e:error=str(e)
    after=[list(struct.unpack('<8I',guest.mem_read(0x4c6958+n*32,32)))for n in range(prefix)]
    masks=list(struct.unpack('<512I',guest.mem_read(context+0x25c,2048)))
    platform=[]if synchronization is None else synchronization.events
    if synchronization:synchronization.close()
    report=dict(scope=__doc__,completed=error is None,error=error,original_pc=guest.reg_read(R.UC_X86_REG_EIP),
        entry=0x44710,main_prefix=prefix,instructions=len(trace),faults=faults,units_before=before,units_after=after,
        masks_before=before_masks,masks_after=masks,changed_units=[n for n,(x,y)in enumerate(zip(before,after))if x!=y],
        admitted_units=[n for n,u in enumerate(after)if u[0]&1],platform_calls=platform,original_sha256=NEW_PIN,tool_sha256=sha(__file__),input_bank_sha256=sha(snapshot/'before.bin'),
        instruction_patches=[],replaced_game_services=[],native_culling_verified=False,shadow_gpu_submission=False,original_framebuffer_parity=False)
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');(a.output/'instructions.json').write_text(json.dumps(trace)+'\n')
    if error is None:
        with(a.output/'after.bin').open('wb')as f:
            for address,size in banks:f.write(struct.pack('<II',address,size));f.write(guest.mem_read(address,size))
    print(json.dumps({k:report[k]for k in ('completed','error','original_pc','main_prefix','instructions','changed_units')}))
    assert error is None,'Original ordinary main culling stopped at the retained boundary'


if __name__=='__main__':main()
