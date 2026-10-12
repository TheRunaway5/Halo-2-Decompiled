#!/usr/bin/env python3
"""Separate unchanged active-item scheduling/current-state investigation.

The actual player/PVS prefix is imported read-only from its owning source
extension. No cluster, activation mask, pose or contact is supplied here.
"""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EBX,UC_X86_REG_EDI,UC_X86_REG_ESI
import oracle_mp_spawn_player_visibility as player_source
from oracle_mp_item_current_producers_1_1 import CurrentTrace,ROOT,digest,encoded,frozen_write,metadata
from oracle_mp_pickup_transfer_options_1_1 import FROZEN as PRIOR_PINS
from oracle_mp_inventory_spatial_1_1 import cache_record
from functions import _trace
from oracle_mp_spawn_process_heap_world import KernelBoundary
from oracle_smg_firing_1_1 import STOP,SEED,TIME

OUT=ROOT/'analysis/multiplayer/integration/inventory-source/item-active-schedule'
FROZEN={**PRIOR_PINS,**player_source.FROZEN,
 'tools/oracle_mp_spawn_player_visibility.py':'aeafe79cff80e9d607e843d5bdb9f7c86b2814df15ec1b1179ebf4fe505e8572',
 'docs/MP_PLAYER_VISIBILITY_RESEARCH.md':'0383d7b128c173322e2902452c03143522193211309b283853afe6c96033347b',
 'analysis/multiplayer/integration/spawn-source/player-visibility/report.json':'f58232e963b26bcb4a6471db8c40488f2bb195c4be5cf8e0617331ae4d5f78dd',
 'tools/oracle_mp_pickup_transfer_options_1_1.py':'48deb72187702c3660fd3d018cabbe22d2647c6fb9fa94118213d3437904022d',
 'tools/oracle_mp_pickup_transfer_options_audit_1_1.py':'361b8697415c9894a030d3d5df8e9be0c855c2bd45f390e749a9ddcb20ba6ce6',
 'docs/MP_PICKUP_TRANSFER_OPTIONS_RESEARCH.md':'9a339d7082e7d4438309d8cd1e982cf800a1c22f5f9c85a8ba9ac8be05de4d2b',
 'analysis/multiplayer/integration/inventory-source/pickup-transfer-options/report.json':'db8b94107fe570e29e0a75d0fc62c91f667fce565a116efd2d9216a9ad94f411',
 'analysis/multiplayer/integration/inventory-source/pickup-transfer-options/manifest.json':'fa3c56dd60ed6f6ab591a2c68224534c3809b06c0b678de2dd69406340a652a6'}
ENTRIES={0x13a380,0x13a2a0,0x151ec0,0x16f8a0,0xb7ce0,0xb7c50,
 0xb8180,0xb81f0,0x1c88f0,0x1c8970,0xb7e00,0xb7f50,0xb8040,0xb92a0,0xb9350,
 0xbd360,0xbd710,0x10cdf0,0x10cd10,0x10ce60,0xf9b10,0xf9ea0,0xf99a0,
 0x10db90,0xbc910,0x16b240,0x16ba30,0x1e20c0,0x246a50,
 0xbdf80,0xbede0,0xb96a0,0xb94f0,0x1ce650,0x1ce7e0,
 0x1c7660,0x1c7050,0x1c6f50,0x1019f0,0x1021d0,0xc0270,0x15c110,
 0x15c320,0x15c370,0xc7e90,0xe3b20,0x1e9540,0x30bd0}
STATE_OWNERS={0xb7ce0,0xb8180,0xb81f0,0xb7e00,0xb7f50,0xb8040,0xbd360,0xbd710,0xf9b10,0xf9ea0,
 0x16b240,0x246a50,0xbdf80,0xbede0,0xb96a0,0xb94f0,0x1ce650,
 0x1c7660,0x15c110,0xc7e90,0xe3b20,0x1e9540,0x1019f0,0xc0270,0xb92a0,0xb9350}
CALLBACK_SITES={0x10ce3b,0x10cd5a,0x10cea1}

class ActiveTrace(player_source.PlayerTrace):
 item_objects=CurrentTrace.item_objects
 registry_chains=CurrentTrace.registry_chains
 weapon_resources=CurrentTrace.weapon_resources
 def __init__(self,*args,**kwargs):
  self.item_calls=[];self.item_pending=[];self.item_stores=[];self.item_states=[];self.item_state_ids={};self.item_order=0
  self.item_stages=[];self.item_callbacks=[];self.item_observe=False
  super().__init__(*args,**kwargs)
 def item_snapshot(self):
  pool=self.u(0x4e78d0);capacity,stride,data,bitmap=(self.u(pool+n)for n in(0x20,0x24,0x44,0x48))
  context=self.u(0x4e58c4)
  return dict(current=CurrentTrace.current_snapshot(self),player=self.player_state(),world=self.state(),
   object_context=dict(global_address='004e58c4',pointer=f'{context:08x}',raw84_hex=bytes(self.g.mem_read(context,0x84)).hex(),
    byte03=f'{self.u(context+3,1):02x}'),
   full_object_header_pool=dict(pointer=f'{pool:08x}',header4c_hex=bytes(self.g.mem_read(pool,0x4c)).hex(),
    data_pointer=f'{data:08x}',capacity=capacity,stride=stride,data_hex=bytes(self.g.mem_read(data,capacity*stride)).hex(),
    bitmap_pointer=f'{bitmap:08x}',bitmap_hex=bytes(self.g.mem_read(bitmap,((capacity+31)//32)*4)).hex()),
   map_weapon_rows=[dict(index=i,address=f'{self.entries+i*8:08x}',raw_hex=bytes(self.g.mem_read(self.entries+i*8,8)).hex())for i in (0,1,12)])
 def item_state_id(self):
  value=self.item_snapshot();raw=json.dumps(value,sort_keys=True,separators=(',',':')).encode();h=hashlib.sha256(raw).hexdigest()
  if h not in self.item_state_ids:
   self.item_state_ids[h]=len(self.item_states);self.item_states.append(dict(id=len(self.item_states),sha256=h,**value))
  return self.item_state_ids[h]
 def item_returns(self,a):
  matching=[q for q in self.item_pending if q[0]==a and q[2]['phase']==self.active]
  if not matching:return
  owning_sp=matching[-1][1]
  for ret,sp,row in reversed(matching):
   if sp==owning_sp:
    row.update(returned_eax=f'{self.g.reg_read(UC_X86_REG_EAX):08x}',return_order=self.item_order,
     return_esp_delta=self.g.reg_read(UC_X86_REG_ESP)-sp)
    self.item_order+=1
    if int(row['entry'],16)in STATE_OWNERS:row['after_state']=self.item_state_id()
    if 'outer_output_pointer'in row:
     p=int(row['outer_output_pointer'],16);row['outer_result_hex']=bytes(self.g.mem_read(p,0x5c)).hex()
     if self.u(p)==1:
      collision,bsp=self.u(0x4e790c),self.u(0x4e7918);surface,plane,material=self.u(p+0x50),self.u(p+0x54),self.u(p+0x5a,2)
      row['actual_contact_cache']=dict(surface=surface,plane=f'{plane:08x}',material=material,
       surface_record=cache_record(self,self.u(collision+0x2c)+surface*8,8),
       plane_record=cache_record(self,self.u(collision+0xc)+(plane&0x7fff)*16,16),
       material_record=cache_record(self,self.u(bsp+0x10)+material*0x14,0x14))
    if 'lower_output_pointer'in row:
     p=int(row['lower_output_pointer'],16);descriptor=int(row['lower_descriptor_pointer'],16)
     row['lower_descriptor_after_hex']=bytes(self.g.mem_read(descriptor,0x28)).hex()
     row['lower_result_hex']=bytes(self.g.mem_read(p,0x420)).hex()
     row['visited_leaves']=[self.u(p+0x1c+n*4)for n in range(min(self.u(p+0x18),256))]
    # A tail jump (notablyB8040→C0270) shares the caller's return address
    # and original entry stack depth. Close both observed routine frames;
    # recursive calls at different stack depths remain pending.
    self.item_pending.remove((ret,sp,row))
 def hook(self,g,a,size,user):
  if self.item_observe:
   self.item_returns(a)
   if a in CALLBACK_SITES:
    self.item_callbacks.append(dict(instruction=f'{a:08x}',phase=self.active,order=self.item_order,
     target=f'{g.reg_read(UC_X86_REG_EAX):08x}',datum_argument=f'{self.u(g.reg_read(UC_X86_REG_ESP)):08x}',registers=self.registers()))
    self.item_order+=1
   if a in ENTRIES:
    sp=g.reg_read(UC_X86_REG_ESP)
    row=dict(id=len(self.item_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',phase=self.active,order=self.item_order,
     registers=self.registers(),stack=[f'{self.u(sp+4+i*4):08x}'for i in range(6)])
    self.item_order+=1
    if a in STATE_OWNERS:row['before_state']=self.item_state_id()
    if a==0x16b240:
     row.update(outer_mask=f'{self.u(sp+4):08x}',outer_start=self.vec(self.u(sp+8)),outer_delta=self.vec(self.u(sp+12)),
      outer_output_pointer=f'{self.u(sp+24):08x}',outer_ignored=[f'{self.u(sp+16):08x}',f'{self.u(sp+20):08x}'])
    if a==0x1e20c0:
     descriptor=self.u(sp+4);p=self.u(descriptor+0x18)
     row.update(lower_descriptor_pointer=f'{descriptor:08x}',lower_descriptor_before_hex=bytes(g.mem_read(descriptor,0x28)).hex(),
      lower_output_pointer=f'{p:08x}')
    if a==0x246a50:
     cluster=g.reg_read(UC_X86_REG_EAX);p=self.u(self.u(0x4e7918)+0xa0)+cluster*0xb0
     row.update(cluster=cluster,actual_cluster_cache=cache_record(self,p,0xb0),source_cluster_a8_count=self.u(p+0xa8))
    self.item_calls.append(row);self.item_pending.append((self.u(sp),sp,row))
  super().hook(g,a,size,user)
 def mem_write(self,g,access,address,size,value,user):
  if self.item_observe:
   base,cursor=self.u(0x4ed650),self.u(0x4ed654);game=self.u(0x4edf18)
   watched=(base<=address<base+cursor or game+0x1138<=address<game+0x11f8 or address==SEED or address==TIME+8)
   if watched:
    record=dict(phase=self.active,order=self.item_order,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',
     address=f'{address:08x}',size=size,before_hex=bytes(g.mem_read(address,size)).hex(),
     written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex())
    context=self.u(0x4e58c4)
    if context<=address<context+0x84:record['source_target']=dict(kind='object_context',pointer=f'{context:08x}',offset=f'{address-context:02x}')
    self.item_stores.append(record)
    self.item_order+=1
  super().mem_write(g,access,address,size,value,user)
 def item_stage(self,name,entry,args=(),regs=()):
  self.active=name;self.new_invalid=None;begin=len(self.events);before=self.item_state_id()
  try:self.run(entry,args,regs);completion='complete_original_return';self.item_returns(STOP)
  except (unicorn.UcError,KernelBoundary):
   assert self.new_invalid is not None
   completion='blocked_original_instruction'
  row=dict(name=name,entry=f'{entry:08x}',arguments=[f'{v:08x}'for v in args],
   input_registers={str(k):f'{v:08x}'for k,v in regs},completion=completion,
   stopped_eip=f'{self.g.reg_read(UC_X86_REG_EIP):08x}',invalid=self.new_invalid,
   before_state=before,after_state=self.item_state_id(),services=self.events[begin:])
  self.item_stages.append(row)
  return completion=='complete_original_return'

def instruction_evidence(t,out):
 entries=set(player_source.ENTRIES)|ENTRIES|{0x139750}
 for a in t.executed:
  i=t.code.at(a);assert t.image.read(a,len(i.bytes))==i.bytes
  assert bytes(t.g.mem_read(a,len(i.bytes)))==i.bytes
 def listing(name,ins):
  for i in ins:
   assert t.image.read(i.address,len(i.bytes))==i.bytes
   assert bytes(t.g.mem_read(i.address,len(i.bytes)))==i.bytes
  raw=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode()
  frozen_write(out/'instructions'/name,raw)
  return dict(file='instructions/'+name,sha256=hashlib.sha256(raw).hexdigest(),rows=len(ins),executed_rows=sum(i.address in t.executed for i in ins))
 result=[]
 for entry in sorted(entries):
  routine=_trace(t.code,entry,t.starts|entries)
  result.append(dict(entry=f'{entry:08x}',**listing(f'{entry:08x}.asm',[t.code.at(a)for a in sorted(routine.code)]),
   direct_calls=[dict(site=f'{a:08x}',target=f'{b:08x}')for a,b,k in routine.sites if k=='call']))
 result.append(dict(kind='executed_original_instructions',**listing('executed-original-instructions.asm',[t.code.at(a)for a in sorted(t.executed)])))
 result.append(dict(kind='observed_original_store_sites',**listing('observed-store-sites.asm',[t.code.at(a)for a in sorted({int(r['instruction'],16)for r in t.item_stores})])))
 return result

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');args=ap.parse_args()
 source_hash=digest(player_source.__file__)
 oracle_hash=digest(__file__)
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 proc=subprocess.Popen(['wine',str(player_source.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,
  env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(player_source.CONTEXT_OUT/'wine-prefix')))
 try:
  data=json.loads(proc.stdout.readline());t=player_source.prepare(data,trace_type=ActiveTrace)
  complete=t.player_stages[-1]['completion']=='complete_original_return'
  if complete:complete=player_source.spawn_and_activate(t,0,activate=False)
  owner=json.loads((player_source.OUT/'report.json').read_text())
  owner_case=next(c for c in owner['cases']if c['case']=='bound_player_first_ordinary_motion')
  assert t.player_stages==owner_case['stages'][:4],'Original owning player prefix changed under read-only observer'
  prefix_correlation=dict(source_report_sha256=digest(player_source.OUT/'report.json'),
   case=owner_case['case'],stage_indices=[0,1,2,3],byte_identical_original_stage_records=True)
  if complete:
   t.item_observe=True
   stages=[('original_netgame_row_initialization',0x15c320,(),((UC_X86_REG_ESI,t.entries),)),
    ('original_netgame_round_reset',0x15c370,(),((UC_X86_REG_EDI,t.entries),))]
   stages += [(f'original_authored_weapon_row_{i}_allocation',0x15c110,(t.entries,i),())for i in (0,1,12)]
   stages += [('original_source_player_PVS_and_object_activation',0x13a380,(),()),
    ('original_ordinary_object_pass',0xb7e00,(),()),('original_world_physics_pass',0x1c7660,(),()),
    ('original_dirty_object_pass',0xb7f50,(),()),('original_new_object_pass',0xb8040,(),())]
   for name,entry,arguments,registers in stages:
    if not t.item_stage(name,entry,arguments,registers):break
  assert digest(player_source.__file__)==source_hash,'Owning source extension changed during execution'
  assert digest(__file__)==oracle_hash,'Measurement tool changed during execution'
  for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
  listings=instruction_evidence(t,args.output.parent)
  result=dict(schema=1,oracle_sha256=oracle_hash,owning_player_tool_sha256=source_hash,
   preparation=t.producers,player_prefix_stages=t.player_stages,owning_prefix_correlation=prefix_correlation,
   stages=t.item_stages,calls=t.item_calls,callbacks=t.item_callbacks,stores=t.item_stores,states=t.item_states,
   resources=t.resource_delivery,services=t.events,executed_addresses=[f'{a:08x}'for a in sorted(t.executed)],
   source_pins=FROZEN,metadata=metadata(t),instructions=listings,instruction_patches=[],new_supplied_pose_contact_activation_fields=[],
   boundaries=owner['boundaries']+[
    'Read-only source player helper stops after actual14EAE0..14ED0C. Original15C320/15C370 and selected15C110 rows0/1/12 then execute before full13A380 and B7E00→1C7660→B7F50→B8040. This is a selected bounded producer composition, not the complete139750 frame or all authored world objects.',
    'Inherited netgame row-table backing and source-selected default game options/property inputs remain declared inputs. Exact authored object roots/rowindices and full allocated datums are retained; no near-root/activation/pose/contact/allocation/settling value is supplied.',
    'All occupied object bodies/current node banks/bounds/locations and full2048x12 header/bitmap/source1D88 queue records are literal stored snapshots. Source registry chains follow actual next pointers without sorting.',
    'The first normal item update is sourceB8040, after ordinary/physics/dirty passes skip new items. Positive BSP contact comes from unchanged actual collision cache traversals. Actual246A50 cluster-query bodies execute and return AL0 for both casts; cluster+A8 counts624/96 are retained without an inferred shape-count interpretation. Positive Havok/object/instanced contacts and subsequent continuous settling remain unproved.',
    'Original tick/rate and compatibility worker inputs remain inherited. Source clock/session/full title/input tail and complete live world are not established; Xbox reciprocal/contact parity remains open.',
    'Earlier optional attachment, sound and playback boundaries persist during spawn/allocation. No supplied game/Havok return executes in the four measured scheduled phases. Unexecuted deletion/queue/unavailable branches remain source-listed only.'])
  frozen_write(args.output,encoded(result))
  print(json.dumps(dict(output=str(args.output),sha256=digest(args.output),
   stages=[(r['name'],r['completion'],r['stopped_eip'])for r in t.item_stages],
   prefix=[(r['name'],r['completion'],r['stopped_eip'])for r in t.player_stages],calls=len(t.item_calls),states=len(t.item_states))))
 finally:
  _,err=proc.communicate('x',timeout=60)
 assert proc.returncode==0,err
if __name__=='__main__':main()
