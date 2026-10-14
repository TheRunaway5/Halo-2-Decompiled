#!/usr/bin/env python3
"""Execute the unchanged complete per-trigger phase in both original updates.

The entry is the declared102833/relocated caller ABI after magazine update;
the exit is1031F0 before the next trigger. Raw caller locals, trigger/barrel/
analog/target stores and clock are explicit inputs. Barrel admission and
acquisition/tracking results are supplied predicates. Effect allocation,
zoom/action notifications, effect stop and weapon destruction are captured
unavailable owners. All trigger branches/helpers and real CRT run unchanged.
This is not whole-world scheduling, real target acquisition or native gameplay parity.
"""
import argparse,hashlib,json,random,struct
from pathlib import Path
from oracle_weapon_trigger_tu15 import Machine,ROOT,WEAPON,DEF,TRIGGER,TIME,WI,UI,POOL,TAGS,sha
from oracle_smg_firing_1_1 import STACK,GAME,BARREL
from unicorn.x86_const import *
CORRESPONDENCE=ROOT/'analysis/weapons-tu15-goal-20261013/trigger-correspondence-v2'
class PhaseMachine(Machine):
 def __init__(self,later,report):
  super().__init__(later,report);self.phase_calls={};self.predicate_flags=0;self.gate_mask=0
  calls={int(c['old_target'],16):int(c['new_target'],16) for o in report['owners'] for c in o['calls']}
  self.phase_calls={calls[a] if later else a:(kind,pop) for a,kind,pop in
      ((0x1066f0,5,12),(0x178cd0,6,4),(0x10a410,7,8),(0x10a350,8,8),(0xb9430,9,4))}
  old=(CORRESPONDENCE/'instructions/001021d0-old.asm').read_text().splitlines();new=(CORRESPONDENCE/'instructions/001021d0-u5.asm').read_text().splitlines();assert len(old)==len(new)
  mapping={int(a.split()[0],16):int(b.split()[0],16) for a,b in zip(old,new)}
  self.begin=mapping[0x102833] if later else 0x102833;self.end=mapping[0x1031f0] if later else 0x1031f0
  operands={int(m['old_va'],16):int(m['new_va'],16) for o in report['owners'] for m in o['memory']}
  self.w(operands[0x4edf18] if later else 0x4edf18,GAME)
 def hook(self,g,address,size,user):
  super().hook(g,address,size,user)
  if address not in getattr(self,'phase_calls',{}):return
  kind,pop=self.phase_calls[address];sp=g.reg_read(UC_X86_REG_ESP);args=[self.u(sp+4+n*4) for n in range(pop//4)]
  if kind==5:
   assert args[0]==WI and args[1]<2 and args[2]==0,args
   self.calls.append([kind,args[1],args[2]]);result=bool(self.gate_mask&(1<<args[1]))
  elif kind==6:self.calls.append([kind,g.reg_read(UC_X86_REG_EDI),*args]);result=0
  elif kind in (7,8):
   self.calls.append([kind]);result=bool(self.predicate_flags&(1<<(kind-7)))
   self.w(args[0],self.query_datum);self.g.mem_write(args[1],bytes([self.query_marker]))
  else:assert args==[WI];self.calls.append([kind]);result=0
  g.reg_write(UC_X86_REG_EAX,int(result));g.reg_write(UC_X86_REG_ESP,sp+4+pop);g.reg_write(UC_X86_REG_EIP,self.u(sp))
 def phase(self,slot,rate,count,item_flags,owner,spent,allocated,definition,state,barrels,analog,
      context,target,marker,gate_mask,predicates,query,query_marker,detonation):
  #Same declared raw object ABI as the independently compared transition owners.
  self.g.mem_write(WEAPON,bytes(0x400));self.w(WEAPON,0);self.w(TAGS+8,DEF)
  self.w(DEF+0x2cc,TRIGGER);self.w(DEF+0x2d0,count);self.w(DEF+0x2d4,BARREL);self.w(DEF+0x180,detonation)
  self.g.mem_write(TRIGGER+slot*64,definition);self.g.mem_write(WEAPON+0x20c+slot*12,state)
  self.g.mem_write(WEAPON+0x1a4,barrels);self.g.mem_write(WEAPON+0x170,analog)
  self.g.mem_write(WEAPON+0x12c,bytes([item_flags]));self.w(WEAPON+0x154,owner);self.w(WEAPON+0x184,spent)
  self.w(WEAPON+0x194,target);self.g.mem_write(WEAPON+0x177,bytes([marker]));self.sh(WEAPON+0x16e,0x40 if context&8 else 0)
  self.sh(TIME+2,rate);self.g.mem_write(GAME+12,bytes([4 if context&64 else 1]));self.allocated=allocated;self.calls=[]
  self.gate_mask=gate_mask;self.predicate_flags=predicates;self.query_datum=query;self.query_marker=query_marker
  self.g.mem_write(STACK,bytes(0x100));self.g.mem_write(STACK+0x16,bytes([bool(context&4)]));self.w(STACK+0x28,slot)
  self.w(STACK+0x2c,DEF);self.w(STACK+0x30,0 if context&32 else 0xffffffff);self.w(STACK+0x34,WEAPON);self.w(STACK+0x4c,WI)
  selector=struct.unpack_from('<h',definition,4)[0];assert 0<=selector<=2
  self.g.mem_write(STACK+0x1c+selector,bytes([bool(context&1)]));self.g.mem_write(STACK+0x20+selector,bytes([bool(context&2)]))
  g=self.g;g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_EAX,slot);g.reg_write(UC_X86_REG_EBP,WEAPON);g.reg_write(UC_X86_REG_EDI,DEF)
  g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80);g.reg_write(UC_X86_REG_FPSW,0);g.reg_write(UC_X86_REG_FPTAG,0xffff)
  before=bytes(g.mem_read(WEAPON,0x400));g.emu_start(self.begin,self.end,count=100000);assert g.reg_read(UC_X86_REG_EIP)==self.end
  after=bytes(g.mem_read(WEAPON,0x400));normalized=bytearray(after)
  for start,size in ((0x20c+slot*12,12),(0x1a4,104),(0x170,5),(0x194,4),(0x177,1)):normalized[start:start+size]=before[start:start+size]
  assert bytes(normalized)==before,'Unexpected source store outside per-trigger phase contract'
  return [after[0x20c+slot*12:0x218+slot*12].hex(),after[0x1a4:0x20c].hex(),after[0x170:0x175].hex(),self.u(WEAPON+0x194),self.u(WEAPON+0x177,1),self.u(STACK+0x13,1),self.calls]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--scope',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--fixture',type=Path,required=True);a=p.parse_args()
 report=json.loads((CORRESPONDENCE/'report.json').read_text());machines=[PhaseMachine(later,report) for later in (False,True)]
 scope=json.loads(a.scope.read_text());contracts={}
 for map_ in scope['maps']:
  for weapon in map_['weapons']:
   if not weapon['resident']:continue
   for trigger in weapon['triggers']:
    raw=trigger['raw_hex'];count=len(weapon['barrels']);detonation=struct.unpack_from('<I',bytes.fromhex(weapon['root_hex']),0x180)[0]
    contracts.setdefault((raw,count,detonation),dict(weapon=weapon['tag']['name'],map=map_['path'],definition=raw,barrels=count,detonation=detonation))
 rng=random.Random(0x1030f0);rows=[];histogram={};float_controls=(0,0x3f000000,0x3f800000,0x3fc00000,0x7fc00000)
 def add(definition,count,detonation,n):
  slot=n&1;rate=(-1,0,1,30,60,32767)[n%6];item_flags=n&1;owner=UI if n%3 else 0xffffffff
  spent=float_controls[n%len(float_controls)];allocated=(0xffffffff,0x81234567)[n%2]
  mode=n%8;state=bytes([mode,0xa5])+struct.pack('<hII',rng.choice((-1,0,1,2,30)),rng.getrandbits(8),rng.choice((0xffffffff,0x81234567)))
  barrels=bytearray(rng.randbytes(104))
  for i in range(2):barrels[i*52+1]=rng.randrange(4)
  analog=bytes((rng.choice((0,1,128,249,250,255)),rng.choice((0,1,128,255)),rng.randrange(256),rng.choice((0,1,2,255)),rng.choice((0,1,127,128,255))))
  context=rng.randrange(128);target=rng.choice((0xffffffff,0x70001));marker=rng.randrange(256);gate_mask=rng.randrange(4);predicates=rng.randrange(4);query=rng.choice((target,0x70002));query_marker=rng.randrange(256)
  values=(slot,rate,count,item_flags,owner,spent,allocated,definition,state,bytes(barrels),analog,context,target,marker,gate_mask,predicates,query,query_marker,detonation)
  results=[m.phase(*values) for m in machines];assert results[0]==results[1],(n,results)
  after,after_barrels,after_analog,after_target,after_marker,dirty,calls=results[0]
  fields=['P',*(f'{v&0xffffffff:08x}' for v in (slot,slot,rate&65535,count,item_flags,owner,spent,allocated)),definition.hex(),state.hex(),bytes(barrels).hex(),analog.hex(),*(f'{v&0xffffffff:08x}' for v in (context,target,marker,gate_mask,predicates,query,query_marker,detonation,after_target,after_marker)),after,after_barrels,after_analog,f'{dirty:08x}',str(len(calls))]
  for call in calls:fields.extend([str(len(call)),*(f'{v&0xffffffff:08x}' for v in call)])
  rows.append(' '.join(fields));key=f'{struct.unpack_from("<h",definition,6)[0]}/{mode}';histogram[key]=histogram.get(key,0)+1
 for (raw,count,detonation) in contracts:
  definition=bytes.fromhex(raw)
  for n in range(96):add(definition,count,detonation,n)
 base=bytearray(bytes.fromhex(next(iter(contracts))[0]))
 for n in range(4096):
  struct.pack_into('<h',base,6,n%6);struct.pack_into('<hh',base,0x18,rng.randrange(4),rng.randrange(4));struct.pack_into('<h',base,0x24,rng.randrange(3));struct.pack_into('<f',base,0x2c,rng.choice((0.,.2)))
  add(bytes(base),2,0xffffffff,n)
 a.fixture.write_text('\n'.join(rows)+'\n')
 for m in machines:
  for address,raw in m.instructions.items():assert m.image.read(address,len(bytes.fromhex(raw)))==bytes.fromhex(raw)
 data=dict(schema='original-weapon-trigger-phase-v1',scope=__doc__,cases=len(rows),contracts=list(contracts.values()),behavior_and_mode_histogram=histogram,fixture_sha256=sha(a.fixture),tool_sha256=sha(__file__),base_oracle_sha256=sha(ROOT/'tools/oracle_weapon_trigger_tu15.py'),correspondence_sha256=sha(CORRESPONDENCE/'report.json'),instruction_patches=[],image_results_equal=True,images=[dict(xbe_sha256=hashlib.sha256(m.image.data).hexdigest(),begin=f'{m.begin:08x}',end=f'{m.end:08x}',supplied_services={f'{k:08x}':v for k,v in (m.supplied|m.phase_calls).items()},reached_instruction_bytes={f'{k:08x}':v for k,v in sorted(m.instructions.items())}) for m in machines])
 a.output.write_text(json.dumps(data,indent=2,sort_keys=True)+'\n');print('phase cases',len(rows),'contracts',len(contracts),flush=True)
if __name__=='__main__':main()
