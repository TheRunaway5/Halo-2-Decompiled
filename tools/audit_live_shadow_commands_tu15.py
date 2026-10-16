#!/usr/bin/env python3
"""Execute original40E80 after an actual native secondary shadow cull.

Only32340's literal CPU caller publications are supplied. No command, shader
LOD, geometry, resource completion or GPU return is substituted. This is a
reference command-producer probe, not a framebuffer comparison.
"""
import argparse,ctypes,hashlib,json,mmap,struct
from pathlib import Path
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_INVALID
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from differential_physics_native_frame_tu15_20261012 import regions,segment
REGS=[R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,
      R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI]
def sha(p):
    with Path(p).open('rb')as f:return hashlib.file_digest(f,'sha256').hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--capture',type=Path,required=True);p.add_argument('--owner',type=int,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--construct-transparent-storage',action='store_true',
                   help='Run original1E0F0 with declared real host virtual-memory allocation boundary')
    p.add_argument('--populate-bsp-planes',action='store_true',help='Run original BSP activation1E1A0')
    p.add_argument('--outer',nargs=4,type=int,metavar=('TOP','LEFT','BOTTOM','RIGHT'))
    p.add_argument('--local-players',type=int,choices=(1,2,3,4))
    a=p.parse_args()
    assert bool(a.outer)==bool(a.local_players)
    assert not a.output.exists() and sha(NEW_DEFAULT)==NEW_PIN;a.output.mkdir(parents=True)
    case=a.capture/'shadow-admission'/str(a.owner)/str(0x44710)
    abi=json.loads((case/'before-abi.json').read_text());assert abi['x87_depth']==0
    call=json.loads((case/'call.json').read_text())
    row=next(r for r in json.loads((a.capture/'report.json').read_text())['shadow_root_frames']
             if r['owner']==a.owner and r['tick']==call['tick'] and r['view']==call['view'])
    guest=Uc(UC_ARCH_X86,UC_MODE_32);mapped=set();banks=[]
    def mount(address,data):
        pages=[p for p in range(address&~4095,(address+len(data)+4095)&~4095,4096)if p not in mapped];begin=0
        while begin<len(pages):
            end=begin+1
            while end<len(pages)and pages[end]==pages[end-1]+4096:end+=1
            guest.mem_map(pages[begin],(end-begin)*4096);begin=end
        mapped.update(pages);guest.mem_write(address,bytes(data))
    for address,data in regions(case/'after.bin'):mount(address,data);banks.append((address,len(data)))
    u=lambda p:int.from_bytes(guest.mem_read(p,4),'little')
    assert u(0x4cd3a0)==0x5598f8 and u(0x5598f8+0x2a78)==2
    # Actual32340 instructions32889..3290A.30C80 clears bit5, already absent
    # from the literal0x14. Geometry and command storage remain captured input.
    publications={0x4c6838:b'\x01',0x4c6834:struct.pack('<I',0x14),
                  0x4d1828:struct.pack('<I',0x48c5b8),0x492250:struct.pack('<I',row['frame_words'][28])}
    for address,data in publications.items():guest.mem_write(address,data)
    # The caller's next ordinary source call has a fresh ABI; preserve the
    # actual retained return slot and platform segment/control inputs.
    abi['gp']=[0,0,0,0,json.loads((case/'before-abi.json').read_text())['gp'][4],0,0,0]
    gdt=0x7fff0000;assert gdt not in mapped;mount(gdt,bytes(4096))
    guest.mem_write(gdt+8,segment(0,0x9b)+segment(0,0x93)+segment(abi['fs_base'],0x93))
    guest.reg_write(R.UC_X86_REG_GDTR,(0,gdt,0xfff,0))
    for r,v in ((R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)):guest.reg_write(r,v)
    for r,v in zip(REGS,abi['gp']):guest.reg_write(r,v)
    for n,lanes in enumerate(abi['xmm']):guest.reg_write(R.UC_X86_REG_XMM0+n,sum(v<<(32*k)for k,v in enumerate(lanes)))
    for r,v in ((R.UC_X86_REG_EFLAGS,abi['eflags']),(R.UC_X86_REG_FPCW,abi['x87_control']),(R.UC_X86_REG_FPSW,abi['x87_status']),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,abi['mxcsr'])):guest.reg_write(r,v)
    image=Xbe(NEW_DEFAULT);trace=[];faults=[];stop=u(abi['gp'][4]);allocations=[];pages=[]
    next_source=0x72000000
    def code(cpu,pc,size,_):
        nonlocal next_source
        if a.construct_transparent_storage and pc==0x800000b8:
            sp=cpu.reg_read(R.UC_X86_REG_ESP)
            returned,base_cell,zero_bits,size_cell,kind,protection=struct.unpack('<6I',cpu.mem_read(sp,24))
            requested=u(size_cell);rounded=(requested+4095)&~4095
            assert returned==0x33b3c5 and not u(base_cell) and not zero_bits and kind==0x101000 and protection==4
            assert rounded and all(p not in mapped for p in range(next_source,next_source+rounded,4096))
            backing=mmap.mmap(-1,rounded,flags=mmap.MAP_PRIVATE|mmap.MAP_ANONYMOUS,prot=mmap.PROT_READ|mmap.PROT_WRITE)
            native=ctypes.addressof(ctypes.c_char.from_buffer(backing));assert native>0xffffffff
            cpu.mem_map_ptr(next_source,rounded,3,native);pages.append(backing)
            mapped.update(range(next_source,next_source+rounded,4096))
            cpu.mem_write(base_cell,struct.pack('<I',next_source));cpu.mem_write(size_cell,struct.pack('<I',rounded))
            allocations.append(dict(source=next_source,native=native,requested=requested,reserved=rounded))
            next_source+=rounded
            cpu.reg_write(R.UC_X86_REG_EAX,0);cpu.reg_write(R.UC_X86_REG_ESP,sp+24);cpu.reg_write(R.UC_X86_REG_EIP,returned);return
        assert bytes(cpu.mem_read(pc,size))==image.read(pc,size),hex(pc);trace.append(pc)
    def fault(cpu,access,address,size,value,_):
        faults.append(dict(pc=cpu.reg_read(R.UC_X86_REG_EIP),access=access,address=address,size=size,gp=[cpu.reg_read(r)for r in REGS]));return False
    guest.hook_add(UC_HOOK_CODE,code);guest.hook_add(UC_HOOK_MEM_INVALID,fault);error=None
    phases={}
    def run(entry):
        # A fresh actual source-call ABI. No return, command or expected
        # output from native execution is supplied to the original.
        for r,v in zip(REGS,abi['gp']):guest.reg_write(r,v)
        guest.reg_write(R.UC_X86_REG_EFLAGS,abi['eflags'])
        for r,v in ((R.UC_X86_REG_FPCW,abi['x87_control']),(R.UC_X86_REG_FPSW,abi['x87_status']),
                    (R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,abi['mxcsr'])):guest.reg_write(r,v)
        for n,lanes in enumerate(abi['xmm']):guest.reg_write(R.UC_X86_REG_XMM0+n,sum(v<<(32*k)for k,v in enumerate(lanes)))
        if entry==0x2c470:guest.reg_write(R.UC_X86_REG_EAX,a.local_players)
        first=len(trace);guest.emu_start(entry,stop,count=1000000)
        assert guest.reg_read(R.UC_X86_REG_EIP)==stop,hex(entry)
        phases[hex(entry)]=trace[first:]
    try:
        if a.construct_transparent_storage:
            assert not any(u(p)for p in (0x4c2aa8,0x4c2aac,0x52dfd0))
            assert 0x80000000 not in mapped;mount(0x80000000,b'\x0f\x0b'*2048)
            guest.emu_start(0x1e0f0,stop,count=1000000)
            assert guest.reg_read(R.UC_X86_REG_EIP)==stop and guest.reg_read(R.UC_X86_REG_EAX)&255==1
            assert all(u(p)for p in (0x4c2aa8,0x4c2aac,0x52dfd0))
            for r,v in zip(REGS,abi['gp']):guest.reg_write(r,v)
        if a.outer:
            assert a.outer[2]>a.outer[0] and a.outer[3]>a.outer[1]
            guest.mem_write(0x4c65f0,struct.pack('<4h',*a.outer))
            run(0x2c470);run(0x1be90)
        if a.populate_bsp_planes:run(0x1e1a0)
        run(0x40e80)
    except(UcError,AssertionError,RuntimeError)as e:error=str(e)
    command_banks=[]
    for address in (0x4ce280,0x4ce298):
        header=list(struct.unpack('<6I',guest.mem_read(address,24)));assert header[1]<=header[0]
        command_banks.append(dict(address=address,header=header,commands=[list(struct.unpack('<6I',guest.mem_read(header[5]+n*24,24)))for n in range(header[1])]))
    # Native command outputs are read only after original execution returns.
    # They never provide inputs to the reference producer.
    native_commands_verified='commands' in row and error is None and row['commands']==command_banks
    differences=[]
    if 'commands' in row and error is None and not native_commands_verified:
        differences.append('Native ordered command banks differ from original40E80')
    report=dict(scope=__doc__,completed=error is None,error=error,pc=guest.reg_read(R.UC_X86_REG_EIP),instructions=len(trace),faults=faults,command_banks=command_banks,
        caller_publications={hex(p):b.hex()for p,b in publications.items()},original_sha256=NEW_PIN,tool_sha256=sha(__file__),input_bank_sha256=sha(case/'after.bin'),
        instruction_patches=[],replaced_game_services=[],external_host_virtual_memory=allocations,
        declared_outer=a.outer,declared_local_players=a.local_players,bsp_planes_populated=a.populate_bsp_planes,
        shader_view_words=list(struct.unpack('<7I',guest.mem_read(0x492348,28))),
        native_commands_verified=native_commands_verified,differences=differences,
        shadow_gpu_submission=False,original_framebuffer_parity=False)
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');(a.output/'instructions.json').write_text(json.dumps(trace)+'\n')
    for entry,path in phases.items():(a.output/(entry+'-instructions.json')).write_text(json.dumps(path)+'\n')
    if error is None:
        with(a.output/'after.bin').open('wb')as file:
            for address,size in banks:file.write(struct.pack('<II',address,size));file.write(guest.mem_read(address,size))
            for allocation in allocations:
                address,size=allocation['source'],allocation['reserved']
                file.write(struct.pack('<II',address,size));file.write(guest.mem_read(address,size))
    print(json.dumps({k:report[k]for k in ('completed','error','pc','instructions','faults')}))
    assert error is None,'Original shadow command producer stopped at its retained owner boundary'
    assert not differences,differences
if __name__=='__main__':main()
