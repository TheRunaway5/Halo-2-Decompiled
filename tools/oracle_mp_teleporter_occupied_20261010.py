#!/usr/bin/env python3
"""Execute a teleporter with a second genuinely allocated player at its exit.

Authored entrance/exit positions are explicit diagnostic inputs to original
B8490. No object, overlap, capsule, success, damage or velocity output is supplied.
"""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
import oracle_mp_teleporter_player_1_1 as base
from unicorn import x86_const as reg

class OccupiedTrace(base.TeleporterTrace):
 def __init__(self,data):
  self.query_observations=[]
  super().__init__(data)
 def hook(self,g,a,n,user):
  if self.teleport_capture and a in (0x1a04c0,0x1a04fa):
   sp=g.reg_read(reg.UC_X86_REG_ESP)
   self.query_observations.append(dict(phase=self.active,instruction=f'{a:08x}',al=g.reg_read(reg.UC_X86_REG_EAX)&255,
    collection_counts=list(struct.unpack('<4h',bytes(g.mem_read(sp+0x64,8)))),
    contact_header_hex=bytes(g.mem_read(sp+0x34,0x30)).hex()))
  super().hook(g,a,n,user)
 def player_record(self,datum):
  p=self.u(self.u(0x4f01f4)+0x44)+(datum&0xffff)*0x21c
  unit=self.u(p+0x2c);heads=self.u(self.u(0x4e78d0)+0x44);body=self.u(heads+(unit&0xffff)*12+8)
  return dict(datum=f'{datum:08x}',unit=f'{unit:08x}',flags=f'{(self.u(p)>>16)&0xffff:04x}',
   record_hex=bytes(self.g.mem_read(p,0x21c)).hex(),root_hex=bytes(self.g.mem_read(body+0x64,12)).hex(),
   velocity_hex=bytes(self.g.mem_read(body+0x88,12)).hex())

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--map',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 assert not a.output.exists();a.output.parent.mkdir(parents=True,exist_ok=True)
 source,inventory,player=base.source,base.inventory,base.player
 cache_type=source.Cache
 def selected_cache(path):return cache_type(a.map if path.name=='lockout.map' else path)
 source.Cache=selected_cache;inventory.Cache=selected_cache;source.CACHE_PIN=hashlib.sha256(a.map.read_bytes()).hexdigest()
 err=(a.output.parent/'runtime-stderr.txt').open('w')
 proc=subprocess.Popen(['wine',str(player.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=err,text=True,
  env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(player.CONTEXT_OUT/'wine-prefix')))
 data=json.loads(proc.stdout.readline())
 try:
  t=player.prepare(data,trace_type=OccupiedTrace);assert player.spawn_and_activate(t)
  entrant=t.spawned_player;entrant_unit=t.spawned_unit
  assert player.spawn_and_activate(t,index=1);occupant=t.spawned_player;occupant_unit=t.spawned_unit
  assert entrant!=occupant and entrant_unit!=occupant_unit
  t.teleport_capture=True;assert t.stage('original_teleporter_pair_initializer',0x1a0340)
  engine=t.u(0x4f10b8);assert t.u(engine+0xbfc)>0
  entry_index,exit_index=struct.unpack('<2I',bytes(t.g.mem_read(engine+0xafc,8)))
  flags=t.cache.block(t.cache.scenario+0x118,0x20)
  for label,unit,index in [('occupant_at_authored_exit',occupant_unit,exit_index),('entrant_at_authored_entrance',entrant_unit,entry_index)]:
   p=base.PLACEMENT+0x1000;t.g.mem_write(p,bytes(t.cache.data[flags[index]:flags[index]+12]))
   assert t.stage('explicit_original_'+label+'_position_delivery',0xb8490,(unit,0,0),
    ((reg.UC_X86_REG_EAX,p),(reg.UC_X86_REG_ECX,0),(reg.UC_X86_REG_EDX,0)))
  before=dict(entrant=t.player_record(entrant),occupant=t.player_record(occupant))
  complete=t.stage('original_occupied_exit_teleporter_update',0x1a0580)
  after=dict(entrant=t.player_record(entrant),occupant=t.player_record(occupant))
  assert complete and t.new_invalid is None
  for who in ('entrant','occupant'):
   assert before[who]['root_hex']==after[who]['root_hex']
   assert before[who]['velocity_hex']==after[who]['velocity_hex']
   assert int(before[who]['flags'],16)&4==0 and int(after[who]['flags'],16)&4
  contacts=[q for q in t.query_observations if q['instruction']=='001a04fa']
  assert len(contacts)==2 and all(q['al']==1 for q in contacts)
  assert {struct.unpack_from('<I',bytes.fromhex(q['contact_header_hex']))[0]for q in contacts}=={entrant_unit,occupant_unit}
  assert not any(c['entry'] in ('000ba940','000b8490')and c['phase']=='original_occupied_exit_teleporter_update'for c in t.teleport_calls)
  report=dict(schema=1,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),source_sha256=source.PIN,
   map_path=str(a.map),map_sha256=source.CACHE_PIN,entry_index=entry_index,exit_index=exit_index,complete=complete,stop=t.new_invalid,
   before=before,after=after,query_observations=t.query_observations,calls=t.teleport_calls,stores=t.teleport_stores,
   preparation=t.producers,stages=t.player_stages,services=t.events,
   source_pair_bank_hex=bytes(t.g.mem_read(engine+0xafc,0x104)).hex(),
   boundaries=['Two genuine original joins and spawns; actual objects, capsules and registry owners. Original authored positions delivered only through B8490 as diagnostic inputs.',
    'No direct root, overlap, shape, occupancy, success, velocity, telefrag or damage result is supplied. Not complete title startup, network transport, world stepping or hardware parity.'])
  a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
  print(json.dumps(dict(complete=complete,stop=t.new_invalid,entrant_before=before['entrant']['root_hex'],entrant_after=after['entrant']['root_hex'],
   occupant_before=before['occupant']['root_hex'],occupant_after=after['occupant']['root_hex'],queries=t.query_observations)),flush=True)
 finally:
  proc.stdin.write('x');proc.stdin.flush();proc.stdin.close();proc.wait(timeout=60);proc.stdout.close();err.close()
if __name__=='__main__':main()
