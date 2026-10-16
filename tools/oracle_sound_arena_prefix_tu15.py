#!/usr/bin/env python3
"""Original main12008..1201F sound arena allocation prefix, before12025.

The source12001 EBX=0 value is a declared fragment input. Only the actual
NtAllocateVirtualMemory import uses fresh native64 anonymous zeroed pages.
No main return, sound initializer or audio readiness result is supplied.
"""
import argparse,ctypes,hashlib,json,mmap,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert not a.output.exists()
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
 cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x600000);cpu.mem_map(0x2000000,0x20000);cpu.mem_map(0x80000000,0x1000)
 for section in image.sections:
  raw=image.section_bytes(section)
  if section.va+len(raw)<=0x610000:cpu.mem_write(section.va,raw)
 stack=0x2008000;gp=[0x11000000+i*0x111111 for i in range(8)];gp[3]=0;gp[4]=stack
 registers=[getattr(R,'UC_X86_REG_'+n) for n in ['EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI']]
 for r,v in zip(registers,gp):cpu.reg_write(r,v)
 cpu.reg_write(R.UC_X86_REG_EFLAGS,0x202);cpu.reg_write(R.UC_X86_REG_FPCW,0x27f);cpu.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
 executed={};instructions=[];stores=[];allocations=[];pages=[]
 def word(p):return struct.unpack('<I',cpu.mem_read(p,4))[0]
 def put(p,v):cpu.mem_write(p,struct.pack('<I',v))
 def hook(m,pc,size,_):
  if pc==0x800000b8:
   sp=m.reg_read(R.UC_X86_REG_ESP);returned,base_cell,zero,size_cell,kind,protection=struct.unpack('<6I',m.mem_read(sp,24))
   assert returned==0x33b3c5 and not word(base_cell) and not zero and word(size_cell)==0x23000 and kind==0x1000 and protection==4
   raw=mmap.mmap(-1,0x23000,flags=mmap.MAP_PRIVATE|mmap.MAP_ANONYMOUS,prot=mmap.PROT_READ|mmap.PROT_WRITE)
   native=ctypes.addressof(ctypes.c_char.from_buffer(raw));assert native>0xffffffff and raw[:]==bytes(0x23000)
   m.mem_map_ptr(0x10000000,0x23000,3,native);pages.append(raw);put(base_cell,0x10000000);put(size_cell,0x23000)
   allocations.append(dict(source=0x10000000,bytes=0x23000,native=native,allocation_type=kind,protection=protection))
   m.reg_write(R.UC_X86_REG_EAX,0);m.reg_write(R.UC_X86_REG_ESP,sp+24);m.reg_write(R.UC_X86_REG_EIP,returned);return
  raw=bytes(m.mem_read(pc,size));assert raw==image.read(pc,size);executed[f'{pc:08x}']=raw.hex();instructions.append(pc)
 def store(m,access,address,size,value,_):stores.append([m.reg_read(R.UC_X86_REG_EIP),address,size,value&((1<<(size*8))-1)])
 cpu.hook_add(UC_HOOK_CODE,hook);cpu.hook_add(UC_HOOK_MEM_WRITE,store)
 try:
  cpu.emu_start(0x12008,0x12025,count=10000)
  assert cpu.reg_read(R.UC_X86_REG_EIP)==0x12025 and cpu.reg_read(R.UC_X86_REG_ESP)==stack
  assert bytes(cpu.mem_read(0x51db00,12))==struct.pack('<3I',0x10000000,0,0)
  assert bytes(cpu.mem_read(0x10000000,0x23000))==bytes(0x23000) and len(allocations)==1
  report=dict(passed=True,scope=__doc__,original_xbe_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),entry=0x12008,end_exclusive=0x12025,input_gp=gp,returned_gp=[cpu.reg_read(r)&0xffffffff for r in registers],returned_eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS),instructions=instructions,source_stores=stores,executed_instruction_bytes=executed,allocations=allocations,globals_hex=bytes(cpu.mem_read(0x51db00,12)).hex(),raw_pages_zero=True,instruction_patches=[],supplied_game_results=[],main_or_audio_ready=False)
  a.output.mkdir(parents=True);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(f'Original sound arena prefix: {len(instructions)} instructions, {len(executed)} unchanged sites')
 finally:
  for raw in pages:cpu.mem_unmap(0x10000000,0x23000);raw.close()
if __name__=='__main__':main()
