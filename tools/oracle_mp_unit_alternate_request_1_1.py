#!/usr/bin/env python3
"""Complete1DF670 and1CFA90 from genuine creation/current-manager owners."""
import argparse,json,sys
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_ESI
import oracle_mp_unit_channel_request_1_1 as channel
original=channel.original;ROOT=original.ROOT
class AlternateTrace(channel.ChannelTrace):
 def buffers(self,row):
  e=int(row['entry'],16);s=[int(q,16)for q in row['stack']]
  if e==0x1cfa90:
   row['manager']=row['registers']['esi']
   if 'unit'not in row and self.root_unit is not None:row['unit']=self.root_unit
  result=super().buffers(row)
  if e==0x1df670:
   for i,p in enumerate(s[5:8]):
    if p:result[f'O{i}']=self.raw(p,4)
  if e==0x1cfa90:
   result['R']=self.raw(self.u(0x4ee9d8),8);result['G']=self.raw(0x518410,2)
  return result

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();out=a.output.resolve()
 original.OWNERS=tuple(dict.fromkeys(channel.tail.OLD_OWNERS+channel.tail.DIRECT+channel.tail.PURE+channel.OWN+channel.variant.ENTRIES+channel.RES+(0x1cfa90,)))
 original.LEAVES=tuple(q for q in channel.tail.OLD_LEAVES if q not in original.OWNERS)+tuple(q for q in channel.tail.LOWER if q not in original.OWNERS)
 original.ConstructorTrace=AlternateTrace
 def execute(data):
  t=original.owning.prepare(data,trace_type=AlternateTrace);t.ctor_enabled=t.selection_enabled=True
  assert original.owning.spawn_and_activate(t,activate=False);t.finish(original.owning.STOP);t.variant_finish(original.owning.STOP);assert not t.ctor_pending and not t.variant_pending
  t.correlated_stages=4;t.caller_variations=[];t.constructor_stops=[];t.root_unit=t.spawned_unit;t.baseline=t.state()
  body=int(t.object(t.spawned_unit)['body'],16);manager=body+t.u(body+0x12a,2);first=len(t.ctor_calls);t.active='explicit_original1CFA90_actual_owned_manager'
  t.run(0x1cfa90,(),((UC_X86_REG_ESI,manager),));t.finish(original.owning.STOP);t.variant_finish(original.owning.STOP);assert not t.ctor_pending and not t.variant_pending
  for tag in t.cache.tags.values():
   if tag['group']=='jmad':t.tag(int(tag['datum'],16))
  extra=dict(schema='alternate-request-owning-extension-v1',baseline=t.baseline,after=t.state(),actual_manager=f'{manager:08x}',explicit_root_first_call=first,literal_tags=t.literal_tags,new_guest_fields=[],new_success_services=[])
  original.save(out/'extension.json',original.encoded(extra));return t
 original.execute=execute;saved=sys.argv
 try:sys.argv=[saved[0],'--output',str(out/'owning')];original.main()
 finally:sys.argv=saved
 r=json.loads((out/'owning/report.json').read_text());x=json.loads((out/'extension.json').read_text());prior=ROOT/'analysis/multiplayer/integration/spawn-source/unit-channel-request/measured-draft/extension.json';assert x['baseline']==json.loads(prior.read_text())['baseline']
 cs=[c for c in r['calls']if c['entry']=='001df670'];positive=[c['id']for c in cs if bytes.fromhex(c['after']['P'])[2:4]!=b'\xff\xff'];assert positive
 value=dict(schema='complete-alternate-request-original-v1',tool_sha256=original.sha(__file__),owning_report_sha256=original.sha(out/'owning/report.json'),extension_sha256=original.sha(out/'extension.json'),prior_baseline_sha256=original.sha(prior),unchanged_baseline=True,alternate_cases=len(cs),positive_cases=positive,availability_cases=len([c for c in r['calls']if c['entry']=='001cfa90']),source_calls=len(r['calls']),retained_and_scratch_stores=len(r['stores']),new_guest_fields=[],new_success_services=[],scope='Original constructor positive alternate requests and explicit actual-manager availability. No admitted channel/body/state/device/clock/resource field supplied.')
 original.save(out/'report.json',original.encoded(value));print(json.dumps(value))
if __name__=='__main__':main()
