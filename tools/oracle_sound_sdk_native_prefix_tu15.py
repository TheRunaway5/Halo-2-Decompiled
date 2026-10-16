#!/usr/bin/env python3
"""Whole unchanged SDK factory/optional driver through a required native frontier.

Reference-only TU1.5 execution begins at the genuine CRT statistics constructors,
then387877, retaining original COM objects, accounting, refcounts and APU CPU
constructors. Actual native allocation storage/capacity and exclusion are the
declared platform boundaries. Stereo0 is an explicit native preference input;
no EEPROM/SMC, readiness or whole factory result is supplied. Optional composed
reset uses offline control inputs derived from its actual native control graph;
the native runtime owns real operations and never mounts those register bytes.
"""
import argparse,ctypes,hashlib,json,mmap,struct,threading
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from export_sound_sdk_prefix_data_tu15 import COLD,CONFIG,READONLY
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R

GP=[getattr(R,'UC_X86_REG_'+n)for n in ['EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI']]
STACK,OUTPUT,STOP,FS,GDT,POOL=0x2008000,0x2001000,0x2100000,0x60200000,0x60400000,0x18000000
DMA_READONLY=[(0x41b644,4),(0x41b7bc,4)] # Separate actual DMA import owner.
REGIONS=COLD+CONFIG+READONLY+DMA_READONLY
DRIVER_COLD=[(0x51db00,12),(0x52d674,4),(0x56e225,1),(0x56e258,0x400)]
DRIVER_READONLY=[(0x455b20,16),(0x41b6d4,4)]

def segment(base,access):return struct.pack('<HHBBBB',0xffff,base&65535,(base>>16)&255,access,0xcf,(base>>24)&255)
def pool_layout(request):
 if request<=0xfd8:
  extent=((request+0x27)>>5)<<5;return extent,8,extent-8,32
 extent=(request+4095)&~4095;return extent,0,extent,4096

def run(image,fill,executed,driver=False,through_reset=False,through_storage=False):
 cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x600000);cpu.mem_map(0x2000000,0x20000);cpu.mem_map(STOP,4096);cpu.mem_map(0x80000000,4096)
 for sec in image.sections:
  raw=image.section_bytes(sec)
  if sec.va+len(raw)<=0x610000:cpu.mem_write(sec.va,raw)
 cpu.mem_write(0x2000000,bytes([fill])*0x20000);cpu.mem_write(OUTPUT,bytes(4));cpu.mem_map(FS,4096);cpu.mem_map(GDT,4096)
 cpu.mem_write(GDT+8,segment(0,0x9b)+segment(0,0x93)+segment(FS,0x93));cpu.reg_write(R.UC_X86_REG_GDTR,(0,GDT,4095,0))
 for reg,value in [(R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)]:cpu.reg_write(reg,value)
 assert cpu.reg_read(R.UC_X86_REG_FS_BASE)==FS
 for address,size in COLD:assert bytes(cpu.mem_read(address,size))==bytes(size),(hex(address),size)
 assert bytes(cpu.mem_read(0x390be8,4))==b'\xff'*4
 if driver:
  for address,size in DRIVER_COLD:assert bytes(cpu.mem_read(address,size))==bytes(size)
 if through_reset:
  # Offline original register inputs express the actual native reset owner's
  # cold control graph. There is no such memory bank in the native runtime.
  cpu.mem_map(0xfe800000,0x60000)
  for address in range(0xfe802054,0xfe802078,4):cpu.mem_write(address,struct.pack('<I',0xffff))
 # Explicit source-supported configuration input. The matching native test
 # publishes this only from its obtained, live stereo SDL route and saved UI setting.
 cpu.mem_write(0x390be8,bytes(4))
 gp=[0x11000000+i*0x111111 for i in range(8)];gp[4]=STACK
 if driver:gp[3]=0 # Reached source12001 fragment input.
 for reg,value in zip(GP,gp):cpu.reg_write(reg,value)
 cpu.reg_write(R.UC_X86_REG_EFLAGS,0x202);cpu.reg_write(R.UC_X86_REG_FPCW,0x27f);cpu.reg_write(R.UC_X86_REG_FPSW,0);cpu.reg_write(R.UC_X86_REG_FPTAG,0xffff);cpu.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
 raw=mmap.mmap(-1,0x10000,flags=mmap.MAP_PRIVATE|mmap.MAP_ANONYMOUS,prot=mmap.PROT_READ|mmap.PROT_WRITE)
 native=ctypes.addressof(ctypes.c_char.from_buffer(raw));assert native>0xffffffff;cpu.mem_map_ptr(POOL,0x10000,3,native)
 cursor=POOL;allocations=[];platform=[];instructions=[];stores=[];static_accesses=set();critical=threading.RLock();depth=0
 exclusion=threading.RLock();level=0;reset_exclusion=[];arena=[];factory={};physical=[];dma_boundaries=[]
 def word(p):return struct.unpack('<I',cpu.mem_read(p,4))[0]
 def hook(m,pc,size,_):
  nonlocal cursor,depth,level
  sp=m.reg_read(R.UC_X86_REG_ESP)
  if pc in [0x800000a6,0x800000b4,0x800000ad]:
   assert through_storage;returned=word(sp)
   if pc==0x800000a6:
    requested,lowest,highest,alignment,protection=[word(sp+i*4)for i in range(1,6)]
    assert returned==0x3879e2 and lowest==0 and highest==0xffffffff and protection==4 and word(sp+28)==0x38e734
    capacity=(requested+4095)&~4095;align=max(alignment,4096);address=(0x05000000+align-1)&~(align-1)
    for row in physical:
     if address<row['physical']+row['capacity']and address+capacity>row['physical']:address=(row['physical']+row['capacity']+align-1)&~(align-1)
    source=address|0x80000000;pages=mmap.mmap(-1,capacity,flags=mmap.MAP_PRIVATE|mmap.MAP_ANONYMOUS,prot=mmap.PROT_READ|mmap.PROT_WRITE)
    native_address=ctypes.addressof(ctypes.c_char.from_buffer(pages));assert native_address>0xffffffff
    m.mem_map_ptr(source,capacity,3,native_address)
    row=dict(source=source,physical=address,requested=requested,capacity=capacity,alignment=alignment,protection=protection,source_descriptor=m.reg_read(R.UC_X86_REG_EDI),pages=pages)
    physical.append(row);physical.sort(key=lambda row:row['source']);value=source;pop=24
    dma_boundaries.append(dict(kind='allocate',source=source,physical=address,requested=requested,capacity=capacity,alignment=alignment,protection=protection,source_descriptor=row['source_descriptor']))
   else:
    row=next(r for r in physical if r['source']==word(sp+4));pop=8
    if pc==0x800000b4:assert returned==0x3879ef and word(sp+12)==0x38e734;value=row['capacity'];kind='capacity'
    else:assert returned==0x38e751;value=row['physical'];kind='physical'
    dma_boundaries.append(dict(kind=kind,source=row['source'],value=value))
   m.reg_write(R.UC_X86_REG_EAX,value);m.reg_write(R.UC_X86_REG_ESP,sp+pop);m.reg_write(R.UC_X86_REG_EIP,returned);return
  if pc==0x800000b8:
   assert driver and not arena
   returned,base_cell,zero,size_cell,kind,protection=struct.unpack('<6I',m.mem_read(sp,24))
   assert returned==0x33b3c5 and not word(base_cell) and not zero and word(size_cell)==0x23000 and kind==0x1000 and protection==4
   pages=mmap.mmap(-1,0x23000,flags=mmap.MAP_PRIVATE|mmap.MAP_ANONYMOUS,prot=mmap.PROT_READ|mmap.PROT_WRITE)
   address=ctypes.addressof(ctypes.c_char.from_buffer(pages));assert address>0xffffffff
   m.mem_map_ptr(0x10000000,0x23000,3,address);arena.append((pages,address))
   m.mem_write(base_cell,struct.pack('<I',0x10000000));m.mem_write(size_cell,struct.pack('<I',0x23000))
   m.reg_write(R.UC_X86_REG_EAX,0);m.reg_write(R.UC_X86_REG_ESP,sp+24);m.reg_write(R.UC_X86_REG_EIP,returned);return
  if pc in [0x800000a0,0x800000a1]:
   assert through_reset;returned=word(sp);cl=m.reg_read(R.UC_X86_REG_ECX)&255
   if pc==0x800000a0:
    assert returned==0x383d8e and cl==2 and level==0;exclusion.acquire();level=2
    m.reg_write(R.UC_X86_REG_EAX,m.reg_read(R.UC_X86_REG_EAX)&0xffffff00)
   else:assert returned==0x383da3 and cl==0 and level==2;level=0;exclusion.release()
   m.mem_write(FS+0x24,bytes([level]));reset_exclusion.append([pc,returned]);m.reg_write(R.UC_X86_REG_ESP,sp+4);m.reg_write(R.UC_X86_REG_EIP,returned);return
  if pc in [0x80000115,0x80000126,0x8000000f,0x80000017]:
   returned=word(sp)
   if pc==0x80000115:
    assert returned==0x383f98 and word(sp+4)==0x390bf8;critical.acquire();depth+=1;pop=8;kind='enter';value=None
   elif pc==0x80000126:
    assert returned in [0x3878b6,0x3842d0,0x384317]and word(sp+4)==0x390bf8 and depth;critical.release();depth-=1;pop=8;kind='leave';value=None
   elif pc==0x8000000f:
    assert returned==0x38797b;requested,tag=word(sp+4),word(sp+8);assert tag in [0x626f5344,0x61645344]
    extent,offset,capacity,alignment=pool_layout(requested);base=(cursor+alignment-1)&~(alignment-1);assert base+extent<=POOL+0x10000
    cpu.mem_write(base,bytes(extent))
    if offset:cpu.mem_write(base+3,bytes([extent>>5]))
    value=base+offset;allocations.append(dict(source=base,bytes=extent,pointer=value,requested=requested,capacity=capacity,tag=tag));cursor=base+extent;pop=12;kind='allocate'
   else:
    assert returned==0x387988;pointer=word(sp+4);row=next(r for r in allocations if r['pointer']==pointer);value=row['capacity'];pop=8;kind='capacity'
    if pointer&4095:assert bytes(cpu.mem_read(pointer-5,1))[0]*32-8==value
   platform.append(dict(kind=kind,target=pc,returned=returned,args=[word(sp+i*4)for i in range(1,pop//4)],result=value,lock_depth=depth))
   if value is not None:m.reg_write(R.UC_X86_REG_EAX,value)
   m.reg_write(R.UC_X86_REG_ESP,sp+pop);m.reg_write(R.UC_X86_REG_EIP,returned);return
  if driver and pc==0x387877:
   assert word(sp)==0x224f2b and word(sp+4)==0 and word(sp+8)==word(0x52d674)+0x2ab0 and word(sp+12)==0
   factory.update(gp=[m.reg_read(r)for r in GP],eflags=m.reg_read(R.UC_X86_REG_EFLAGS),output_cell=word(sp+8),instructions=instructions.copy(),source_stores=stores.copy())
  code=bytes(m.mem_read(pc,size));assert code==image.read(pc,size);assert executed.setdefault(f'{pc:08x}',code.hex())==code.hex();instructions.append(pc)
 def access(m,kind,address,size,value,_):
  if 0x10000<=address<0x610000:static_accesses.update(range(address,address+size))
 def store(m,kind,address,size,value,_):stores.append([m.reg_read(R.UC_X86_REG_EIP),address,size,value&((1<<(size*8))-1)])
 cpu.hook_add(UC_HOOK_CODE,hook);cpu.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,access);cpu.hook_add(UC_HOOK_MEM_WRITE,store)
 try:
  prior=[]
  arena_prefix=None
  if driver:
   cpu.emu_start(0x12008,0x12025,count=10000)
   assert cpu.reg_read(R.UC_X86_REG_EIP)==0x12025 and word(0x51db00)==0x10000000 and word(0x51db04)==0 and word(0x51db08)==0
   arena_prefix=dict(instructions=instructions.copy(),source_stores=stores.copy(),returned_gp=[cpu.reg_read(r)for r in GP],returned_eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS))
   instructions.clear();stores.clear()
  for entry in [0x387baa,0x387bb5]+([0x38a988,0x38a993]if through_storage else []):
   cpu.mem_write(STACK,struct.pack('<I',STOP));cpu.reg_write(R.UC_X86_REG_ESP,STACK);cpu.emu_start(entry,STOP,count=10000)
   assert cpu.reg_read(R.UC_X86_REG_EIP)==STOP;prior.append(dict(entry=entry,instructions=instructions.copy(),source_stores=stores.copy(),returned_gp=[cpu.reg_read(r)for r in GP],returned_eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS)))
   instructions.clear();stores.clear()
  frame=[0x128055,0x455b20]if driver else[STOP,0,OUTPUT,0]
  cpu.mem_write(STACK,struct.pack('<'+'I'*len(frame),*frame));cpu.reg_write(R.UC_X86_REG_ESP,STACK);input_gp=[cpu.reg_read(r)for r in GP]
  target=0x38e8c9 if through_storage else 0x38e996 if through_reset else 0x38ec0e
  site=0x38eca3 if through_storage else 0x38ec96 if through_reset else 0x38ec8f
  returned=0x38eca8 if through_storage else 0x38ec9b if through_reset else 0x38ec94
  cpu.emu_start(0x224ec0 if driver else 0x387877,target,count=1000000)
  assert cpu.reg_read(R.UC_X86_REG_EIP)==target and word(cpu.reg_read(R.UC_X86_REG_ESP))==returned and not level
  output_argument=word(0x52d674)+0x2ab0 if driver else OUTPUT
  assert len(allocations)==(4 if through_storage else 3)and depth==1 and word(output_argument)==0
  assert word(0x3912a4)==0x390bc0 and word(0x3912a0)==0x390bc4
  assert word(0x390bc0)==sum(r['capacity']for r in allocations)and word(0x390bc4)==sum(r['capacity']for r in physical)
  assert word(0x390be4)==0 and word(0x390be8)==0
  for address in static_accesses:assert any(p<=address<p+n for p,n in REGIONS+(DRIVER_COLD+DRIVER_READONLY if driver else [])),f'unowned SDK static operand {address:08x}'
  return dict(fill=fill,driver=driver,through_reset=through_reset,through_storage=through_storage,initial_gp=gp,prior=prior,arena_prefix=arena_prefix,factory=factory,input_gp=input_gp,instructions=instructions,source_stores=stores,returned_gp=[cpu.reg_read(r)for r in GP],returned_eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS),fs_base=FS,frontier=dict(instruction=site,target=target,return_address=returned,source_subobject=cpu.reg_read(R.UC_X86_REG_ECX)),allocations=[dict(r,hex=bytes(cpu.mem_read(r['source'],r['bytes'])).hex())for r in allocations],dma_allocations=[dict({k:v for k,v in r.items()if k!='pages'},hex=bytes(cpu.mem_read(r['source'],r['capacity'])).hex())for r in physical],dma_boundaries=dma_boundaries,platform=platform,reset_exclusion=reset_exclusion,regions=[dict(address=p,bytes=n,hex=bytes(cpu.mem_read(p,n)).hex())for p,n in COLD+CONFIG+(DRIVER_COLD if driver else [])],stack_hex=bytes(cpu.mem_read(0x2000000,0x20000)).hex(),arena_hex=bytes(cpu.mem_read(0x10000000,0x23000)).hex()if driver else None,lock_depth=depth,source_output=word(output_argument),statistics=word(0x390bc0),dma_statistics=word(0x390bc4),device=word(0x391278),context=word(0x391200),declared_platform_backing_native64=True)
 finally:
  while depth:critical.release();depth-=1
  if level:exclusion.release()
  for pages,address in arena:cpu.mem_unmap(0x10000000,0x23000);pages.close()
  for row in physical:cpu.mem_unmap(row['source'],row['capacity']);row['pages'].close()
  cpu.mem_unmap(POOL,0x10000);raw.close()

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--driver',action='store_true');p.add_argument('--through-reset',action='store_true');p.add_argument('--through-storage',action='store_true');a=p.parse_args();assert not a.output.exists()
 if a.through_storage:a.through_reset=True
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
 assert [int.from_bytes(image.read(p,4),'little')for p in [0x46d7e4,0x46d7e8,0x46d7ec,0x46d7f0]]==[0x387baa,0x387bb5,0x38a988,0x38a993]
 executed={};cases=[run(image,fill,executed,a.driver,a.through_reset,a.through_storage)for fill in [0,0x5a,0xa5]]
 report=dict(passed=True,scope=__doc__,original_xbe_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),cases=cases,executed_instruction_bytes=executed,instruction_patches=[],supplied_game_or_sdk_results=[],console_eeprom_or_smc_values_supplied=[],declared_native_preference=0,platform_policy='Real host raw storage with recovered capacity, recursive lock and passive thread; platform ABI preserves unrelated registers/flags.',native_reset_control_inputs_declared=a.through_reset,original_whole224ec0_prefix=a.driver,full_factory_completed=False,reset_dsp_or_capabilities_verified=False)
 a.output.mkdir(parents=True);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(f'{len(cases)} original whole SDK prefixes, {len(executed)} unchanged sites, frontier{cases[0]["frontier"]["target"]:08x}')
if __name__=='__main__':main()
