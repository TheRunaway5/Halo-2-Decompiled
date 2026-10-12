#!/usr/bin/env python3
"""Unchanged BB240/local-save/counter owners on genuine constructed objects.

Observe actual unit/weapon bodies, complete pool headers, current/saved local
banks, raw clock/FPU context and original store order. Add no guest state.
"""
import argparse,json,sys
from pathlib import Path
import oracle_mp_unit_constructor_tail_1_1 as tail
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESI,UC_X86_REG_ECX,UC_X86_REG_EBX,UC_X86_REG_XMM0
original=tail.original;ROOT=original.ROOT
NEW=(0xbb240,0x1cefe0,0x1dce20,0x1dcc30,0xb8250)
PRODUCERS=(0x1dcd10,)
class TransitionTrace(tail.TailTrace):
 def owned_objects(self):
  p=self.pool();at=int(p['headers'],16);rows=[]
  for i in range(p['high_water']):
   h=at+i*12;salt=self.u(h,2)
   if not salt:continue
   rows.append(dict(datum=f'{(salt<<16)|i:08x}',**self.object((salt<<16)|i)))
  return rows
 def buffers(self,row):
  e=int(row['entry'],16);regs=row['registers'];args=[int(q,16)for q in row['stack']]
  if e==0xbb240:row['unit']=regs['esi']
  if e==0xb8250:row['unit']=regs['eax']
  if e==0x1cefe0:
   row.update(manager=args[0],saved_locals=args[2],current_locals=args[3],node_count=regs['eax'])
  if e in(0x1dce20,0x1dcc30)+PRODUCERS:row['counter']=regs['ecx']
  result=super().buffers(row)
  if e in NEW:
   key='clock_before'if'after'not in row and'entry_transition_clock'not in row else 'clock_after'
   clock=self.u(0x518444);record=dict(pointer=f'{clock:08x}',raw0c_hex=self.raw(clock,12),fpcw=self.cpu()['fpcw'],mxcsr=self.cpu()['mxcsr'])
   row[key]=record;row['entry_transition_clock']=True
  if e==0x1cefe0:
   n=row['node_count'];assert 0<=n<10000
   result.update(S=self.raw(row['saved_locals'],n*32),L=self.raw(row['current_locals'],n*32))
  if e in(0x1dce20,0x1dcc30)+PRODUCERS:result['C']=self.raw(row['counter'],4)
  if e==0xbb240:
   p=self.pool();result['POOL']=self.raw(int(p['headers'],16),p['capacity']*12)
   key='objects_before'if'objects_before'not in row else 'objects_after';row[key]=self.owned_objects()
  return result
 def label(self,p,n,row):
  e=int(row['entry'],16)
  if e in(0x1dce20,0x1dcc30)+PRODUCERS and row['counter']<=p and p+n<=row['counter']+4:return('C',p-row['counter'])
  if e==0x1cefe0:
   for name,key in(('S','saved_locals'),('L','current_locals')):
    at=row[key]
    if at<=p and p+n<=at+row['node_count']*32:return(name,p-at)
  if e==0xbb240:
   at=int(row['pool']['headers'],16)
   if at<=p and p+n<=at+row['pool']['capacity']*12:return('POOL',p-at)
  return super().label(p,n,row)
 def metadata(self,row):
  result=super().metadata(row)
  if int(row['entry'],16)==0xbb240:
   b=int(self.object(row['unit'])['body'],16);datum=self.u(b);self.tag(datum)
   at=self.u(self.u(0x4eb114)+(datum&65535)*16+8)
   result['object_definition']=dict(datum=f'{datum:08x}',pointer=f'{at:08x}',model=f'{self.u(at+0x38):08x}')
  return result
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 original.OWNERS=tuple(dict.fromkeys(tail.OLD_OWNERS+tail.DIRECT+tail.PURE+NEW+PRODUCERS))
 original.LEAVES=tuple(x for x in tail.OLD_LEAVES if x not in tail.DIRECT)+tuple(x for x in tail.LOWER if x not in NEW)
 original.ConstructorTrace=TransitionTrace
 def capture(data):
  # Compose at the successful owning stage, before the prior oracle's
  # deliberate invalid bind variants. The latter has an actual dependency
  # stop, and its post-stop buffers must not be promoted to live state.
  t=original.owning.prepare(data,trace_type=TransitionTrace)
  assert t.player_stages[-1]['completion']=='complete_original_return'
  t.ctor_enabled=True;assert original.owning.spawn_and_activate(t,activate=False)
  t.finish(original.owning.STOP);assert not t.ctor_pending
  t.correlated_stages=4;t.caller_variations=[];t.constructor_stops=[]
  t.transition_baseline_state=t.state();t.transition_baseline_call_count=len(t.ctor_calls)
  unit=t.spawned_unit;b=int(t.object(unit)['body'],16);manager=b+t.u(b+0x12a,2);counter=manager+0x64
  normal=t.u(0x44c6b4);special=t.u(0x465140);one=t.u(0x465030)
  t.transition_caller_roots=[]
  def run(name,entry,args=(),regs=()):
   t.active='explicit_original_transition_'+name
   t.transition_caller_roots.append(dict(name=name,entry=f'{entry:08x}',arguments=[f'{q:08x}'for q in args],registers=[dict(register=r,value=f'{v:08x}')for r,v in regs],state_source='Unchanged source-owned body, local banks, manager and clock; only declared caller duration changes.'))
   t.run(entry,args,regs);t.finish(tail.original.owning.STOP)
   if t.ctor_pending:
    original.save(out/'pending-observation.json',original.encoded(dict(name=name,cpu=t.cpu(),pending=[dict(return_address=f'{q[0]:08x}',entry_sp=f'{q[1]:08x}',row=q[2])for q in t.ctor_pending])))
    raise AssertionError('Unclosed original observation: '+name)
  run('normal_request',0xbb240,(normal,),((UC_X86_REG_ESI,unit),))
  run('counter_tick',0x1dcd10,regs=((UC_X86_REG_ECX,counter),))
  run('shorter_request',0xbb240,(special,),((UC_X86_REG_ESI,unit),))
  run('remaining_value',0x1dce20,regs=((UC_X86_REG_ECX,counter),))
  remaining=t.g.reg_read(UC_X86_REG_XMM0)&0xffffffff
  run('equal_remaining_request',0xbb240,(remaining,),((UC_X86_REG_ESI,unit),))
  run('zero_request',0xbb240,(0,),((UC_X86_REG_ESI,unit),))
  # These are explicit pure counter caller requests, not a guessed gameplay
  # duration. The actual produced counter is never rewritten between calls.
  direction=(t.g.reg_read(UC_X86_REG_EBX)&0xffffff00)|1
  run('zero_counter',0x1dcc30,(0,),((UC_X86_REG_ECX,counter),(UC_X86_REG_EBX,direction)))
  run('one_second_counter',0x1dcc30,(one,),((UC_X86_REG_ECX,counter),(UC_X86_REG_EBX,direction)))
  run('normal_counter_rescale',0x1dcc30,(normal,),((UC_X86_REG_ECX,counter),(UC_X86_REG_EBX,direction)))
  t.transition_after_state=t.state()
  for tag in t.cache.tags.values():
   if tag['group']=='jmad':t.tag(int(tag['datum'],16))
  value=dict(schema='unit-local-transition-literal-tags-v1',tags=t.literal_tags,source_xbe_sha256=original.PIN,source_cache_sha256=original.CACHE_PIN)
  original.save(out/'literal-tags.json',original.encoded(value));
  original.save(out/'caller-roots.json',original.encoded(dict(baseline_state=t.transition_baseline_state,baseline_call_count=t.transition_baseline_call_count,declared_caller_roots=t.transition_caller_roots,after_state=t.transition_after_state)))
  return t
 original.execute=capture
 saved=sys.argv
 try:sys.argv=[saved[0],'--output',str(out/'owning')];original.main()
 finally:sys.argv=saved
 r=json.loads((out/'owning/report.json').read_text());prior=ROOT/'analysis/multiplayer/integration/spawn-source/unit-constructor-tail/measured-complete/owning/report.json';old=json.loads(prior.read_text())
 roots=json.loads((out/'caller-roots.json').read_text())
 assert r['source_stages']==old['source_stages']
 common=set(tail.OLD_OWNERS+tail.OLD_LEAVES+tail.DIRECT+tail.PURE+tail.LOWER)-set(NEW)
 keys=('entry','caller','phase','registers','stack','before','after','return_eax')
 expected=[c for c in old['calls']if not c['phase'].startswith('explicit_original_')]
 assert [{k:c.get(k)for k in keys}for c in r['calls'][:roots['baseline_call_count']]if int(c['entry'],16)in common]==[{k:c.get(k)for k in keys}for c in expected if int(c['entry'],16)in common]
 result=dict(schema='genuine-unit-local-transition-observation-v1',tool_sha256=original.sha(__file__),owning_report_sha256=original.sha(out/'owning/report.json'),prior_constructor_tail_report_sha256=original.sha(prior),prior_successful_prefix_stages_and_common_calls_identical=True,new_owners=[f'{e:08x}'for e in NEW],source_calls=len(r['calls']),new_guest_state=[],instruction_patches=[],supplied_success_returns=[],boundaries=r['boundaries']+['Original BB240 uses actual biped/weapon definitions, source-owned manager/current/saved banks and full object headers. Raw existing clock+FPU fields are recorded, not initialized or advanced here.','New explicit duration/counter requests occur immediately after successful unchanged spawn/cluster ownership, before earlier deliberate invalid bind experiments. No prior negative-case state is restored or reused.'])
 original.save(out/'report.json',original.encoded(result));print(json.dumps(dict(report=str(out/'report.json'),sha256=original.sha(out/'report.json'),calls=len(r['calls']))))
if __name__=='__main__':main()
