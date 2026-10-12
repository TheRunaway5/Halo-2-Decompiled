#!/usr/bin/env python3
"""Read-only original bytes, stored motion operands and ownership audit."""
import argparse,collections,hashlib,json
from pathlib import Path
from oracle_mp_item_motion_boundaries_1_1 import ROOT,OUT,FROZEN,digest,encoded,frozen_write,CORE,NAMED
from xbe import Xbe

def obj(states,state,datum):
 return next(o for o in states[state]['current']['objects']if o['datum']==datum)

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output-dir',type=Path,default=OUT);args=ap.parse_args();out=args.output_dir
 d=json.loads((out/'report.json').read_text());assert digest(ROOT/'tools/oracle_mp_item_motion_boundaries_1_1.py')==d['oracle_sha256']
 for p,h in d['source_pins'].items():assert digest(ROOT/p)==h,p
 assert d['source_pins']==FROZEN and not d['instruction_patches']and not d['new_guest_inputs']and not d['new_supplied_service_results']
 own=json.loads((ROOT/'analysis/multiplayer/integration/inventory-source/item-active-schedule/report.json').read_text())
 correlation=d['owning_runtime_correlation'];runtime={k:own[k]for k in correlation['byte_identical_keys']}
 assert hashlib.sha256(encoded(runtime)).hexdigest()==correlation['runtime_sha256']
 x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');cache=(ROOT/'assets/disc/maps/lockout.map').read_bytes();sites={};instruction_rows=0
 for listing in d['instructions']:
  p=out/listing['file'];assert digest(p)==listing['sha256'];lines=p.read_text().splitlines();assert len(lines)==listing['rows']
  for line in lines:
   a,raw,*_=line.split();a=int(a,16);raw=bytes.fromhex(raw);assert x.read(a,len(raw))==raw;sites[a]=raw
  instruction_rows+=len(lines)
 for state in d['states']:
  value={k:v for k,v in state.items()if k not in('id','sha256')}
  assert hashlib.sha256(json.dumps(value,sort_keys=True,separators=(',',':')).encode()).hexdigest()==state['sha256']
  pool=state['full_object_header_pool'];assert len(bytes.fromhex(pool['data_hex']))==2048*12 and len(bytes.fromhex(pool['bitmap_hex']))==256
  assert pool['capacity']==2048 and pool['stride']==12
 for tag in d['definitions']:
  record=tag['record'];at=int(record['file_offset'],16);assert cache[at:at+tag['size']].hex()==record['raw_hex']
 calls=d['calls'];states=d['states'];counts=collections.Counter(r['entry']for r in calls);rows=collections.Counter();operand_origins=collections.Counter()
 for call in calls:
  assert 'return_order'in call and call['order']<call['return_order']
  assert len(call['stack_words'])==9 and len(call['entry_cpu']['xmm128'])==8 and len(call['return_cpu']['fp80'])==8
  assert len(call['packets'])==len(call['after_packets'])
  for q,p in zip(call['packets'],call['after_packets']):
   assert all(q[k]==p[k]for k in('field','pointer','size'))
   assert (q['raw_hex']is None)==(q['pointer']=='00000000')
   if q['raw_hex']is not None:assert len(bytes.fromhex(q['raw_hex']))==q['size']==len(bytes.fromhex(p['raw_hex']))
  if int(call['entry'],16)not in CORE:continue
  datum=call['scope']['datum'];before=obj(states,call['before_state'],datum);after=obj(states,call['after_state'],datum)
  source=next(r for r in own['calls']if r['phase']=='original_new_object_pass'and r['entry']==call['entry']and r['stack'][0]==datum)
  assert before==obj(own['states'],source['before_state'],datum)and after==obj(own['states'],source['after_state'],datum)
  assert call['entry_cpu']['registers']==source['registers']and call['return_cpu']['registers']['eax']==source['returned_eax']
  assert call['return_esp_delta']==source['return_esp_delta']
  rows[(call['scope']['row'],call['entry'])]+=1
  body=int(before['body_address'],16);buffer=bytearray.fromhex(before['body_hex'])
  events=[(r['order'],'read',r)for r in d['reads']if call['order']<r['order']<call['return_order']and r['scope']['datum']==datum]
  events +=[(r['order'],'store',r)for r in d['stores']if call['order']<r['order']<call['return_order']and r['scope']['datum']==datum]
  for _,kind,r in sorted(events):
   p=int(r['address'],16);size=r['size']
   if body<=p and p+size<=body+len(buffer):
    off=p-body;assert buffer[off:off+size].hex()==r['raw_hex'if kind=='read'else'before_hex'],(kind,r)
    if kind=='store':buffer[off:off+size]=bytes.fromhex(r['written_hex'])
  assert buffer.hex()==after['body_hex'],call['id']
 for r in d['reads']:
  assert int(r['instruction'],16)in sites and len(bytes.fromhex(r['raw_hex']))==r['size'];operand_origins[r['origin']['kind']]+=1
  if r['origin']['kind']=='original_xbe_bytes':assert x.read(int(r['address'],16),r['size']).hex()==r['raw_hex']
  if r['origin']['kind']=='original_weapon_definition':
   tag=next(q for q in d['definitions']if q['datum']==r['origin']['definition']);off=int(r['origin']['offset'],16)
   assert bytes.fromhex(tag['record']['raw_hex'])[off:off+r['size']].hex()==r['raw_hex']
  if 'cache'in r['origin']:
   q=r['origin']['cache'];at=int(q['file_offset'],16);assert cache[at:at+r['size']].hex()==r['raw_hex']==q['raw_hex']
 for r in d['stores']:
  a=int(r['instruction'],16);assert a in sites and x.read(a,len(sites[a]))==sites[a]
  assert len(bytes.fromhex(r['before_hex']))==r['size']==len(bytes.fromhex(r['written_hex']))
 for addresses in d['executed_in_scope'].values():assert all(int(a,16)in sites for a in addresses)
 assert all(rows[(row,f'{entry:08x}')]==1 for row in(0,1,12)for entry in CORE)
 delivered=[r for r in own['calls']if r['phase']=='original_new_object_pass'and r['entry']in('0016b240','0016ba30','001e20c0','00246a50')]
 assert delivered==d['source_outer_delivery'];outer=[r for r in delivered if r['entry']=='0016b240'];clusters=[r for r in delivered if r['entry']=='00246a50']
 assert len(outer)==2 and [int(r['returned_eax'],16)&255 for r in outer]==[0,1]
 assert len(clusters)==2 and [int(r['returned_eax'],16)&255 for r in clusters]==[0,0]
 assert [r['source_cluster_a8_count']for r in clusters]==[624,96]
 actual_named=set(int(r['entry'],16)for r in calls)&NAMED
 assert d['unexecuted_named_entries']==[f'{e:08x}'for e in sorted(NAMED-actual_named)]
 result=dict(schema=1,report_sha256=digest(out/'report.json'),audit_tool_sha256=digest(__file__),prior_pins=len(FROZEN),
  owning_runtime_byte_identical=True,calls=len(calls),call_counts=dict(sorted(counts.items())),states=len(states),
  actual_operand_reads=len(d['reads']),operand_origins=dict(sorted(operand_origins.items())),ordered_stores=len(d['stores']),
  math_branch_points=len(d['points']),core_paths=6,actual_outer_casts=len(outer),actual_terrain_hits=1,actual_cluster_misses=len(clusters),
  instruction_files=len(d['instructions']),instruction_rows=instruction_rows,unexecuted_named_entries=d['unexecuted_named_entries'],
  owning_body_operand_store_replay=True,source_instruction_bytes_verified=True,instructions_or_guest_inputs_patched=False)
 frozen_write(out/'audit.json',encoded(result));print(json.dumps(result))
if __name__=='__main__':main()
