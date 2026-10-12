#!/usr/bin/env python3
"""Original captured-attacker damage and retained player-slot lifecycle.

Whole amount, leave, requested allocation and birth owners execute in three
pinned Xbox images. Original D9180 caller prefixes stop at their actual amount
callee entry, with no supplied return. Backed pools, nonlocal creation packet,
clock, damage-event identity and live field values are declared inputs. The
separate TU1.1 object-callback lifecycle reuses the bounded original datum
oracle, including its explicitly supplied body-storage service and prefixes.
No patched instructions or invented invalid-context result is used.
"""
import argparse,json,re,struct,sys
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EDI,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_XMM0,UC_X86_REG_FPCW,UC_X86_REG_MXCSR
from oracle_damage_player_inputs import Inputs,CREATION,PG,H,T,U,NONE,R,G,M,P
from oracle_mp_full_scoreboard_1_5 import ROOT,HudCache,CACHE_PIN,literal,complete_tokens,references,sha
from oracle_mp_spawn_datums import DatumOracle,lifecycle_case,Cache,PLAYER_POOL
from oracle_game_variants import STACK,STOP

OUT=ROOT/'analysis/multiplayer/game-variants/damage-stale-attacker-20261011'
EVENT,OUTPUT=H+0x5000,H+0x5100
PATTERNS={
 'leave':rb'\x53\x56\x8b\xf0\xa1....\x8b\x48\x08\x83\xcb\xff\x83\xf9\x01',
 'allocate':rb'\x85\xff\x7c.\x3b\x7a\x20\x7d.\x8b\x4a\x44\x56\x8b\x72\x24\x0f\xaf\xf7',
 'amount':rb'\xf3\x0f\x10\x41\x10\x83\xec\x08\x0f\x2e\x41\x14\x56\x8b\xf0\x9f\xf6\xc4\x44',
 'damage_caller':rb'\x81\xec\xf4\x00\x00\x00\x8b\x0d....\x53\x8b\x9c\x24\x00\x01\x00\x00\x55\x8b\xac\x24\x00\x01\x00\x00',
 'captured_owner':rb'\x8b\x15....\x8b\x52\x44\x25\xff\xff\x00\x00\x8d\x04\x40\x8b\x44\x82\x08\x8b\x90\xc8\x00\x00\x00',
}
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def fields(g,at):
 raw=bytes(g.mem_read(at,0x21c))
 return dict(salt=struct.unpack_from('<H',raw)[0],membership=struct.unpack_from('<b',raw,0xc0)[0],
     strength=raw[0xc1],field194=struct.unpack_from('<i',raw,0x194)[0],inside=struct.unpack_from('<h',raw,0x1b8)[0],
     raw_fields_hex=dict(c0_c1=raw[0xc0:0xc2].hex(),field194=raw[0x194:0x198].hex(),inside=raw[0x1b8:0x1ba].hex()),player_hex=raw.hex())
class Stale(Inputs):
 def __init__(self,name,cache):
  super().__init__(name,cache);self.capture=[];self.stop_amount=False;self.prefix_seen=None
  section=next(s for s in self.image.sections if s.name=='.text');raw=self.image.section_bytes(section)
  for key,pattern in PATTERNS.items():
   matches=[section.va+m.start()for m in re.finditer(pattern,raw,re.S)]
   assert len(matches)==1,(name,key,matches);self.i[key]=matches[0]
  _,a=literal(self.code,self.i['allocate'],self.starts)
  calls=[int(i.op_str,16)for i in a if i.mnemonic=='call'];assert len(calls)==1;self.i['clear_and_salt']=calls[0]
  _,a=literal(self.code,self.i['leave'],self.starts)
  # Release has a unique full source prefix; derive it from actual leave calls.
  candidates=[]
  for i in a:
   if i.mnemonic=='call':
    entry=int(i.op_str,16)
    if self.image.read(entry,11)==bytes.fromhex('53568bd88b4244578b7a24'):candidates.append(entry)
  assert len(candidates)==1;self.i['release']=candidates[0];self.i['multiplier']=self.owners['damage']
  self.g.hook_add(UC_HOOK_CODE,self.observe_damage)
  self.tag=next(q for q in cache.tags.values()if q['group']=='jpt!'and q['name']=='objects\\weapons\\rifle\\smg\\damage_effects\\smg_bullet')
  self.definition=cache.magic+self.tag['offset']
 def observe_damage(self,g,pc,n,unused):
  if pc==self.i['amount']and self.stop_amount:
   event=g.reg_read(UC_X86_REG_EAX);sp=g.reg_read(UC_X86_REG_ESP)
   self.prefix_seen=dict(entry=pc,event_pointer=event,event_hex=bytes(g.mem_read(event,0x88)).hex(),
       attacker=self.u(event+8),captured_unit=self.u(event+0xc),target=g.reg_read(UC_X86_REG_EBX),
       definition=g.reg_read(UC_X86_REG_ECX),actual_return=self.u(sp))
   g.emu_stop()
  if pc==self.owners['damage']:
   attacker=g.reg_read(UC_X86_REG_ECX);victim=g.reg_read(UC_X86_REG_EAX)
   self.capture.append(dict(entry=pc,attacker=attacker,victim=victim,
       attacker_slot_fields=fields(g,R+(attacker&65535)*0x21c)if attacker!=NONE else None,
       victim_slot_fields=fields(g,R+(victim&65535)*0x21c)if victim!=NONE else None))
 def damage_objects(self):
  self.w(H+0x24,12);self.w(H+0x38,2);self.w(H+0x44,T)
  self.g.mem_write(T,struct.pack('<HBBIIHBBII',0x8aaa,0,0,0,U,0x8aaa,0,0,0,U+0x800))
  for slot in (0,1):
   at=U+slot*0x800;self.g.mem_write(at,bytes(0x800))
   for off in (0x14,0x12c,0x13c,0x140,0x24c):self.w(at+off,NONE)
  self.w(U+0x13c,0x9abc0003)
 def amount(self,attacker,captured_unit):
  self.g.mem_write(EVENT,bytes(0x88));self.w(EVENT,int(self.tag['datum'],16));self.w(EVENT+8,attacker);self.w(EVENT+0xc,captured_unit)
  self.w(EVENT+0x54,bits(1));self.w(EVENT+0x5c,bits(1));self.g.mem_write(EVENT+0x68,b'\x01');self.capture=[]
  before=bytes(self.g.mem_read(EVENT,0x88));players=bytes(self.g.mem_read(R,16*0x21c))
  self.call(self.i['amount'],(OUTPUT,),((UC_X86_REG_EAX,EVENT),(UC_X86_REG_EBX,0x8aaa0000),(UC_X86_REG_ECX,self.definition+0x10)))
  assert players==bytes(self.g.mem_read(R,16*0x21c));assert len(self.capture)==1
  return dict(attacker=attacker,captured_unit=captured_unit,input_event_hex=before.hex(),event_hex=bytes(self.g.mem_read(EVENT,0x88)).hex(),
      amount_bits=self.g.reg_read(UC_X86_REG_XMM0)&NONE,multiplier_calls=self.capture.copy())
 def caller_prefix(self,attacker):
  self.g.mem_write(EVENT,bytes(0x88));self.w(EVENT,int(self.tag['datum'],16));self.w(EVENT+8,attacker);self.w(EVENT+0xc,0x8aab0001)
  before=bytes(self.g.mem_read(EVENT,0x88));self.prefix_seen=None;self.stop_amount=True
  self.g.mem_write(STACK,struct.pack('<7I',STOP,EVENT,0x8aaa0000,NONE,NONE,NONE,0));self.g.reg_write(UC_X86_REG_ESP,STACK)
  self.g.reg_write(UC_X86_REG_FPCW,0x27f);self.g.reg_write(UC_X86_REG_MXCSR,0x1f80)
  self.g.emu_start(self.i['damage_caller'],STOP,count=200000)
  assert self.g.reg_read(UC_X86_REG_EIP)==self.i['amount']
  self.stop_amount=False;assert self.prefix_seen and before==bytes(self.g.mem_read(EVENT,0x88))
  return self.prefix_seen
 def run_case(self,row):
  self.birth(row['strength']);self.g.mem_write(R+0xc0,bytes([row['member']&255]));self.w(R+0x194,29);self.g.mem_write(R+0x1b8,struct.pack('<h',row['inside']))
  self.g.mem_write(R+3*0x21c+0xc0,bytes([row['victim_member']&255]));self.w(R+3*0x21c+0x194,0);self.g.mem_write(R+3*0x21c+0x1b8,bytes(2))
  self.w(G+0x184,row['global_flags']);self.w(G+0x22c,row['hill_flags']);self.g.mem_write(P+0x2a,b'\x04\x00');self.w(P+0x3c,2)
  self.g.mem_write(P+0x40,b'\xbd\x9a');self.w(PG,2)
  result=[]
  for phase in ('before_leave','after_leave','same_slot_rejoin'):
   if phase=='after_leave':
    self.w(H+0x38,0);self.call(self.i['leave'],registers=((UC_X86_REG_EAX,0x9abc0000),))
   if phase=='same_slot_rejoin':
    datum=self.call(self.i['allocate'],registers=((UC_X86_REG_EDX,P),(UC_X86_REG_EDI,0)))&NONE
    assert datum==0x9abd0000;allocated=fields(self.g,R)
    assert all(allocated[k]==0 for k in ('membership','strength','field194','inside'))
    self.g.mem_write(CREATION+0x1c+0x7d,b'\x03');self.call(self.i['birth'],(datum,1,CREATION))
   self.damage_objects();state=fields(self.g,R)
   amounts=[self.amount(attacker,unit)for attacker in (0x9abc0000,0x9abd0000,NONE)for unit in (NONE,0x8aaa0001,0x8aab0001)]
   # Both full salts select identical raw slot data; NONE retains early 1.0.
   assert [q['amount_bits']for q in amounts[:3]]==[q['amount_bits']for q in amounts[3:6]]
   assert all(q['amount_bits']==bits(4.75)for q in amounts[6:])
   result.append(dict(phase=phase,fields=state,allocated=allocated if phase=='same_slot_rejoin'else None,
       damage=amounts,caller_prefix=self.caller_prefix(0x9abc0000)))
  before,after,rejoin=[q['fields']for q in result]
  assert all(before[k]==after[k]for k in ('membership','strength','field194','inside'))and after['salt']==0
  assert rejoin['strength']==3 and rejoin['field194']==rejoin['inside']==0
  assert rejoin['membership']==(-1 if row['global_flags']&1 else 0),(self.name,row,rejoin)
  return result

class RetainedLifecycle(DatumOracle):
 """Same actual object-callback lifecycle with explicit nonzero player inputs."""
 def fixture(self,c):super().fixture(c);self.damage_fields_seeded=False
 def identities(self,player,units,projectile=None):
  at=self.player_ptr(player)
  if len(units)==2 and self.u(at+0x2c)==units[-1]and not self.damage_fields_seeded:
   self.g.mem_write(at+0xc0,b'\x07\x02');self.w(at+0x194,29);self.sh(at+0x1b8,17);self.damage_fields_seeded=True
  state=super().identities(player,units,projectile);state['damage_input_fields']=fields(self.g,at);return state

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');a=ap.parse_args();a.output.parent.mkdir(parents=True,exist_ok=True)
 assert sha(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN;cache=HudCache(ROOT/'assets/disc/maps/lockout.map')
 rows=[dict(strength=2,member=7,inside=17,victim_member=3,global_flags=128,hill_flags=4),
       dict(strength=2,member=0,inside=17,victim_member=0,global_flags=0,hill_flags=0),
       dict(strength=2,member=7,inside=17,victim_member=7,global_flags=1,hill_flags=0),
       dict(strength=2,member=7,inside=17,victim_member=0,global_flags=1,hill_flags=0),
       dict(strength=2,member=-1,inside=17,victim_member=-1,global_flags=1,hill_flags=0),
       dict(strength=2,member=7,inside=17,victim_member=3,global_flags=128|(1<<12),hill_flags=0)]
 sources={};versions={};listings=[];pairs=[]
 for name in ('retail','tu11','u5'):
  t=Stale(name,cache);sources[name]=t;versions[name]=dict(image_sha256=t.spec['pin'],owners=t.i,results=[t.run_case(row)for row in rows],damage_tag=t.tag)
 for name,t in sources.items():
  for key in (*PATTERNS,'clear_and_salt','release','resolver','object_get_mask3','multiplier'):
   _,ins=literal(t.code,t.i[key],t.starts);listings.append(dict(image=name,owner=key,entry=t.i[key],instructions=[dict(address=i.address,bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str)for i in ins]))
   if name!='u5':
    u=sources['u5'];_,later=literal(u.code,u.i[key],u.starts);eq=complete_tokens(ins,t.image)==complete_tokens(later,u.image)
    pairs.append(dict(earlier=name,later='u5',owner=key,complete_instructions=len(ins),later_complete_instructions=len(later),complete_shape_equal=eq,
        relocated_operands=references(ins,later,t.image,u.image)if eq else []))
 # Retain all raw records and absolute PCs, compare actual damage/slot outputs.
 def normalize(v):
  return [[dict(phase=q['phase'],fields={k:x for k,x in q['fields'].items()if k!='player_hex'},
      damage=[{k:x for k,x in d.items()if k!='multiplier_calls'}for d in q['damage']])for q in case]for case in v['results']]
 assert normalize(versions['retail'])==normalize(versions['tu11'])==normalize(versions['u5'])
 c=Cache(ROOT/'assets/disc/maps/lockout.map');o=RetainedLifecycle(c);life=[lifecycle_case(o,c,slot,delete)for slot in (0,3,15)for delete in (False,True)]
 for q in life:
  selected={s['stage']:s['state']for s in q['stages']};before=selected['same_player_bound_to_respawn_unit']['damage_input_fields'];after=selected['actual_player_leave_with_object_callbacks']['damage_input_fields']
  assert all(before[k]==after[k]for k in ('membership','strength','field194','inside'))and after['salt']==0
 deps={}
 for mod in list(sys.modules.values()):
  path=getattr(mod,'__file__',None)
  if path:
   p=Path(path).resolve()
   if p.is_relative_to(ROOT/'tools')and p.suffix=='.py':deps[str(p.relative_to(ROOT))]=sha(p)
 prior=ROOT/'analysis/multiplayer/integration/spawn-source/datums-report.json'
 report=dict(schema='original-damage-stale-attacker-v1',oracle_sha256=sha(__file__),boundary=__doc__,lockout_sha256=CACHE_PIN,
     inputs=rows,versions=versions,whole_amount_calls=len(rows)*3*3*9,whole_empty_object_leave_calls=len(rows)*3,
     all_image_empty_object_requested_allocation_calls=len(rows)*3,all_image_empty_object_birth_calls=len(rows)*3*2,original_caller_prefix_calls=len(rows)*3*3,
     complete_source_pairs=pairs,instruction_listings=listings,object_callback_lifecycle_cases=life,
     prior_object_lifecycle=dict(path=str(prior.relative_to(ROOT)),sha256=sha(prior)),dependency_sha256=dict(sorted(deps.items())),
     original_amount_instruction_patches=[],original_amount_supplied_returns=[],empty_object_lifecycle_supplied_returns=[],
     object_callback_lifecycle_boundary='Reuses separately declared DatumOracle body-storage service, original constructor/death prefixes and optional control notification; all leave/rejoin owners execute whole. No supplied service executes inside the complete leave owner.')
 a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(report_sha256=sha(a.output),whole_amount_calls=report['whole_amount_calls'],whole_leave_calls=report['whole_empty_object_leave_calls']+len(life),
     original_caller_prefix_calls=report['original_caller_prefix_calls'],equal_pairs=sum(p['complete_shape_equal']for p in pairs),pairs=len(pairs)),indent=2))
if __name__=='__main__':main()
