#!/usr/bin/env python3
"""Independent byte/operand/full-buffer audit of unchanged contact/effect/node producers."""
import argparse,collections,hashlib,json,struct
from pathlib import Path
from oracle_mp_item_contact_producers_1_1 import ROOT,OUT,FROZEN,ROOTS,digest,encoded,frozen_write
from xbe import Xbe

def obj(state,datum):
 return next(o for o in state['current']['current']['objects']if o['datum']==datum)

def buffers(s):
 result=[]
 def add(p,h,label):
  if h:result.append((int(p,16),bytes.fromhex(h),label))
 def pool(p,label):
  add(p['pointer'],p['header4c_hex'],label+'.header')
  add(p['data_pointer'],p['data_hex'],label+'.data');add(p['bitmap_pointer'],p['bitmap_hex'],label+'.bitmap')
 pool(s['current']['full_object_header_pool'],'objects')
 for o in s['current']['current']['objects']:add(o['body_address'],o['body_hex'],o['datum'])
 for r in s['registries']:
  add(r['global_address'],r['triplet_hex'],'registry.triplet');add(r['heads_pointer'],r['heads800_hex'],'registry.heads')
  pool(r['forward'],'registry.forward');pool(r['reverse'],'registry.reverse')
 for q in s['tags']:add(q['address'],q['raw_hex'],'tag.'+q['datum'])
 for q in s['source_graph_resources']:add(q['resident_pointer'],q['raw_hex'],'resource.'+q['descriptor'])
 add(s['source_fallback_node_bank']['address'],s['source_fallback_node_bank']['raw_hex'],'fallback.nodes')
 q=s['source_quaternion_lazy_constants'];add(q['address'],q['raw8_hex'],'quaternion.constants')
 q=s['source_cluster_visit_globals'];add(q['address'],q['prefix8_hex'],'cluster.globals');add(q['visited_address'],q['visited_hex'],'cluster.visited')
 q=s['source_effect_globals'];add(q['default_material']['address'],q['default_material']['raw2_hex'],'default.material')
 for v in q['vectors']:add(v['pointer'],v['raw12_hex'],'effect.vector.'+v['global_address'])
 q=s['current']['object_context'];add(q['pointer'],q['raw84_hex'],'context')
 return result

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output-dir',type=Path,default=OUT);out=ap.parse_args().output_dir
 d=json.loads((out/'report.json').read_text());assert digest(ROOT/'tools/oracle_mp_item_contact_producers_1_1.py')==d['oracle_sha256']
 for p,h in d['source_pins'].items():assert digest(ROOT/p)==h,p
 assert d['source_pins']==FROZEN and not d['instruction_patches']and not d['new_guest_inputs']and not d['new_supplied_service_results']
 motion=json.loads((ROOT/'analysis/multiplayer/integration/inventory-source/item-motion-boundaries/report.json').read_text())
 active=json.loads((ROOT/'analysis/multiplayer/integration/inventory-source/item-active-schedule/report.json').read_text())
 transform=json.loads((ROOT/'analysis/multiplayer/integration/inventory-source/item-transform-producers/report.json').read_text())
 for key,own in(('owning_motion_correlation',motion),('owning_active_correlation',active),('owning_transform_correlation',transform)):
  c=d[key];assert c['byte_identical']and hashlib.sha256(encoded({k:own[k]for k in c['keys']})).hexdigest()==c['runtime_sha256']
 x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');cache=(ROOT/'assets/disc/maps/lockout.map').read_bytes();sites={};rows=0
 for listing in d['instructions']:
  p=out/listing['file'];assert digest(p)==listing['sha256'];lines=p.read_text().splitlines();assert len(lines)==listing['rows'];rows+=len(lines)
  for line in lines:
   a,raw,*_=line.split();a=int(a,16);raw=bytes.fromhex(raw);assert x.read(a,len(raw))==raw;sites[a]=raw
 for s in d['states']:
  v={k:q for k,q in s.items()if k not in('id','sha256')};assert hashlib.sha256(json.dumps(v,sort_keys=True,separators=(',',':')).encode()).hexdigest()==s['sha256']
  for r in s['registries']:
   assert len(bytes.fromhex(r['heads800_hex']))==0x800
   for k,stride in(('forward',0x20),('reverse',0xc)):
    p=r[k];assert p['capacity']==2048 and p['stride']==stride
    assert len(bytes.fromhex(p['data_hex']))==2048*stride and len(bytes.fromhex(p['bitmap_hex']))==256
  assert len(bytes.fromhex(s['current']['full_object_header_pool']['data_hex']))==2048*12
  # A tag remains literal source-backed metadata. Source modifications, if any,
  # are measured by the store replay rather than silently treated as cache input.
  for q in s['tags']:assert len(bytes.fromhex(q['raw_hex']))==q['size']
  for q in s['source_graph_resources']:
   with (ROOT/q['file']).open('rb')as f:f.seek(q['file_offset']);raw=f.read(q['size'])
   assert raw.hex()==q['raw_hex']and hashlib.sha256(raw).hexdigest()==q['raw_sha256']
 counts=collections.Counter();origins=collections.Counter();replay_ranges=0;replay_events=0
 for call in d['calls']:
  counts[call['entry']]+=1;assert call['order']<call['return_order'];assert len(call['stack_words'])==16
  assert len(call['entry_cpu']['xmm128'])==8 and len(call['return_cpu']['fp80'])==8
  for q,p in zip(call['packets'],call['after_packets']):
   assert all(q[k]==p[k]for k in('field','pointer','size'));assert (q['raw_hex']is None)==(q['pointer']=='00000000')
   if q['raw_hex']is not None:assert len(bytes.fromhex(q['raw_hex']))==q['size']==len(bytes.fromhex(p['raw_hex']))
  assert len(call['packets'])==len(call['after_packets'])
  if not call['root_call']:continue
  assert int(call['entry'],16)in ROOTS
  before=d['states'][call['before_state']];after=d['states'][call['after_state']]
  starting=buffers(before);final={(p,label):raw for p,raw,label in buffers(after)}
  events=[(r['order'],'read',r)for r in d['reads']if call['order']<r['order']<call['return_order']]
  events +=[(r['order'],'store',r)for r in d['stores']if call['order']<r['order']<call['return_order']]
  events.sort()
  for base,raw,label in starting:
   buf=bytearray(raw)
   for _,kind,r in events:
    a=int(r['address'],16);size=r['size']
    if base<=a and a+size<=base+len(buf):
     off=a-base;assert buf[off:off+size].hex()==r['raw_hex'if kind=='read'else'before_hex'],(call['id'],label,kind,r)
     if kind=='store':buf[off:off+size]=bytes.fromhex(r['written_hex'])
     replay_events+=1
   assert bytes(buf)==final[(base,label)],(call['id'],label);replay_ranges+=1
  if call['entry']in('000faf20','000fbbe0'):
   old=next(r for r in motion['calls']if r['entry']==call['entry']and r['scope']['datum']==call['scope']['datum'])
   for side in ('before','after'):
    assert d['states'][call[side+'_state']]['current']=={k:v for k,v in motion['states'][old[side+'_state']].items()if k not in('id','sha256')}
   assert call['entry_cpu']==old['entry_cpu']and call['return_cpu']==old['return_cpu']
  if call['entry']=='000bdf80':
   candidates=[r for r in active['calls']if r['phase']==call['scope']['phase']and r['entry']==call['entry']and r['stack'][0]==call['scope']['datum']]
   assert any(before['current']=={k:v for k,v in active['states'][r['before_state']].items()if k not in('id','sha256')}and after['current']=={k:v for k,v in active['states'][r['after_state']].items()if k not in('id','sha256')}for r in candidates)
 for r in d['reads']:
  assert int(r['instruction'],16)in sites and len(bytes.fromhex(r['raw_hex']))==r['size'];origins[r['origin']['kind']]+=1
  if r['origin']['kind']=='original_xbe_bytes':assert x.read(int(r['address'],16),r['size']).hex()==r['raw_hex']
  if 'cache'in r['origin']:
   q=r['origin']['cache'];a=int(q['file_offset'],16);assert cache[a:a+r['size']].hex()==r['raw_hex']==q['raw_hex']
 for r in d['stores']:
  assert int(r['instruction'],16)in sites and len(bytes.fromhex(r['written_hex']))==r['size']==len(bytes.fromhex(r['before_hex']))
 for r in d['points']:assert int(r['instruction'],16)in sites
 assert all(int(a,16)in sites for a in d['executed_in_scope'])
 assert len(d['scopes'])==sum(c['root_call']for c in d['calls'])
 result=dict(schema=1,report_sha256=digest(out/'report.json'),audit_tool_sha256=digest(__file__),prior_pins=len(FROZEN),
  owning_transform_motion_and_active_runtime_byte_identical=True,calls=len(d['calls']),call_counts=dict(sorted(counts.items())),
  scopes=len(d['scopes']),states=len(d['states']),actual_operand_reads=len(d['reads']),operand_origins=dict(sorted(origins.items())),
  ordered_stores=len(d['stores']),branch_math_points=len(d['points']),full_buffer_replay_ranges=replay_ranges,full_buffer_replay_events=replay_events,
  full_object_node_registry_pool_bitmap_store_replay=True,instruction_files=len(d['instructions']),instruction_rows=rows,
  source_instruction_bytes_verified=True,instructions_or_guest_inputs_patched=False)
 frozen_write(out/'audit.json',encoded(result));print(json.dumps(result))
if __name__=='__main__':main()
