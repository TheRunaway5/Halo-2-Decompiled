#!/usr/bin/env python3
"""Separate unchanged transform/node/bounds/location/registry producer capture.

Extends the immutable actual item-motion run only with read-only observers.
No poses, world records, admission or successful service results are assigned.
"""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
import oracle_mp_item_motion_boundaries_1_1 as motion
from oracle_mp_item_motion_boundaries_1_1 import ROOT,digest,encoded,frozen_write
from oracle_mp_inventory_spatial_1_1 import cache_record
from functions import _trace

OUT=ROOT/'analysis/multiplayer/integration/inventory-source/item-transform-producers'
FROZEN={**motion.FROZEN,
 'tools/oracle_mp_item_motion_boundaries_1_1.py':'397bf7574733573884f839eaecee40a9aa787b31fa27152f83c6829a31a9bc92',
 'tools/oracle_mp_item_motion_boundaries_audit_1_1.py':'406b06eb48b0eaf3826a0d63c664c8df10d2aa53614bb021dd4ba33863043e6a',
 'docs/MP_ITEM_MOTION_BOUNDARIES_RESEARCH.md':'c284cdeee1530324246f5a9212405f2c62e7021fa793851fe204989870d5d11c',
 'analysis/multiplayer/integration/inventory-source/item-motion-boundaries/report.json':'bcd0ca394ee876841e4eba140cff5430fa55809c3adeb429395902e05f759f52',
 'analysis/multiplayer/integration/inventory-source/item-motion-boundaries/audit.json':'db3896e707f3a5169669b2d0c14fdeed7d7ec6fefe713600d1adb5bb48508954',
 'analysis/multiplayer/integration/inventory-source/item-motion-boundaries/manifest.json':'34015ee4eee80df466c0e5187780d45e711f8ec61022aa23dff08cc2aae3b072'}
ROOTS={0xb8490,0xb8250,0xba1c0,0xb8320,0xbd710}
LEAVES={0xbdf10,0xbdf80,0xbede0,0xb96a0,0xb94f0,0x1ce650,0x1ce830,
 0xb97d0,0x16ceb0,0x16d2c0,0x16d4c0,0x11d4a0,0x14b9d0,0x14bd00,0x14bac0,
 0x16f3c0,0x143960,0x143860,0x142d00,0x1441d0,0xbeb30,0xba0c0,
 0x1c83b0,0x10d420,0x10d260,0x10ce60,0xbf0c0,0xbfe20,0xb7c50,0xb8180,
 0xb81f0,0xc0400,0xc0490,0xc0b80,0x326af0,0x1e1800,0x11d460,0x30bd0}
ENTRIES=ROOTS|LEAVES
FRAME_SIZES={0xb8490:0x34,0xba1c0:0x88,0xb8320:0x14,0xbd710:0x2c,
 0xbdf80:0xf8,0xbede0:0x24,0xb94f0:0x70,0x1ce650:0x214,0x14bac0:0x428}

class TransformTrace(motion.MotionTrace):
 def __init__(self,*args,**kwargs):
  self.tp_calls=[];self.tp_pending=[];self.tp_scope=None;self.tp_scopes=[];self.tp_states=[];self.tp_state_ids={}
  self.tp_reads=[];self.tp_stores=[];self.tp_points=[];self.tp_order=0;self.tp_executed=set()
  super().__init__(*args,**kwargs)
  self.tp_routines={e:_trace(self.code,e,self.starts|ENTRIES)for e in ENTRIES}
  for e in ROOTS|{0xbdf80,0xbede0,0xb94f0,0xb96a0,0x1ce650,0x1ce830}:
   for _,target,kind in self.tp_routines[e].sites:
    if kind=='call'and target not in self.tp_routines:self.tp_routines[target]=_trace(self.code,target,self.starts|ENTRIES)
  self.tp_owners={a:e for e,r in self.tp_routines.items()for a in r.code}
  self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.tp_read)
 def tp_pool(self,p):
  capacity,stride,data,bitmap=(self.u(p+n)for n in(0x20,0x24,0x44,0x48))
  return dict(pointer=f'{p:08x}',header4c_hex=self.raw(p,0x4c),capacity=capacity,stride=stride,
   data_pointer=f'{data:08x}',data_hex=self.raw(data,capacity*stride),
   bitmap_pointer=f'{bitmap:08x}',bitmap_hex=self.raw(bitmap,((capacity+31)//32)*4))
 def tp_snapshot(self):
  registries=[]
  for globalp in(0x4e58b0,0x4e58a4):
   head,forward,reverse=(self.u(globalp+i*4)for i in range(3))
   registries.append(dict(global_address=f'{globalp:08x}',triplet_hex=self.raw(globalp,12),
    heads_pointer=f'{head:08x}',heads800_hex=self.raw(head,0x800),forward=self.tp_pool(forward),reverse=self.tp_pool(reverse)))
  tags=[];maxnodes=0
  for definition in(0xe192001d,0xe57b0406):
   tag=self.cache.tags[definition];model=self.cache.tags[self.cache.u(tag['offset']+0x38)];render=self.cache.tags[self.cache.u(model['offset']+4)]
   maxnodes=max(maxnodes,self.cache.u(render['offset']+0x48))
   for q in(tag,model,render):
    p=q['offset']+self.cache.magic
    tags.append(dict(datum=q['datum'],name=q['name'],group=q['group'],address=f'{p:08x}',file_offset=f'{q["offset"]:08x}',
     size=q['size'],raw_hex=self.raw(p,q['size'])))
  clusters=self.u(self.u(0x4e7918)+0x9c)
  return dict(current=self.item_snapshot(),registries=registries,tags=tags,
   source_fallback_node_bank=dict(address='004e38c0',size=maxnodes*0x20,raw_hex=self.raw(0x4e38c0,maxnodes*0x20)),
   source_quaternion_lazy_constants=dict(address='00566760',raw8_hex=self.raw(0x566760,8)),
   source_cluster_visit_globals=dict(address='004ee9e0',prefix8_hex=self.raw(0x4ee9e0,8),
    visited_address='004ee9e8',cluster_count=clusters,visited_hex=self.raw(0x4ee9e8,clusters*4)))
 def tp_state(self):
  value=self.tp_snapshot();h=hashlib.sha256(json.dumps(value,sort_keys=True,separators=(',',':')).encode()).hexdigest()
  if h not in self.tp_state_ids:
   self.tp_state_ids[h]=len(self.tp_states);self.tp_states.append(dict(id=len(self.tp_states),sha256=h,**value))
  return self.tp_state_ids[h]
 def tp_frame(self):
  return[dict(call_id=r['id'],entry=r['entry'],address=f'{sp-FRAME_SIZES[int(r["entry"],16)]:08x}',
   raw_hex=self.raw(sp-FRAME_SIZES[int(r['entry'],16)],FRAME_SIZES[int(r['entry'],16)]+4))
   for _,sp,r in self.tp_pending if int(r['entry'],16)in FRAME_SIZES]
 def tp_packet(self,e,reg,s):
  result=motion.MotionTrace.packet(self,e,reg,s)
  def p(label,address,size):result.append(dict(field=label,pointer=f'{address:08x}',size=size,raw_hex=self.raw(address,size)if address else None))
  if e==0xba1c0:
   p('s0_marker_record6c',s[0],0x6c);p('s1_world_matrix34',s[1],0x34)
  elif e==0x143960:p('s0_matrix34_output',s[0],0x34);p('s1_node_record20',s[1],0x20)
  elif e==0x16f3c0:
   count=self.u(s[0]+0x48);p('s0_render_model50',s[0],0x50);p('render4c_nodes60',self.u(s[0]+0x4c),count*0x60);p('s1_nodes20_output',s[1],count*0x20)
  elif e==0xbeb30:
   p('eax_up12',reg['eax'],12);p('ecx_forward12',reg['ecx'],12);p('s0_root12',s[0],12)
   p('s3_parent_matrix34',s[3],0x34);p('s4_matrix34_output',s[4],0x34)
  elif e==0x11d4a0:p('edx_point12',reg['edx'],12);p('esi_location8_output',reg['esi'],8)
  elif e==0x14b9d0:p('ecx_collision40',reg['ecx'],0x40);p('edx_point12',reg['edx'],12)
  elif e==0xb94f0:p('s1_location8',s[1],8)
  elif e==0xb97d0:p('edi_element14_output',reg['edi'],0x14)
  elif e==0x14bd00:
   count=s[3]&0xffff;count=count if count<0x8000 else count-0x10000
   p('s1_center12',s[1],12);p('s4_clusters_output',s[4],max(0,count)*2)
  elif e==0x14bac0:
   p('ecx_center12',reg['ecx'],12);p('edx_bsp_a4',reg['edx'],0xa4)
   index=reg['eax']&0xffff;index=index if index<0x8000 else index-0x10000
   p('source_ax_portal24',self.u(reg['edx']+0x60)+index*0x24,0x24)
  elif e==0x1ce650:
   for label,address,size in(('s0_registry_triplet',s[0],12),('s2_reverse_head',s[2],4),('s3_center12',s[3],12),
    ('s5_location8',s[5],8),('s7_element',s[7],s[6]),('s8_overflow_output',s[8],1)):p(label,address,size)
  elif e==0x1ce830:p('edi_registry_triplet',reg['edi'],12);p('s1_reverse_head',s[1],4)
  return result
 def tp_datum(self,e,reg,s):
  if e==0xb8250:return reg['eax']
  if e==0xba1c0:return reg['ebx']
  return s[0]
 def tp_returns(self,a):
  matches=[q for q in self.tp_pending if q[0]==a]
  if not matches:return
  depth=matches[-1][1]
  for ret,sp,r in reversed(matches):
   if sp!=depth:continue
   r.update(return_order=self.tp_order,return_cpu=self.cpu(),return_esp_delta=self.g.reg_read(UC_X86_REG_ESP)-sp,
    after_state=self.tp_state(),after_frames=self.tp_frame(),
    after_packets=[dict(q,raw_hex=self.raw(int(q['pointer'],16),q['size'])if int(q['pointer'],16)else None)for q in r['packets']])
   self.tp_order+=1;self.tp_pending.remove((ret,sp,r))
   if r['root_call']:
    assert not self.tp_pending
    self.tp_scopes.append(dict(id=r['scope']['id'],root_call_id=r['id'],row=r['scope']['row'],datum=r['scope']['datum'],
     entry=r['entry'],before_state=r['before_state'],after_state=r['after_state'],entry_order=r['order'],return_order=r['return_order']))
    self.tp_scope=None
 def hook(self,g,a,size,user):
  if self.item_observe and self.active=='original_new_object_pass':
   self.tp_returns(a)
   if a in ROOTS and self.tp_scope is None:
    sp=g.reg_read(UC_X86_REG_ESP);reg={k:int(v,16)for k,v in self.registers().items()};s=[self.u(sp+4+i*4)for i in range(9)]
    datum=self.tp_datum(a,reg,s);found=next((o for o in self.item_objects()if o['datum']==f'{datum:08x}'and o.get('row')in(0,1,12)and o['parent']=='ffffffff'),None)
    if found:self.tp_scope=dict(id=len(self.tp_scopes),datum=found['datum'],row=found['row'],body=found['body_address'],body_size=found['body_size'])
   if self.tp_scope:
    self.tp_executed.add(a)
    if a in self.tp_routines:
     sp=g.reg_read(UC_X86_REG_ESP);reg={k:int(v,16)for k,v in self.registers().items()};s=[self.u(sp+4+i*4)for i in range(9)]
     r=dict(id=len(self.tp_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',order=self.tp_order,scope=dict(self.tp_scope),
      root_call=not self.tp_pending,entry_cpu=self.cpu(),stack_words=[f'{v:08x}'for v in s],before_state=self.tp_state(),
      before_frames=self.tp_frame(),packets=self.tp_packet(a,reg,s))
     self.tp_order+=1;self.tp_calls.append(r);self.tp_pending.append((self.u(sp),sp,r))
    if a in self.tp_owners:
     ins=self.code.at(a)
     if ins.mnemonic.startswith(('j','f'))or ins.mnemonic in('call','divss','sqrtss','rcpss')or a==0x1439e3:
      self.tp_points.append(dict(order=self.tp_order,instruction=f'{a:08x}',owner=f'{self.tp_owners[a]:08x}',
       scope=dict(self.tp_scope),cpu=self.cpu(),frames=self.tp_frame(),stack20_hex=self.raw(g.reg_read(UC_X86_REG_ESP),0x20)));self.tp_order+=1
  super().hook(g,a,size,user)
 def tp_origin(self,p,size):
  b=int(self.tp_scope['body'],16)
  if b<=p and p+size<=b+self.tp_scope['body_size']:return dict(kind='owning_item_body',offset=f'{p-b:03x}')
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
 def tp_read(self,g,access,address,size,value,user):
  if self.tp_scope and g.reg_read(UC_X86_REG_EIP)in self.tp_owners:
   self.tp_reads.append(dict(order=self.tp_order,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{address:08x}',
    size=size,raw_hex=self.raw(address,size),origin=self.tp_origin(address,size),scope=dict(self.tp_scope)));self.tp_order+=1
 def mem_write(self,g,access,address,size,value,user):
  if self.tp_scope:
   self.tp_stores.append(dict(order=self.tp_order,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{address:08x}',
    size=size,before_hex=self.raw(address,size),written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex(),scope=dict(self.tp_scope)));self.tp_order+=1
  super().mem_write(g,access,address,size,value,user)

def listings(t,out):
 result=[];entries=set(t.tp_routines)
 def save(name,ins,**fields):
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
  raw=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode();f='instructions/'+name;frozen_write(out/f,raw)
  result.append(dict(file=f,sha256=hashlib.sha256(raw).hexdigest(),rows=len(ins),**fields))
 for e in sorted(entries):
  r=_trace(t.code,e,t.starts|entries);save(f'{e:08x}.asm',[t.code.at(a)for a in sorted(r.code)],entry=f'{e:08x}',
   executed_rows=sum(a in t.tp_executed for a in r.code),direct_calls=[dict(site=f'{a:08x}',target=f'{b:08x}',executed=a in t.tp_executed)for a,b,k in r.sites if k=='call'])
 save('executed-transform-instructions.asm',[t.code.at(a)for a in sorted(t.tp_executed)],kind='executed_in_transform_scopes')
 return result

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');args=ap.parse_args()
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 own=json.loads((motion.OUT/'report.json').read_text());base=json.loads((motion.owning.OUT/'report.json').read_text());tool_hash=digest(__file__)
 proc=subprocess.Popen(['wine',str(motion.owning.player_source.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,
  env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(motion.owning.player_source.CONTEXT_OUT/'wine-prefix')))
 try:
  t=motion.owning.player_source.prepare(json.loads(proc.stdout.readline()),trace_type=TransformTrace)
  assert t.player_stages[-1]['completion']=='complete_original_return';assert motion.owning.player_source.spawn_and_activate(t,0,activate=False)
  t.item_observe=True
  for r in base['stages']:assert t.item_stage(r['name'],int(r['entry'],16),tuple(int(v,16)for v in r['arguments']),tuple((int(k),int(v,16))for k,v in r['input_registers'].items()))
  reproduced=dict(calls=t.motion_calls,states=t.motion_states,reads=t.motion_reads,stores=t.motion_stores,points=t.motion_points,
   definitions=list(t.motion_definitions.values()),executed_in_scope={str(row):[f'{a:08x}'for a in sorted(v)]for row,v in t.motion_executed.items()})
  for k,v in reproduced.items():assert v==own[k],('Owning motion output changed',k)
  active=dict(preparation=t.producers,player_prefix_stages=t.player_stages,stages=t.item_stages,calls=t.item_calls,callbacks=t.item_callbacks,
   stores=t.item_stores,states=t.item_states,resources=t.resource_delivery,services=t.events,executed_addresses=[f'{a:08x}'for a in sorted(t.executed)],metadata=motion.owning.metadata(t))
  for k,v in active.items():assert v==base[k],('Owning active output changed',k)
  assert not t.tp_pending and t.tp_scope is None
  for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
  assert digest(__file__)==tool_hash
  d=dict(schema=1,oracle_sha256=tool_hash,source_pins=FROZEN,calls=t.tp_calls,scopes=t.tp_scopes,states=t.tp_states,
   reads=t.tp_reads,stores=t.tp_stores,points=t.tp_points,executed_in_scope=[f'{a:08x}'for a in sorted(t.tp_executed)],
   instructions=listings(t,args.output.parent),owning_motion_correlation=dict(report_sha256=digest(motion.OUT/'report.json'),
    keys=list(reproduced),runtime_sha256=hashlib.sha256(encoded(reproduced)).hexdigest(),byte_identical=True),
   owning_active_correlation=dict(report_sha256=digest(motion.owning.OUT/'report.json'),keys=list(active),runtime_sha256=hashlib.sha256(encoded(active)).hexdigest(),byte_identical=True),
   boundaries=own['boundaries']+['Read-only unchanged transform/dirty scopes selected from actual full world-item datums in sourceB8040. No pose, world record, success/admission/result or original instruction is supplied/changed.',
    'Complete registry heads and both forward/reverse pool records/data/bitmaps are actual owning buffers. Source cache/static/fallback inputs and original math scratch/pointer aliasing remain literal.',
    'Only this original first item motion/dirty schedule is executed. Parent/child, non-NONE physical/network, animated/Havok and alternative BSP portal branches outside actual coverage remain source-listed or unresolved; no complete settling/frame/Xbox parity claim.'],
   instruction_patches=[],new_guest_inputs=[],new_supplied_service_results=[])
  frozen_write(args.output,encoded(d));print(json.dumps(dict(output=str(args.output),sha256=digest(args.output),scopes=len(t.tp_scopes),calls=len(t.tp_calls),states=len(t.tp_states),reads=len(t.tp_reads),stores=len(t.tp_stores),points=len(t.tp_points))))
 finally:
  forced=False
  try:_,err=proc.communicate('x',timeout=15)
  except subprocess.TimeoutExpired:forced=True;proc.kill();_,err=proc.communicate(timeout=15)
 assert proc.returncode==0 or forced,err
if __name__=='__main__':main()
