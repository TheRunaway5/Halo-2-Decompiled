#!/usr/bin/env python3
"""Separate original FP owner registration gate on the source-created nonlocal player.

Reuses whole measured compatibility context/clocks verbatim. Does not assign a
local player/view, owner identity, animation event, registration result or audio
admission. Previous world, FP and audio freezes remain byte-identical.
"""
import argparse,json
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP
from oracle_smg_audio_valid_owner_1_1 import ValidOwnerTrace,ROOT,PIN,CACHE_PIN,FROZEN as OLDER,digest,prepare,spawn_and_activate
from oracle_smg_firing_1_1 import STOP
from functions import _trace

OUT=ROOT/'analysis/multiplayer/map-pack/smg-audio-fp-owner'
FROZEN={**OLDER,
 'tools/oracle_smg_audio_valid_owner_1_1.py':'e2dc14bfd463dce0e1a2a1362ee745a3f90c9ea27fba98c792df6ad0ec5c974c',
 'tools/audit_smg_audio_valid_owner_1_1.py':'3a6f237b0a1fe59d68d4793a56b201297b5d36c20c5dd513d9fdbe3bf80dee79',
 'docs/SMG_AUDIO_VALID_OWNER_1_1_RESEARCH.md':'c821aa287599d16d19ef82527b8c18b9cf8a4a4facd1390e1f8fc85f0cd49de8',
 'analysis/multiplayer/map-pack/smg-audio-valid-owner/frozen/suite.json':'9c138cd8448c7b5005d53004cf2d0aaac4362b4e959750a4f0582d993d3a8ff5',
 'analysis/multiplayer/map-pack/smg-audio-valid-owner/source-fixture.json':'cf3a91f4086d82ba380101d18320592085b0116bfbcd4010e3f0eff610bac330',
 'analysis/multiplayer/map-pack/smg-audio-valid-owner/audit.json':'c06959d238e678594585162a7fc2a079d0e785dbf3e40cc1ce3e8eb08b0067c6',
 'analysis/multiplayer/map-pack/smg-audio-valid-owner/measured/report.json':'89c443a8d47331d8c3ff527c122d009bd154d23d92cf39cfe31b60874df09661',
 'analysis/multiplayer/map-pack/smg-audio-valid-owner/reexecution/report.json':'af6c55f696f08e9c6c833957bc8f2e7f9a42983d10bb4a71db52c6232f0ff553'}
ENTRIES=(0x1677C3,0x125490,0x1254D0,0x1677E5,0x1678C1,0x14F5B0,0x169D8A,0x169965,0x169E15,0xCD1A0,0x168866,0x168841,0x168853,0x1687C2,0x18AA90)
STATIC=ENTRIES+(0x139750,0x179150,0x18AA40,0x18ACD0,0x18AD70,0x18EF20,0x18DDE0,0x18BA10,0x21F220)

class FPNonlocalTrace(ValidOwnerTrace):
 def __init__(self,data,replay):
  self.fp_owner_enabled=False;self.fp_owner_calls=[];self.fp_owner_returns=[];self.fp_owner_stores=[];self.fp_owner_steps=[]
  super().__init__(data,replay);self.fp_owner_enabled=True
 def fp_owner_state(self):
  users=self.u(0x4F01F0);views=self.u(0x4F1198);scratch=self.u(0x4F1194)
  return {'world':self.source_state(),'player_globals_pointer':f'{users:08x}',
   'player_globals_130_hex':bytes(self.g.mem_read(users,0x130)).hex(),
   'user_player_datums':[f'{self.u(users+0xC+4*i):08x}'for i in range(4)],
   'view_storage_pointer':f'{views:08x}','view_storage_8330_hex':bytes(self.g.mem_read(views,0x8330)).hex()if views else None,
   'scratch_pointer':f'{scratch:08x}','scratch_8000_hex':bytes(self.g.mem_read(scratch,0x8000)).hex()if scratch else None}
 def finish_fp_owner(self,a):
  for ret,row in self.fp_owner_returns[:]:
   if ret==a:row['return_eax']=f'{self.g.reg_read(UC_X86_REG_EAX):08x}';self.fp_owner_returns.remove((ret,row))
 def hook(self,g,a,size,user):
  if self.fp_owner_enabled:
   self.finish_fp_owner(a)
   if a in ENTRIES:
    sp=g.reg_read(UC_X86_REG_ESP);row={'phase':self.active,'entry':f'{a:08x}',
     'registers':self.registers(),'stack':[f'{self.u(sp+4+i*4):08x}'for i in range(4)],'return_site':f'{self.u(sp):08x}'}
    self.fp_owner_calls.append(row);self.fp_owner_returns.append((self.u(sp),row))
  super().hook(g,a,size,user)
 def mem_write(self,g,access,address,size,value,user):
  if self.fp_owner_enabled and self.active.startswith('original_fp_'):
   self.fp_owner_stores.append({'phase':self.active,'instruction':f'{g.reg_read(UC_X86_REG_EIP):08x}',
    'address':f'{address:08x}','size':size,'before_hex':bytes(g.mem_read(address,size)).hex(),
    'after_hex':(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()})
  super().mem_write(g,access,address,size,value,user)
 def fp_owner_action(self,name,entry):
  before=self.fp_owner_state();start=len(self.fp_owner_calls);writes=len(self.fp_owner_stores)
  self.fp_action(name,entry);self.finish_fp_owner(STOP)
  row={'name':name,'entry':f'{entry:08x}','before':before,'after':self.fp_owner_state(),
   'calls':[start,len(self.fp_owner_calls)],'stores':[writes,len(self.fp_owner_stores)],'completion':'complete_original_return'}
  self.fp_owner_steps.append(row);return row

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);args=ap.parse_args()
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 prior=ROOT/'analysis/multiplayer/map-pack/smg-audio-valid-owner/measured/report.json';report=json.loads(prior.read_text())
 context=prior.parent/'raw-host-context.json';assert digest(context)==report['context_snapshot_sha256'];data=json.loads(context.read_text())
 clocks=report['cases'][0]['clock_calls'][:2]
 t=prepare(data,trace_type=lambda d:FPNonlocalTrace(d,clocks));assert spawn_and_activate(t,activate=False)
 allocated=t.fp_owner_state()
 for name,entry in(('original_fp_storage_constructor_actual_arena',0x1677C3),('original_fp_storage_reset',0x1677E5),('original_fp_observer_nonlocal_source_created_player',0x1678C1)):
  t.fp_owner_action(name,entry)
 assert t.replay_cursor==len(clocks)
 queries=[z for z in t.fp_owner_calls if z['phase']=='original_fp_observer_nonlocal_source_created_player'and z['entry']=='0014f5b0']
 assert len(queries)==4 and all(z['return_eax']=='ffffffff'for z in queries)
 assert not any(z['entry']in('00169d8a','00169965','000cd1a0','00168866','001687c2','0018aa90')for z in t.fp_owner_calls)
 args.output.parent.mkdir(parents=True,exist_ok=True);directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True)
 listings=[]
 for e in STATIC:
  f=_trace(t.code,e,t.starts|set(STATIC));ins=[t.code.at(a)for a in sorted(f.code)]
  for z in ins:assert t.image.read(z.address,len(z.bytes))==z.bytes==bytes(t.g.mem_read(z.address,len(z.bytes)))
  p=directory/f'{e:08x}.asm';p.write_text(''.join(f'{z.address:08x} {z.bytes.hex()} {z.mnemonic} {z.op_str}\n'for z in ins))
  listings.append({'entry':f'{e:08x}','file':p.name,'sha256':digest(p),'instructions':len(ins),
   'calls':[{'site':f'{a:08x}','target':f'{b:08x}','kind':k}for a,b,k in f.sites]})
 ins=[t.code.at(a)for a in sorted(t.owner_executed)]
 for z in ins:assert t.image.read(z.address,len(z.bytes))==z.bytes==bytes(t.g.mem_read(z.address,len(z.bytes)))
 p=directory/'executed-original-instructions.asm';p.write_text(''.join(f'{z.address:08x} {z.bytes.hex()} {z.mnemonic} {z.op_str}\n'for z in ins))
 for q,h in FROZEN.items():assert digest(ROOT/q)==h,q
 result={'oracle_sha256':digest(__file__),'source_sha256':PIN,'cache_sha256':CACHE_PIN,'frozen_prior_pins':FROZEN,
  'whole_context_and_clock_source_report':str(prior.relative_to(ROOT)),'whole_context_sha256':digest(context),
  'context_mode':'unchanged_whole_measured_compatibility_context_replay','clock_mode':'verbatim_two_measured_startup_inputs',
  'clock_calls':t.clock_calls,'allocated':allocated,'steps':t.fp_owner_steps,'calls':t.fp_owner_calls,'stores':t.fp_owner_stores,
  'source_player_stages':t.player_stages,'source_player_stores':t.player_stores,
  'inherited_supplied_services':{f'{a:08x}':str(v)for a,v in t.services.items()},
  'source_resource_delivery':t.resource_delivery,'source_resource_requests':t.resource_requests,
  'instruction_listings':listings,'executed_original_listing':{'file':p.name,'sha256':digest(p),'instructions':len(ins)},
  'view_player_weapon_or_event_inputs_supplied':False,'ordinary_local_FP_sound_request_executed':False,'native_gameplay_update_applied':False}
 args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({'output':str(args.output),'source_view_queries':len(queries),'ordinary_FP_request_executed':False,'prior_pins':len(FROZEN)}))
if __name__=='__main__':main()
