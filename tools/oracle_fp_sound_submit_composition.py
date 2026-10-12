#!/usr/bin/env python3
"""Observe complete original authored FP -> world -> raw submit composition.

The earlier equipped/local/world/clock fixture and its unavailable audio
startup remain unchanged. No lower-owner or scheduler result is replayed.
"""
import argparse,hashlib,json,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import *
from oracle_sound_object_request_1_1 import ObjectTrace,regions,pins,sha,save,encoded
from oracle_smg_fp_sound_1_1 import ROOT,PIN,CACHE_PIN,AUDIO_ARENA,AUDIO_CAPACITY
from oracle_smg_firing_1_1 import UNIT

class ChainTrace(ObjectTrace):
 def __init__(self):
  self.chain_enabled=False;self.chain=None;self.chains=[]
  super().__init__();self.chain_enabled=True
  self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.chain_read)
 def hook(self,g,a,size,user):
  if self.chain_enabled:
   r=self.chain
   if r and a==r['return_integer'] and g.reg_read(UC_X86_REG_ESP)>r['stack_integer']:
    r['after_fp']=self.fp_state();r['memory']=regions(r.pop('memory_integer'))
    r['marker_after_hex']=bytes(g.mem_read(r['marker_integer'],40)).hex()
    r['location_after_hex']=bytes(g.mem_read(r['location_integer'],48)).hex()
    r['request_after_hex']=bytes(g.mem_read(r['raw_integer'],160)).hex()
    for key in list(r):
     if key.endswith('_integer'):r.pop(key)
    self.chain=None;r=None
   if a==0x1687c2:
    assert self.chain is None
    sp=g.reg_read(UC_X86_REG_ESP)
    r=dict(tick=self.tick,phase=self.active,event20_hex=bytes(g.mem_read(self.u(sp+8),20)).hex(),
     view=self.u(sp+4),before_fp=self.fp_state(),current_bsp=f'{self.u(0x46fb64,2):04x}',
     fpcw=f'{g.reg_read(UC_X86_REG_FPCW):04x}',before_audio=self.audio_state(),
     locals=self.local_context(),roots=self.roots(),return_integer=self.u(sp),stack_integer=sp,
     memory_integer={},raw_stores=[],submit_stores=[],submit_calls=0)
    self.chain=r;self.chains.append(r)
   if r and a==0x18dde0:
    sp=g.reg_read(UC_X86_REG_ESP);r['marker_integer']=self.u(sp+12);r['location_integer']=self.u(sp+16)
    r['marker_before_hex']=bytes(g.mem_read(r['marker_integer'],40)).hex()
    r['location_before_hex']=bytes(g.mem_read(r['location_integer'],48)).hex()
   if r and a==0x18ba10:
    sp=g.reg_read(UC_X86_REG_ESP);source=g.reg_read(UC_X86_REG_EAX);raw=sp-0xa0
    r['source88_hex']=bytes(g.mem_read(source,88)).hex();r['raw_integer']=raw
    r['raw_before_hex']=bytes(g.mem_read(raw,160)).hex()
   if r and a==0x21f220:
    raw=g.reg_read(UC_X86_REG_EAX);assert raw==r['raw_integer']
    r['request_at_submit_hex']=bytes(g.mem_read(raw,160)).hex();r['submit_calls']+=1
    r['submit_state']={name:f'{self.u(addr):08x}'for name,addr in(
     ('system',0x4ed950),('tag_table',0x4eb114),('current_matg',0x4e791c),
     ('current_ugh',0x526464),('current_scenario',0x4e7920),('effect_pool',0x526470),
     ('controller_pool',0x526468),('controller_root',0x52646c))}
  super().hook(g,a,size,user)
 def chain_read(self,g,access,address,size,value,user):
  r=self.chain
  if not r or 0x2000000<=address<0x2200000:return
  pc=g.reg_read(UC_X86_REG_EIP)
  if not (0x18aa40<=pc<=0x18f200 or pc in self.object_owned or
          0x21f220<=pc<0x21fec0 or 0x21b5b0<=pc<0x21c000 or
          0xbbea0<=pc<=0xbbef0 or 0x156eb0<=pc<=0x156f40):return
  for n,b in enumerate(bytes(g.mem_read(address,size))):r['memory_integer'].setdefault(address+n,b)
 def mem_write(self,g,access,address,size,value,user):
  r=self.chain
  if r and 'raw_integer'in r:
   pc=g.reg_read(UC_X86_REG_EIP);raw=r['raw_integer']
   row=dict(instruction=f'{pc:08x}',address=f'{address:08x}',bytes=size,
    before=f'{self.u(address,size):0{size*2}x}',after=f'{value&((1<<(size*8))-1):0{size*2}x}')
   if raw<=address and address+size<=raw+160 and 0x18ba10<=pc<0x18bb41:
    row['offset']=address-raw;r['raw_stores'].append(row)
   elif 0x21f220<=pc<0x21fec0 or 0x21b5b0<=pc<0x21c000:
    if AUDIO_ARENA<=address<AUDIO_ARENA+AUDIO_CAPACITY or raw<=address<raw+160:
     r['submit_stores'].append(row)
  super().mem_write(g,access,address,size,value,user)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args()
 prior=pins();cases=[];definitions={};contexts=[]
 for scenario,ticks in(('ready',2),('reload',48),('empty_reload',125)):
  t=ChainTrace();initialization=t.partial_initialization();t.authored_sound()
  if scenario=='ready':t.g.mem_write(UNIT+0x212,b'\xff')
  t.execute_fp(ticks,'idle'if scenario=='ready'else scenario)
  assert t.chain is None and len(t.chains)==len(t.object_rows)==1
  row=t.chains[0];row['scenario']=scenario;row['object_request']=t.object_rows[0]
  row['declared_controls']=[dict(address=f'{UNIT+0x212:08x}',bytes=1,value_hex='ff',
   purpose='Unchanged prior ready fixture control selecting no current weapon before original FP clock; not a source-produced inventory transition.')]if scenario=='ready'else[]
  assert row['object_request']['result_al']==1 and row['submit_calls']==1
  call=next(c for c in t.audio_calls if c['entry']=='0021f220')
  row['submit_return']=call['return_eax'];assert row['submit_return']=='ffffffff'
  assert not row['submit_stores'] and row['request_at_submit_hex']==row['request_after_hex']
  row['inherited_initialization']=initialization
  row['inherited_services']={f'{k:08x}':v for k,v in t.services.items()}
  row['inherited_context']=t.fp_external_inputs
  c=t.cache;sncl=next(tag for tag in c.tags.values()if tag['group']=='sncl');classes=c.block(sncl['offset'],92)
  sound=struct.unpack_from('<I',bytes.fromhex(row['event20_hex']),4)[0];tag=c.tags[sound]
  d=bytes(c.data[tag['offset']:tag['offset']+20]);index=struct.unpack_from('<b',d,2)[0]
  row['sound']=f'{sound:08x}';row['definition20_hex']=d.hex();row['class92_hex']=bytes(c.data[classes[index]:classes[index]+92]).hex()
  headers=t.u(t.u(0x4e78d0)+0x44);objects=[]
  for datum in(0x10001,t.initial_datum):
   h=headers+(datum&65535)*12;b=t.u(h+8)
   objects.append(dict(datum=f'{datum:08x}',type=t.u(h+3,1),parent=f'{t.u(b+0x14):08x}',
    flags_12c=t.u(b+0x12c,1),associated_154=f'{t.u(b+0x154):08x}'))
  row['associations']=objects;cases.append(row);del t
 pair=ROOT/'analysis/multiplayer/integration/fp-sound-request-native/u5-final'
 report=dict(schema='original-fp-world-raw-submit-composition-v1',tool_sha256=sha(__file__),
  xbe_sha256=PIN,cache_sha256=CACHE_PIN,prior_pins=prior,cases=cases,
  paired_complete_bodies=dict(report_sha256=sha(pair/'report.json'),providers_sha256=sha(pair/'providers.json')),
  additional_guest_fields_relative_to_prior_fixture=[],instruction_patches=[],supplied_owner_results=[],supplied_submit_results=[],
  scope='Fresh original authored ready/reload/empty-reload calls run through actual18DDE0 and whole21F220. Existing equipped/local/world/clock fixture inputs remain explicit; startup/map reset are genuinely stopped at earlier native services, not completed. Actual raw caller backing and source objects are observed. Cold submit returnsNONE naturally. No native live body/local listener/activeBSP producer, ready/admission/create/voice/playback success or SDL/Xbox capability equivalence is asserted.')
 pins();save(a.output/'report.json',encoded(report))
 print(json.dumps(dict(cases=len(cases),raw_stores=sum(len(q['raw_stores'])for q in cases),
  object_stores=sum(len(q['object_request']['stores'])for q in cases),report_sha256=sha(a.output/'report.json'))))
if __name__=='__main__':main()
