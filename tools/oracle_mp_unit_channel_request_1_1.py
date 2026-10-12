#!/usr/bin/env python3
"""Actual1CEC20/general channel install from genuine retained source creation.

Only declared class/weapon call arguments from prior original selector outputs
are added. No unit/channel/manager/seed/clock/readiness bytes are assigned.
"""
import argparse,json,sys,struct
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_ESI,UC_X86_REG_ECX,UC_X86_REG_EDX
import oracle_mp_unit_constructor_tail_1_1 as tail
import oracle_animation_variant_1_1 as variant
from oracle_unit_animation_manager_1_1 import ClockTrace
original=tail.original;ROOT=original.ROOT
OWN=(0x1cec20,0x1cee00,0x1c9e60,0x1c9f50,0x1ca3a0,0x1deb60,0x1deb10,0x29c6a0,0x1e0f50,0x1df670)
RES=(0x1e13c0,0x1e1230,0x1de9b0,0x124e40)
class ChannelTrace(variant.VariantObserver,tail.TailTrace):
 def __init__(self,data):
  self.root_unit=None;self.clock_metadata={};super().__init__(data)
 def buffers(self,row):
  e=int(row['entry'],16);r=row['registers'];s=[int(v,16)for v in row['stack']]
  if e==0x1cec20:row.update(manager=s[0],channel=r['esi'])
  elif e==0x1cee00:row.update(manager=r['eax'],channel=s[0])
  elif e in(0x1c9e60,0x1c9f50):row['channel']=r['eax']
  elif e==0x1ca3a0:row['channel']=r['esi']
  if'channel'in row and'unit'not in row and self.root_unit is not None:row['unit']=self.root_unit
  result=super().buffers(row)
  if'channel'in row:result['C']=self.raw(row['channel'],32)
  if e in OWN+RES:
   result['R']=self.raw(self.u(0x4ee9d8),8);result['G']=self.raw(0x518410,2)
   key='clock_before'if'clock_before'not in row else'clock_after';row[key]=dict(address=f'{self.u(0x518444):08x}',raw_hex=self.raw(self.u(0x518444),12),fpcw=self.cpu()['fpcw'],mxcsr=self.cpu()['mxcsr'])
  if e in(0x1deb60,0x1df670):row['packed_output']=s[1];result['P']=self.raw(s[1],4)
  if e==0x1e0f50:
   count=self.u(s[0]);pointer=self.u(s[0]+4);assert count<100000
   row['bank']=dict(address=f'{s[0]:08x}',count=count,stride=s[2],raw_hex=self.raw(pointer,count*s[2])if count else'')
  return result
 def label(self,p,n,row):
  if'channel'in row and row['channel']<=p and p+n<=row['channel']+32:return('C',p-row['channel'])
  rng=self.u(0x4ee9d8)
  if rng<=p and p+n<=rng+8:return('R',p-rng)
  return super().label(p,n,row)
 def metadata(self,row):
  result=super().metadata(row)
  if int(row['entry'],16)in(0x1c9f50,0x1ca3a0):ClockTrace.clip(self,row['channel'])
  return result

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();out=a.output.resolve()
 original.OWNERS=tuple(dict.fromkeys(tail.OLD_OWNERS+tail.DIRECT+tail.PURE+OWN+variant.ENTRIES+RES))
 original.LEAVES=tuple(q for q in tail.OLD_LEAVES if q not in tail.DIRECT)+tuple(q for q in tail.LOWER if q not in original.OWNERS)
 original.ConstructorTrace=ChannelTrace
 def execute(data):
  t=original.owning.prepare(data,trace_type=ChannelTrace);t.ctor_enabled=t.selection_enabled=True
  assert original.owning.spawn_and_activate(t,activate=False);t.finish(original.owning.STOP);t.variant_finish(original.owning.STOP);assert not t.ctor_pending and not t.variant_pending
  t.correlated_stages=4;t.caller_variations=[];t.constructor_stops=[];t.root_unit=t.spawned_unit
  t.baseline=t.state();t.baseline_calls=len(t.ctor_calls)
  body=int(t.object(t.spawned_unit)['body'],16);manager=body+t.u(body+0x12a,2)
  prior=ROOT/'analysis/multiplayer/integration/spawn-source/animation-selection/measured/report.json';source=json.loads(prior.read_text());packets=[]
  for c in source['calls']:
   if c['entry']=='001cfde0'and int(c['return_eax'],16)&255 and int(c['outputs_after']['packed'][4:8],16)!=0xffff:
    for kind in('requested','resolved'):
     raw=bytes.fromhex(c['outputs_after'][kind]);ids=struct.unpack('<4I',raw);key=ids[1:3]
     if key not in[(q['wc'],q['weapon'])for q in packets]:packets.append(dict(source_call=c['id'],kind=kind,wc=key[0],weapon=key[1]))
  t.extra_roots=[]
  for name,wc,weapon in [('actual_current_sentinels',0x07000101,0x07000101)]+[(f'original_packet_{p["source_call"]}_{p["kind"]}',p['wc'],p['weapon'])for p in packets]:
   t.active='explicit_original_channel_request_'+name;t.extra_roots.append(dict(name=name,entry='001cec20',manager=f'{manager:08x}',channel=f'{manager:08x}',wc=f'{wc:08x}',weapon=f'{weapon:08x}',first_call=len(t.ctor_calls)))
   try:
    t.run(0x1cec20,(manager,),((UC_X86_REG_ESI,manager),(UC_X86_REG_ECX,wc),(UC_X86_REG_EDX,weapon)));t.finish(original.owning.STOP);t.variant_finish(original.owning.STOP);assert not t.ctor_pending and not t.variant_pending
    t.extra_roots[-1]['completion']='complete_original_return'
   except Exception as error:
    t.extra_roots[-1].update(completion='original_or_observer_stop',error=str(error),cpu=t.cpu());break
  for tag in t.cache.tags.values():
   if tag['group']=='jmad':t.tag(int(tag['datum'],16))
  extra=dict(schema='source-channel-request-observer-extension-v1',baseline=t.baseline,baseline_calls=t.baseline_calls,extra_roots=t.extra_roots,prior_packet_report=str(prior.relative_to(ROOT)),prior_packet_report_sha256=original.sha(prior),source_definitions=t.clock_metadata,variant_calls=t.variant_calls,variant_stores=t.variant_stores,variant_reads=t.variant_reads,after_state=t.state(),jump_table=dict(address='001ced84',raw_hex=t.raw(0x1ced84,24)),literal_tags=t.literal_tags,new_guest_fields=[],new_success_services=[])
  original.save(out/'extension.json',original.encoded(extra));return t
 original.execute=execute
 saved=sys.argv
 try:sys.argv=[saved[0],'--output',str(out/'owning')];original.main()
 finally:sys.argv=saved
 r=json.loads((out/'owning/report.json').read_text());x=json.loads((out/'extension.json').read_text());old=json.loads((ROOT/'analysis/multiplayer/integration/spawn-source/unit-local-transition/measured-positive/caller-roots.json').read_text());assert x['baseline']==old['baseline_state']
 value=dict(schema='retained-unit-channel-request-original-v1',tool_sha256=original.sha(__file__),owning_report_sha256=original.sha(out/'owning/report.json'),extension_sha256=original.sha(out/'extension.json'),successful_original_prefix_state_identical=True,new_guest_fields=[],supplied_success=[],roots=x['extra_roots'],scope='Actual positive prefix then declared request class/weapon values from earlier original outputs. Clock/RNG/resource-byte compatibility fixture boundaries remain explicit. No live local request or nativeUnit bridge.')
 original.save(out/'report.json',original.encoded(value));print(json.dumps(dict(report=str(out/'report.json'),sha256=original.sha(out/'report.json'),calls=len(r['calls']))))
if __name__=='__main__':main()
