#!/usr/bin/env python3
"""Original camouflage state ownership and caller-order evidence.

Whole selected-engine slot5, nonlocal player birth/leave/rejoin, mode4 spawn
and original frame rounding CRT owner execute unchanged in all three images.
TU object-callback leave reuses the explicit storage/prefix boundaries of the
datum oracle. Cold full constructor and actual unit-wrapper evidence are pinned
prior original executions, re-read, not newly replayed whole world admission.
Static complete caller listings distinguish ordering evidence from execution.
"""
import argparse,json,re,struct,sys
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_EDI,UC_X86_REG_FPCW,UC_X86_REG_MXCSR
from oracle_camouflage_fp import CamoFP
from oracle_damage_stale_attacker import Stale,RetainedLifecycle,PG,H,R,G,P,CREATION,NONE
from oracle_mp_spawn_datums import DatumOracle,Cache,lifecycle_case
from oracle_player_lives import C
from oracle_mp_full_scoreboard_1_5 import ROOT,HudCache,CACHE_PIN,literal,sha

OUT=ROOT/'analysis/multiplayer/game-variants/camouflage-state-20261011/owners'
PINS={
 'constructor':('analysis/multiplayer/integration/spawn-source/unit-constructors/measured-final-stop/report.json','8e03219bae06eaf2968d2dfb24142a9446cee1d9ff36da9b246b229c821d3c0e'),
 'unit_wrapper':('analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/report.json','4b9e9ed383e0081fe7d64c57636731304d66d322b3aef23259024a27061733cb')}
def fields(raw):
 return dict(flags=struct.unpack_from('<I',raw,0x134)[0],primary_bits=struct.unpack_from('<I',raw,0x2b0)[0],
     secondary_bits=struct.unpack_from('<I',raw,0x2b4)[0],speed_bits=struct.unpack_from('<I',raw,0x2b8)[0],
     intrinsic_mode=raw[0x2bc],intrinsic_ticks=struct.unpack_from('<h',raw,0x2be)[0],raw_camouflage_hex=raw[0x2b0:0x2c0].hex())
def timers(g,at):return list(struct.unpack('<hh',g.mem_read(at+0x198,4)))
def listing(t,key,entry):
 _,ins=literal(t.code,entry,t.starts)
 return dict(image=t.name,owner=key,entry=entry,instructions=[dict(address=i.address,bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str)for i in ins])
def find(t,pattern):
 s=next(s for s in t.image.sections if s.name=='.text');matches=[s.va+m.start()for m in re.finditer(pattern,t.image.section_bytes(s),re.S)]
 assert len(matches)==1,(t.name,pattern,matches);return matches[0]
def cold_prior():
 path,pin=PINS['constructor'];assert sha(ROOT/path)==pin;r=json.loads((ROOT/path).read_text())
 q=next(q for q in r['calls']if q['entry']=='000c52c0'and q['phase']=='original_normal_player_spawn')
 result={key:q[key]for key in ('entry','caller','id','phase','return_eax','return_order','order')}
 for key in ('object_before','object_after'):
  raw=bytes.fromhex(q[key]['body_hex']);result[key]=dict(body=q[key]['body'],size=q[key]['size'],fields=fields(raw))
  assert fields(raw)==dict(flags=0,primary_bits=0,secondary_bits=0,speed_bits=0,intrinsic_mode=0,intrinsic_ticks=0,raw_camouflage_hex='00'*16)
 result['inherited_boundaries']=r['boundaries'];return result
def wrapper_prior():
 path,pin=PINS['unit_wrapper'];assert sha(ROOT/path)==pin;r=json.loads((ROOT/path).read_text());q=next(q for q in r['calls']if q['entry']=='000c7e90')
 return dict(calls=[{k:s[k]for k in ('entry','caller','id','phase','order','return_order')}for s in r['calls']if q['order']<=s['order']<=q['return_order']],inherited_boundaries=r['boundaries'])
class TimerLifecycle(RetainedLifecycle):
 def identities(self,player,units,projectile=None):
  at=self.player_ptr(player)
  if len(units)==2 and self.u(at+0x2c)==units[-1]and not self.damage_fields_seeded:self.g.mem_write(at+0x198,struct.pack('<hh',15,2))
  state=super().identities(player,units,projectile);state['camouflage_timers']=timers(self.g,at);return state
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');a=ap.parse_args();a.output.parent.mkdir(parents=True,exist_ok=True)
 assert sha(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN;cache=HudCache(ROOT/'assets/disc/maps/lockout.map');versions={};lists=[]
 for name in ('retail','tu11','u5'):
  t=CamoFP(name,cache);t.name=name;slot5=[]
  for mode in (2,4):
   t.setup(dict(unit_none=True));vtable=t.u(t.u(t.spec['table']+mode*4));callback=t.u(vtable+20)
   lists.append(listing(t,f'engine{mode}_slot5',callback))
   for initial in (0,0x12345678):
    t.w(R+0x1b8,initial);before=bytes(t.g.mem_read(R,0x21c));t.call(callback,(0x9abc0000,));after=bytes(t.g.mem_read(R,0x21c))
    actual=bytearray(after);actual[0x1b8:0x1bc]=before[0x1b8:0x1bc];assert bytes(actual)==before
    produced=struct.unpack_from('<I',after,0x1b8)[0];assert produced==(NONE if mode==2 else initial)
    slot5.append(dict(mode=mode,vtable=vtable,callback=callback,initial_dword=initial,produced_dword=produced,player_before_hex=before.hex(),player_after_hex=after.hex()))
  frame=find(t,rb'\xb8\x48\x40\x00\x00\xe8....\x68\xff\xff\x0f\x00\x68\x1f\x00\x09\x00\xe8....')
  _,ins=literal(t.code,frame,t.starts);calls=[int(i.op_str,16)for i in ins if i.mnemonic=='call'and i.op_str.startswith('0x')];crt=calls[1]
  crt_results=[];lists.extend((listing(t,'outer_frame',frame),listing(t,'frame_fp_control',crt)))
  _,ci=literal(t.code,crt,t.starts)
  for call in (int(i.op_str,16)for i in ci if i.mnemonic=='call'):lists.append(listing(t,'frame_fp_control_direct_callee',call))
  for cw in (0x27f,0x67f,0xa7f,0xe7f):
   t.g.reg_write(UC_X86_REG_FPCW,cw);t.g.reg_write(UC_X86_REG_MXCSR,0x1f80);result=t.call(crt,(0x9001f,0xfffff))
   observed=t.g.reg_read(UC_X86_REG_FPCW);assert result==0x9001f and observed==0x23f
   crt_results.append(dict(requested_initial_fpcw=cw,arguments=[0x9001f,0xfffff],return_value=result,returned_fpcw=observed,returned_mxcsr=t.g.reg_read(UC_X86_REG_MXCSR)))
  s=Stale(name,cache);s.name=name;lifecases=[]
  for pair in ((15,2),(-1,32767),(-32768,0)):
   s.birth(0);poison_birth=timers(s.g,R);assert poison_birth==[-23131,-23131];s.g.mem_write(R+0x198,struct.pack('<hh',*pair));s.w(PG,2);s.w(H+0x38,0)
   s.g.mem_write(P+0x2a,b'\x04\x00');s.w(P+0x3c,2);s.g.mem_write(P+0x40,b'\xbd\x9a')
   before=bytes(s.g.mem_read(R,0x21c));s.call(s.i['leave'],registers=((UC_X86_REG_EAX,0x9abc0000),));after=bytes(s.g.mem_read(R,0x21c));assert timers(s.g,R)==list(pair)
   d=s.call(s.i['allocate'],registers=((UC_X86_REG_EDX,P),(UC_X86_REG_EDI,0)))&NONE;allocated=timers(s.g,R);assert d==0x9abd0000 and allocated==[0,0]
   s.call(s.i['birth'],(d,1,CREATION));rejoined=timers(s.g,R);assert rejoined==[0,0]
   s.g.mem_write(R+0x198,struct.pack('<hh',*pair));s.g.mem_write(G+0xc,b'\x04');s.call(s.i['spawn'],(d,));spawn=timers(s.g,R);assert spawn==[0,0]
   lifecases.append(dict(seeded_timers=list(pair),poison_birth_timers=poison_birth,player_before_leave_hex=before.hex(),player_after_leave_hex=after.hex(),retained_timers=list(pair),allocated_timers=allocated,rejoined_timers=rejoined,mode4_spawn_timers=spawn))
  _,li=literal(s.code,s.i['leave'],s.starts);bind=int(next(i.op_str for i in li if i.mnemonic=='call'),16)
  for key,entry in dict(leave=s.i['leave'],nested_bind=bind,birth=s.i['birth'],mode4_spawn=s.i['spawn'],allocation=s.i['allocate']).items():lists.append(listing(s,key,entry))
  versions[name]=dict(image_sha256=t.spec['pin'],engine_slot5=slot5,frame_entry=frame,frame_fp_control_entry=crt,frame_fp_control=crt_results,lifecycle=lifecases)
 # Save complete TU callers as static ordering evidence, not new full executions.
 t=CamoFP('tu11',cache);t.name='tu11'
 for key,entry in dict(preworld_players=0x15d7c0,postworld_players=0x14eff0,object_type_dispatch=0x10cdf0,object_inherited_unit_callback=0xc7e90,
     object_recursive_update=0xbd360,unit_constructor=0xc52c0,damage_disruption_caller=0xcb410,weapon_disruption_caller=0x107160,
     other_disruption_caller=0xe8f30,current_player_cap=0x161510,intrinsic_controller=0xc6aa0).items():lists.append(listing(t,key,entry))
 c=Cache(ROOT/'assets/disc/maps/lockout.map');o=TimerLifecycle(c);life=[lifecycle_case(o,c,slot,False)for slot in (0,3,15)]
 for q in life:
  states={s['stage']:s['state']for s in q['stages']};assert states['same_player_bound_to_respawn_unit']['camouflage_timers']==states['actual_player_leave_with_object_callbacks']['camouflage_timers']==[15,2]
  assert states['same_slot_rejoined_new_player_and_unit']['camouflage_timers']==[0,0]
 primitive=DatumOracle(c);primitive.fixture(c);d=primitive.object();body=primitive.body(d);raw=bytes(primitive.g.mem_read(body,0x800));assert fields(raw)['raw_camouflage_hex']=='00'*16 and fields(raw)['flags']==0
 deps={}
 for mod in list(sys.modules.values()):
  path=getattr(mod,'__file__',None)
  if path:
   p=Path(path).resolve()
   if p.is_relative_to(ROOT/'tools')and p.suffix=='.py':deps[str(p.relative_to(ROOT))]=sha(p)
 report=dict(schema='original-camouflage-state-owners-v1',oracle_sha256=sha(__file__),lockout_sha256=CACHE_PIN,boundary=__doc__,versions=versions,
     full_instruction_listings=lists,pinned_prior_sources={k:dict(path=p,sha256=h)for k,(p,h)in PINS.items()},prior_whole_constructor=cold_prior(),prior_whole_unit_wrapper=wrapper_prior(),
     fresh_unit_primitive=dict(datum=d,body=body,body_hex=raw.hex(),fields=fields(raw),events=primitive.datum_events,
       boundary='Whole original BD170 clears poisoned arena storage. Explicit 18FCE0 body-storage service supplies arena address/success, original game header/allocation/clear execute. Actual bipd/type/NONE fields are declared, then bounded C52C0 prefix through C5464 exclusive runs. Prefix alone does not write fade amounts. Full constructor authority is separate pinned prior execution.'),
     whole_object_callback_leave_cases=life,dependency_sha256=dict(sorted(deps.items())),instruction_patches=[],
     no_supplied_return_scopes=['all-image slot5','all-image frame FP CRT owner and callees','all-image nonlocal current-unit-NONE lifecycle','TU whole leave with actual object callbacks'],
     boundaries=['Mode4 whole spawn is not ordinary live spawn; current unit NONE bypasses allocation.',
       'TU object lifecycle reuses explicit storage arena and constructor/death prefixes; whole leave has no supplied return.',
       'Full constructor and unit-wrapper prior reports are immutable re-read evidence and preserve all inherited boundaries.',
       'Static outer/preworld/postworld caller inspection and composed timeline do not prove full natural admitted successive 139750 frames.'])
 a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(report_sha256=sha(a.output),whole_slot5_calls=12,whole_frame_fp_calls=12,whole_empty_object_leave_calls=9,whole_object_callback_leave_calls=len(life)),indent=2))
if __name__=='__main__':main()
