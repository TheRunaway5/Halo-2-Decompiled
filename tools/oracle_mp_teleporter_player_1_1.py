#!/usr/bin/env python3
"""New actual-map composition of frozen source player/world construction.

Cache redirection changes only the explicit cache input binding. Original
instructions, source allocation, object ownership and BSP/registry writers run.
Diagnostic entrance placement is delivered by original B8490, not a root store.
"""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
import unicorn
from unicorn import x86_const as reg
import oracle_smg_firing_1_1 as source
import oracle_mp_inventory_1_1 as inventory
import oracle_mp_spawn_player_visibility as player
from oracle_mp_spawn_biped_physics import PLACEMENT

class TeleporterTrace(player.PlayerTrace):
 def __init__(self,data):
  self.teleport_capture=False;self.teleport_calls=[];self.teleport_stores=[]
  super().__init__(data)
 def fixup(self):
  try:return super().fixup()
  except AssertionError:
   print(json.dumps(dict(fixup_failure=True,cache=str(self.cache.path),bipd=self.bipd,returned=f'{self.g.reg_read(reg.UC_X86_REG_EAX):08x}',tag_table=f'{self.u(0x4eb114):08x}',index_pointer=f'{self.u(0x54fa50):08x}',count=self.u(self.u(0x54fa50)+24),wanted_record=bytes(self.g.mem_read(self.u(0x4eb114)+(int(self.bipd['datum'],16)&0xffff)*16,16)).hex())),flush=True);raise
 def hook(self,g,a,n,user):
  if a==0x1243c0 and self.active=='original_selected_tag_iterator':
   # The older Lockout-only fixture's synthetic index+c.magic view overlaps
   # Beaver Creek's actual resident BSP. Relocate the exact cache index header
   # input, without changing any field, tag identity or source iterator result.
   q=self.cache.u(16);p=PLACEMENT+0x2000
   raw=bytes(self.cache.data[q:q+32]);g.mem_write(p,raw);self.w(0x54fa50,p)
   self.index_input_relocation=dict(file_offset=q,raw20_hex=raw.hex(),backing=f'{p:08x}',reason='Avoid synthetic cache-index view overlapping actual resident BSP; literal cache input only.')
  if self.teleport_capture and a in (0x1a0340,0x1a0390,0x1a0580,0x1a0680,0x1a0790,0x1a0400,0xe0c20,0x16bec0,0x247fc0,0x1a24f0,0xba940,0xb8490,0x188ee0,0x1a0280,0xa9880):
   sp=g.reg_read(reg.UC_X86_REG_ESP)
   self.teleport_calls.append(dict(phase=self.active,entry=f'{a:08x}',registers=self.registers(),stack_hex=bytes(g.mem_read(sp,40)).hex()))
  super().hook(g,a,n,user)
 def mem_write(self,g,access,address,size,value,user):
  if self.teleport_capture:
   self.teleport_stores.append(dict(phase=self.active,instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,before_hex=bytes(g.mem_read(address,size)).hex(),value_hex=(value&((1<<(8*size))-1)).to_bytes(size,'little').hex()))
  super().mem_write(g,access,address,size,value,user)
 def compact(self):
  engine=self.u(0x4f10b8);pool=self.u(0x4f01f4);p=self.u(pool+0x44)+(self.spawned_player&0xffff)*0x21c
  heads=self.u(self.u(0x4e78d0)+0x44);unit=self.u(heads+(self.spawned_unit&0xffff)*12+8)
  return dict(engine=f'{engine:08x}',bank104_hex=bytes(self.g.mem_read(engine+0xafc,0x104)).hex(),
   player=f'{p:08x}',player21c_hex=bytes(self.g.mem_read(p,0x21c)).hex(),unit=f'{unit:08x}',unit_body_hex=bytes(self.g.mem_read(unit,0x800)).hex(),
   position_hex=bytes(self.g.mem_read(unit+0x64,12)).hex(),forward_hex=bytes(self.g.mem_read(unit+0x70,12)).hex(),velocity_hex=bytes(self.g.mem_read(unit+0x88,12)).hex())

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--map',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();a.output.parent.mkdir(parents=True,exist_ok=True)
 cache_type=source.Cache
 def selected_cache(path):return cache_type(a.map if path.name=='lockout.map' else path)
 source.Cache=selected_cache;inventory.Cache=selected_cache;source.CACHE_PIN=hashlib.sha256(a.map.read_bytes()).hexdigest()
 err=(a.output.parent/'runtime-stderr.txt').open('w');proc=subprocess.Popen(['wine',str(player.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=err,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(player.CONTEXT_OUT/'wine-prefix')))
 data=json.loads(proc.stdout.readline())
 try:
  t=player.prepare(data,trace_type=TeleporterTrace);assert player.spawn_and_activate(t)
  initial=t.compact();t.teleport_capture=True
  assert t.stage('original_teleporter_pair_initializer',0x1a0340)
  initialized=t.compact();engine=t.u(0x4f10b8);count=t.u(engine+0xbfc);assert count>0
  pair=t.u(engine+0xafc);flags=t.cache.block(t.cache.scenario+0x118,0x20);q=flags[pair]
  p=PLACEMENT+0x1000;t.g.mem_write(p,bytes(t.cache.data[q:q+12]))
  assert t.stage('explicit_original_authored_entrance_position_delivery',0xb8490,(t.spawned_unit,0,0),((reg.UC_X86_REG_EAX,p),(reg.UC_X86_REG_ECX,0),(reg.UC_X86_REG_EDX,0)))
  placed=t.compact();complete=t.stage('original_teleporter_engine_update',0x1a0580);after=t.compact()
  report=dict(schema=1,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),source_sha256=source.PIN,map_path=str(a.map),map_sha256=source.CACHE_PIN,source_player=t.spawned_player,source_unit=t.spawned_unit,
   initial=initial,initialized=initialized,placed=placed,after=after,complete=complete,stop=t.new_invalid,calls=t.teleport_calls,stores=t.teleport_stores,
   preparation=t.producers,stages=t.player_stages,services=t.events,
   boundaries=['New actual cache binding supplied to unchanged frozen constructors; not the frozen Lockout fixture and not complete title/map startup.','Original B8490 delivers actual authored entrance as an explicit diagnostic position input. No direct root/velocity/pose/contact/membership/success store.','All original current unit, object heap/body/pose/registry and source capsule/query ownership execute as admitted. Source-neutral controls/device admission and original platform/Xbox estimate limits remain explicit.'])
  a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(dict(complete=complete,stop=t.new_invalid,position=after['position_hex'],velocity=after['velocity_hex'],calls=len(t.teleport_calls))),flush=True)
 finally:
  proc.stdin.write('x');proc.stdin.flush();proc.stdin.close();proc.wait(timeout=60);proc.stdout.close();err.close()
if __name__=='__main__':main()
