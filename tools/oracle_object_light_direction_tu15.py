#!/usr/bin/env python3
"""Run unchanged TU1.5 11A360 lighting pole limit, without service replacements."""
import argparse,hashlib,json,random,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert not a.output.exists()
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
 body=complete_body(_Code(image,'.text'),0x11a360);u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
 for section in image.sections:u.mem_write(section.va,image.section_bytes(section))
 u.mem_map(0x1000000,0x4000);seen=set();trace=[]
 def hook(cpu,pc,size,_):
  assert pc in body.code and bytes(cpu.mem_read(pc,size))==image.read(pc,size);seen.add(pc);trace.append(pc)
 u.hook_add(UC_HOOK_CODE,hook);rng=random.Random(0x11a360);cases=[]
 for i in range(1200):
  v=[rng.uniform(-1,1) for _ in range(3)];angle=rng.uniform(0,1.5707963)
  if i<96:
   v[0]=[0,.009999999,.01,-.01][i%4];v[1]=[0,.00009999,.0001,.01][i//4%4];v[2]=[-1,0,1][i//16%3];angle=[0,.2617994,1.5707963][i//32%3]
  raw=struct.pack('<3f',*v);arg=struct.pack('<f',angle);u.mem_write(0x1000000,raw);u.mem_write(0x1002000,struct.pack('<I',0x1003000)+arg)
  u.reg_write(UC_X86_REG_ESP,0x1002000);u.reg_write(UC_X86_REG_EAX,0x1000100);u.reg_write(UC_X86_REG_ECX,0x1000000);trace=[]
  u.emu_start(0x11a360,0x1003000,count=1000);assert u.reg_read(UC_X86_REG_EIP)==0x1003000
  cases.append(dict(input=list(struct.unpack('<3I',raw)),angle=struct.unpack('<I',arg)[0],expected=list(struct.unpack('<3I',u.mem_read(0x1000100,12)))))
 a.output.write_text(json.dumps(dict(reference='OG Xbox TU1.5',xbe_sha256=NEW_PIN,entry=0x11a360,whole_instructions=len(body.code),executed_instructions=len(seen),service_replacements=[],cases=cases),indent=2)+'\n');print(f'PASS {len(cases)} unchanged TU1.5 pole-limit cases; {len(seen)}/{len(body.code)} instructions')
if __name__=='__main__':main()
