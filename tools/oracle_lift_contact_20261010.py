#!/usr/bin/env python3
"""Execute complete original lift admission gates with explicit raw inputs.

The component/object/tag/phantom packets are declared gate controls. No
collision, contact membership, body force or world result is supplied/proven.
"""
import argparse,hashlib,itertools,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
BASE=0x1000000;POOL=BASE;COMPONENTS=BASE+0x1000;OBJECTPOOL=BASE+0x2000;HEADERS=BASE+0x3000
OWN=BASE+0x4000;OTHER=BASE+0x5000;TAGS=BASE+0x6000;DEFINITION=BASE+0x7000
MODEL=BASE+0x8000;PHMO=BASE+0x9000;SHAPES=BASE+0xa000;RULES=BASE+0xb000
STACK=BASE+0xd000;STOP=BASE+0xe000
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 inputs=[]
 for kind,control,player,flags,size,limits in itertools.product((0,1,2,6,7,11,15,31),(0,4,255),(0,1),
     (0,8|16,0x08000018,*[1<<i for i in range(32)]),(-128,-1,0,1,2,127),((-128,127),(-1,1),(2,1),(0,0))):
  inputs.append([flags,*limits,size,kind,control,player,0])
 inputs.append([0,-128,127,0,0,0,0,1]) # NONE counterpart: no pointer chase.
 banks=[];fixture=['H2_LIFT_CONTACT_V1',str(len(inputs))]
 for label,path,pin,entry,slots in [('1.1',ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',PIN,0x1d56f0,(0x526248,0x4e78d0,0x4eb114)),
     ('u5',NEW_DEFAULT,NEW_PIN,0x1d9fb0,(0x52be5c,0x4ecbd0,0x4f0414))]:
  assert sha(path)==pin;x=Xbe(path);g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
  for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
  g.mem_map(BASE,0x10000)
  def w(p,v):g.mem_write(p,struct.pack('<I',v&0xffffffff))
  for p,v in ((slots[0],POOL),(slots[1],OBJECTPOOL),(slots[2],TAGS),(POOL+0x44,COMPONENTS),
      (OBJECTPOOL+0x44,HEADERS),(COMPONENTS+8,0x12340000),(COMPONENTS+0xa8,0x12350001),
      (HEADERS+8,OWN),(HEADERS+20,OTHER),(OWN,0x99990000),(TAGS+8,DEFINITION),(TAGS+24,MODEL),(TAGS+40,PHMO),
      (DEFINITION+0x38,0x99990001),(MODEL+0x24,0x99990002),(PHMO+0x44,SHAPES),(PHMO+0x2c,RULES),(SHAPES+8,0)):w(p,v)
  outputs=[]
  for row in inputs:
   flags,minimum,maximum,size,kind,control,player,none=row
   w(RULES,flags);g.mem_write(RULES+4,struct.pack('<bb',minimum,maximum));g.mem_write(COMPONENTS+0xbc,struct.pack('<b',size))
   g.mem_write(HEADERS+15,bytes([kind]));g.mem_write(OTHER+0x10a,bytes([control]));w(OTHER+0x13c,0x12345678 if player else 0xffffffff)
   w(STACK,STOP)
   for r,v in ((R.UC_X86_REG_ESP,STACK),(R.UC_X86_REG_EAX,0),(R.UC_X86_REG_ECX,0xabcd0000),(R.UC_X86_REG_EDX,0xffffffff if none else 0xbcde0001)):
    g.reg_write(r,v)
   g.emu_start(entry,STOP,count=500);assert g.reg_read(R.UC_X86_REG_EIP)==STOP
   outputs.append(g.reg_read(R.UC_X86_REG_EAX)&255)
  assert all(v in(0,1)for v in outputs)
  banks.append(dict(version=label,image_sha256=pin,cases=len(inputs),allowed=sum(outputs)))
  if label=='1.1':reference=outputs
  else:assert outputs==reference
 for row,result in zip(inputs,reference):fixture.append(' '.join(map(str,(*row,result))))
 p=a.output/'fixture.txt';p.write_text('\n'.join(fixture)+'\n')
 (a.output/'report.json').write_text(json.dumps(dict(tool_sha256=sha(__file__),fixture_sha256=sha(p),banks=banks,boundary=__doc__,native_live_integration=False),sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(executions=len(inputs)*2,allowed=sum(reference),fixture_sha256=sha(p))))
if __name__=='__main__':main()
