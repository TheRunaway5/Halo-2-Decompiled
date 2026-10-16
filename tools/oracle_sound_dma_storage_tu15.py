#!/usr/bin/env python3
"""Whole unchanged sound38E71C/3879CB leaves with genuine native64 pages.

The source helper is independently invoked with declared descriptor inputs;
this does not bypass its earlier MCPX reset/DSP prerequisites in live startup.
Only contiguous allocation, actual capacity and logical physical address are
native boundaries. Source code clears every byte and updates real statistics.
"""
import argparse,ctypes,hashlib,json,mmap,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R

def run(image,size,alignment,executed):
 cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x600000);cpu.mem_map(0x2000000,0x20000);cpu.mem_map(0x2100000,4096);cpu.mem_map(0x80000000,4096)
 for section in image.sections:
  raw=image.section_bytes(section)
  if section.va+len(raw)<=0x610000:cpu.mem_write(section.va,raw)
 stack,descriptor,stop=0x2008000,0x2001000,0x2100000
 cpu.mem_write(descriptor,struct.pack('<3I',0,0,size));cpu.mem_write(stack,struct.pack('<4I',stop,descriptor,alignment,4))
 regs=[getattr(R,'UC_X86_REG_'+n) for n in ['EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI']];gp=[0x11000000+i*0x111111 for i in range(8)];gp[4]=stack
 for r,v in zip(regs,gp):cpu.reg_write(r,v)
 cpu.reg_write(R.UC_X86_REG_EFLAGS,0x202);cpu.reg_write(R.UC_X86_REG_FPCW,0x27f);cpu.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
 pages=[];boundaries=[];instructions=[];stores=[]
 def word(p):return struct.unpack('<I',cpu.mem_read(p,4))[0]
 def hook(m,pc,count,_):
  sp=m.reg_read(R.UC_X86_REG_ESP)
  if pc in [0x800000a6,0x800000b4,0x800000ad]:
   if pc==0x800000a6:
    returned,requested,lowest,highest,align,protection=struct.unpack('<6I',m.mem_read(sp,24))
    assert returned==0x3879e2 and requested==size and lowest==0 and highest==0xffffffff and align==alignment and protection==4 and word(sp+28)==0x38e734
    capacity=(size+4095)&~4095;raw=mmap.mmap(-1,capacity,flags=mmap.MAP_PRIVATE|mmap.MAP_ANONYMOUS,prot=mmap.PROT_READ|mmap.PROT_WRITE);raw[:]=b'\xa5'*capacity
    native=ctypes.addressof(ctypes.c_char.from_buffer(raw));assert native>0xffffffff;cpu.mem_map_ptr(0x85000000,capacity,3,native);pages.append((capacity,native,raw));value=0x85000000;pop=24
    boundaries.append(dict(kind='allocate',requested=size,capacity=capacity,native=native,source=value,physical=0x5000000))
   else:
    returned=word(sp);assert word(sp+4)==0x85000000
    if pc==0x800000b4:assert returned==0x3879ef and word(sp+12)==0x38e734;value=pages[0][0]
    else:assert returned==0x38e751;value=0x5000000
    pop=8;boundaries.append(dict(kind='capacity' if pc==0x800000b4 else 'physical_address',value=value))
   m.reg_write(R.UC_X86_REG_EAX,value);m.reg_write(R.UC_X86_REG_ESP,sp+pop);m.reg_write(R.UC_X86_REG_EIP,returned);return
  raw=bytes(m.mem_read(pc,count));assert raw==image.read(pc,count);assert executed.setdefault(f'{pc:08x}',raw.hex())==raw.hex();instructions.append(pc)
 def store(m,access,address,count,value,_):stores.append([m.reg_read(R.UC_X86_REG_EIP),address,count,value&((1<<(count*8))-1)])
 cpu.hook_add(UC_HOOK_CODE,hook);cpu.hook_add(UC_HOOK_MEM_WRITE,store)
 try:
  cpu.mem_write(stack,struct.pack('<I',stop));cpu.emu_start(0x387bb5,stop,count=1000)
  init_instructions=instructions.copy();init_stores=stores.copy();instructions.clear();stores.clear();cpu.mem_write(stack,struct.pack('<4I',stop,descriptor,alignment,4));cpu.reg_write(R.UC_X86_REG_ESP,stack)
  cpu.emu_start(0x38e71c,stop,count=1000000)
  assert cpu.reg_read(R.UC_X86_REG_EIP)==stop and cpu.reg_read(R.UC_X86_REG_ESP)==stack+16 and cpu.reg_read(R.UC_X86_REG_EAX)==0
  capacity=pages[0][0];assert bytes(cpu.mem_read(0x85000000,capacity))==bytes(capacity) and word(0x390bc4)==capacity and word(0x3912a0)==0x390bc4
  assert bytes(cpu.mem_read(descriptor,12))==struct.pack('<3I',0x85000000,0x5000000,size)
  return dict(requested=size,alignment=alignment,input_gp=gp,init_instructions=init_instructions,init_stores=init_stores,instructions=instructions,source_stores=stores,returned_gp=[cpu.reg_read(r)&0xffffffff for r in regs],returned_eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS),capacity=capacity,descriptor_hex=bytes(cpu.mem_read(descriptor,12)).hex(),statistics_bytes=word(0x390bc4),boundaries=boundaries,all_capacity_bytes_source_cleared=True)
 finally:
  for capacity,native,raw in pages:cpu.mem_unmap(0x85000000,capacity);raw.close()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert not a.output.exists()
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
 assert image.read(0x390bc4,4)==bytes(4) and image.read(0x3912a0,4)==bytes(4)
 executed={};cases=[run(image,n,align,executed) for n,align in [(0x2000,0x1000),(0x8000,0x4000),(0x1060,0x4000),(0x13000,0x4000)]]
 a.output.mkdir(parents=True);report=dict(passed=True,scope=__doc__,original_xbe_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),cases=cases,executed_instruction_bytes=executed,physical_namespace_policy='Native sound pages begin at05000000 above retained game/renderer arena; original unrestricted request unchanged',instruction_patches=[],supplied_game_results=[],hardware_dsp_hrtf_or_full_startup_verified=False)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(f'Original sound DMA: {len(cases)} cases, {len(executed)} unchanged sites')
if __name__=='__main__':main()
