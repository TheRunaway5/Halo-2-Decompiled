#!/usr/bin/env python3
"""Retain complete original21A6E0 cache initialization with real raw storage.

The unchanged full126B20 caller still stops at2205C0. Physical-limit controls
are declared constructor inputs, not genuine whole-title startup observations.
"""
import argparse,json
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EIP
from oracle_smg_audio_rng_startup_1_1 import RngTrace,ROOT,FROZEN
from oracle_mp_title_update_display import OLD,sha,save,encoded

PRIOR=ROOT/'analysis/multiplayer/map-pack/smg-audio-rng-startup/frozen-measurement'
DEPENDENCY=ROOT/'tools/oracle_smg_audio_rng_startup_1_1.py'
DEPENDENCY_PIN='c67ddaa11d18fb50a67eea73e213dd926b37f343dee1845d15f37ba702ef9cc6'
REPORT_PIN='43cf5c78f8fd6b2af8fd6e9de8c65ac249564ecda13463a340a1a333599b2075'
GLOBALS={'general_allocator_identity':0x46fbf8,'static_allocator_identity':0x46fbfc,
 'entry_pool':0x5098dc,'page_allocator':0x5098e4,'physical_buffer':0x5098e0}
CASES=('original_arena','equal_lower','insufficient_arena','zero_reservation','signed_negative_rejected','second_arena_slot')

class CacheStartupTrace(RngTrace):
 def __init__(self,data,replay,case):
  self.cache_active=False;self.cache_completed=False;self.cache_case=case
  self.cache_stages=[];self.cache_stores=[];self.cache_executed=set();self.cache_regions=[];self.cache_services=[];self.cache_input_changes=[]
  super().__init__(data,replay)
 def snapshot(self):
  return dict(globals={name:f'{self.u(va):08x}'for name,va in GLOBALS.items()},
    physical_arena_hex=bytes(self.g.mem_read(0x4ed9f0,0x34)).hex(),
    regions=[dict(address=f'{va:08x}',requested_bytes=requested,rounded_bytes=rounded,
      raw_hex=bytes(self.g.mem_read(va,rounded)).hex())for va,requested,rounded in self.cache_regions])
 def stage(self,name,pc):
  self.cache_stages.append(dict(phase=name,instruction=f'{pc:08x}',state=self.snapshot(),store_count=len(self.cache_stores)))
 def controlled_inputs(self):
  index=self.u(0x4ed9f0);upper=self.u(0x4eda10+index*4);lower=self.u(0x4ed9fc+index*4)
  candidate=(upper-0x300000)&0xffffffff
  changes=[]
  if self.cache_case=='equal_lower':changes=[(0x4ed9fc+index*4,candidate)]
  elif self.cache_case=='insufficient_arena':changes=[(0x4ed9fc+index*4,(candidate+1)&0xffffffff)]
  elif self.cache_case=='zero_reservation':changes=[(0x4eda10+index*4,0x300000),(0x4ed9fc+index*4,0)]
  elif self.cache_case=='signed_negative_rejected':changes=[(0x4eda10+index*4,0x80300000),(0x4ed9fc+index*4,0)]
  elif self.cache_case=='second_arena_slot':
   assert index==0;changes=[(0x4ed9f0,1),(0x4eda14,upper),(0x4eda00,lower)]
  for va,value in changes:
   self.cache_input_changes.append(dict(address=f'{va:08x}',before=f'{self.u(va):08x}',after=f'{value:08x}'))
   self.w(va,value)
 def cache_vm_provider(self):
  active=self.cache_active;before=len(self.raw_providers)
  super().cache_vm_provider()
  if active:
   row=self.raw_providers[before];va=int(row['resulting_base'],16)
   self.cache_regions.append((va,row['requested_size'],row['rounded_size']))
   self.cache_services.append(dict(service='raw_allocate',source_request=dict(owner_identity=f'{self.u(0x46fbf8):08x}',bytes=row['requested_size']),
     output_alias=row['resulting_base'],backing_bytes=row['rounded_size'],original_return_site=row['original_return_site']))
   self.stage('first_raw_allocation'if len(self.cache_regions)==1 else'second_raw_allocation',self.g.reg_read(UC_X86_REG_EIP))
 def physical_provider(self,a):
  active=self.cache_active;before=len(self.raw_providers)
  if active:self.stage('before_real_protection',0x21a79e)
  super().physical_provider(a)
  if active:
   row=self.raw_providers[before]
   self.cache_services.append(dict(service='raw_protect',source_request=row['args'],original_return_site=row['original_return_site'],actual_host_operation=row['raw_provider_operation']))
   self.stage('after_real_protection',0x21a7a3)
 def hook(self,g,address,size,user):
  if address==0x21a6e0:
   assert not self.cache_active and not self.cache_completed
   self.stage('source_produced_inputs',address);self.controlled_inputs();self.cache_active=True;self.stage('entry',address)
  if self.cache_active:
   if self.code.at(address):self.cache_executed.add(address)
   if address==0x21a717:self.stage('first_pool_constructed',address)
   if address==0x21a754:self.stage('page_allocator_constructed',address)
   if address==0x126b8d:
    self.stage('returned',address);self.cache_active=False;self.cache_completed=True
  super().hook(g,address,size,user)
 def mem_write(self,g,access,address,size,value,user):
  tracked=(any(va<=address<va+rounded for va,requested,rounded in self.cache_regions)
      or address in(0x5098dc,0x5098e4,0x5098e0)or 0x4ed9f0<=address<0x4eda24)
  if self.cache_active and tracked:
   self.cache_stores.append(dict(instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{address:08x}',bytes=size,
      before=f'{self.u(address,size):0{size*2}x}',after=f'{value&((1<<(size*8))-1):0{size*2}x}'))
  super().mem_write(g,access,address,size,value,user)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 assert sha(DEPENDENCY)==DEPENDENCY_PIN and sha(PRIOR/'report.json')==REPORT_PIN
 for path,pin in FROZEN.items():assert sha(ROOT/path)==pin,path
 prior=json.loads((PRIOR/'report.json').read_text());context=json.loads((PRIOR/'raw-host-context.json').read_text())
 assert sha(PRIOR/'raw-host-context.json')==prior['context_snapshot_sha256'] and sha(OLD)==prior['source_sha256']
 cases=[];executed=set()
 for name in CASES:
  t=CacheStartupTrace(context,prior['clock_calls'],name);t.execute();assert t.cache_completed
  assert t.u(t.u(0x4ed950)+0x78,1)==0 and t.u(0x4ed944)==0
  executed.update(t.cache_executed)
  raw=[r for r in t.raw_providers if r.get('phase')=='original_sound_constructor_through_controller_until_device']
  save(a.output/'raw-host-providers'/f'{name}.json',encoded(raw))
  cases.append(dict(name=name,source_input_changes=t.cache_input_changes,stages=t.cache_stages,stores=t.cache_stores,
      services=t.cache_services,original_full_caller_stop=t.new_invalid,
      source_cache_completed=True,source_device_constructor_completed=False,
      raw_host_provider_evidence_file=f'raw-host-providers/{name}.json'))
  print(json.dumps(dict(case=name,stores=len(t.cache_stores),allocations=len(t.cache_regions),protection_calls=sum(q['service']=='raw_protect'for q in t.cache_services))),flush=True)
 rows=[t.code.at(pc)for pc in sorted(executed)]
 for row in rows:assert row.bytes==t.image.read(row.address,len(row.bytes))==bytes(t.g.mem_read(row.address,len(row.bytes)))
 save(a.output/'instructions/original-executed-cache.asm',''.join(f'{q.address:08x} {q.bytes.hex()} {q.mnemonic} {q.op_str}\n'for q in rows).encode())
 for path,pin in FROZEN.items():assert sha(ROOT/path)==pin,path
 report=dict(schema='original-sound-cache-startup-owner-v1',oracle_sha256=sha(__file__),source_sha256=sha(OLD),
   dependency_sha256=DEPENDENCY_PIN,prior_report_sha256=REPORT_PIN,recorded_context_sha256=sha(PRIOR/'raw-host-context.json'),
   cases=cases,executed_instruction_rows=len(rows),instruction_patches=[],fabricated_success_returns=[],
   boundaries=['Unchanged full126B20 owning call executes complete21A6E0 plus13E8E0/pool constructors, then stops at real2205C0 without a device return.',
     'The original_arena case consumes genuine earlier executed12CB70/game-arena CPU outputs. Five additional physical index/limit controls are explicitly changed constructor inputs, not normal title startup observations.',
     'Actual host mmap owns both requested raw blocks; actual mprotect backs source protection. Deterministic32-bit aliases are platform-adapter inputs, not retail Xbox addresses.',
     'Whole prior compatibility context and measured clock inputs replayed, not new Wine or Xbox observations. Raw nondeterministic host addresses retained separately from repeatable source-visible records.',
     'Static startup arena/game-arena fragment/earlier world/resource boundaries stay exactly declared in the pinned dependency. No map activation, ready flag, page-ready completion, listener, voice or playback admission supplied.'],
   native_audio_connected=False)
 save(a.output/'report.json',encoded(report));print(json.dumps(dict(report_sha256=sha(a.output/'report.json'),cases=len(cases),rows=len(rows))))

if __name__=='__main__':main()
