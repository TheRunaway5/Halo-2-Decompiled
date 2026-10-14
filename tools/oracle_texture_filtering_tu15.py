#!/usr/bin/env python3
"""Run complete unchanged TU1.5 D3D sampler submission, including real bias conversion."""
import argparse,hashlib,itertools,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert not a.output.exists()
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;c=_Code(image,'D3D');sites=set(complete_body(c,0x409ee0).code)|set(complete_body(c,0x406fe0).code)
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
 for section in image.sections:u.mem_write(section.va,image.section_bytes(section))
 u.mem_map(0x1000000,0x20000);device=0x1000000;buffer=0x1010000;stack=0x100e000;stop=0x100f000;seen=set();cases=[]
 def word(at,value):u.mem_write(at,struct.pack('<I',value))
 def hook(cpu,pc,size,_):
  assert pc in sites and bytes(cpu.mem_read(pc,size))==image.read(pc,size);seen.add(pc)
 u.hook_add(UC_HOOK_CODE,hook)
 for stage,minimum,magnify,mip,aniso in itertools.product(range(4),range(1,4),range(1,4),range(3),range(5)):
  u.mem_write(device,bytes(0x1000));word(device,buffer);word(device+4,buffer+0xf000);word(device+stage*4+0xf88,1)
  state=0x40eeb0+stage*0x80;u.mem_write(state,bytes(0x80))
  for offset,value in [(0,1),(4,1),(8,1),(12,magnify),(16,minimum),(20,mip),(32,aniso)]:word(state+offset,value)
  u.mem_write(stack,struct.pack('<III',stop,device,1<<stage));u.reg_write(UC_X86_REG_ESP,stack);u.emu_start(0x409ee0,stop,count=1000);assert u.reg_read(UC_X86_REG_EIP)==stop
  command=list(struct.unpack('<5I',u.mem_read(buffer,20)));assert command[0]==0x81b08+stage*0x40 and command[3]==0x41b14+stage*0x40,command
  control=command[2];filter_word=command[4];minimum_code=(filter_word>>16)&15;magnify_code=(filter_word>>24)&15
  assert minimum_code in range(1,7)and magnify_code in (1,2)
  cases.append(dict(stage=stage,min_filter=minimum,mag_filter=magnify,mip_filter=mip,anisotropy=aniso,commands=command,
    expected_min_filter=1 if minimum_code in (1,3,5)else 2,expected_mag_filter=magnify_code,expected_anisotropy=1<<((control>>4)&3)))
 a.output.write_text(json.dumps(dict(reference='OG Xbox TU1.5',xbe_sha256=NEW_PIN,entry=0x409ee0,whole_instructions=len(sites),executed_instructions=len(seen),service_replacements=[],boundary='Supplied source sampler banks and sufficient pushbuffer capacity; unchanged full submission and bias routine, no allocation/callback replacements.',cases=cases),indent=2)+'\n');print(f'PASS {len(cases)} original sampler submissions; {len(seen)}/{len(sites)} instructions')
if __name__=='__main__':main()
