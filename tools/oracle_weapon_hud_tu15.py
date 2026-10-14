#!/usr/bin/env python3
"""Original TU1.1/TU1.5 weapon status projection, HUD inputs and masks.

Actual authored weapon/magazine/trigger bytes plus declared object/inventory/
clock/state storage are inputs. Infinite-ammo, lock-query and appearance
results are explicit supplied boundaries; no real lock target, marker/model,
rendered HUD or complete world is asserted. All reached code is unchanged.
"""
import argparse,hashlib,json,random,struct
from pathlib import Path
from oracle_weapon_reserve_tu15 import Original as Reserve,OBJECTS,WI,UI
from oracle_weapon_barrel_timing_tu15 import WEAPON,UNIT,DEF,BARRELS,STACK,STOP,HEADERS,TAGS,TIME,pack,u32
from weapon_reference_versions import images
from unicorn.x86_const import *

ROOT=Path(__file__).resolve().parents[1]
ITEM,STATUS,VALUES,SELECTORS,TAG_OUT,MASKS,POINTERS,GAME=(0x1080000+n*0x1000 for n in range(8))
class Original(Reserve):
 def __init__(self,label,image,report):
  super().__init__(label,image);mapping={int(o['old'],16):int(o['u5'],16) for o in report['owners']}
  choose=lambda old:mapping[old] if label=='TU1.5' else old
  self.project=choose(0x13c719);self.inputs=choose(0x13c4b8);self.builder=choose(0x13b7c0);self.visible=choose(0x13c3f7)
  owner=next(o for o in report['owners'] if o['old']=='001046d0');self.lock_entry=int(next(c['new_target'] if label=='TU1.5' else c['old_target'] for c in owner['calls'] if c['old_target']=='0010a410'),16)
  owner=next(o for o in report['owners'] if o['old']=='0013b7c0');self.appearance=int(owner['calls'][0]['new_target'] if label=='TU1.5' else owner['calls'][0]['old_target'],16)
  global_=next(m for m in owner['memory'] if m['old_va']=='004edf18');self.g.mem_write(int(global_['new_va'] if label=='TU1.5' else global_['old_va'],16),pack([GAME]));self.g.mem_write(GAME+8,pack([2]));self.acquire=False
 def audit(self,g,address,size,data):
  super().audit(g,address,size,data)
  if address not in (self.lock_entry,self.appearance):return
  sp=g.reg_read(UC_X86_REG_ESP);pop=8 if address==self.lock_entry else 0
  g.reg_write(UC_X86_REG_EAX,int(self.acquire) if address==self.lock_entry else 0);g.reg_write(UC_X86_REG_ESP,sp+4+pop);g.reg_write(UC_X86_REG_EIP,u32(g.mem_read(sp,4)))
 def call(self,entry,args=(),regs=()):
  g=self.g;g.mem_write(STACK,pack([STOP,*args]));g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80);g.reg_write(UC_X86_REG_FPSW,0);g.reg_write(UC_X86_REG_FPTAG,0xffff)
  for register,value in regs:g.reg_write(register,value)
  g.emu_start(entry,STOP,count=100000);assert g.reg_read(UC_X86_REG_EIP)==STOP
 def execute_hud(self,root,mags,triggers,control,states,slots,mismatch):
  g=self.g;has_unit,infinite,heat,age,charge,flags,input_,mode,trigger_state,trigger_ticks,rate,acquire,thresholds=control
  #Reuse the independent reserve ABI setup, retaining all actual magazine records.
  maxima=[struct.unpack_from('<h',m,0xc)[0] for m in mags];words=[0,len(mags),0,has_unit,infinite,0,0,mismatch,*slots,*(maxima+[0]*(2-len(maxima))),*states]
  if mags:self.execute(root,mags,words)
  else:
   fake=bytearray(0x5c);fake_root=bytearray(root);struct.pack_into('<I',fake_root,0x2c0,1);self.execute(bytes(fake_root),[bytes(fake)],words[:1]+[1]+words[2:]);g.mem_write(DEF,root);g.mem_write(DEF+0x2c4,pack([BARRELS]))
  g.mem_write(DEF+0x2cc,pack([BARRELS+0x1800]))
  for n,t in enumerate(triggers):g.mem_write(BARRELS+0x1800+n*0x40,t)
  g.mem_write(WEAPON+0x180,pack([heat,age]));g.mem_write(WEAPON+0x175,bytes([charge]));g.mem_write(WEAPON+0x16c,struct.pack('<HH',flags,input_));g.mem_write(WEAPON+0x224,struct.pack('<h',mode));g.mem_write(WEAPON+0x20c,bytes([trigger_state]));g.mem_write(WEAPON+0x20e,struct.pack('<h',trigger_ticks));g.mem_write(WEAPON+0x194,pack([0xffffffff]));g.mem_write(WEAPON+0x177,b'\xff');g.mem_write(TIME+2,struct.pack('<h',rate));g.mem_write(TIME+4,pack([0x3d088889]));g.mem_write(TIME+8,pack([300]));self.acquire=bool(acquire)
  reserve=self.call_reserve() if mags else 0
  loaded=struct.unpack('<h',g.mem_read(WEAPON+0x22c,2))[0]
  inputs=[WI,heat,age,charge,flags,input_,int(bool(mags)),mode&65535,loaded&65535,struct.unpack_from('<H',mags[0],10)[0] if mags else 0,struct.unpack_from('<H',mags[0],12)[0] if mags else 0,reserve,int(bool(triggers)),struct.unpack_from('<H',triggers[0],6)[0] if triggers else 0,struct.unpack_from('<H',triggers[0],0x18)[0] if triggers else 0,trigger_ticks&65535,rate&65535,trigger_state,acquire]
  g.mem_write(ITEM,b'\xa5'*56);self.call(self.project,(TAG_OUT,),((UC_X86_REG_EAX,ITEM),(UC_X86_REG_ECX,WI)));projected=bytes(g.mem_read(ITEM,56));assert u32(g.mem_read(TAG_OUT,4))==u32(root,0x2b4)
  result=[]
  for start in (48,52,56):
   codes=[start+i if start+i<=57 else 2 for i in range(4)];g.mem_write(SELECTORS,bytes(codes));self.call(self.inputs,(0,SELECTORS,VALUES),((UC_X86_REG_ECX,ITEM),(UC_X86_REG_EDX,STATUS)));result.extend(struct.unpack('<4I',g.mem_read(VALUES,16)))
  elapsed=result[10];inputs.append(elapsed);result=result[:10]
  masks=[]
  for type_ in range(6):
   g.mem_write(STATUS,bytes(0x300))
   for offset in (0x50,0x88,0xc0,0xf8):g.mem_write(STATUS+offset,projected)
   self.call(self.builder,(0,type_,POINTERS,POINTERS+4,POINTERS+8,POINTERS+12,POINTERS+16),((UC_X86_REG_ESI,STATUS),));masks.append(int.from_bytes(g.mem_read(POINTERS+12,2),'little'))
  visible=[]
  for bit in (1,8,0x10,0x20,0x40,0x80,0x100,0x200,0x400):
   g.mem_write(MASKS,struct.pack('<8H3B',0,0,bit,0,0,0,0,0,*thresholds));self.call(self.visible,(0,0,0),((UC_X86_REG_EAX,MASKS),(UC_X86_REG_ECX,ITEM),(UC_X86_REG_EDX,masks[1])));visible.append(g.reg_read(UC_X86_REG_EAX)&255)
  return inputs+[0xa5a5a5a5]*14+list(struct.unpack('<14I',projected))+result+masks+list(thresholds)+visible
 def call_reserve(self):
  self.call(self.reserve_entries[0],(WI,0),((UC_X86_REG_EAX,0),));return self.g.reg_read(UC_X86_REG_EAX)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--scope',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--fixture',type=Path,required=True);a=p.parse_args()
 report=json.loads((ROOT/'analysis/weapons-tu15-goal-20261013/hud-correspondence/report.json').read_text());machines=[Original(label,image,report) for label,image,_ in images() if label in ('TU1.1','TU1.5')];scope=json.loads(a.scope.read_text());rng=random.Random(0x13da89);definitions=[];seen=set();lines=[];count=0
 for map_ in scope['maps']:
  for w in map_['weapons']:
   if not w['resident']:continue
   root=bytes.fromhex(w['root_hex']);mags=[bytes.fromhex(m['raw_hex']) for m in w['magazines']];triggers=[bytes.fromhex(t['raw_hex']) for t in w['triggers']]
   if len(mags)>2:continue
   key=(w['tag']['name'],tuple((struct.unpack_from('<h',m,10)[0],struct.unpack_from('<h',m,12)[0]) for m in mags),tuple(struct.unpack_from('<h',t,6)[0] for t in triggers))
   if key in seen:continue
   seen.add(key);definitions.append(dict(map=map_['path'],tag=w['tag'],root_hex=root.hex(),magazines_hex=[m.hex() for m in mags],triggers_hex=[t.hex() for t in triggers]))
   capacity=struct.unpack_from('<h',mags[0],10)[0] if mags else 0
   for n in range(64):
    reserves=(117,116,0,0,0) if n==0 else rng.choice(((0,0,0,0,0),(1,2,3,4,5),(32767,32767,32767,32767,32767),(-1,0,1,-2,3)))
    loaded=capacity if n==0 else rng.choice((0,1,max(0,capacity-1),-1,32767));states=[]
    for reserve in reserves:
     raw=bytearray(32)
     for magazine in range(2):struct.pack_into('<3h',raw,magazine*16+6,reserve,loaded,0)
     states.extend(struct.unpack('<8I',raw))
    heat=rng.choice((0,0x3f000000,0x3f800000));age=rng.choice((0,0x3f000000,0x3f800000,0x3fa00000))
    control=(1 if n==0 else rng.randrange(2),0 if n==0 else rng.randrange(2),heat,age,rng.choice((0,128,255)),rng.choice((0,2,6,0xffff)),rng.choice((0,2,0x20)),rng.choice((-1,0,1,6,7)),rng.choice((0,2,5,6)),rng.choice((-1,0,7,15,30)),rng.choice((0,30,60,-1)),rng.randrange(2),(rng.choice((0,1,255)),rng.choice((0,1,60,255)),rng.choice((0,1,120,255))))
    slots=(1,0xffffffff,0xffffffff,0xffffffff) if n==0 else rng.choice(((1,2,3,4),(0,0xffffffff,1,2),(1,1,1,1),(0xffffffff,)*4));mismatch=0 if n==0 else rng.randrange(32)
    values=[m.execute_hud(root,mags,triggers,control,states,slots,mismatch) for m in machines];assert values[0]==values[1],(key,n,values)
    assert len(values[0])==76;lines.append(' '.join(f'{v&0xffffffff:08x}' for v in values[0]));count+=1
 a.fixture.write_text('\n'.join(lines)+'\n');a.output.write_text(json.dumps(dict(scope=__doc__,definitions=definitions,cases=count,fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),scope_sha256=hashlib.sha256(a.scope.read_bytes()).hexdigest(),instruction_patches=[],image_results_equal=True,images=[dict(label=m.label,xbe_sha256=hashlib.sha256(m.image.data).hexdigest(),supplied_service_addresses=[f'{m.infinite_entry:08x}',f'{m.lock_entry:08x}',f'{m.appearance:08x}'],reached_instruction_bytes={f'{k:08x}':v for k,v in sorted(m.instructions.items())}) for m in machines]),indent=2,sort_keys=True)+'\n');print('definitions',len(definitions),'cases',count,flush=True)
if __name__=='__main__':main()
