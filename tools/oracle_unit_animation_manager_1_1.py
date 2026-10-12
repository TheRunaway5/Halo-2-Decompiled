#!/usr/bin/env python3
"""Observer-only manager/channel clocks on the frozen equipped unit schedule.

No new guest input, instruction patch or successful service is introduced.
This suite retains the old explicit preallocated inventory/device/time boundaries.
"""
import argparse, hashlib, json, struct
from pathlib import Path
import unicorn
from oracle_spartan_equipped_unit_phase_1_1 import UnitPhaseTrace
from oracle_spartan_equipped_animation_1_1 import ROOT, UNIT, MANAGER_OFFSET, PIN, TIME
from oracle_smg_firing_1_1 import STOP, _trace
from unicorn.x86_const import *

PINS={
 'tools/oracle_spartan_equipped_animation_1_1.py':'3cff76069e4770392b0cee0df2bafaec5a4def1ba4945493047cdb57977e7a65',
 'tools/oracle_spartan_equipped_unit_phase_1_1.py':'cb62dcd1595260984ae37e2e5aedc3a0e09cdc921ad49d104c4ac8d122567a38',
 'analysis/multiplayer/map-pack/spartan-equipped-unit-phase-suite/suite.json':'a9724a76d52551cea10c1b379ecb8503e1c9c2d96cb553bd0643d5f8289f418e',
 'analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/report.json':'4b9e9ed383e0081fe7d64c57636731304d66d322b3aef23259024a27061733cb',
}
OWNERS=(0x1CF100,0x1CA2B0,0x1CA090,0x1C9F50,0x1CA3A0,0x1CAE40,0x1CF070,0x1D0ED0,0x1DCD10)
BOUNDARIES=(0x1CEDA0,0x1CEFE0,0xC04F0)
ROOTS=(0x1CF100,0x1CA2B0,0x1CA090,0x1C9F50,0x1D0ED0)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def encoded(x):return (json.dumps(x,sort_keys=True,indent=2)+'\n').encode()
def save(p,b):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==b,('Immutable output differs',str(p))
 else:p.write_bytes(b)

class ClockTrace(UnitPhaseTrace):
 def __init__(self):
  self.clock_enabled=False;self.clock_calls=[];self.clock_pending=[];self.clock_stores=[];self.clock_executed=set();self.clock_metadata={}
  super().__init__()
  self.clock_routines={e:_trace(self.code,e,self.starts|set(OWNERS+BOUNDARIES))for e in OWNERS+BOUNDARIES}
  self.clock_owners={a:e for e,r in self.clock_routines.items()for a in r.code}
 def initialize_equipped(self):
  r=super().initialize_equipped();self.clock_enabled=True;return r
 def clock_snapshot(self,c):
  m=UNIT+MANAGER_OFFSET
  return {'manager':bytes(self.g.mem_read(m,0x90)).hex(),
   'channel':bytes(self.g.mem_read(c,32)).hex()if c else None}
 def clip(self,c):
  graph,packed=self.u(c),self.u(c+4)
  if graph==0xffffffff or packed>>16==0xffff:return
  key=f'{graph:08x}:{packed:08x}'
  if key in self.clock_metadata:return
  root=self.u(self.u(0x4EB114)+(graph&0xffff)*16+8);resolved=root
  if packed&0xffff!=0xffff:
   inherited=struct.unpack('<h',struct.pack('<H',packed&0xffff))[0]
   assert 0<=inherited<self.u(root+0x4C)
   datum=self.u(self.u(root+0x50)+inherited*32+4)
   resolved=self.u(self.u(0x4EB114)+(datum&0xffff)*16+8)
  at=self.u(resolved+0x30)+(packed>>16)*0x6C
  assert packed>>16<self.u(resolved+0x2C)
  raw=bytes(self.g.mem_read(at,0x6C))
  def block(base,offset,stride):
   n,p=self.u(base+offset),self.u(base+offset+4)
   assert n<10000
   return bytes(self.g.mem_read(p,n*stride)).hex()if n else ''
  self.clock_metadata[key]=dict(graph=f'{graph:08x}',packed=f'{packed:08x}',root=f'{root:08x}',resolved=f'{resolved:08x}',pointer=f'{at:08x}',raw=raw.hex(),
   timing=block(at,0x4C,4),sounds=block(at,0x54,8),effects=block(at,0x5C,4),
   sound_refs=block(resolved,0x14,12),effect_refs=block(resolved,0x1C,12))
 def finish(self,a):
  sp=self.g.reg_read(UC_X86_REG_ESP)
  matches=[q for q in self.clock_pending if q[0]==a and sp>q[1]]
  if not matches:return
  _,_,r=matches[-1]
  r['after']=self.clock_snapshot(r['channel_pointer']);r['return_eax']=f'{self.g.reg_read(UC_X86_REG_EAX):08x}'
  r['store_end']=len(self.clock_stores);r['call_end']=len(self.clock_calls)
  if r['entry']=='001ceda0':r['selected_packed']=f'{self.u(r["registers"]["esi"]):08x}'
  self.clock_pending.remove(matches[-1])
 def run(self,*args,**kwargs):
  super().run(*args,**kwargs)
  if self.clock_enabled:self.finish(STOP)
 def hook(self,g,a,size,user):
  if self.clock_enabled:
   self.finish(a)
   if a in self.clock_owners:self.clock_executed.add(a)
   if a in OWNERS+BOUNDARIES:
    sp=g.reg_read(UC_X86_REG_ESP);regs={k:int(v,16)for k,v in self.registers().items()};s=[self.u(sp+4+i*4)for i in range(6)]
    c=(regs['eax']if a in(0x1CA2B0,0x1C9F50)else s[0]if a==0x1CA090 else regs['esi']if a==0x1CA3A0 else 0)
    if a==0x1CF100:c=regs['esi']
    if c:self.clip(c)
    if a==0x1CEDA0:
     c=next((q[2]['channel_pointer']for q in reversed(self.clock_pending)if q[2]['entry']=='001ca090'),0)
    if a==0xC04F0:
     c=next((q[2]['channel_pointer']for q in reversed(self.clock_pending)if q[2]['channel_pointer']),0)
    r=dict(id=len(self.clock_calls),tick=self.tick,phase=self.active,entry=f'{a:08x}',caller=f'{self.u(sp):08x}',registers=regs,
     stack=[f'{v:08x}'for v in s],channel_pointer=c,channel_offset=c-(UNIT+MANAGER_OFFSET)if UNIT+MANAGER_OFFSET<=c<UNIT+MANAGER_OFFSET+0x60 else -1,
     before=self.clock_snapshot(c),dt_bits=f'{self.u(TIME+4):08x}',feature=self.u(0x4770DC,1),store_begin=len(self.clock_stores))
    if a==0xC04F0:r['event_packet']=bytes(g.mem_read(s[2],20)).hex()
    self.clock_calls.append(r);self.clock_pending.append((self.u(sp),sp,r))
  super().hook(g,a,size,user)
 def mem_write(self,g,access,p,size,value,user):
  if self.clock_enabled:
   ip=g.reg_read(UC_X86_REG_EIP)
   if ip in self.clock_owners:
    self.clock_stores.append(dict(instruction=f'{ip:08x}',address=f'{p:08x}',size=size,before=bytes(g.mem_read(p,size)).hex(),written=(value&((1<<(8*size))-1)).to_bytes(size,'little').hex()))
  super().mem_write(g,access,p,size,value,user)

def generate(out,only=None):
 for p,h in PINS.items():assert sha(ROOT/p)==h,(p,'prior pin changed')
 cases=[]
 for scenario,ticks in (('idle',75),('aim',12),('move_input',12),('fire',60),('reload',135),('empty_reload',225)):
  if only and scenario!=only:continue
  t=ClockTrace();result=t.execute_unit_phase(ticks,scenario);assert not t.clock_pending
  oldpath=ROOT/'analysis/multiplayer/map-pack/spartan-equipped-unit-phase-suite'/f'{scenario}.json'
  old=json.loads(oldpath.read_text());result.pop('oracle_tool_sha256');old.pop('oracle_tool_sha256')
  assert result==old,('Frozen original unit schedule changed',scenario)
  report=dict(schema='literal-unit-animation-clock-v1',source_pins=PINS,oracle_sha256=sha(__file__),scenario=scenario,
   original_runtime_correlation=dict(path=str(oldpath.relative_to(ROOT)),sha256=sha(oldpath),byte_identical_fields=True),
   calls=t.clock_calls,stores=t.clock_stores,definitions=t.clock_metadata,executed=[f'{a:08x}'for a in sorted(t.clock_executed)],
   supplied_scope='Unchanged inherited preallocated unit/inventory, resident cache resources, processed input and 30Hz clock fixture; original physics classification is not a live movement producer.',
   new_guest_inputs=[],new_supplied_success_or_pose=[])
  p=out/f'{scenario}.json';save(p,encoded(report));cases.append(dict(scenario=scenario,path=p.name,sha256=sha(p),calls=len(t.clock_calls)))
  print(json.dumps(cases[-1]),flush=True)
  for e,r in t.clock_routines.items():
   raw=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in(t.code.at(a)for a in sorted(r.code))).encode()
   for a in r.code:
    i=t.code.at(a);assert bytes(t.g.mem_read(a,len(i.bytes)))==i.bytes
   save(out/'instructions'/f'{e:08x}.asm',raw)
  del t
 save(out/'report.json',encoded(dict(schema='literal-unit-animation-clock-index-v1',source_pins=PINS,oracle_sha256=sha(__file__),cases=cases)))

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--scenario');args=p.parse_args();generate(args.output,args.scenario)
