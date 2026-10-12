#!/usr/bin/env python3
"""New measured genuine-owner melee request/metadata/first update extension.

The initial exploratory report is retained. This observer additionally respects
the actual return ESP, so an unobserved nested handler at the same indirect-call
return address cannot prematurely close an outer handler record. No guest code,
input or source service outcome changes. No impact progress or clock is supplied.
"""
import argparse,json,os,struct,subprocess,hashlib
from pathlib import Path
import oracle_mp_melee_admission_1_1 as original
from oracle_smg_firing_1_1 import STOP
from functions import _trace

owner=original.owner;ROOT=owner.ROOT
OUT=ROOT/'analysis/multiplayer/integration/inventory-source/melee-admission-measured'
PINS={**original.PINS,
 'tools/oracle_mp_melee_admission_1_1.py':'696d31f97743f63ff83334ef917e614c8ff7cc9f5660787273c23ac872b663f9',
 'analysis/multiplayer/integration/inventory-source/melee-admission/report.json':'b6ce37056e0eca4da9c41604952116c9c1fb5842cee7d0cf871722affdf86333'}
EXTRA={0x115650,0x115800,0x1c9e30,0x148160,0xd0830,0xe7ff0,0xc9a40,0xb8250,0x1eb150}
owner.ENTRIES=owner.ENTRIES|EXTRA
owner.STATE_ENTRIES=owner.STATE_ENTRIES|EXTRA

class MeleeTrace(original.MeleeTrace):
 def packets(self,e,reg,s):
  r=super().packets(e,reg,s)
  def add(name,p,n):r.append(dict(field=name,pointer=f'{p:08x}',size=n,raw_hex=self.raw(p,n)if p else None))
  if e in (0xeb530,0xeb080,0xeb700,0xeb830):
   unit=s[0];p=self.u(self.u(self.u(0x4e78d0)+0x44)+(unit&0xffff)*12+8)
   at=struct.unpack('<h',bytes(self.g.mem_read(p+0x346,2)))[0]
   add('actual_unit800',p,0x800);add('actual_signed346_action44',p+at,0x44)
  if e==0xeb000:add('original_melee_variant_table48',0x446e34,0x30)
  if e in (0x1de810,0x1de840):
   add('actual_ecx_animation_record6c',reg['ecx'],0x6c)
   count=self.u(reg['ecx']+0x4c);assert count<4096
   if count:add('actual_frame_events_count4',self.u(reg['ecx']+0x50),count*4)
  if e==0xeade0 and s[2]:add('stack2_actual_channel_pointer_output4',s[2],4)
  if e==0x148160:add('actual_game_clock24',self.u(0x518444),0x24)
  return r
 def probe_returns(self,a):
  esp=self.g.reg_read(owner.UC_X86_REG_ESP)
  matches=[q for q in self.pending if q[0]==a and q[1]<esp]
  if not matches:return
  depth=matches[-1][1]
  for ret,sp,r in reversed(matches):
   if sp!=depth:continue
   r.update(return_order=self.order,return_cpu=self.cpu(),return_esp_delta=esp-sp,
    after_packets=[dict(p,raw_hex=self.raw(int(p['pointer'],16),p['size'])if int(p['pointer'],16)else None)for p in r['packets']])
   self.order+=1
   if int(r['entry'],16)in owner.STATE_ENTRIES:r['after_state']=self.state_id()
   if r['entry']=='00169f3b':
    p=self.g.reg_read(owner.UC_X86_REG_EAX)
    if p:
     count=self.u(p+0x4c);assert count<4096
     r['actual_returned_animation_record']=dict(pointer=f'{p:08x}',raw6c_hex=self.raw(p,0x6c),
      event_count=count,event_pointer=f'{self.u(p+0x50):08x}',event_raw_hex=self.raw(self.u(p+0x50),count*4)if count else '')
   self.pending.remove((ret,sp,r))

def correlations(t,cases):
 prior=json.loads((ROOT/'analysis/multiplayer/integration/inventory-source/melee-admission/report.json').read_text())
 assert len(prior['cases'])==len(cases)
 result=[]
 def state(r,i):return {k:v for k,v in r['states'][i].items()if k not in('id','sha256')}
 current=dict(states=t.states)
 for a,b in zip(prior['cases'],cases):
  assert a['case']==b['case'] and a['declared_input']==b['declared_input']
  assert a['final_cpu']==b['final_cpu']
  assert len(a['stages'])==len(b['stages'])
  for x,y in zip(a['stages'],b['stages']):
   assert {k:v for k,v in x.items()if k not in('before_state','after_state')}=={k:v for k,v in y.items()if k not in('before_state','after_state')}
   for name in('before_state','after_state'):assert state(prior,x[name])==state(current,y[name]),(a['case'],x['name'],name)
  result.append(dict(case=a['case'],declared_input_identical=True,all_stage_entry_exit_source_states_identical=True,completion_service_stop_cpu_identical=True,
   preserved_prior_report=PINS['analysis/multiplayer/integration/inventory-source/melee-admission/report.json'],
   observer_return_boundary='The initial report closes EB530 at an unobserved nested type19 handler return sharing E7F49. New actual-ESP admission closes it only at its own return. Initial per-call EB530 AL/state are not authoritative; immutable stage states and unchanged executed source outcomes remain correlated.'))
 return result

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output-dir',type=Path,default=OUT);a=p.parse_args();pin=owner.digest(__file__)
 for path,h in PINS.items():assert owner.digest(ROOT/path)==h,path
 proc=subprocess.Popen(['wine',str(owner.command.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,
  env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(owner.command.CONTEXT_OUT/'wine-prefix')))
 try:
  data=json.loads(proc.stdout.readline());owner.CommandPoseTrace=MeleeTrace;t,neutral=owner.prepare(data)
  assert [(lo,hi,p)for lo,hi,p,_ in t.source_fixture]==list(t.g.mem_regions())
  memory=[dict(lo=f'{lo:08x}',hi=f'{hi:08x}',permissions=p,size=len(raw),sha256=hashlib.sha256(raw).hexdigest())for lo,hi,p,raw in t.source_fixture]
  cases=[]
  for name,flag in [('declared_flag20',0x20),('declared_flag40',0x40),('declared_flag04000000',0x4000000)]:
   cases.append(original.execute(t,name,flag));print(json.dumps(dict(case=name,last=cases[-1]['stages'][-1],calls=cases[-1]['ranges']['calls'])),flush=True)
  prior=correlations(t,cases);rows=owner.listings(t,a.output_dir)
  for path,h in PINS.items():assert owner.digest(ROOT/path)==h,path
  assert owner.digest(__file__)==pin
  report=dict(schema=1,tool_sha256=pin,frozen_pins=PINS,prior_neutral_identity=neutral,prior_exploratory_source_correlations=prior,
   restored_exact_source_guest_state=memory,source_xbe_sha256=owner.command.PIN,cache_sha256=owner.command.CACHE_PIN,
   cases=cases,calls=t.calls,states=t.states,reads=t.reads,stores=t.stores,points=t.points,listings=rows,
   source_descriptors=json.loads((ROOT/'analysis/multiplayer/integration/inventory-source/melee-admission/report.json').read_text())['source_descriptors'],
   instruction_patches=[],new_supplied_gameplay_or_physics_outputs=[],
   boundary='Only declared command flag stimuli; genuine allocated source player/biped/SMG/current manager/PVS/world/physical state. Original request/admission/weapon metadata/event timing/shared RNG/first action update execute. Full live device/local scheduling/readiness and owning continuous source tick remain open. No target, contact, pose, impulse, channel success, impact progress, clock increment or damage result supplied. D0830 impact body is listed-only because first original update does not reach impact. TU1.1 dynamic execution only.')
  owner.frozen_write(a.output_dir/'report.json',owner.encoded(report))
  print(json.dumps(dict(output=str(a.output_dir/'report.json'),sha256=owner.digest(a.output_dir/'report.json'),cases=len(cases),calls=len(t.calls),states=len(t.states))))
 finally:
  try:_,err=proc.communicate('x',timeout=20)
  except subprocess.TimeoutExpired:
   proc.kill();proc.stdout.close();proc.stderr.close();proc.wait(timeout=5);err='host worker shutdown timeout'
 assert proc.returncode==0,err
if __name__=='__main__':main()
