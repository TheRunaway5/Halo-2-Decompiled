#!/usr/bin/env python3
"""Read-only original item motion service/operand capture on the frozen world.

No guest inputs, contacts, admission, activation, or service returns are added.
The entire owning active-item runtime report must reproduce unchanged.
"""
import argparse, collections, hashlib, json, os, subprocess
from pathlib import Path
import unicorn
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP,
 UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_XMM0, UC_X86_REG_FP0,
 UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG, UC_X86_REG_MXCSR,
 UC_X86_REG_EFLAGS)
import oracle_mp_item_active_schedule_1_1 as owning
from oracle_mp_item_active_schedule_1_1 import ROOT, digest, encoded, frozen_write
from oracle_mp_inventory_spatial_1_1 import cache_record
from oracle_smg_firing_1_1 import STOP
from functions import _trace

OUT=ROOT/'analysis/multiplayer/integration/inventory-source/item-motion-boundaries'
FROZEN={**owning.FROZEN,
 'tools/oracle_mp_item_active_schedule_1_1.py':'fd63e85b9b799bcf5654b11873ae08bb782d7d0f4e22b64a1c5075287c54dc23',
 'tools/oracle_mp_item_active_schedule_audit_1_1.py':'83046c94612efee322419884acabd85382e1f900c31d23087727f77d333e0ace',
 'docs/MP_ITEM_ACTIVE_SCHEDULE_RESEARCH.md':'5c2f27e60b87544f8d2f79a9308d5d978197ea3c5b297f40b0eadacfd693004f',
 'analysis/multiplayer/integration/inventory-source/item-active-schedule/report.json':'e347e495f8c37fb3f56b69835692754cb65b77947a8459fea34352ce61ce4be2',
 'analysis/multiplayer/integration/inventory-source/item-active-schedule/audit.json':'8bd757188504fb2d9ca69964650fb61dca29d4c3b9f42132320fdada396bf830',
 'analysis/multiplayer/integration/inventory-source/item-active-schedule/manifest.json':'aeabd0ea8833e620c403ea301e805e819fb4882740e8baddcf9e507c4bc84150'}
CORE={0xf9b10,0xf9ea0}
NAMED={0xf99a0,0x10db90,0xb8490,0xb8630,0x1c8430,0xfbac0,0xfaf20,0xfb620}
HELPERS={0xfbbe0,0xba1c0,0xba0c0,0xbaa80,0xb8320,0xb8250,0x10d420,
 0x143860,0x142d00,0x1441d0,0x30bd0,0xb9c20,0x1dc8f0,0x1dc940,
 0x1218c0,0xbc910,0x16b240,0x16a9c0,0x1e20c0,0x246a50,
 0x189bb0,0x18a800,0x18b020}
ENTRIES=CORE|NAMED|HELPERS

class MotionTrace(owning.ActiveTrace):
 def __init__(self,*args,**kwargs):
  self.motion_calls=[];self.motion_pending=[];self.motion_stores=[];self.motion_reads=[]
  self.motion_points=[];self.motion_states=[];self.motion_state_ids={};self.motion_order=0
  self.motion_scope=None;self.motion_definitions={};self.motion_instruction_owner={}
  self.motion_executed={row:set()for row in(0,1,12)}
  super().__init__(*args,**kwargs)
  # Complete unchanged routines define observation coverage only.
  self.motion_routines={e:_trace(self.code,e,self.starts|ENTRIES)for e in ENTRIES}
  for e in CORE|NAMED|HELPERS-{0x16b240,0x16a9c0,0x1e20c0,0x246a50,0x189bb0,0x18a800,0x18b020}:
   for a in self.motion_routines[e].code:self.motion_instruction_owner[a]=e
  for e in CORE:
   for _,target,kind in self.motion_routines[e].sites:
    if kind=='call' and target not in self.motion_routines:
     self.motion_routines[target]=_trace(self.code,target,self.starts|ENTRIES)
  self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.motion_read)
 def raw(self,p,size):return bytes(self.g.mem_read(p,size)).hex()
 def cpu(self):
  return dict(registers=self.registers(),esp=f'{self.g.reg_read(UC_X86_REG_ESP):08x}',
   eflags=f'{self.g.reg_read(UC_X86_REG_EFLAGS):08x}',
   xmm128=[f'{self.g.reg_read(UC_X86_REG_XMM0+i):032x}'for i in range(8)],
   fpcw=f'{self.g.reg_read(UC_X86_REG_FPCW):04x}',fpsw=f'{self.g.reg_read(UC_X86_REG_FPSW):04x}',
   fptag=f'{self.g.reg_read(UC_X86_REG_FPTAG):04x}',mxcsr=f'{self.g.reg_read(UC_X86_REG_MXCSR):08x}',
   fp80=[list(self.g.reg_read(UC_X86_REG_FP0+i))for i in range(8)])
 def state_id(self):
  value=self.item_snapshot();raw=json.dumps(value,sort_keys=True,separators=(',',':')).encode();h=hashlib.sha256(raw).hexdigest()
  if h not in self.motion_state_ids:
   self.motion_state_ids[h]=len(self.motion_states);self.motion_states.append(dict(id=len(self.motion_states),sha256=h,**value))
  return self.motion_state_ids[h]
 def frame(self):
  if not self.motion_scope:return None
  return [dict(call_id=r['id'],entry=r['entry'],address=f'{sp-size:08x}',size=size+8,
   raw_hex=self.raw(sp-size,size+8))for _,sp,r in self.motion_pending if int(r['entry'],16)in CORE
   for size in (0xbc if int(r['entry'],16)==0xf9b10 else 0x13c,)]
 def packet(self,entry,registers,stack):
  # Each pointed-buffer size below follows the unchanged routine's literal reads.
  result=[]
  def pointer(label,p,size):
   result.append(dict(field=label,pointer=f'{p:08x}',size=size,raw_hex=self.raw(p,size)if p else None))
  if entry==0x10db90:
   pointer('ebx_linear_inout',registers['ebx'],12);pointer('stack1_angular_output',stack[1],12)
  elif entry==0xb8490:
   for label,key in (('eax_point','eax'),('ecx_forward','ecx'),('edx_up','edx')):pointer(label,registers[key],12)
   pointer('stack1_location8',stack[1],8)
  elif entry==0xb8630:
   pointer('ecx_linear',registers['ecx'],12);pointer('edi_angular',registers['edi'],12)
  elif entry==0x1c8430:
   pointer('ebx_linear',registers['ebx'],12);pointer('stack0_angular',stack[0],12)
  elif entry==0xfaf20:
   pointer('ecx_velocity',registers['ecx'],12);pointer('esi_contact5c',registers['esi'],0x5c)
   pointer('stack1_point',stack[1],12)
  elif entry==0xfb620:pointer('stack0_impulse',stack[0],12)
  elif entry==0xfbbe0:
   pointer('eax_matrix34_output',registers['eax'],0x34);pointer('edi_normal',registers['edi'],12)
   pointer('stack1_point',stack[1],12)
  elif entry==0x30bd0:pointer('eax_vector',registers['eax'],12)
  elif entry==0x10d420:
   pointer('stack0_point',stack[0],12);pointer('stack1_forward',stack[1],12)
  elif entry==0xb8320:
   for i,label,size in ((1,'stack1_point',12),(2,'stack2_forward',12),(3,'stack3_up',12),(4,'stack4_location8',8)):
    pointer(label,stack[i],size)
  elif entry==0x143860:
   pointer('eax_up',registers['eax'],12);pointer('ecx_forward',registers['ecx'],12)
   pointer('esi_point',registers['esi'],12);pointer('edx_matrix34_output',registers['edx'],0x34)
  elif entry==0x142d00:
   pointer('eax_matrix34_output',registers['eax'],0x34);pointer('ecx_matrix34_input',registers['ecx'],0x34)
  elif entry==0x1441d0:
   pointer('ecx_matrix34_left',registers['ecx'],0x34);pointer('edx_matrix34_right',registers['edx'],0x34)
   pointer('stack0_matrix34_output',stack[0],0x34)
  elif entry==0x16b240:
   pointer('stack1_start',stack[1],12);pointer('stack2_delta',stack[2],12);pointer('stack5_result5c',stack[5],0x5c)
  elif entry==0x1e20c0:
   pointer('stack0_descriptor28',stack[0],0x28);pointer('descriptor18_result420',self.u(stack[0]+0x18),0x420)
  return result
 def origin(self,p,size):
  scope=self.motion_scope;body=int(scope['body'],16)
  if body<=p and p+size<=body+scope['body_size']:return dict(kind='owning_item_body',offset=f'{p-body:03x}')
  for q,r in self.motion_definitions.items():
   if q<=p and p+size<=q+r['size']:return dict(kind='original_weapon_definition',definition=r['datum'],offset=f'{p-q:03x}')
  if 0x2000000<=p and p+size<=0x2200000:return dict(kind='original_stack')
  c=self.cache
  if c.magic<=p and p+size<=c.magic+len(c.data):return dict(kind='original_cache_metadata',cache=cache_record(self,p,size))
  # Relocated collision operands are retained by the original outer/lower captures.
  if 0x10000<=p and p+size<=0x810000:
   try:equal=self.image.read(p,size)==bytes(self.g.mem_read(p,size))
   except Exception:equal=False
   return dict(kind='original_xbe_bytes'if equal else'source_runtime_global')
  base,cursor=self.u(0x4ed650),self.u(0x4ed654)
  return dict(kind='source_game_arena'if base<=p and p+size<=base+cursor else'inherited_runtime_storage')
 def motion_returns(self,a):
  matches=[q for q in self.motion_pending if q[0]==a]
  if not matches:return
  depth=matches[-1][1]
  for ret,sp,row in reversed(matches):
   if sp!=depth:continue
   row.update(after_state=self.state_id(),return_order=self.motion_order,return_cpu=self.cpu(),
    return_esp_delta=self.g.reg_read(UC_X86_REG_ESP)-sp,after_frames=self.frame(),
    after_packets=[dict(q,raw_hex=self.raw(int(q['pointer'],16),q['size'])if int(q['pointer'],16)else None)for q in row['packets']])
   self.motion_order+=1;self.motion_pending.remove((ret,sp,row))
   if int(row['entry'],16)==0xf9b10:self.motion_scope=None
 def hook(self,g,a,size,user):
  if self.item_observe:
   self.motion_returns(a)
   if a==0xf9b10 and self.active=='original_new_object_pass':
    sp=g.reg_read(UC_X86_REG_ESP);datum=self.u(sp+4)
    found=next(o for o in self.item_objects()if o['datum']==f'{datum:08x}')
    assert found['row']in(0,1,12)and found['parent']=='ffffffff'
    self.motion_scope=dict(datum=found['datum'],row=found['row'],body=found['body_address'],body_size=found['body_size'])
    tag=self.cache.tags[int(found['definition'],16)];p=tag['offset']+self.cache.magic
    self.motion_definitions[p]=dict(datum=found['definition'],name=tag['name'],size=tag['size'],record=cache_record(self,p,tag['size']))
   if self.motion_scope:
    self.motion_executed[self.motion_scope['row']].add(a)
    if a in self.motion_routines:
     sp=g.reg_read(UC_X86_REG_ESP);reg={k:int(v,16)for k,v in self.registers().items()};stack=[self.u(sp+4+i*4)for i in range(9)]
     row=dict(id=len(self.motion_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',order=self.motion_order,
      scope=dict(self.motion_scope),entry_cpu=self.cpu(),stack_words=[f'{v:08x}'for v in stack],
      before_state=self.state_id(),before_frames=self.frame(),packets=self.packet(a,reg,stack))
     self.motion_order+=1;self.motion_calls.append(row);self.motion_pending.append((self.u(sp),sp,row))
    owner=self.motion_instruction_owner.get(a)
    if owner is not None:
     ins=self.code.at(a)
     if ins.mnemonic.startswith('j')or ins.mnemonic=='call'or ins.mnemonic.startswith('f')or ins.mnemonic in('divss','sqrtss','rcpss'):
      self.motion_points.append(dict(order=self.motion_order,instruction=f'{a:08x}',owner=f'{owner:08x}',
       scope=dict(self.motion_scope),cpu=self.cpu(),frames=self.frame(),stack20_hex=self.raw(g.reg_read(UC_X86_REG_ESP),0x20)))
      self.motion_order+=1
  super().hook(g,a,size,user)
 def motion_read(self,g,access,address,size,value,user):
  if self.motion_scope and g.reg_read(UC_X86_REG_EIP)in self.motion_instruction_owner:
   self.motion_reads.append(dict(order=self.motion_order,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',
    address=f'{address:08x}',size=size,raw_hex=self.raw(address,size),origin=self.origin(address,size),scope=dict(self.motion_scope)))
   self.motion_order+=1
 def mem_write(self,g,access,address,size,value,user):
  if self.motion_scope:
   self.motion_stores.append(dict(order=self.motion_order,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',
    address=f'{address:08x}',size=size,before_hex=self.raw(address,size),
    written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex(),scope=dict(self.motion_scope)))
   self.motion_order+=1
  super().mem_write(g,access,address,size,value,user)

def listings(t,out):
 entries=set(t.motion_routines);result=[];actual=set().union(*t.motion_executed.values())
 for entry in sorted(entries):
  r=_trace(t.code,entry,t.starts|entries);ins=[t.code.at(a)for a in sorted(r.code)]
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
  raw=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode();p=f'instructions/{entry:08x}.asm'
  frozen_write(out/p,raw);result.append(dict(entry=f'{entry:08x}',file=p,sha256=hashlib.sha256(raw).hexdigest(),rows=len(ins),
   executed_rows=sum(i.address in actual for i in ins),direct_calls=[dict(site=f'{a:08x}',target=f'{b:08x}',executed=a in actual)for a,b,k in r.sites if k=='call']))
 executed=[t.code.at(a)for a in sorted(actual)]
 for i in executed:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
 raw=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in executed).encode()
 frozen_write(out/'instructions/executed-motion-instructions.asm',raw)
 result.append(dict(kind='executed_motion_instructions',file='instructions/executed-motion-instructions.asm',sha256=hashlib.sha256(raw).hexdigest(),rows=len(executed)))
 return result

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');args=ap.parse_args()
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 own=json.loads((owning.OUT/'report.json').read_text());tool_hash=digest(__file__)
 proc=subprocess.Popen(['wine',str(owning.player_source.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,
  env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(owning.player_source.CONTEXT_OUT/'wine-prefix')))
 try:
  t=owning.player_source.prepare(json.loads(proc.stdout.readline()),trace_type=MotionTrace)
  assert t.player_stages[-1]['completion']=='complete_original_return'
  assert owning.player_source.spawn_and_activate(t,0,activate=False)
  t.item_observe=True
  for r in own['stages']:
   assert t.item_stage(r['name'],int(r['entry'],16),tuple(int(v,16)for v in r['arguments']),
    tuple((int(k),int(v,16))for k,v in r['input_registers'].items()))
  reproduced=dict(preparation=t.producers,player_prefix_stages=t.player_stages,stages=t.item_stages,calls=t.item_calls,
   callbacks=t.item_callbacks,stores=t.item_stores,states=t.item_states,resources=t.resource_delivery,services=t.events,
   executed_addresses=[f'{a:08x}'for a in sorted(t.executed)],metadata=owning.metadata(t))
  for k,v in reproduced.items():assert v==own[k],('Read-only owning runtime changed',k)
  assert not t.motion_pending and t.motion_scope is None
  for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
  assert digest(__file__)==tool_hash
  result=dict(schema=1,oracle_sha256=tool_hash,source_pins=FROZEN,
   owning_runtime_correlation=dict(report_sha256=digest(owning.OUT/'report.json'),
    byte_identical_keys=list(reproduced),runtime_sha256=hashlib.sha256(encoded(reproduced)).hexdigest()),
   calls=t.motion_calls,states=t.motion_states,reads=t.motion_reads,stores=t.motion_stores,points=t.motion_points,
   definitions=list(t.motion_definitions.values()),instructions=listings(t,args.output.parent),
   executed_in_scope={str(row):[f'{a:08x}'for a in sorted(addresses)]for row,addresses in t.motion_executed.items()},
   source_outer_delivery=[r for r in t.item_calls if r['phase']=='original_new_object_pass'and r['entry']in('0016b240','0016ba30','001e20c0','00246a50')],
   instruction_patches=[],new_guest_inputs=[],new_supplied_service_results=[],
   unexecuted_named_entries=[f'{e:08x}'for e in sorted(NAMED)if not any(int(r['entry'],16)==e for r in t.motion_calls)],
   boundaries=own['boundaries']+['Read-only motion observer retains exact original calls, current object states, source scratch frames, pointed ABI buffers, operands, x87/SSE checkpoint state and ordered stores. It supplies no guest inputs or return values.',
    'Only the three admitted first world-item paths are measured. Unexecuted branches and alternative non-NONE physical/network/parent states are source-listed only. Positive Havok/object/instanced delivery, more frames/continuous settling, full frame clocks and Xbox numerical parity remain unresolved.'])
  frozen_write(args.output,encoded(result));print(json.dumps(dict(output=str(args.output),sha256=digest(args.output),calls=len(t.motion_calls),states=len(t.motion_states),reads=len(t.motion_reads),stores=len(t.motion_stores),points=len(t.motion_points))))
 finally:
  # This host compatibility worker stays alive through every guest read.
  # After emulation/evidence ends, stop only our own worker if Wine shutdown
  # stalls; no guest result or frozen expected value depends on host teardown.
  forced_shutdown=False
  try:_,err=proc.communicate('x',timeout=15)
  except subprocess.TimeoutExpired:
   forced_shutdown=True;proc.kill();_,err=proc.communicate(timeout=15)
   print(json.dumps(dict(host_worker_shutdown='own_worker_stopped_after_completed_emulation')))
 assert proc.returncode==0 or forced_shutdown,err
if __name__=='__main__':main()
