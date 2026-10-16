#!/usr/bin/env python3
"""Run whole unchanged TU1.5 startup slots59..68 on declared retained native64 storage.

Only kernel184 NtAllocateVirtualMemory crosses a platform boundary. Original
arena allocation, CRC, AI schema masks, x87 trigonometric tables and all pool constructors execute.
The declared prior arena/vtable/cursor are inputs, never supplied game results.
This bounded constructor probe does not admit a map or prove rendered gameplay.
"""
import argparse,ctypes,hashlib,json,mmap,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_MXCSR
ENTRIES=[0x1cfa40,0x1cb130,0x31d400,0x31d400,0x31d400,0x219080,0x137870,0x31d400,0x22c980,0x31d400]

GP=[UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI]
ARENA,ARENA_BYTES,ARENA_OBJECT,STACK,STOP=0x81000000,0x3fe000,0x2001000,0x2008000,0x2100000

def run(image,fill,executed):
 cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x600000)
 for section in image.sections:
  raw=image.section_bytes(section)
  if section.va+len(raw)<=0x610000:cpu.mem_write(section.va,raw)
 cpu.mem_map(0x2000000,0x20000);cpu.mem_map(STOP,0x1000);cpu.mem_map(0x80000000,0x1000)
 pages=[];allocations=[];instructions=[];stores=[];accessed={};written={};next_source=0x10000000
 def backing(source,size):
  raw=mmap.mmap(-1,size,flags=mmap.MAP_PRIVATE|mmap.MAP_ANONYMOUS,prot=mmap.PROT_READ|mmap.PROT_WRITE);raw[:]=bytes([fill])*size
  native=ctypes.addressof(ctypes.c_char.from_buffer(raw));assert native>0xffffffff
  cpu.mem_map_ptr(source,size,3,native);pages.append((source,size,raw));return native
 native_arena=backing(ARENA,ARENA_BYTES)
 prior_renderer=0x12000000;backing(prior_renderer,0x46000)
 put=lambda p,v:cpu.mem_write(p,struct.pack('<I',v));word=lambda p:struct.unpack('<I',cpu.mem_read(p,4))[0]
 put(0x51df2c,ARENA_OBJECT);put(ARENA_OBJECT,0x45f1d8)
 assert word(0x45f1d8)==0x127210
 put(0x51df44,prior_renderer)
 put(0x4f2950,ARENA);put(0x4f2954,0x100);put(0x4f2958,0x200);put(0x4f295c,0x12345678)
 cpu.mem_write(0x56e225,b'\0');cpu.mem_write(0x56e258,bytes([fill])*0x400)
 # The earlier generic-pool allocator default is a declared immutable owner.
 assert word(0x474cbc)==0x486038
 for i,r in enumerate(GP):cpu.reg_write(r,0x11000000+i*0x111111)
 cpu.reg_write(UC_X86_REG_EFLAGS,0x202);cpu.reg_write(UC_X86_REG_FPCW,0x27f);cpu.reg_write(UC_X86_REG_MXCSR,0x1f80)
 def hook(m,pc,count,_):
  nonlocal next_source
  if pc==0x800000b8:
   sp=m.reg_read(UC_X86_REG_ESP);returned,base_cell,zero,size_cell,kind,protection=struct.unpack('<6I',m.mem_read(sp,24));size=word(size_cell)
   assert returned==0x33b3c5 and not word(base_cell) and zero==0 and kind==0x101000 and protection==4
   rounded=(size+4095)&~4095;base=next_source;native=backing(base,rounded);next_source+=rounded
   put(base_cell,base);put(size_cell,rounded);allocations.append(dict(source=base,native=native,requested=size,reserved=rounded))
   m.reg_write(UC_X86_REG_EAX,0);m.reg_write(UC_X86_REG_ESP,sp+24);m.reg_write(UC_X86_REG_EIP,returned);return
  raw=bytes(m.mem_read(pc,count));assert image.read(pc,count)==raw,(hex(pc),raw.hex())
  assert executed.setdefault(f'{pc:08x}',raw.hex())==raw.hex();instructions.append(pc)
 def track(m,access,address,size,value,_):
  if 0x410000<=address and address+size<=0x600000 or 0x1afb48<=address<0x1afb5c:
   for p in range(address,address+size):accessed[p]=image.read(p,1)[0]
 def store(m,access,address,size,value,_):
  if 0x410000<=address and address+size<=0x600000:
   for p in range(address,address+size):written[p]=image.read(p,1)[0]
  track(m,access,address,size,value,_)
  stores.append((m.reg_read(UC_X86_REG_EIP),address,size,value&((1<<(size*8))-1)))
 cpu.hook_add(UC_HOOK_CODE,hook);cpu.hook_add(UC_HOOK_MEM_WRITE,store);cpu.hook_add(UC_HOOK_MEM_READ,track)
 results=[]
 try:
  for slot,entry in enumerate(ENTRIES,59):
   put(STACK,STOP);cpu.reg_write(UC_X86_REG_ESP,STACK);before=len(instructions)
   cpu.emu_start(entry,STOP,count=2000000)
   assert cpu.reg_read(UC_X86_REG_EIP)==STOP and cpu.reg_read(UC_X86_REG_ESP)==STACK+4
   results.append(dict(slot=slot,entry=entry,instructions=len(instructions)-before,returned_gp=[cpu.reg_read(r)&0xffffffff for r in GP],returned_eflags=cpu.reg_read(UC_X86_REG_EFLAGS),arena_cursor=word(0x4f2954),arena_top=word(0x4f2958),crc=word(0x4f295c)))
  assert [r['requested'] for r in allocations]==[0x1450,0x850,0x1409b,0x106470]
  assert word(0x4f2958)==0x200
  def regions(values):
   rows=[]
   for address in sorted(values):
    if not rows or address!=rows[-1][0]+rows[-1][1]:rows.append([address,1])
    else:rows[-1][1]+=1
   return rows
  fields=[dict(address=a,bytes=n,initial_hex=image.read(a,n).hex(),hex=bytes(cpu.mem_read(a,n)).hex()) for a,n in regions(written)]
  initial=[dict(address=a,bytes=n,initial_hex=image.read(a,n).hex()) for a,n in regions(accessed)]
  data=[dict(source=a,bytes=n,hex=bytes(cpu.mem_read(a,n)).hex()) for a,n,_ in pages if a!=ARENA]
  return dict(fill=fill,declared_input=dict(arena_source=ARENA,arena_bytes=ARENA_BYTES,arena_object=ARENA_OBJECT,cursor=0x100,top=0x200,crc=0x12345678),native_arena=native_arena,allocations=allocations,steps=results,fields=fields,accessed_static_regions=initial,prior_renderer_source=prior_renderer,arena_hex=bytes(cpu.mem_read(ARENA,ARENA_BYTES)).hex(),allocations_bytes=data,crc_table_hex=bytes(cpu.mem_read(0x56e258,0x400)).hex(),instructions=len(instructions),source_store_count=len(stores),source_stores=stores,source_store_sha256=hashlib.sha256(json.dumps(stores,separators=(',',':')).encode()).hexdigest())
 finally:
  for base,size,raw in pages:cpu.mem_unmap(base,size);raw.close()

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);args=p.parse_args();assert not args.output.exists()
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
 executed={};cases=[run(image,fill,executed) for fill in [0,0x5a,0xa5]]
 args.output.mkdir(parents=True);report=dict(passed=True,scope=__doc__,original_xbe_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),cases=cases,executed_instruction_bytes=executed,instruction_patches=[],supplied_game_results=[],full_startup_verified=False,gameplay_or_gpu_parity_verified=False)
 (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(f'{len(cases)} original cases, {len(executed)} unchanged sites; slots59..68')
if __name__=='__main__':main()
