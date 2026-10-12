#!/usr/bin/env python3
"""Original vehicle constructor/reset execution with explicit cold inputs.

Runs F01C0/EFFF0 and the unparented/componentNONE BADE0/BACC0 point
producer. Animation admission, BAA80 and1D1940 are declared external test
services. No vehicle allocation, body construction, seats or driving is proven.
"""
import argparse,hashlib,itertools,json,random,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
BASE=0x1000000;POOL=BASE;HEADERS=BASE+0x1000;BODY=BASE+0x2000
TAGS=BASE+0x3000;DEFINITION=BASE+0x4000;MODEL=BASE+0x5000;STACK=BASE+0xd000;STOP=BASE+0xe000
UNIT=0xa1230000
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def cases():
 rng=random.Random(0xf01c0);rows=[]
 for phmo,flags,offset,admission,seed in itertools.product((0xffffffff,0x99880002),(0,1,255),(-1,0x500,-128,0),(0,1),range(4)):
  body=bytearray(rng.randbytes(0x600));struct.pack_into('<I',body,0,0xb1230000)
  struct.pack_into('<I',body,0x14,0xffffffff);struct.pack_into('<I',body,0xb4,0xffffffff)
  struct.pack_into('<3f',body,0x64,1.25,-2.75,3.5);struct.pack_into('<h',body,0x12a,offset)
  rows.append(dict(phmo=phmo,flags=flags,offset=offset,admission=admission,body_hex=body.hex()))
 return rows
def run(version,path,pin,inputs):
 assert sha(path)==pin;x=Xbe(path);g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
 for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
 g.mem_map(BASE,0x10000)
 def w(p,v):g.mem_write(p,struct.pack('<I',v&0xffffffff))
 def u(p):return struct.unpack('<I',g.mem_read(p,4))[0]
 def ret(clean=0,result=None):
  sp=g.reg_read(R.UC_X86_REG_ESP);target=u(sp);g.reg_write(R.UC_X86_REG_ESP,sp+4+clean)
  if result is not None:g.reg_write(R.UC_X86_REG_EAX,result)
  g.reg_write(R.UC_X86_REG_EIP,target)
 later=version=='u5';entry=0xf10e0 if later else 0xf01c0
 for p,v in ((0x4ecbd0 if later else 0x4e78d0,POOL),(0x4f0414 if later else 0x4eb114,TAGS),
     (POOL+0x44,HEADERS),(HEADERS+8,BODY),(TAGS+8,DEFINITION),(TAGS+24,MODEL),(DEFINITION+0x38,0xc1230001)):w(p,v)
 services={0xbbbc0 if later else 0xbaa80:'baa80',0x111b10 if later else 0x110bf0:'animation',0x1d6200 if later else 0x1d1940:'reset'}
 state={}
 def hook(_g,p,_n,_user):
  if p not in services:return
  kind=services[p];sp=g.reg_read(R.UC_X86_REG_ESP)
  if kind=='baa80':
   assert g.reg_read(R.UC_X86_REG_EDX)&255==1;state['events'].append(['baa80',u(sp+4)]);ret(4)
  elif kind=='animation':
   assert g.reg_read(R.UC_X86_REG_EAX)==0
   args=list(struct.unpack('<7I',g.mem_read(sp+4,28)));state['events'].append(['animation',*args]);ret(28,state['case']['admission'])
  else:state['events'].append(['reset',(g.reg_read(R.UC_X86_REG_EDI)-BODY)&0xffffffff]);ret()
 def store(_g,_access,p,n,value,_user):
  if BODY<=p and p+n<=BODY+0x600:
   state['stores'].append([p-BODY,n,value&((1<<(8*n))-1)])
 g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,store);outputs=[]
 for row in inputs:
  state.update(case=row,events=[],stores=[]);g.mem_write(BODY,bytes.fromhex(row['body_hex']));w(MODEL+0x24,row['phmo']);g.mem_write(DEFINITION+0x2ac,bytes([row['flags']]))
  g.mem_write(STACK,struct.pack('<4I',STOP,UNIT,0,0));g.reg_write(R.UC_X86_REG_ESP,STACK)
  g.emu_start(entry,STOP,count=5000);assert g.reg_read(R.UC_X86_REG_EIP)==STOP and g.reg_read(R.UC_X86_REG_ESP)==STACK+16
  outputs.append(dict(result=g.reg_read(R.UC_X86_REG_EAX)&255,body_hex=bytes(g.mem_read(BODY,0x600)).hex(),events=state['events'],stores=state['stores']))
 return outputs
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 inputs=cases();old=run('1.1',ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',PIN,inputs);new=run('u5',NEW_DEFAULT,NEW_PIN,inputs);assert old==new
 p=a.output/'fixture.json';p.write_text(json.dumps(dict(schema=1,inputs=inputs,outputs=new),separators=(',',':'),sort_keys=True)+'\n')
 report=dict(tool_sha256=sha(__file__),fixture_sha256=sha(p),images=[PIN,NEW_PIN],cases_per_image=len(inputs),stores_per_image=sum(len(r['stores'])for r in old),boundary=__doc__,native_live_integration=False)
 (a.output/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
