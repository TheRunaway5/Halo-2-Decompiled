#!/usr/bin/env python3
"""Original codec constructor prefix and bounded command helpers.

Reference-only physical codec inputs are explicit, never runtime mounts.
Native ABI comparison excludes only hardware-only38ED2C/38ECFF instructions,
their physical reads/writes and private stack scratch. All retained callers,
action stores, result/branch logic, GP and defined flags are compared.
"""
import argparse,ctypes,hashlib,json,mmap,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_sound_sdk_native_prefix_tu15 import GP,FS,GDT,POOL,STOP,segment
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
STACK=0x2018000
def run(image,storage,fill,entry,executed,ready=True):
    cpu=Uc(UC_ARCH_X86,UC_MODE_32)
    for address,size in [(0x10000,0x600000),(0x2000000,0x20000),(STOP,4096),(0x80000000,4096),(FS,4096),(GDT,4096),(0xfec00000,4096)]:cpu.mem_map(address,size)
    for section in image.sections:
        raw=image.section_bytes(section)
        if section.va+len(raw)<=0x610000:cpu.mem_write(section.va,raw)
    for row in storage['regions']:cpu.mem_write(row['address'],bytes.fromhex(row['hex']))
    cpu.mem_write(0x2010000,bytes([fill])*0x10000)
    cpu.mem_write(GDT+8,segment(0,0x9b)+segment(0,0x93)+segment(FS,0x93));cpu.reg_write(R.UC_X86_REG_GDTR,(0,GDT,4095,0))
    for reg,value in [(R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)]:cpu.reg_write(reg,value)
    mappings=[]
    for source,capacity,contents in [(POOL,0x10000,None)]+[(r['source'],r['capacity'],bytes.fromhex(r['hex']))for r in storage['dma_allocations']]:
        backing=mmap.mmap(-1,capacity,flags=mmap.MAP_PRIVATE|mmap.MAP_ANONYMOUS,prot=mmap.PROT_READ|mmap.PROT_WRITE)
        if contents:backing[:]=contents
        native=ctypes.addressof(ctypes.c_char.from_buffer(backing));assert native>0xffffffff;cpu.mem_map_ptr(source,capacity,3,native);mappings.append((source,capacity,backing))
    for row in storage['allocations']:cpu.mem_write(row['source'],bytes.fromhex(row['hex']))
    controller=storage['frontier']['source_subobject']+0x14;assert bytes(cpu.mem_read(controller,0x3c))==bytes(0x3c)
    gp=[0x21000000+i*0x111111 for i in range(8)];gp[1]=controller-0x14 if entry==0x38e484 else controller;gp[4]=STACK
    for reg,value in zip(GP,gp):cpu.reg_write(reg,value)
    for reg,value in [(R.UC_X86_REG_EFLAGS,0x202),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80)]:cpu.reg_write(reg,value)
    # Offline representative ready physical codec inputs. Native runtime
    # instead creates actual converter resources and a serialized command.
    cpu.mem_write(0xfec0012c,struct.pack('<I',0));cpu.mem_write(0xfec00130,struct.pack('<I',0x100 if ready else 0));cpu.mem_write(0xfec00134,b'\0')
    code=_Code(image,'DSOUND');leaves=set().union(*(_trace(code,e,{e}).code for e in [0x38ed2c,0x38ecff]))
    instructions=[];stores=[];scratch=set();leaf_instructions=[];frontier={};stalls=[]
    def hook(m,pc,size,_):
        if pc==0x80000097:
            sp=m.reg_read(R.UC_X86_REG_ESP);returned,delay=struct.unpack('<2I',m.mem_read(sp,8));assert not ready and returned==0x38ed6a and delay==20
            stalls.append(dict(returned=returned,microseconds=delay));m.reg_write(R.UC_X86_REG_ESP,sp+8);m.reg_write(R.UC_X86_REG_EIP,returned);return
        if pc==0x8000002c:
            sp=m.reg_read(R.UC_X86_REG_ESP);assert struct.unpack('<I',m.mem_read(sp,4))[0]==0x38f4ca
            assert struct.unpack('<I',m.mem_read(sp+4,4))[0]==6
            frontier.update(target=pc,instruction=0x38f4c4,gp=[m.reg_read(r)for r in GP],eflags=m.reg_read(R.UC_X86_REG_EFLAGS));m.emu_stop();return
        assert image.read(pc,size)==bytes(m.mem_read(pc,size));executed[f'{pc:08x}']=image.read(pc,size).hex()
        (leaf_instructions if pc in leaves else instructions).append(pc)
    def store(m,kind,address,size,value,_):
        pc=m.reg_read(R.UC_X86_REG_EIP)
        if pc in leaves:
            if address<0xfec00000:scratch.update(range(address,address+size))
        else:stores.append([pc,address,size,value&((1<<(8*size))-1)])
    cpu.hook_add(UC_HOOK_CODE,hook);cpu.hook_add(UC_HOOK_MEM_WRITE,store)
    try:
        cpu.mem_write(STACK,struct.pack('<2I',STOP,1));cpu.emu_start(entry,STOP,count=50000);assert bool(frontier)==ready
        prefix=dict(instructions=instructions.copy(),source_stores=stores.copy(),frontier=frontier,stack_hex=bytes(cpu.mem_read(0x2010000,0x10000)).hex(),controller_hex=bytes(cpu.mem_read(controller,0x3c)).hex(),hardware_private_stack_bytes=sorted(scratch),hardware_leaf_instructions=leaf_instructions.copy())
        if not ready:
            assert cpu.reg_read(R.UC_X86_REG_EIP)==STOP and cpu.reg_read(R.UC_X86_REG_EAX)==0x88780078 and len(stalls)==1000
            prefix.update(returned_gp=[cpu.reg_read(r)for r in GP],returned_eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS),offline_stall_calls=len(stalls))
        components=[]
        for reg,value,returned in ([(1,0,0x38f503),(0xc,0x808,0x38f519)]if ready else[]):
            instructions.clear();stores.clear();scratch.clear();leaf_instructions.clear()
            cpu.mem_write(0x201b000,struct.pack('<3I',returned,reg,value));cpu.reg_write(R.UC_X86_REG_ESP,0x201b000)
            initial=[cpu.reg_read(r)for r in GP];flags=cpu.reg_read(R.UC_X86_REG_EFLAGS)
            cpu.emu_start(0x38ed90,returned,count=1000);assert cpu.reg_read(R.UC_X86_REG_EIP)==returned
            components.append(dict(index=reg,value=value,returned=returned,input_gp=initial,input_eflags=flags,instructions=instructions.copy(),source_stores=stores.copy(),returned_gp=[cpu.reg_read(r)for r in GP],returned_eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS),stack_hex=bytes(cpu.mem_read(0x201b000,12)).hex(),hardware_private_stack_bytes=sorted(scratch),hardware_leaf_instructions=leaf_instructions.copy()))
        return dict(fill=fill,entry=entry,controller=controller,initial_gp=gp,native_route_abi_result=1 if ready else 0,prefix=prefix,components=components,retained_raw_hex=[dict(source=r['source'],hex=bytes(cpu.mem_read(r['source'],r['bytes'])).hex())for r in storage['allocations']],retained_dma_hex=[dict(source=r['source'],hex=bytes(cpu.mem_read(r['source'],r['capacity'])).hex())for r in storage['dma_allocations']])
    finally:
        for address,capacity,backing in reversed(mappings):cpu.mem_unmap(address,capacity);backing.close()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--storage-reference',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert not a.output.exists()
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    storage=json.loads(a.storage_reference.read_text());assert storage['passed']and storage['original_xbe_sha256']==NEW_PIN
    baseline=storage['cases'][0];assert baseline['through_storage']and len(baseline['dma_allocations'])==16
    assert image.read(0x390b94,8)==struct.pack('<2I',0x10,0x70)
    imports=[(0x41b70c,0x8000006b),(0x41b7a8,0x8000002c),(0x41b7b4,0x8000006d),(0x41b7b0,0x80000062),(0x41b670,0x8000002f)]
    for address,value in imports:assert image.read(address,4)==struct.pack('<I',value)
    executed={};cases=[run(image,baseline,fill,entry,executed)for fill,entry in [(0,0x38e484),(0x5a,0x38f45f)]]
    failure_cases=[run(image,baseline,fill,entry,executed,False)for fill,entry in [(0x5a,0x38e484),(0xa5,0x38f45f)]]
    a.output.mkdir(parents=True);report=dict(passed=True,original_xbe_sha256=NEW_PIN,storage_reference_sha256=hashlib.sha256(a.storage_reference.read_bytes()).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),cases=cases,failure_result_cases=failure_cases,executed_instruction_bytes=executed,instruction_patches=[],runtime_register_bank=False,native_abi_leaf_entries=[0x38ed2c,0x38ecff],offline_physical_inputs={"fec0012c":0,"fec00130":256,"fec00134":0},failure_case_offline_pcr=0,failure_native_abi_result_input=0,actual_native_allocation_failure_forced=False,whole_successful_constructor_returned=False,irq6_result_supplied=False,source_codec_ready=False,source_channel_ready=False,producer_ready=False,full_startup_completed=False)
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(f'{len(cases)} original codec constructor prefixes and4 command components; IRQ6 remains required')
if __name__=='__main__':main()
