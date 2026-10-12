#!/usr/bin/env python3
"""Complete114F00 over original header/body/component producer state.

No body/header/component/channel fields are assigned. A second explicit root
uses the unit produced by the unchanged owning prefix, preserving its NONE
subchannel gates. Full original header capacity/backing is observed read-only.
"""
import argparse,json,sys,struct
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX
import oracle_mp_unit_channel_request_1_1 as channel
original=channel.original;ROOT=original.ROOT
class CallerTrace(channel.ChannelTrace):
 def buffers(self,row):
  if int(row['entry'],16)==0x114f00:row['unit']=row['registers']['eax']
  result=super().buffers(row)
  if int(row['entry'],16)==0x114f00:
   pool=self.pool();at=int(pool['headers'],16);result['POOL']=self.raw(at,pool['capacity']*12)
   result['R']=self.raw(self.u(0x4ee9d8),8);result['G']=self.raw(0x518410,2)
  return result

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();out=a.output.resolve()
 original.OWNERS=tuple(dict.fromkeys(channel.tail.OLD_OWNERS+channel.tail.DIRECT+channel.tail.PURE+channel.OWN+channel.variant.ENTRIES+channel.RES+(0x114f00,)))
 original.LEAVES=tuple(q for q in channel.tail.OLD_LEAVES if q not in original.OWNERS)+tuple(q for q in channel.tail.LOWER if q not in original.OWNERS)
 original.ConstructorTrace=CallerTrace
 def execute(data):
  t=original.owning.prepare(data,trace_type=CallerTrace);t.ctor_enabled=t.selection_enabled=True
  assert original.owning.spawn_and_activate(t,activate=False);t.finish(original.owning.STOP);t.variant_finish(original.owning.STOP);assert not t.ctor_pending and not t.variant_pending
  t.correlated_stages=4;t.caller_variations=[];t.constructor_stops=[];t.root_unit=t.spawned_unit
  t.baseline=t.state();before=len(t.ctor_calls);t.active='explicit_original114F00_actual_owned_unit'
  t.run(0x114f00,(),((UC_X86_REG_EAX,t.spawned_unit),));t.finish(original.owning.STOP);t.variant_finish(original.owning.STOP);assert not t.ctor_pending and not t.variant_pending
  for tag in t.cache.tags.values():
   if tag['group']=='jmad':t.tag(int(tag['datum'],16))
  extra=dict(schema='unit-subchannel-caller-original-extension-v1',baseline=t.baseline,after=t.state(),explicit_root_first_call=before,actual_unit=f'{t.spawned_unit:08x}',literal_tags=t.literal_tags,source_definitions=t.clock_metadata,new_guest_fields=[],new_success_services=[])
  original.save(out/'extension.json',original.encoded(extra));return t
 original.execute=execute;saved=sys.argv
 try:sys.argv=[saved[0],'--output',str(out/'owning')];original.main()
 finally:sys.argv=saved
 r=json.loads((out/'owning/report.json').read_text());x=json.loads((out/'extension.json').read_text());prior_path=ROOT/'analysis/multiplayer/integration/spawn-source/unit-channel-request/measured-draft/extension.json';prior=json.loads(prior_path.read_text());assert x['baseline']==prior['baseline']
 cases=[c for c in r['calls']if c['entry']=='00114f00'];assert len(cases)==2
 for c in cases:
  assert c['before']==c['after'];nested=[q for q in r['calls']if c['id']in q['ancestors']and q['entry']=='001cec20'];assert len(nested)==3
  for q in nested:assert int(q['return_eax'],16)&255==0 and q['before']==q['after']
 value=dict(schema='complete114F00-original-caller-v1',tool_sha256=original.sha(__file__),report_sha256=original.sha(out/'owning/report.json'),extension_sha256=original.sha(out/'extension.json'),prior_channel_extension_sha256=original.sha(prior_path),unchanged_positive_baseline=True,caller_cases=2,complete_lower_calls=6,source_calls=len(r['calls']),retained_and_scratch_stores=len(r['stores']),new_guest_fields=[],new_success_services=[],scope='Actual source-created pool/body; constructor caller plus explicit actual-unit caller. SourceNONE gates retained. No finalUnit injection or admitted localQ.')
 original.save(out/'report.json',original.encoded(value));print(json.dumps(value))
if __name__=='__main__':main()
