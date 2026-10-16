#!/usr/bin/env python3
"""Original cold APU reset with explicit native exclusion/control inputs.

MMIO exists only in this offline oracle. Its inputs represent the tested native
control graph (empty/unlinked voice dispatch, unconfigured processors), not a
successful Xbox device. No game HRESULT, queue-ready or DSP result is supplied.
"""
import argparse,hashlib,json,struct,threading
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R

def run(image,pre_reset,voices,executed):
 cpu=Uc(UC_ARCH_X86,UC_MODE_32)
 for address,size in [(0x10000,0x600000),(0x2000000,0x20000),(0x2100000,4096),(0x19000000,4096),(0x80000000,4096),(0xfe800000,0x60000)]:cpu.mem_map(address,size)
 for section in image.sections:
  raw=image.section_bytes(section)
  if section.va+len(raw)<=0x610000:cpu.mem_write(section.va,raw)
 stack,apu,stop,fs=0x2008000,0x19000000,0x2100000,0x2001000
 regs=[getattr(R,'UC_X86_REG_'+n) for n in ['EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI']]
 gp=[0x11000000+i*0x111111 for i in range(8)];gp[1]=apu;gp[4]=stack
 for reg,value in zip(regs,gp):cpu.reg_write(reg,value)
 # FS-base is set through the same descriptor mechanism as other original
 # SDK probes; source scoped exclusion reads the actual declared native level.
 cpu.mem_map(0x2200000,4096)
 def segment(base,access):
  return struct.pack('<Q',(base&0xffff)<<16 | (base&0xff0000)<<16 | (base&0xff000000)<<32 | 0xffff | 0xf<<48 | access<<40 | 0xc<<52)
 cpu.mem_write(0x2200008,segment(0,0x9b)+segment(0,0x93)+segment(fs,0x93));cpu.reg_write(R.UC_X86_REG_GDTR,(0,0x2200000,0xfff,0))
 for register,value in [(R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)]:cpu.reg_write(register,value)
 assert cpu.reg_read(R.UC_X86_REG_FS_BASE)==fs
 cpu.reg_write(R.UC_X86_REG_EFLAGS,0x202);cpu.reg_write(R.UC_X86_REG_FPCW,0x27f);cpu.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
 instructions=[];stores=[];boundaries=[];lock=threading.RLock();level=0
 def word(p):return struct.unpack('<I',cpu.mem_read(p,4))[0]
 def hook(m,pc,count,_):
  nonlocal level
  if pc in [0x800000a0,0x800000a1]:
   sp=m.reg_read(R.UC_X86_REG_ESP);returned=word(sp);cl=m.reg_read(R.UC_X86_REG_ECX)&255
   if pc==0x800000a0:
    assert returned==0x383d8e and cl==2 and level==0;lock.acquire();level=2
    m.reg_write(R.UC_X86_REG_EAX,m.reg_read(R.UC_X86_REG_EAX)&0xffffff00)
   else:
    assert returned==0x383da3 and cl==0 and level==2;level=0;lock.release()
   m.mem_write(fs+0x24,bytes([level]));boundaries.append([pc,returned]);m.reg_write(R.UC_X86_REG_ESP,sp+4);m.reg_write(R.UC_X86_REG_EIP,returned);return
  raw=bytes(m.mem_read(pc,count));assert raw==image.read(pc,count);assert executed.setdefault(f'{pc:08x}',raw.hex())==raw.hex();instructions.append(pc)
 def store(m,access,address,count,value,_):stores.append([m.reg_read(R.UC_X86_REG_EIP),address,count,value&((1<<(count*8))-1)])
 cpu.hook_add(UC_HOOK_CODE,hook);cpu.hook_add(UC_HOOK_MEM_WRITE,store)
 cpu.mem_write(stack,struct.pack('<2I',stop,0));cpu.emu_start(0x38871c,stop,count=10000)
 ctor_instructions=instructions.copy();ctor_stores=stores.copy();assert word(apu)==0x4212b0 and word(apu+8)==0x4212ac and word(0x391200)==apu+0x1c
 for address in range(0xfe802054,0xfe802078,4):cpu.mem_write(address,struct.pack('<I',0xffff))
 def reset():
  reset_gp=gp.copy();reset_gp[1]=apu+8;reset_gp[6]=apu+8
  for reg,value in zip(regs,reset_gp):cpu.reg_write(reg,value)
  cpu.reg_write(R.UC_X86_REG_EFLAGS,0x202)
  cpu.mem_write(stack,struct.pack('<4I',0x38ec94,gp[7],gp[6],0x389389))
  cpu.emu_start(0x38ec0e,0x38ec94,count=10000)
  assert cpu.reg_read(R.UC_X86_REG_EIP)==0x38ec94 and level==0
  return reset_gp
 if pre_reset:reset()
 if voices:cpu.mem_write(0xfe802054,struct.pack('<I',7))
 instructions.clear();stores.clear();boundaries.clear();reset_gp=reset()
 assert word(0x391208)==0 and word(apu+0x1c)==0 and word(apu+0x20)==0
 return dict(pre_reset=pre_reset,voices=voices,input_gp=reset_gp,constructor_input_gp=gp,constructor_instructions=ctor_instructions,constructor_stores=ctor_stores,
  instructions=instructions,source_stores=stores,returned_gp=[cpu.reg_read(reg)&0xffffffff for reg in regs],returned_eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS),
  apu_hex=bytes(cpu.mem_read(apu,0x540)).hex(),stack_hex=bytes(cpu.mem_read(stack-0x100,0x110)).hex(),boundaries=boundaries)

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();assert not args.output.exists()
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
 executed={};cases=[run(image,pre,voices,executed) for pre,voices in [(False,False),(False,True),(True,True)]]
 args.output.mkdir(parents=True)
 report=dict(passed=True,scope=__doc__,original_xbe_sha256=NEW_PIN,cases=cases,executed_instruction_bytes=executed,instruction_patches=[],supplied_game_results=[],
  native_platform_abi='Real recursive exclusion domain mapped to original scoped helper; previous level0 and request2, preserved GP exceptAL/SP on acquisition',
  hardware_inputs='Only offline register storage; values derived from the tested native graph with unconfigured processors. No ready/FIFO/DSP result.',
  dsp_or_complete_startup_verified=False,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
 (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(f'Original APU reset: {len(cases)} cases, {len(executed)} unchanged sites')
if __name__=='__main__':main()
