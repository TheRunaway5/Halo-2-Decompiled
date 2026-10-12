#!/usr/bin/env python3
"""Capture untouched original physics world ABI through its next required owner.

This extends the retained original schedule, never supplies contact results or
patches instructions. The previous complete batch measurement must stay equal.
"""
import argparse,json,hashlib
from pathlib import Path
import unicorn
from oracle_smg_firing_1_1 import Xbe,_Code
from capstone.x86_const import X86_OP_REG,X86_OP_MEM
from unicorn import x86_const as reg
import oracle_mp_power_contact_batches_1_1 as prior
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from export_mp_power_body_slots import full_snapshot
from export_mp_physics_contact_lifecycle import add,segments
ROOT=prior.ROOT
OUT=ROOT/'analysis/multiplayer/sandbox-source/physics-world-listener-runtime-20261011'
PAIR=OUT/'correspondence-v8/complete-report.json'
PIN='fa14b9224e90167338b19bc1193398e195d78c022dc03124dd6825a9210e5d16'
from original_physics_file_hash import StatCheckedSha256
sha=StatCheckedSha256()
def verify_retained():
 # Earlier native sources have evolved in this shared checkout. Original
 # execution depends on pinned measurement inputs and tools, not those newer
 # native files. For changed native sources require the identical historical
 # frozen copy where available; report historical native drift separately.
 # Native implementations never execute in this original-only measurement.
 module=prior;seen=set();count=0;native_drift=[]
 while hasattr(module,'MANIFEST') and module.__name__ not in seen:
  seen.add(module.__name__);p=module.MANIFEST
  assert sha(p)==getattr(module,'PIN',getattr(module,'MANIFEST_PIN',None)),(p,'manifest drift')
  files=json.loads(p.read_text())['files']
  for relative,pin in files.items():
   target=ROOT/relative
   if sha(target)!=pin:
    assert relative.startswith(('src/','include/','tests/','CMakeLists.txt','cmake/')),(relative,'input/tool drift')
    copies=[ROOT/q for q,h in files.items() if h==pin and q.endswith('/'+relative) and '/frozen-native/' in q]
    if not copies or not any(sha(q)==pin for q in copies):
     native_drift.append(dict(path=relative,historical_sha256=pin,current_sha256=sha(target)))
   count+=1
  if not hasattr(module,'prior'):break
  module=module.prior
 return count,native_drift
def abi(g):
 return dict(gp=[g.reg_read(getattr(reg,'UC_X86_REG_'+n)) for n in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')],
  xmm=[f'{g.reg_read(getattr(reg,f"UC_X86_REG_XMM{n}")):032x}' for n in range(8)],
  eflags=g.reg_read(reg.UC_X86_REG_EFLAGS),fs_base=g.reg_read(reg.UC_X86_REG_FS_BASE),
  x87_control_word=g.reg_read(reg.UC_X86_REG_FPCW),mxcsr=g.reg_read(reg.UC_X86_REG_MXCSR),
  x87_status=g.reg_read(reg.UC_X86_REG_FPSW),x87_tag=g.reg_read(reg.UC_X86_REG_FPTAG))
def captured_abi(g):
 result=abi(g);top=(result['x87_status']>>11)&7;tag=result['x87_tag'];values=[];empty=False
 for i in range(8):
  physical=(top+i)&7;kind=(tag>>(physical*2))&3
  if kind==3:empty=True;continue
  assert not empty,'Sparse original x87 stack needs a separate owner'
  mantissa,exponent=g.reg_read(getattr(reg,f'UC_X86_REG_FP{physical}'))
  values.append(mantissa.to_bytes(8,'little').hex()+exponent.to_bytes(2,'little').hex())
 result['x87_values']=values;return result
class Trace(prior.BatchTrace):
 def __init__(self,data):
  assert sha(PAIR)==PIN;p=json.loads(PAIR.read_text());self.known={int(x,16) for x in p['instruction_map']}
  self.contact_cases=[];self.contact_active=None;self.contact_order=0;self.cpu_pending=None
  self.runtime_code=_Code(Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'),'.text')
  super().__init__(data);self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.contact_read)
 def finish(self,entry):
  c=self.contact_active;c['exit']=captured_abi(self.g);c['exit_entry']=entry;c['complete']=entry==c['return_address'];self.contact_active=None
 def hook(self,g,a,size,user):
  if self.contact_active:
   if self.cpu_pending:
    c=self.contact_active;kind,site,target,value=self.cpu_pending;self.cpu_pending=None
    if kind=='rsqrtss':c['estimates'].append(dict(instruction=site,input=value,output=g.reg_read(getattr(reg,'UC_X86_REG_'+target.upper()))&0xffffffff))
    else:c['timestamps'].append(dict(instruction=site,output=(g.reg_read(reg.UC_X86_REG_EDX)<<32)|g.reg_read(reg.UC_X86_REG_EAX)))
   if a not in self.known:self.finish(a)
  if self.slot_ready and a==0x317650:
   assert self.contact_active is None
   sp=g.reg_read(reg.UC_X86_REG_ESP)
   self.contact_active=dict(id=len(self.contact_cases),phase=self.active,entry=a,entry_abi=captured_abi(g),return_address=self.u(sp),
    events=[],instructions=[],estimates=[],timestamps=[],packets=[])
   self.contact_cases.append(self.contact_active)
  c=self.contact_active
  if c:
   c['instructions'].append(a)
   if a==0x300100:
    sp=g.reg_read(reg.UC_X86_REG_ESP);p=self.u(sp+4)
    c['packets'].append(dict(input=p,destination=g.reg_read(reg.UC_X86_REG_ECX),input_hex=bytes(g.mem_read(p,128)).hex()))
   instruction=self.runtime_code.at(a)
   if instruction.mnemonic=='rsqrtss':
    operand=instruction.operands[1]
    if operand.type==X86_OP_REG:value=g.reg_read(getattr(reg,'UC_X86_REG_'+instruction.reg_name(operand.reg).upper()))&0xffffffff
    else:
     assert operand.type==X86_OP_MEM and not operand.mem.segment
     address=operand.mem.disp
     if operand.mem.base:address+=g.reg_read(getattr(reg,'UC_X86_REG_'+instruction.reg_name(operand.mem.base).upper()))
     if operand.mem.index:address+=operand.mem.scale*g.reg_read(getattr(reg,'UC_X86_REG_'+instruction.reg_name(operand.mem.index).upper()))
     value=self.u(address&0xffffffff)
    self.cpu_pending=('rsqrtss',a,instruction.reg_name(instruction.operands[0].reg),value)
   elif instruction.mnemonic=='rdtsc':self.cpu_pending=('rdtsc',a,None,None)
  super().hook(g,a,size,user)
 def contact_read(self,g,access,address,size,value,user):
  if self.contact_active:
   self.contact_active['events'].append(dict(kind='R',instruction=g.reg_read(reg.UC_X86_REG_EIP),address=address,raw=bytes(g.mem_read(address,size)).hex()))
 def mem_write(self,g,access,address,size,value,user):
  if self.contact_active:
   self.contact_active['events'].append(dict(kind='W',instruction=g.reg_read(reg.UC_X86_REG_EIP),address=address,
    before=bytes(g.mem_read(address,size)).hex(),written=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()))
  super().mem_write(g,access,address,size,value,user)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();tool_pin=sha(__file__);verified,native_drift=verify_retained()
 power=prior
 while not hasattr(power,'power'):power=power.prior
 power=power.power
 t=power.player.prepare(json.loads((prior.OWNER/'raw-host-context.json').read_text()),trace_type=Trace)
 t.selected_physics_fixup(0xe6f80583);t.bridge_enabled=True;t.slot_ready=True
 factory=t.factory(0xe6f80583);old=json.loads((prior.OWNER/'report.json').read_text())['case'];keys=set(factory)-{'calls','ordered_stores'};assert {k:factory[k]for k in keys}=={k:old[k]for k in keys}
 def close():
  for method in(t.bridge_return,t.factory_return,t.slot_return,t.prepare_return,t.group_return,t.dispose_return,t.world_return,t.broad_return,t.pair_return,t.agent_return,t.current_return,t.child_return,t.cache_return,t.narrow_return,t.gjk_return,t.batch_return):method(prior.STOP)
 close();t.sandbox_enabled=False;assert power.player.spawn_and_activate(t,0,activate=False);t.power_observe=True;expected=json.loads((power.OUT/'frozen/measured/report.json').read_text())
 for s in expected['stages']:
  assert t.phase(s['name'],int(s['entry'],16),tuple(int(v,16) for v in s['arguments']));close()
 assert not t.contact_active
 oldp=prior.OUT/'final/measured/report.json';assert sha(oldp)=='3aac27adba706d7af4e78bf3f6d309f9bf654b49a24a93777a3fec3d397ee90a';old=json.loads(oldp.read_text())
 for name,value in (('calls',t.batch_calls),('reads',t.batch_reads),('stores',t.batch_stores),('states',t.batch_states)):
  assert value==old[name],name
 for c in t.contact_cases:
  memory={}
  # Only observed input bytes and their first-before values are initial inputs.
  # No post-write byte is inserted as an expected initial state.
  current={}
  for e in c['events']:
   p=e['address'];raw=bytes.fromhex(e['raw'] if e['kind']=='R' else e['before'])
   for n,v in enumerate(raw):
    if p+n not in current:memory[p+n]=v;current[p+n]=v
    assert current[p+n]==v,(c['id'],e)
   if e['kind']=='W':
    for n,v in enumerate(bytes.fromhex(e['written'])):current[p+n]=v
  c['initial_banks']=[dict(address=p,hex=b.hex()) for p,b in segments(memory)]
  c['final_banks']=[dict(address=p,hex=b.hex()) for p,b in segments(current)]
  c['writes']=[e for e in c.pop('events') if e['kind']=='W']

 assert sha(__file__)==tool_pin,'Original capture tool changed during execution'
 report=dict(schema=1,tool_sha256=tool_pin,hash_helper_sha256=sha(ROOT/'tools/original_physics_file_hash.py'),correspondence_sha256=PIN,prior_original_measurement_sha256=sha(oldp),
  prior_complete_measurement_identical=True,authenticated_parent_manifest_entries=verified,historical_native_sources_excluded_from_original_execution_scope=True,cases=t.contact_cases,instruction_patches=[],native_expected_data_used=False,
  original_retained_schedule_not_live_xbox_gameplay=True)
 a.output.mkdir(parents=True,exist_ok=False);p=a.output/'fixture.json';frozen_write(p,encoded(report));frozen_write(a.output/'parent-native-drift.json',encoded(native_drift))
 print(json.dumps(dict(sha256=sha(p),cases=len(t.contact_cases),packets=sum(len(c['packets']) for c in t.contact_cases),instructions=sum(len(c['instructions']) for c in t.contact_cases),stops=[hex(c['exit_entry']) for c in t.contact_cases if not c['complete']])))
if __name__=='__main__':main()
