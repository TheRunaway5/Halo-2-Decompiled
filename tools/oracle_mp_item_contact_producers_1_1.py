#!/usr/bin/env python3
"""Separate unchanged SMG contact/effect/marker/node producer capture.

Extends the immutable actual item-motion run only with read-only observers.
No poses, world records, admission or successful service results are assigned.
"""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
import oracle_mp_item_transform_producers_1_1 as transform
motion=transform.motion
from oracle_mp_item_motion_boundaries_1_1 import ROOT,digest,encoded,frozen_write
from oracle_mp_inventory_spatial_1_1 import cache_record
from functions import _trace

OUT=ROOT/'analysis/multiplayer/integration/inventory-source/item-contact-producers'
FROZEN={**transform.FROZEN,
 'tools/oracle_mp_item_transform_producers_1_1.py':'49a4c7371445aaabc4223264d61f0a4ee03a7f8bcd9adb4635b5a5043713e202',
 'tools/oracle_mp_item_transform_producers_audit_1_1.py':'597a7d37faa9bed09a4164b47c901c8a27a51661730c224035c4d7484058b7c3',
 'docs/MP_ITEM_TRANSFORM_PRODUCERS_RESEARCH.md':'8be6c033dda227e7f1922442fdc06fce4e6902dae50729888de20a4de3502548',
 'analysis/multiplayer/integration/inventory-source/item-transform-producers/report.json':'52935310c33d6b5df1f77db7ae5c9bc6b5971f8369cf8a7ae29cd86cc172f255',
 'analysis/multiplayer/integration/inventory-source/item-transform-producers/audit.json':'9e683f8ab3cfbb4c718fcd70ae8dd4ab0dcf05019aff32977d2f6cd2dafb54dc',
 'analysis/multiplayer/integration/inventory-source/item-transform-producers/manifest.json':'47fe2c930d84a847e682e6b4ed597df850b5087560cbc170992236896552aedb'}
ROOTS={0xfaf20,0xfbbe0,0xbdf80}
LEAVES={0xbdf10,0xbdf80,0xbede0,0xb96a0,0xb94f0,0x1ce650,0x1ce830,
 0xb97d0,0x16ceb0,0x16d2c0,0x16d4c0,0x11d4a0,0x14b9d0,0x14bd00,0x14bac0,
 0x16f3c0,0x143960,0x143860,0x142d00,0x1441d0,0xbeb30,0xba0c0,
 0x1c83b0,0x10d420,0x10d260,0x10ce60,0xbf0c0,0xbfe20,0xb7c50,0xb8180,
 0xb81f0,0xc0400,0xc0490,0xc0b80,0x326af0,0x1e1800,0x11d460,0x30bd0}
LEAVES|=transform.ROOTS|{0xb9c20,0xb9b90,0x1dc8f0,0x1dc940,0x143580,0x1218c0,0x189bb0,0x189e10,0x18a800,0x18b020,0x18ae30,0x178050}
ENTRIES=ROOTS|LEAVES
FRAME_SIZES={0xb8490:0x34,0xba1c0:0x88,0xb8320:0x14,0xbd710:0x2c,
 0xbdf80:0xf8,0xbede0:0x24,0xb94f0:0x70,0x1ce650:0x214,0x14bac0:0x428,0xfaf20:0x48,0xfbbe0:0xc0,0xb9c20:0x14,0x1dc940:0x1c,0x1218c0:4,0x189bb0:0x58,0x189e10:0x28,0xbea50:0x10}

class ContactTrace(transform.TransformTrace):
 def __init__(self,*args,**kwargs):
  self.ct_calls=[];self.ct_pending=[];self.ct_scope=None;self.ct_scopes=[];self.ct_states=[];self.ct_state_ids={}
  self.ct_reads=[];self.ct_stores=[];self.ct_points=[];self.ct_order=0;self.ct_executed=set()
  super().__init__(*args,**kwargs)
  self.ct_routines={e:_trace(self.code,e,self.starts|ENTRIES)for e in ENTRIES}
  for e in ROOTS|{0xbdf80,0xbede0,0xb94f0,0xb96a0,0x1ce650,0x1ce830}:
   for _,target,kind in self.ct_routines[e].sites:
    if kind=='call'and target not in self.ct_routines:self.ct_routines[target]=_trace(self.code,target,self.starts|ENTRIES)
  self.ct_owners={a:e for e,r in self.ct_routines.items()for a in r.code}
  self.ct_call_targets={b for r in self.ct_routines.values()for _,b,k in r.sites if k=='call'}
  self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.ct_read)
 def ct_pool(self,p):
  capacity,stride,data,bitmap=(self.u(p+n)for n in(0x20,0x24,0x44,0x48))
  return dict(pointer=f'{p:08x}',header4c_hex=self.raw(p,0x4c),capacity=capacity,stride=stride,
   data_pointer=f'{data:08x}',data_hex=self.raw(data,capacity*stride),
   bitmap_pointer=f'{bitmap:08x}',bitmap_hex=self.raw(bitmap,((capacity+31)//32)*4))
 def ct_snapshot(self):
  registries=[]
  for globalp in(0x4e58b0,0x4e58a4):
   head,forward,reverse=(self.u(globalp+i*4)for i in range(3))
   registries.append(dict(global_address=f'{globalp:08x}',triplet_hex=self.raw(globalp,12),
    heads_pointer=f'{head:08x}',heads800_hex=self.raw(head,0x800),forward=self.ct_pool(forward),reverse=self.ct_pool(reverse)))
  tags=[];maxnodes=0
  for definition in(0xe192001d,0xe57b0406):
   tag=self.cache.tags[definition];model=self.cache.tags[self.cache.u(tag['offset']+0x38)];render=self.cache.tags[self.cache.u(model['offset']+4)]
   maxnodes=max(maxnodes,self.cache.u(render['offset']+0x48))
   graph=self.cache.u(model['offset']+0x14)
   records=(tag,model,render,self.cache.tags[self.cache.u(tag['offset']+0x58)])
   if graph!=0xffffffff:records+=self.cache.tags[graph],
   for q in records:
    p=q['offset']+self.cache.magic
    tags.append(dict(datum=q['datum'],name=q['name'],group=q['group'],address=f'{p:08x}',file_offset=f'{q["offset"]:08x}',
     size=q['size'],raw_hex=self.raw(p,q['size'])))
  clusters=self.u(self.u(0x4e7918)+0x9c)
  return dict(current=self.item_snapshot(),registries=registries,tags=tags,
   source_graph_resources=[dict(r,raw_hex=self.raw(int(r['resident_pointer'],16),r['size']))for r in self.resource_delivery],
   source_fallback_node_bank=dict(address='004e38c0',size=maxnodes*0x20,raw_hex=self.raw(0x4e38c0,maxnodes*0x20)),
   source_effect_globals=dict(default_material=dict(address='00484de0',raw2_hex=self.raw(0x484de0,2)),vectors=[dict(global_address=f'{q:08x}',pointer=f'{self.u(q):08x}',raw12_hex=self.raw(self.u(q),12))for q in (0x46fc5c,0x46fc44)]),
   source_quaternion_lazy_constants=dict(address='00566760',raw8_hex=self.raw(0x566760,8)),
   source_cluster_visit_globals=dict(address='004ee9e0',prefix8_hex=self.raw(0x4ee9e0,8),
    visited_address='004ee9e8',cluster_count=clusters,visited_hex=self.raw(0x4ee9e8,clusters*4)))
 def ct_state(self):
  value=self.ct_snapshot();h=hashlib.sha256(json.dumps(value,sort_keys=True,separators=(',',':')).encode()).hexdigest()
  if h not in self.ct_state_ids:
   self.ct_state_ids[h]=len(self.ct_states);self.ct_states.append(dict(id=len(self.ct_states),sha256=h,**value))
  return self.ct_state_ids[h]
 def ct_frame(self):
  return[dict(call_id=r['id'],entry=r['entry'],address=f'{sp-FRAME_SIZES[int(r["entry"],16)]:08x}',
   raw_hex=self.raw(sp-FRAME_SIZES[int(r['entry'],16)],FRAME_SIZES[int(r['entry'],16)]+4))
   for _,sp,r in self.ct_pending if int(r['entry'],16)in FRAME_SIZES]
 def ct_packet(self,e,reg,s):
  result=transform.TransformTrace.tp_packet(self,e,reg,s)
  def p(label,address,size):result.append(dict(field=label,pointer=f'{address:08x}',size=size,raw_hex=self.raw(address,size)if address else None))
  if e==0xfbbe0:
   result=[q for q in result if q['field']!='eax_matrix34_output']
   p('eax_point12_input',reg['eax'],12)
  elif e==0xb9c20:p('ebx_marker70_output',reg['ebx'],(s[2]&0xffff)*0x70)
  elif e==0x1dc940:
   o=next(o for o in self.item_objects()if o['datum']==self.ct_scope['datum'])
   b=int(o['body_address'],16);n=self.u(b+0x114,2)//0x34
   p('s0_region_mapping',s[0],self.u(b+0x118,2));p('s1_node_mapping',s[1],n*4)
   p('s2_current_nodes34',s[2],n*0x34);p('s4_markers70_output',s[4],s[5]*0x70)
  elif e==0x143580:p('ecx_quaternion16',reg['ecx'],16);p('eax_rotation24_output',reg['eax'],0x24)
  elif e==0x1218c0:p('eax_velocity12_inout',reg['eax'],12);p('ecx_fallback12',reg['ecx'],12)
  elif e==0x189bb0:
   p('eax_contact_point12',reg['eax'],12);p('ecx_contact_normal12',reg['ecx'],12)
   p('s5_velocity12',s[5],12);p('s6_location8',s[6],8)
  elif e==0x189e10:
   for i,size in ((6,12),(7,12),(8,4),(9,4),(10,4),(11,4)):p(f's{i}_request_output',s[i],size)
  elif e==0x18a800:p('eax_vector12',reg['eax'],12)
  elif e==0xbea50:
   o=next(o for o in self.item_objects()if o['datum']==self.ct_scope['datum'])
   b=int(o['body_address'],16)
   assert s[2]==b+self.u(b+0x112,2)and s[1]==b+self.u(b+0x12a,2)
   p('s0_render_model50',s[0],0x50);p('s1_animation_manager6c',s[1],0x6c);p('s2_local_nodes20_inout',s[2],self.u(b+0x110,2))
  return result
 def ct_datum(self,e,reg,s):
  return s[0]
 def ct_returns(self,a):
  matches=[q for q in self.ct_pending if q[0]==a]
  if not matches:return
  depth=matches[-1][1]
  for ret,sp,r in reversed(matches):
   if sp!=depth:continue
   r.update(return_order=self.ct_order,return_cpu=self.cpu(),return_esp_delta=self.g.reg_read(UC_X86_REG_ESP)-sp,
    after_state=self.ct_state(),after_frames=self.ct_frame(),
    after_packets=[dict(q,raw_hex=self.raw(int(q['pointer'],16),q['size'])if int(q['pointer'],16)else None)for q in r['packets']])
   self.ct_order+=1;self.ct_pending.remove((ret,sp,r))
   if r['root_call']:
    assert not self.ct_pending
    self.ct_scopes.append(dict(id=r['scope']['id'],root_call_id=r['id'],row=r['scope']['row'],datum=r['scope']['datum'],
     entry=r['entry'],before_state=r['before_state'],after_state=r['after_state'],entry_order=r['order'],return_order=r['return_order']))
    self.ct_scope=None
 def hook(self,g,a,size,user):
  if self.item_observe and self.active in('original_new_object_pass','original_authored_weapon_row_12_allocation'):
   self.ct_returns(a)
   if a in ROOTS and self.ct_scope is None and ((a==0xbdf80)==(self.active=='original_authored_weapon_row_12_allocation')):
    sp=g.reg_read(UC_X86_REG_ESP);reg={k:int(v,16)for k,v in self.registers().items()};s=[self.u(sp+4+i*4)for i in range(16)]
    datum=self.ct_datum(a,reg,s);found=next((o for o in self.item_objects()if o['datum']==f'{datum:08x}'and o['definition']=='e57b0406'and o['parent']=='ffffffff'),None)
    if found:self.ct_scope=dict(id=len(self.ct_scopes),datum=found['datum'],row=found['row'],body=found['body_address'],body_size=found['body_size'],phase=self.active)
   if self.ct_scope:
    self.ct_executed.add(a)
    if a not in self.ct_routines and (a in self.starts or a in self.ct_call_targets):
     routine=_trace(self.code,a,self.starts|ENTRIES);self.ct_routines[a]=routine
     self.ct_owners.update({p:a for p in routine.code})
     self.ct_call_targets.update(b for _,b,k in routine.sites if k=='call')
    if a in self.ct_routines:
     sp=g.reg_read(UC_X86_REG_ESP);reg={k:int(v,16)for k,v in self.registers().items()};s=[self.u(sp+4+i*4)for i in range(16)]
     r=dict(id=len(self.ct_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',order=self.ct_order,scope=dict(self.ct_scope),
      root_call=not self.ct_pending,entry_cpu=self.cpu(),stack_words=[f'{v:08x}'for v in s],before_state=self.ct_state(),
      before_frames=self.ct_frame(),packets=self.ct_packet(a,reg,s))
     self.ct_order+=1;self.ct_calls.append(r);self.ct_pending.append((self.u(sp),sp,r))
    if a in self.ct_owners:
     ins=self.code.at(a)
     if ins.mnemonic.startswith(('j','f'))or ins.mnemonic in('call','divss','sqrtss','rcpss')or a==0x1439e3:
      self.ct_points.append(dict(order=self.ct_order,instruction=f'{a:08x}',owner=f'{self.ct_owners[a]:08x}',
       scope=dict(self.ct_scope),cpu=self.cpu(),frames=self.ct_frame(),stack20_hex=self.raw(g.reg_read(UC_X86_REG_ESP),0x20)));self.ct_order+=1
  super().hook(g,a,size,user)
 def ct_origin(self,p,size):
  b=int(self.ct_scope['body'],16)
  if b<=p and p+size<=b+self.ct_scope['body_size']:return dict(kind='owning_item_body',offset=f'{p-b:03x}')
  if 0x2000000<=p and p+size<=0x2200000:return dict(kind='original_stack')
  if self.cache.magic<=p and p+size<=self.cache.magic+len(self.cache.data):
   try:r=cache_record(self,p,size);return dict(kind='original_cache_operand',cache=r)
   except AssertionError:return dict(kind='source_modified_cache_operand')
  if 0x10000<=p and p+size<=0x810000:
   try:equal=self.image.read(p,size)==bytes(self.g.mem_read(p,size))
   except Exception:equal=False
   return dict(kind='original_xbe_bytes'if equal else'source_runtime_global')
  base,cursor=self.u(0x4ed650),self.u(0x4ed654)
  return dict(kind='source_game_arena'if base<=p and p+size<=base+cursor else'inherited_runtime_storage')
 def ct_read(self,g,access,address,size,value,user):
  if self.ct_scope and g.reg_read(UC_X86_REG_EIP)in self.ct_owners:
   self.ct_reads.append(dict(order=self.ct_order,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{address:08x}',
    size=size,raw_hex=self.raw(address,size),origin=self.ct_origin(address,size),scope=dict(self.ct_scope)));self.ct_order+=1
 def mem_write(self,g,access,address,size,value,user):
  if self.ct_scope:
   self.ct_stores.append(dict(order=self.ct_order,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{address:08x}',
    size=size,before_hex=self.raw(address,size),written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex(),scope=dict(self.ct_scope)));self.ct_order+=1
  super().mem_write(g,access,address,size,value,user)

def listings(t,out):
 result=[];entries=set(t.ct_routines)
 def save(name,ins,**fields):
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
  raw=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode();f='instructions/'+name;frozen_write(out/f,raw)
  result.append(dict(file=f,sha256=hashlib.sha256(raw).hexdigest(),rows=len(ins),**fields))
 for e in sorted(entries):
  r=_trace(t.code,e,t.starts|entries);save(f'{e:08x}.asm',[t.code.at(a)for a in sorted(r.code)],entry=f'{e:08x}',
   executed_rows=sum(a in t.ct_executed for a in r.code),direct_calls=[dict(site=f'{a:08x}',target=f'{b:08x}',executed=a in t.ct_executed)for a,b,k in r.sites if k=='call'])
 save('executed-contact-instructions.asm',[t.code.at(a)for a in sorted(t.ct_executed)],kind='executed_in_contact_scopes')
 return result

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');args=ap.parse_args()
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 own=json.loads((motion.OUT/'report.json').read_text());base=json.loads((motion.owning.OUT/'report.json').read_text());tool_hash=digest(__file__)
 proc=subprocess.Popen(['wine',str(motion.owning.player_source.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,
  env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(motion.owning.player_source.CONTEXT_OUT/'wine-prefix')))
 try:
  t=motion.owning.player_source.prepare(json.loads(proc.stdout.readline()),trace_type=ContactTrace)
  assert t.player_stages[-1]['completion']=='complete_original_return';assert motion.owning.player_source.spawn_and_activate(t,0,activate=False)
  t.item_observe=True
  for r in base['stages']:assert t.item_stage(r['name'],int(r['entry'],16),tuple(int(v,16)for v in r['arguments']),tuple((int(k),int(v,16))for k,v in r['input_registers'].items()))
  reproduced=dict(calls=t.motion_calls,states=t.motion_states,reads=t.motion_reads,stores=t.motion_stores,points=t.motion_points,
   definitions=list(t.motion_definitions.values()),executed_in_scope={str(row):[f'{a:08x}'for a in sorted(v)]for row,v in t.motion_executed.items()})
  for k,v in reproduced.items():assert v==own[k],('Owning motion output changed',k)
  active=dict(preparation=t.producers,player_prefix_stages=t.player_stages,stages=t.item_stages,calls=t.item_calls,callbacks=t.item_callbacks,
   stores=t.item_stores,states=t.item_states,resources=t.resource_delivery,services=t.events,executed_addresses=[f'{a:08x}'for a in sorted(t.executed)],metadata=motion.owning.metadata(t))
  for k,v in active.items():assert v==base[k],('Owning active output changed',k)
  original_transform=json.loads((transform.OUT/'report.json').read_text())
  transformed=dict(calls=t.tp_calls,scopes=t.tp_scopes,states=t.tp_states,reads=t.tp_reads,stores=t.tp_stores,points=t.tp_points,executed_in_scope=[f'{a:08x}'for a in sorted(t.tp_executed)])
  for k,v in transformed.items():assert v==original_transform[k],('Owning transform output changed',k)
  assert not t.ct_pending and t.ct_scope is None
  for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
  assert digest(__file__)==tool_hash
  d=dict(schema=1,oracle_sha256=tool_hash,source_pins=FROZEN,calls=t.ct_calls,scopes=t.ct_scopes,states=t.ct_states,
   reads=t.ct_reads,stores=t.ct_stores,points=t.ct_points,executed_in_scope=[f'{a:08x}'for a in sorted(t.ct_executed)],
   instructions=listings(t,args.output.parent),owning_transform_correlation=dict(report_sha256=digest(transform.OUT/'report.json'),keys=list(transformed),runtime_sha256=hashlib.sha256(encoded(transformed)).hexdigest(),byte_identical=True),owning_motion_correlation=dict(report_sha256=digest(motion.OUT/'report.json'),
    keys=list(reproduced),runtime_sha256=hashlib.sha256(encoded(reproduced)).hexdigest(),byte_identical=True),
   owning_active_correlation=dict(report_sha256=digest(motion.owning.OUT/'report.json'),keys=list(active),runtime_sha256=hashlib.sha256(encoded(active)).hexdigest(),byte_identical=True),
   boundaries=own['boundaries']+['Read-only unchanged SMG allocation-node/contact/effect scopes selected from actual source-owned full datums and original stage. No pose, world record, success/admission/result or original instruction is supplied/changed.',
    'Complete registry heads and both forward/reverse pool records/data/bitmaps are actual owning buffers. Source cache/static/fallback inputs and original math scratch/pointer aliasing remain literal.',
    'Only the original SMG row12 allocation and first admitted contact/effect schedule are observed. Parent/child, non-NONE physical/network, animated/Havok and alternative BSP portal branches outside actual coverage remain source-listed or unresolved; no complete settling/frame/Xbox parity claim.'],
   instruction_patches=[],new_guest_inputs=[],new_supplied_service_results=[])
  frozen_write(args.output,encoded(d));print(json.dumps(dict(output=str(args.output),sha256=digest(args.output),scopes=len(t.ct_scopes),calls=len(t.ct_calls),states=len(t.ct_states),reads=len(t.ct_reads),stores=len(t.ct_stores),points=len(t.ct_points))))
 finally:
  forced=False
  try:_,err=proc.communicate('x',timeout=15)
  except subprocess.TimeoutExpired:forced=True;proc.kill();_,err=proc.communicate(timeout=15)
 assert proc.returncode==0 or forced,err
if __name__=='__main__':main()
