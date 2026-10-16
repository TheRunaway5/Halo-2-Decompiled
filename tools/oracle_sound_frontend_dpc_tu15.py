#!/usr/bin/env python3
"""Unchanged TU1.5 deferred notifications with genuine base PCM voice constructors.

Already-inactive playback/list admission is a declared bounded input. The native
platform ABI is declared explicitly; no hardware FIFO/configuration/DSP result
is supplied. The fence completion is an actual worker write to real DMA pages.
"""
import argparse,ctypes,hashlib,json,mmap,struct,threading
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from oracle_sound_reset_tu15 import run as reset_reference
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R

def run(image,baseline,executed,fill=0):
 cpu=Uc(UC_ARCH_X86,UC_MODE_32)
 for address,size in [(0x10000,0x600000),(0x2000000,0x20000),(0x2100000,4096),(0x2200000,4096),(0x19000000,0x40000),(0x80000000,4096),(0xfe800000,0x60000)]:cpu.mem_map(address,size)
 for section in image.sections:
  raw=image.section_bytes(section)
  if section.va+len(raw)<=0x610000:cpu.mem_write(section.va,raw)
 cpu.mem_write(0x80000f00,b'\xc3')
 stack,apu,fs=0x2008000,0x19000000,0x2001000
 cpu.mem_write(0x2000000,bytes([fill])*0x20000);cpu.mem_write(fs+0x24,bytes(4));cpu.mem_write(fs+0x58,bytes(4))
 cpu.mem_write(apu,bytes.fromhex(baseline['apu_hex']));cpu.mem_write(stack-0x100,bytes.fromhex(baseline['stack_hex']))
 cpu.mem_write(0xfe801510,struct.pack('<I',1))
 cpu.mem_write(0x391200,struct.pack('<I',apu+0x1c));cpu.mem_write(0x390bd0,struct.pack('<2I',0xc0,0x40))
 def segment(base,access):return struct.pack('<Q',(base&65535)<<16|(base&0xff0000)<<16|(base&0xff000000)<<32|65535|0xf<<48|access<<40|0xc<<52)
 cpu.mem_write(0x2200008,segment(0,0x9b)+segment(0,0x93)+segment(fs,0x93));cpu.reg_write(R.UC_X86_REG_GDTR,(0,0x2200000,4095,0))
 for reg,value in [(R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)]:cpu.reg_write(reg,value)
 regs=[getattr(R,'UC_X86_REG_'+n) for n in ['EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI']]
 gp=baseline['constructor_input_gp'];pages=[];next_physical=0x5000000;instructions=[];stores=[];platform=[];dpcs=[];level=0;ack_pending=False;lock=threading.RLock();sync_frame=None;fence_target=None;fence_values=[]
 def word(p):return struct.unpack('<I',cpu.mem_read(p,4))[0]
 def put(p,v):cpu.mem_write(p,struct.pack('<I',v))
 def hook(m,pc,n,_):
  nonlocal level,next_physical,ack_pending,sync_frame
  if ack_pending:
   assert pc==0x3892e8;m.mem_write(0xfe801000,bytes(4));ack_pending=False
  if pc==0x80000f00:
   assert sync_frame is not None
   saved,sp,ret=sync_frame;result=m.reg_read(R.UC_X86_REG_EAX);sync_frame=None
   for reg,value in zip(regs,saved):m.reg_write(reg,value)
   m.reg_write(R.UC_X86_REG_EAX,result);m.reg_write(R.UC_X86_REG_ESP,sp+16);m.reg_write(R.UC_X86_REG_EIP,ret);lock.release();return
  if pc==0x80000099:
   sp=m.reg_read(R.UC_X86_REG_ESP);ret=word(sp);assert ret==0x389124 and struct.unpack('<3I',m.mem_read(sp+4,12))==(0x3912d8,0x3889f1,apu)
   assert sync_frame is None;lock.acquire();sync_frame=([m.reg_read(r) for r in regs],sp,ret)
   m.mem_write(sp-8,struct.pack('<2I',0x80000f00,apu));m.reg_write(R.UC_X86_REG_ESP,sp-8);m.reg_write(R.UC_X86_REG_EIP,0x3889f1);platform.append([pc,ret,None]);return
  if pc in [0x800000a0,0x800000a1,0x800000a6,0x800000b4,0x800000ad,0x80000077,0x8000006b]:
   sp=m.reg_read(R.UC_X86_REG_ESP);ret=word(sp);pop=4;value=None
   if pc==0x800000a0:
    assert ret==0x383d8e and level==0 and m.reg_read(R.UC_X86_REG_ECX)&255==2;lock.acquire();level=2;value=m.reg_read(R.UC_X86_REG_EAX)&0xffffff00
   elif pc==0x800000a1:
    assert ret==0x383da3 and level==2 and m.reg_read(R.UC_X86_REG_ECX)&255==0;level=0;lock.release()
   elif pc==0x800000a6:
    size,low,high,alignment,protection=struct.unpack('<5I',m.mem_read(sp+4,20));assert low==0 and high==0xffffffff and protection==4 and ret==0x3879e2
    capacity=(size+4095)&~4095;physical=(next_physical+alignment-1)&~(alignment-1);source=physical|0x80000000;next_physical=physical+capacity
    raw=mmap.mmap(-1,capacity,flags=mmap.MAP_PRIVATE|mmap.MAP_ANONYMOUS,prot=mmap.PROT_READ|mmap.PROT_WRITE);native=ctypes.addressof(ctypes.c_char.from_buffer(raw));assert native>0xffffffff
    m.mem_map_ptr(source,capacity,3,native);pages.append(dict(source=source,physical=physical,capacity=capacity,raw=raw));value=source;pop=24
   elif pc in [0x800000b4,0x800000ad]:
    source=word(sp+4);row=next(p for p in pages if p['source']==source);value=row['capacity'] if pc==0x800000b4 else row['physical'];pop=8
   elif pc==0x8000006b:
    assert ret==0x3893bf and struct.unpack('<3I',m.mem_read(sp+4,12))==(apu+0x4c8,0x38918c,apu)
    pop=16
   else:
    assert ret in [0x38b197,0x38931a] and struct.unpack('<3I',m.mem_read(sp+4,12))==(apu+0x4c8,0,0)
    added=not dpcs
    if added:dpcs.append(dict(source=apu+0x4c8,routine=0x38918c,context=apu))
    value=(m.reg_read(R.UC_X86_REG_EAX)&0xffffff00)|int(added);pop=16
   platform.append([pc,ret,value]);m.mem_write(fs+0x24,bytes([level]))
   if value is not None:m.reg_write(R.UC_X86_REG_EAX,value)
   m.reg_write(R.UC_X86_REG_ESP,sp+pop);m.reg_write(R.UC_X86_REG_EIP,ret);return
  raw=bytes(m.mem_read(pc,n));assert raw==image.read(pc,n);assert executed.setdefault(f'{pc:08x}',raw.hex())==raw.hex();instructions.append(pc)
 def store(m,a,p,n,v,_):
  nonlocal ack_pending,fence_target
  site=m.reg_read(R.UC_X86_REG_EIP);stores.append([site,p,n,v&((1<<(n*8))-1)])
  if p==0xfe801000 and site==0x3892e2:ack_pending=True
  if p==0xfe801324:
   assert site==0x388941 and v==word(0x391384);fence_target=next(row for row in pages if row['physical']==v)
  if p==0xfe801334:
   assert site==0x38894f and fence_target is not None
   target=fence_target;value=v
   def transfer():target['raw'][:4]=struct.pack('<I',value)
   worker=threading.Thread(target=transfer);worker.start();worker.join();fence_values.append(value);fence_target=None
 cpu.hook_add(UC_HOOK_CODE,hook);cpu.hook_add(UC_HOOK_MEM_WRITE,store)
 def call(entry,ecx,args=()):
  for reg,value in zip(regs,gp):cpu.reg_write(reg,value)
  cpu.reg_write(R.UC_X86_REG_ECX,ecx);cpu.reg_write(R.UC_X86_REG_ESP,stack);cpu.reg_write(R.UC_X86_REG_EFLAGS,0x202);cpu.reg_write(R.UC_X86_REG_FPCW,0x27f);cpu.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
  cpu.mem_write(stack,struct.pack('<'+'I'*(1+len(args)),0x2100000,*args));cpu.emu_start(entry,0x2100000,count=100000)
  assert cpu.reg_read(R.UC_X86_REG_EIP)==0x2100000 and level==0
 try:
  call(0x387bb5,0)
  for descriptor,size in [(0x391390,0x8000),(0x3913a0,0x4020),(0x391380,4)]:
   put(descriptor+8,size);call(0x38e71c,0,(descriptor,0x4000,4))
  metadata=0x19020000;descriptor=0x19021000;wave=0x19022000
  cpu.mem_write(descriptor,struct.pack('<6I',24,0,0,wave,0,0));cpu.mem_write(wave,struct.pack('<HHIIHHH',1,2,48000,192000,4,16,0))
  # Whole inlined E0 object header constructor at its authentic factory sites.
  cpu.reg_write(R.UC_X86_REG_EAX,metadata);cpu.emu_start(0x387073,0x387082,count=3)
  call(0x386fd1,metadata,(descriptor,))
  channels=[0x19010000,0x19010200];head=apu+0x488
  for channel in channels:
   call(0x38cc02,channel,(apu,metadata));cpu.mem_write(channel+0x64,b'\1\0');cpu.mem_write(channel+0x12,b'\1\0')
   call(0x388817,apu,(channel,));slot=struct.unpack('<H',cpu.mem_read(channel+0xc,2))[0]
   assert slot==64+channels.index(channel);call(0x38bb72,channel+0x68,(slot,))
  # Declared already-active list input for the bounded original idle handler.
  put(head,channels[0]+0x4c);put(head+4,channels[1]+0x4c)
  for i,ch in enumerate(channels):put(ch+0x4c,head if i==1 else channels[i+1]+0x4c);put(ch+0x50,head if i==0 else channels[i-1]+0x4c)
  setup=dict(instructions=instructions.copy(),stores=stores.copy(),platform=platform.copy());instructions.clear();stores.clear();platform.clear()
  # Actual original initialization CALL and declared native void registration
  # ABI. No Xbox KDPC bytes or executed callback result is manufactured.
  for reg,value in zip(regs,gp):cpu.reg_write(reg,value)
  cpu.reg_write(R.UC_X86_REG_ECX,0);cpu.reg_write(R.UC_X86_REG_EBX,0x8000006b);cpu.reg_write(R.UC_X86_REG_ESP,stack+4)
  cpu.mem_write(stack+4,struct.pack('<3I',apu+0x4c8,0x38918c,apu));cpu.emu_start(0x3893bd,0x3893bf,count=10)
  instructions.clear();stores.clear();platform.clear()
  for p in range(0xfe802054,0xfe802078,4):put(p,0xffff)
  put(0xfe802054,64);put(0xfe801100,0x80)
  call(0x38e5ab,apu+8,(1,));mode=dict(instructions=instructions.copy(),stores=stores.copy(),gp=[cpu.reg_read(r)&0xffffffff for r in regs],flags=cpu.reg_read(R.UC_X86_REG_EFLAGS));instructions.clear();stores.clear();platform.clear()
  events=[]
  for slot in [64,65]:
   put(0xfe801100,0xfe0);put(0xfe801000,0x11);put(0xfe801300,0x8000);put(0xfe801304,slot)
   call(0x3892d0,apu)
   # Hardware acknowledgement is W1C; offline raw storage is adjusted after
   # the unchanged write before the final status reload, by the memory hook.
   events.append(dict(slot=slot,instructions=instructions.copy(),stores=stores.copy(),platform=platform.copy(),gp=[cpu.reg_read(r)&0xffffffff for r in regs],flags=cpu.reg_read(R.UC_X86_REG_EFLAGS)))
   instructions.clear();stores.clear();platform.clear()
  assert len(dpcs)==1
  call(0x38918c,0,(apu+0x4c8,apu,0,0))
  deferred=dict(instructions=instructions.copy(),stores=stores.copy(),platform=platform.copy(),gp=[cpu.reg_read(r)&0xffffffff for r in regs],flags=cpu.reg_read(R.UC_X86_REG_EFLAGS),fence_values=fence_values)
  dpcs.pop(0)
  return dict(fill=fill,setup=setup,mode=mode,events=events,dpc=deferred,metadata_hex=bytes(cpu.mem_read(metadata,0xe0)).hex(),fence_hex=bytes(cpu.mem_read(word(0x391380),0x1000)).hex(),apu_hex=bytes(cpu.mem_read(apu,0x540)).hex(),channels_hex=[bytes(cpu.mem_read(ch,0x158)).hex() for ch in channels],
   voice_hex=bytes(cpu.mem_read(word(0x391390),0x8000)).hex(),notifier_hex=bytes(cpu.mem_read(word(0x3913a0),0x5000)).hex(),stack_hex=bytes(cpu.mem_read(stack-0x200,0x210)).hex(),deferred=dpcs)
 finally:
  for p in pages:cpu.mem_unmap(p['source'],p['capacity']);p['raw'].close()
def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();assert not args.output.exists()
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
 executed={};baseline=reset_reference(image,False,False,{})
 cases=[dict(reset=baseline,case=run(image,baseline,executed,fill)) for fill in [0,0x5a,0xa5]];case=cases[0]['case']
 from oracle_sound_configuration_tu15 import run as bit7_reference
 bit7=[bit7_reference(image,fill,0,0,True) for fill in [0,0x5a,0xa5]]
 args.output.mkdir(parents=True);report=dict(passed=True,scope=__doc__,original_xbe_sha256=NEW_PIN,reset=baseline,case=case,cases=cases,bit7_cases=bit7,bit7_tool_sha256=hashlib.sha256(Path(__file__).with_name("oracle_sound_configuration_tu15.py").read_bytes()).hexdigest(),executed_instruction_bytes=executed,
  instruction_patches=[],supplied_source_device_or_dsp_results=[],full_configuration_or_game_playback_verified=False,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
 (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(f'Original DPC: {len(case["dpc"]["instructions"])} instructions, {len(case["dpc"]["stores"])} stores, {len(executed)} unchanged sites')
if __name__=='__main__':main()
