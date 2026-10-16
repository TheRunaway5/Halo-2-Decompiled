#!/usr/bin/env python3
"""Whole unchanged127EE0 sound CPU prefix stops at required224EC0 driver.

Raw virtual allocation, UTC and physical protection are actual native services.
Prior native game-arena bounds and ordinary CRT/FS state are declared inputs.
Original sound/cache/pool constructors and the clock/rand/tick seed execute.
The stopped DirectSound constructor receives no return or initialized state.
"""
import argparse,ctypes,hashlib,json,mmap,struct,time
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from export_startup_sound_storage_data_tu15 import COLD
from oracle_native_tu15_heap_data import segment
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert not a.output.exists()
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
 cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x600000);cpu.mem_map(0x2000000,0x20000);cpu.mem_map(0x80000000,0x1000)
 for section in image.sections:
  raw=image.section_bytes(section)
  if section.va+len(raw)<=0x610000:cpu.mem_write(section.va,raw)
 fs,gdt,stack=0x60200000,0x60400000,0x2008000
 cpu.mem_map(fs,0x15000);cpu.mem_map(gdt,4096);cpu.mem_write(gdt+8,segment(0,0x9b)+segment(0,0x93)+segment(fs,0x93));cpu.reg_write(R.UC_X86_REG_GDTR,(0,gdt,4095,0))
 for register,value in [(R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)]:cpu.reg_write(register,value)
 assert cpu.reg_read(R.UC_X86_REG_FS_BASE)==fs
 def word(p):return struct.unpack('<I',cpu.mem_read(p,4))[0]
 def put(p,v):cpu.mem_write(p,struct.pack('<I',v))
 put(fs+0x28,fs+0x13000);put(0x573840,fs+0x14000);put(fs+0x14014,1);put(0x41b774,fs+0x12000)
 put(0x4f2950,0x81000000);put(0x4f2954,0x100);put(0x4f2958,0x200);put(0x4f295c,0x12345678)
 put(0x51df2c,0x2001000);put(0x2001000,0x45f1d8)
 put(0x4f2cf0,0);put(0x4f2cfc,0x1000000);put(0x4f2d10,0x5000000)
 regs=[getattr(R,'UC_X86_REG_'+n) for n in ['EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI']]
 gp=[0x11000000+i*0x111111 for i in range(8)];gp[3]=0;gp[4]=stack
 for r,v in zip(regs,gp):cpu.reg_write(r,v)
 cpu.reg_write(R.UC_X86_REG_EFLAGS,0x202);cpu.reg_write(R.UC_X86_REG_FPCW,0x27f);cpu.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
 pages=[];allocations=[];protections=[];utc=[];ticks=[];instructions=[];executed={};stores=[];next_source=0x10000000
 def backing(source,size):
  raw=mmap.mmap(-1,size,flags=mmap.MAP_PRIVATE|mmap.MAP_ANONYMOUS,prot=mmap.PROT_READ|mmap.PROT_WRITE);native=ctypes.addressof(ctypes.c_char.from_buffer(raw));assert native>0xffffffff
  cpu.mem_map_ptr(source,size,3,native);pages.append((source,size,native,raw));return native
 native_physical=backing(0x81000000,0x4000000)
 libc=ctypes.CDLL(None,use_errno=True);libc.mprotect.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_int]
 def hook(m,pc,size,_):
  nonlocal next_source
  sp=m.reg_read(R.UC_X86_REG_ESP)
  if pc==0x800000b8:
   returned,base_cell,zero,size_cell,kind,protection=struct.unpack('<6I',m.mem_read(sp,24));request=word(size_cell)
   assert returned==0x33b3c5 and not word(base_cell) and not zero and kind in [0x1000,0x101000] and protection==4
   rounded=(request+4095)&~4095;base=next_source;native=backing(base,rounded);next_source+=rounded
   put(base_cell,base);put(size_cell,rounded);allocations.append(dict(source=base,requested=request,reserved=rounded,native=native,allocation_type=kind))
   m.reg_write(R.UC_X86_REG_EAX,0);m.reg_write(R.UC_X86_REG_ESP,sp+24);m.reg_write(R.UC_X86_REG_EIP,returned);return
  if pc==0x800000b6:
   returned,address,count,protection=struct.unpack('<4I',m.mem_read(sp,16));assert returned==0x21f0a3 and count==0x300000 and protection==4 and 0x81000000<=address and address+count<=0x85000000
   native=native_physical+address-0x81000000;assert libc.mprotect(native,count,mmap.PROT_READ|mmap.PROT_WRITE)==0
   protections.append(dict(source=address,bytes=count,protection=protection,native=native));m.reg_write(R.UC_X86_REG_ESP,sp+16);m.reg_write(R.UC_X86_REG_EIP,returned);return
  if pc==0x80000080:
   assert word(sp)==0x32bbbc and word(sp+4)==m.reg_read(R.UC_X86_REG_EBP)-8
   value=time.time_ns()//100+116444736000000000;cpu.mem_write(word(sp+4),struct.pack('<Q',value));utc.append(value)
   m.reg_write(R.UC_X86_REG_ESP,sp+8);m.reg_write(R.UC_X86_REG_EIP,word(sp));return
  if pc==0x33b595:
   value=(time.monotonic_ns()//1000000)&0xffffffff;put(fs+0x12000,value);ticks.append(value)
  raw=bytes(m.mem_read(pc,size));assert raw==image.read(pc,size);executed[f'{pc:08x}']=raw.hex();instructions.append(pc)
 def store(m,access,address,size,value,_):stores.append([m.reg_read(R.UC_X86_REG_EIP),address,size,value&((1<<(size*8))-1)])
 cpu.hook_add(UC_HOOK_CODE,hook);cpu.hook_add(UC_HOOK_MEM_WRITE,store)
 try:
  cpu.emu_start(0x12008,0x12025,count=100000)
  prefix_instructions=instructions.copy();prefix_stores=stores.copy();instructions.clear();stores.clear()
  put(stack,0x2100000);cpu.reg_write(R.UC_X86_REG_ESP,stack);before_gp=[cpu.reg_read(r)&0xffffffff for r in regs]
  cpu.emu_start(0x127ee0,0x224ec0,count=1000000)
  assert cpu.reg_read(R.UC_X86_REG_EIP)==0x224ec0 and word(cpu.reg_read(R.UC_X86_REG_ESP))==0x128055
  state=word(0x4f2c50);assert state and cpu.mem_read(state+0x78,1)==b'\0' and word(0x4f2c44)==0 and word(0x52d674)==0
  fields=[dict(address=p,bytes=n,hex=bytes(cpu.mem_read(p,n)).hex()) for p,n in COLD]
  banks=[dict(source=p,bytes=n,hex=bytes(cpu.mem_read(p,n)).hex()) for p,n,native,raw in pages if p!=0x81000000]
  report=dict(passed=True,scope=__doc__,original_xbe_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),entry=0x127ee0,boundary=0x224ec0,source_before_gp=before_gp,returned_gp=[cpu.reg_read(r)&0xffffffff for r in regs],returned_eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS),input_fs=fs,prior_crt_seed=1,crt_seed=word(fs+0x14014),utc_filetime=utc,tick_inputs=ticks,allocations=allocations,protections=protections,prefix_instructions=prefix_instructions,prefix_source_stores=prefix_stores,instructions=instructions,source_stores=stores,fields=fields,allocation_bytes=banks,crc_table_hex=bytes(cpu.mem_read(0x56e258,0x400)).hex(),physical_top=word(0x4f2d10),source_ready=False,executed_instruction_bytes=executed,instruction_patches=[],supplied_game_results=[],platform_boundaries=['NtAllocateVirtualMemory','MmSetAddressProtect','KeQuerySystemTime','KeTickCount imported data'],full_startup_or_audio_ready=False)
  a.output.mkdir(parents=True);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(f'Original sound prefix to224EC0: {len(instructions)} instructions; {len(executed)} unchanged sites; no driver result')
 finally:
  for base,size,native,raw in pages:cpu.mem_unmap(base,size);raw.close()
if __name__=='__main__':main()
