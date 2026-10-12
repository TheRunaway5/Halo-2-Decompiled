#!/usr/bin/env python3
"""Observe complete retained114BD0 pose-request constructor chain, unchanged."""
import argparse,json,sys,struct
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX
import oracle_mp_unit_alternate_request_1_1 as alternate
channel=alternate.channel;original=alternate.original;ROOT=original.ROOT
OWN=(0x114bd0,0x1cf4c0,0x1cf3c0,0x1cf360,0x1cef10,0x1cfb90,0x115af0,0x1cfde0,0x1cfce0)
class PoseTrace(alternate.AlternateTrace):
 def buffers(self,row):
  e=int(row['entry'],16);r=row['registers'];s=[int(q,16)for q in row['stack']]
  if e in(0x114bd0,0x115af0):row['unit']=s[0]if e==0x114bd0 else r['eax']
  if e==0x1cf4c0:row.update(manager=s[0],packed_output=s[1])
  elif e==0x1cf3c0:row.update(manager=r['eax'],packed_output=r['ebx'])
  elif e==0x1cf360:row.update(manager=r['eax'],packed_output=r['esi'])
  elif e==0x1cef10:row.update(manager=r['esi'],channel=s[0])
  elif e==0x1cfb90:row.update(manager=r['eax'],packed_output=r['esi'])
  elif e==0x1cfde0:row.update(manager=r['eax'],requested=r['edx'],resolved=r['ecx'],packed_output=s[5])
  if e in OWN and'unit'not in row and self.root_unit is not None:row['unit']=self.root_unit
  result=super().buffers(row)
  if e in OWN:
   result['R']=self.raw(self.u(0x4ee9d8),8);result['G']=self.raw(0x518410,2)
   key='clock_before'if'clock_before'not in row else'clock_after';row[key]=dict(address=f'{self.u(0x518444):08x}',raw_hex=self.raw(self.u(0x518444),12),fpcw=self.cpu()['fpcw'],mxcsr=self.cpu()['mxcsr'])
  if e in(0x114bd0,0x115af0):
   pool=self.pool();assert pool['slot_bytes']==12;result['K']=self.raw(int(pool['headers'],16),pool['capacity']*pool['slot_bytes'])
  if'packed_output'in row:result['P']=self.raw(row['packed_output'],4)
  if e==0x1cfde0:
   result['Q']=self.raw(row['requested'],16);result['S']=self.raw(row['resolved'],16)
  return result
 def label(self,p,n,row):
  for key,name,extent in [('packed_output','P',4),('requested','Q',16),('resolved','S',16)]:
   if key in row and row[key]<=p and p+n<=row[key]+extent:return(name,p-row[key])
  return super().label(p,n,row)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();out=a.output.resolve()
 original.OWNERS=tuple(dict.fromkeys(channel.tail.OLD_OWNERS+channel.tail.DIRECT+channel.tail.PURE+channel.OWN+channel.variant.ENTRIES+channel.RES+(0x1cfa90,)+OWN))
 original.LEAVES=tuple(q for q in channel.tail.OLD_LEAVES if q not in original.OWNERS)+tuple(q for q in channel.tail.LOWER if q not in original.OWNERS)
 original.ConstructorTrace=PoseTrace
 def execute(data):
  t=original.owning.prepare(data,trace_type=PoseTrace);t.ctor_enabled=t.selection_enabled=True
  assert original.owning.spawn_and_activate(t,activate=False);t.finish(original.owning.STOP);t.variant_finish(original.owning.STOP);assert not t.ctor_pending and not t.variant_pending
  t.correlated_stages=4;t.caller_variations=[];t.constructor_stops=[];t.root_unit=t.spawned_unit;t.baseline=t.state();first=len(t.ctor_calls)
  t.active='explicit_original114BD0_actual_owned_unit';t.run(0x114bd0,(t.spawned_unit,));t.finish(original.owning.STOP);t.variant_finish(original.owning.STOP);assert not t.ctor_pending and not t.variant_pending
  for tag in t.cache.tags.values():
   if tag['group']=='jmad':t.tag(int(tag['datum'],16))
  extra=dict(schema='unit-pose-request-owning-extension-v1',baseline=t.baseline,after=t.state(),explicit_root_first_call=first,literal_tags=t.literal_tags,source_definitions=t.clock_metadata,variant_calls=t.variant_calls,variant_stores=t.variant_stores,variant_reads=t.variant_reads,activity_dispatch=dict(byte_table_address='00114ef0',byte_table_raw_hex=t.raw(0x114ef0,12),jump_table_address='00114ee4',jump_table_raw_hex=t.raw(0x114ee4,12)),new_guest_fields=[],new_success_services=[])
  original.save(out/'extension.json',original.encoded(extra));return t
 original.execute=execute;saved=sys.argv
 try:sys.argv=[saved[0],'--output',str(out/'owning')];original.main()
 finally:sys.argv=saved
 r=json.loads((out/'owning/report.json').read_text());x=json.loads((out/'extension.json').read_text());prior=ROOT/'analysis/multiplayer/integration/spawn-source/unit-alternate-request/measured/extension.json';assert x['baseline']==json.loads(prior.read_text())['baseline']
 cases={f'{e:08x}':len([q for q in r['calls']if q['entry']==f'{e:08x}'])for e in OWN}
 value=dict(schema='unit-pose-requests-original-v1',tool_sha256=original.sha(__file__),owning_report_sha256=original.sha(out/'owning/report.json'),extension_sha256=original.sha(out/'extension.json'),prior_baseline_sha256=original.sha(prior),unchanged_baseline=True,owner_cases=cases,source_calls=len(r['calls']),retained_and_scratch_stores=len(r['stores']),new_guest_fields=[],new_success_services=[],scope='Unchanged original unit constructor/equip pose-request owners plus one actual-unit root. No final body offsets/state or resource success supplied.')
 original.save(out/'report.json',original.encoded(value));print(json.dumps(value))
if __name__=='__main__':main()
