#!/usr/bin/env python3
"""Whole unchanged17BA50/17DD50/17DDC0, including encoded FP graph nodes.

Source banks and native caller arguments are declared fixture inputs. Every
callee executes unchanged; no instruction or service return is replaced.
"""
import hashlib,json,math,random,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def main():
 x=Xbe(NEW_DEFAULT);assert hashlib.sha256(x.data).hexdigest()==NEW_PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
 for section in x.sections:u.mem_write(section.va,x.section_bytes(section))
 u.mem_map(0x2400000,0x40000);u.mem_map(0x2800000,0x1000);u.mem_map(0x3000000,4096)
 rng=random.Random(0x17ba50);regs=[R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI]
 trace=[];u.hook_add(UC_HOOK_CODE,lambda g,p,n,_:trace.append(p))
 def matrix():
  angle=rng.uniform(-math.pi,math.pi);c,s=math.cos(angle),math.sin(angle)
  return struct.pack('<13f',rng.choice([.25,1.,2.]),c,s,0.,-s,c,0.,0.,0.,1.,*[rng.uniform(-20,20)for _ in range(3)])
 rows=[]
 for view in (-1,0,1,2,3):
  for mode in (0,1):
   for node in (-1,0,1,3,0x8000,0x8001,0x8003,0x8026,0x803f):
    if view==-1 and node in (0x8026,0x803f):continue
    for hand in (0,1):
     effect,location,pool,headers,body,fpbank,out=0x2400000,0x2400200,0x2401000,0x2401100,0x2402000,0x2410000,0x2408000
     state=bytearray(0x2200);put=lambda off,v:struct.pack_into('<I',state,off,v&0xffffffff)
     owner=(0x1234<<16)|hand
     put(0x48,owner);put(0x58,view);struct.pack_into('<h',state,0x20,-1)
     struct.pack_into('<H',state,0x202,node&65535);state[0x208:0x23c]=matrix()
     put(0x1044,headers)
     for n in (0,1):
      put(0x1108+n*12,body+n*0x800)
     # Separate real object bodies and WORLD node banks.
     bodies=bytearray(0x1000)
     for n in (0,1):
      struct.pack_into('<h',bodies,n*0x800+0x116,0x400)
      for j in range(4):bodies[n*0x800+0x400+j*52:n*0x800+0x400+(j+1)*52]=matrix()
     fp=bytearray(0x8330)
     for v in range(4):
      for h in (0,1):
       at=v*0x20cc+h*0x1010;struct.pack_into('<I',fp,at+0x10,(0x1234<<16)|h)
       for j in range(64):fp[at+0x314+j*52:at+0x314+(j+1)*52]=matrix()
      fp[v*0x20cc+0x2060:v*0x20cc+0x2060+52]=matrix()
     stack=bytearray(0x400);struct.pack_into('<II',stack,0x200,0x3000000,mode)
     banks=[(effect,bytes(state[:0x1200])),(body,bytes(bodies)),(fpbank+max(view,0)*0x20cc,bytes(fp[max(view,0)*0x20cc:(max(view,0)+1)*0x20cc])),(out,bytes(52)),(0x2800000,bytes(stack)),(0x4ecbd0,struct.pack('<I',pool)),(0x4f6498,struct.pack('<I',fpbank))]
     for at,data in banks:u.mem_write(at,data)
     gp=[0x72001000+n*0x1000 for n in range(8)];gp[3]=location;gp[4]=0x2800200;gp[6]=effect;gp[7]=out
     for reg,value in zip(regs,gp):u.reg_write(reg,value)
     for n in range(8):u.reg_write(R.UC_X86_REG_XMM0+n,0)
     for reg,value in ((R.UC_X86_REG_EFLAGS,0x202),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80)):u.reg_write(reg,value)
     trace.clear();u.emu_start(0x17ba50,0x3000000,count=10000);assert u.reg_read(R.UC_X86_REG_EIP)==0x3000000
     rows.append(dict(entry_gp=gp,exit_gp=[u.reg_read(reg)for reg in regs],eflags=u.reg_read(R.UC_X86_REG_EFLAGS),exit_xmm=[f'{u.reg_read(R.UC_X86_REG_XMM0+n):032x}'for n in range(8)],instructions=trace.copy(),initial_banks=[dict(address=at,hex=data.hex())for at,data in banks],final_banks=[dict(address=at,hex=bytes(u.mem_read(at,len(data))).hex())for at,data in banks],view=view,mode=mode,node=node,hand=hand))
 fixture=Path('tests/fixtures/effect_location_resolver_tu15_original.json');fixture.write_text(json.dumps(dict(entry=0x17ba50,original_sha256=NEW_PIN,cases=rows),separators=(',',':'))+'\n')
 Path('analysis/weapon-effects-20261013/live-tu15/effect-location-resolver-oracle.json').write_text(json.dumps(dict(cases=len(rows),original_sha256=NEW_PIN,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),instruction_patches=[],replaced_services=[],scope=__doc__),indent=2)+'\n')
 print('Whole original effect location resolver',len(rows),'cases')
if __name__=='__main__':main()
