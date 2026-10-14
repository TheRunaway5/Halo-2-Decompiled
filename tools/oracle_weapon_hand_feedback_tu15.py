#!/usr/bin/env python3
"""Execute original registration/action/timer and opposite-hand blend owners.

TU1.1 and TU1.5 code is unchanged. Raw object/player/local-view/time storage
is declared input. FP notifications and unrelated unit action requests are
explicit unavailable boundaries; no rendered view or full unit world is run.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import (Trace,ROOT,POOL,FPS,USERS,PLAYERS,PLAYER_DATA,UNIT,WI,UI,TIME)
from weapon_reference_versions import images
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_XMM0

CORRESPONDENCE=ROOT/'analysis/weapons-tu15-goal-20261013/hand-feedback-correspondence'
class Machine(Trace):
 def __init__(self,later,report):
  self.reached={};self.callbacks=[]
  super().__init__(1)
  mapping={int(row['old'],16):int(row['u5'],16) for row in report['owners']}
  self.action=mapping[0x10a140] if later else 0x10a140
  self.action_symbol=mapping[0x1680b3] if later else 0x1680b3
  self.blend=mapping[0xd2940] if later else 0xd2940
  notifications=next(row for row in report['owners'] if row['old']=='000efcc0')['calls']
  targets={int(c['old_target'],16):int(c['new_target'],16) for c in notifications}
  self.supplied={targets[0x16a356] if later else 0x16a356:12,targets[0xe7ef0] if later else 0xe7ef0:4}
  old_globals={0x4e78d0:POOL,0x4f01f0:USERS,0x4f01f4:PLAYERS,0x4f1198:FPS,0x518444:TIME}
  operands={int(m['old_va'],16):int(m['new_va'],16) for owner in report['owners'] for m in owner['memory']}
  self.globals={operands[k] if later else k:v for k,v in old_globals.items()}
  if later:
   self.image=next(image for label,image,_ in images() if label=='TU1.5')
   for section in self.image.sections:self.g.mem_write(section.va,self.image.section_bytes(section))
  for address,value in self.globals.items():self.w(address,value)
  old=(CORRESPONDENCE/'instructions/000c6c50-old.asm').read_text().splitlines()
  new=(CORRESPONDENCE/'instructions/000c6c50-u5.asm').read_text().splitlines();assert len(old)==len(new)
  addresses={int(a.split()[0],16):int(b.split()[0],16) for a,b in zip(old,new)}
  self.decay=(addresses[0xc6cd8],addresses[0xc6d06]) if later else (0xc6cd8,0xc6d06)
 def hook(self,g,address,size,user):
  self.reached.setdefault(address,bytes(g.mem_read(address,size)).hex())
  if address in self.supplied:
   sp=g.reg_read(UC_X86_REG_ESP);pop=self.supplied[address];self.callbacks.append(address)
   g.reg_write(UC_X86_REG_EAX,0);g.reg_write(UC_X86_REG_ESP,sp+4+pop);g.reg_write(UC_X86_REG_EIP,self.u(sp))


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--fixture',type=Path,required=True);a=p.parse_args()
 report=json.loads((CORRESPONDENCE/'report.json').read_text());all_rows=[];images_=dict(images=[])
 for later in (False,True):
  t=Machine(later,report);rows=[];counts={'action':0,'blend':0,'decay':0,'mapping':0}
  for registered in (False,True):
   for owner in (False,True):
    for secondary in (False,True):
     for rate in (-32768,-1,0,1,30,60,120,255,32767):
      for action in range(18):
       t.g.mem_write(FPS,b'\xff'*0x8000);t.w(FPS+0x10,WI if registered else 0xffffffff)
       t.w(USERS+12,0);t.w(PLAYER_DATA+0x2c,UI if owner else 0xffffffff)
       t.g.mem_write(UNIT+0x212,bytes([255 if secondary else 0,255]));t.w(UNIT+0x218,WI)
       t.sh(TIME+2,rate);t.g.mem_write(UNIT+0x1fa,bytes([1,255]))
       #10A140's weapon is EBX, its numeric action is the stack argument.
       from unicorn.x86_const import UC_X86_REG_EBX
       t.run(t.action,(action,),((UC_X86_REG_EBX,WI),))
       after=[t.u(UNIT+0x1fa,1),t.u(UNIT+0x1fb,1)]
       rows.append(' '.join(['A',*(f'{v:08x}' for v in (action,int(registered and owner),int(secondary),rate&65535,1,255,*after))]));counts['action']+=1
  for present in range(4):
   for primary in (False,True):
    for dt in (0,0x3d088889,0xbd088889,0x3c888889,0x3f800000,0x7fc00000,0x7f800000,0xff800000):
     for timer in range(256):
      before=[timer,255-timer];t.g.mem_write(UNIT+0x1fa,bytes(before));t.w(TIME+4,dt)
      t.g.mem_write(UNIT+0x212,bytes([0 if primary else 255,255]));t.w(UNIT+0x218,WI)
      t.run(t.blend,(),((UC_X86_REG_EAX,UI if present&1 else 0xffffffff),(UC_X86_REG_ESI,WI if present&2 else 0xffffffff)))
      result=t.g.reg_read(UC_X86_REG_XMM0)&0xffffffff
      rows.append(' '.join(['B',*(f'{v:08x}' for v in (int(bool(present&1)),int(bool(present&2)),int(primary),dt,*before,result))]));counts['blend']+=1
  for timer in range(256):
   for other in (0,1,255,255-timer):
    t.g.mem_write(UNIT+0x1fa,bytes([timer,other]));t.g.reg_write(UC_X86_REG_EBP,UNIT);t.g.emu_start(*t.decay,count=50)
    rows.append(' '.join(['T',*(f'{v:08x}' for v in (timer,other,t.u(UNIT+0x1fa,1),t.u(UNIT+0x1fb,1)))]));counts['decay']+=1
  for action in range(41):
   t.run(t.action_symbol,(),((UC_X86_REG_ECX,action),));symbol=t.g.reg_read(UC_X86_REG_EAX)
   rows.append(f'M {action:08x} {symbol:08x}');counts['mapping']+=1
  if all_rows:assert rows==all_rows
  else:all_rows=rows
  for address,raw in t.reached.items():assert t.image.read(address,len(bytes.fromhex(raw)))==bytes.fromhex(raw)
  images_['images'].append(dict(xbe_sha256=hashlib.sha256(t.image.data).hexdigest(),counts=counts,globals={f'{k:08x}':f'{v:08x}' for k,v in t.globals.items()},supplied_boundaries={f'{k:08x}':v for k,v in t.supplied.items()},reached_instruction_bytes={f'{k:08x}':v for k,v in sorted(t.reached.items())}));print(counts,flush=True)
 a.fixture.write_text('\n'.join(all_rows)+'\n');a.output.write_text(json.dumps(dict(scope=__doc__,**images_,fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),correspondence_sha256=hashlib.sha256((CORRESPONDENCE/'report.json').read_bytes()).hexdigest(),instruction_patches=[],image_results_equal=True),indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
