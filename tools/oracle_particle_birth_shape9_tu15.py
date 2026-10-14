#!/usr/bin/env python3
"""Whole TU1.5 252480 on actual authored shape9 emitters.

The completed17-value property cache, initial particle and RNG are declared
inputs. The executable, definitions, property helpers and random helper run
unchanged, including Unicorn's existing RSQRTSS policy. No instructions or
services are replaced. This does not establish physical Xbox estimate bits
or complete allocation/emission/attachment scheduling.
"""
import argparse,hashlib,json,random,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn import x86_const as R
from audit_rendering import Cache
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--map',type=Path,required=True)
 p.add_argument('--output',type=Path,required=True);p.add_argument('--cases-per-emitter',type=int,default=128);a=p.parse_args()
 assert 0<a.cases_per_emitter<=2048
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
 cache=Cache(a.map);emitters=[]
 for tag in cache.tags.values():
  if tag['group']!='effe':continue
  _,root=cache.tag(tag['datum'],'effe')
  for event,evt in enumerate(cache.block(root+0x14,0x38)):
   for system,sys in enumerate(cache.block(evt+0x30,0x38)):
    for ordinal,at in enumerate(cache.block(sys+0x30,0xb8)):
     if cache.u32(at+0x78)==9:emitters.append(dict(datum=tag['datum'],name=tag['name'],event=event,system=system,emitter=ordinal,offset=at))
 assert emitters
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
 for section in image.sections:u.mem_write(section.va,image.section_bytes(section))
 base=cache.magic&~4095;u.mem_map(base,(cache.magic+len(cache.data)-base+4095)&~4095);u.mem_write(cache.magic,bytes(cache.data))
 state,stack,stop=0x2400000,0x241f000,0x3000000;u.mem_map(state,0x20000);u.mem_map(stop,4096)
 u.mem_write(0x4f3cd8,struct.pack('<I',state));system=bytearray(84);struct.pack_into('<I',system,24,0xffffffff);u.mem_write(state+0x300,bytes(system))
 count=len(emitters)*a.cases_per_emitter;inputs=bytearray(struct.pack('<I',count));original=bytearray();rng=random.Random(0x25248009)
 for row in emitters:
  for n in range(a.cases_per_emitter):
   values=struct.pack('<17f',*([0.]*17 if n==0 else [1.]*17 if n==1 else [rng.random()for _ in range(17)]))
   seed=rng.getrandbits(32);particle=rng.randbytes(64)
   u.mem_write(state+4,struct.pack('<I',seed));u.mem_write(state+0x100,particle);u.mem_write(state+0x200,values)
   u.mem_write(stack,struct.pack('<III',stop,cache.magic+row['offset'],state+0x300))
   for reg,v in [(R.UC_X86_REG_ESP,stack),(R.UC_X86_REG_EAX,state+0x200),(R.UC_X86_REG_EDI,state+0x100),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80),(R.UC_X86_REG_EFLAGS,2)]:u.reg_write(reg,v)
   u.emu_start(0x252480,stop,count=100000);assert u.reg_read(R.UC_X86_REG_EIP)==stop
   inputs+=struct.pack('<IIII',row['datum'],row['event'],row['system'],row['emitter'])+values+struct.pack('<I',seed)+particle
   original+=bytes(u.mem_read(state+0x100,64))+bytes(u.mem_read(state+4,4))
 a.output.mkdir(parents=True,exist_ok=True);(a.output/'inputs.bin').write_bytes(inputs);(a.output/'original.bin').write_bytes(original)
 report=dict(original_sha256=NEW_PIN,map_sha256=hashlib.sha256(a.map.read_bytes()).hexdigest(),generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),entry='00252480',shape=9,cases=count,emitters=emitters,input_sha256=hashlib.sha256(inputs).hexdigest(),output_sha256=hashlib.sha256(original).hexdigest(),instruction_patches=[],replaced_services=[],boundary=__doc__,full_goal_complete=False)
 (a.output/'provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(a.map.stem,count,'original births from',len(emitters),'authored emitters')
if __name__=='__main__':main()
