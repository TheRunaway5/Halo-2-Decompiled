#!/usr/bin/env python3
"""Independent original-byte and literal owning pose-buffer store audit."""
import argparse,bisect,collections,hashlib,json
from pathlib import Path
from oracle_mp_spartan_owned_pose_1_1 import ROOT,OUT,FROZEN,digest,encoded,frozen_write
from xbe import Xbe

def buffers(s):
 result=[]
 for o in s['objects']:
  result.append((int(o['body_address'],16),bytes.fromhex(o['body_hex']),'object.'+o['datum']))
  result.append((int(o['header_address'],16),bytes.fromhex(o['header_hex']),'header.'+o['datum']))
 q=s['clock'];result.append((int(q['pointer'],16),bytes.fromhex(q['raw24_hex']),'clock'))
 q=s['seed'];result.append((int(q['pointer'],16),bytes.fromhex(q['raw4_hex']),'seed'))
 return result

def unit(d,i):return next(o for o in d['states'][i]['objects']if o.get('definition')=='e9f2086d')

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output-dir',type=Path,default=OUT);out=ap.parse_args().output_dir
 d=json.loads((out/'report.json').read_text());assert digest(ROOT/'tools/oracle_mp_spartan_owned_pose_1_1.py')==d['oracle_sha256']
 assert d['source_pins']==FROZEN
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 assert not d['new_guest_inputs']and not d['new_supplied_pose_classification_admission_or_service_results']and not d['instruction_patches']
 correlation=d['prior_runtime_correlation'];p=ROOT/correlation['report'];prior=json.loads(p.read_text())
 assert digest(p)==correlation['report_sha256']and correlation['byte_identical']
 assert hashlib.sha256(encoded({k:prior[k]for k in correlation['keys']})).hexdigest()==correlation['runtime_sha256']
 x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');sites={};rows=0
 for q in d['instructions']:
  p=out/q['file'];assert digest(p)==q['sha256'];lines=p.read_text().splitlines();assert len(lines)==q['rows'];rows+=len(lines)
  for line in lines:
   a,b,*_=line.split();a=int(a,16);v=bytes.fromhex(b);assert x.read(a,len(v))==v;sites[a]=v
 for s in d['states']:
  v={k:x for k,x in s.items()if k not in('id','sha256')};assert hashlib.sha256(encoded(v)).hexdigest()==s['sha256']
  for o in s['objects']:
   assert len(bytes.fromhex(o['body_hex']))==o['body_size']
   for b in o.get('node_blocks',[]):
    if b['size']:assert b['size']==len(bytes.fromhex(b['raw_hex']))and b['offset']+b['size']<=o['body_size']
 read_counts=collections.Counter();store_counts=collections.Counter();call_counts=collections.Counter()
 for r in d['reads']:
  assert int(r['instruction'],16)in sites and len(bytes.fromhex(r['raw_hex']))==r['size'];read_counts[r['instruction']]+=1
 for r in d['stores']:
  assert int(r['instruction'],16)in sites and len(bytes.fromhex(r['written_hex']))==r['size']==len(bytes.fromhex(r['before_hex']));store_counts[r['instruction']]+=1
 events=sorted([(r['order'],'read',r)for r in d['reads']]+[(r['order'],'store',r)for r in d['stores']])
 event_orders=[e[0]for e in events]
 ranges=0;matches=0;packet_ranges=0;packet_matches=0
 def replay(p,initial,expected,name,lo,hi):
  nonlocal matches
  value=bytearray(initial)
  for order,kind,e in events[bisect.bisect_right(event_orders,lo):bisect.bisect_left(event_orders,hi)]:
   a=int(e['address'],16);n=e['size']
   left,right=max(p,a),min(p+len(value),a+n)
   if left>=right:continue
   off=left-p;start=left-a;length=right-left
   data=bytes.fromhex(e['raw_hex'if kind=='read'else'before_hex'])
   assert value[off:off+length]==data[start:start+length],(name,e)
   if kind=='store':value[off:off+length]=bytes.fromhex(e['written_hex'])[start:start+length]
   matches+=1
  assert bytes(value)==expected,(name,lo,hi)
 for r in d['calls']:
  call_counts[r['entry']]+=1;assert len(r['stack24_words'])==24 and r['return_order']>r['order']
  assert len(r['entry_cpu']['fp80'])==8 and len(r['entry_cpu']['xmm128'])==8
  if 'before_state'in r:
   before=d['states'][r['before_state']];after=d['states'][r['after_state']]
   ends={(p,n):raw for p,raw,n in buffers(after)}
   for p,raw,n in buffers(before):
    if (p,n)in ends and len(raw)==len(ends[p,n]):
     replay(p,raw,ends[p,n],(r['id'],n),r['order'],r['return_order']);ranges+=1
  for a,b in zip(r['packets'],r['after_packets']):
   if int(a['pointer'],16):
    start_matches=matches
    replay(int(a['pointer'],16),bytes.fromhex(a['raw_hex']),bytes.fromhex(b['raw_hex']),(r['id'],a['field']),r['order'],r['return_order'])
    packet_ranges+=1;packet_matches+=matches-start_matches
 e44=next(r for r in d['calls']if r['entry']=='000e44c0');assert e44['stack24_words'][:2]==['00000000','00000000']
 assert e44['packets']==e44['after_packets']
 classification=next(r for r in d['calls']if r['entry']=='001e8f60')
 packet=classification['after_packets'][0];assert packet['field']=='eax_classification_result70'and packet['raw_hex'][:8]=='00000000'
 request=next(r for r in d['calls']if r['entry']=='00112e10');assert request['packets'][0]['raw_hex']=='010100070c00000400000000'
 b,a=unit(d,request['before_state']),unit(d,request['after_state'])
 assert b['selected']['frame_bits']=='00000000'and a['selected']['frame_bits']=='3f800000'
 assert b['selected']['packed']==a['selected']['packed']=='00b9ffff'
 assert a['biped_fields']['owner']=='ec700000'and a['biped_fields']['physical_state_3dc10_hex']=='0100000000006fe2ffffffffffffffff'
 pose=next(r for r in d['calls']if r['entry']=='000bdf80'and r['phase']=='original_new_object_pass'and r['stack24_words'][0]=='e26f0000')
 b,a=unit(d,pose['before_state']),unit(d,pose['after_state']);changed={}
 for x,y in zip(b['node_blocks'],a['node_blocks']):
  if x['field']not in('110','114'):continue
  stride=32 if x['field']=='110' else 52
  v,w=bytes.fromhex(x['raw_hex']),bytes.fromhex(y['raw_hex']);assert len(v)==len(w)
  changed[x['field']]=[i//stride for i in range(0,len(v),stride)if v[i:i+stride]!=w[i:i+stride]]
 assert len(changed['110'])==19 and len(changed['114'])==33
 assert b['root']['bits']==a['root']['bits']==['bfb9579d','3c855b09','c1235809']
 locals_=next(r for r in d['calls']if r['entry']=='00113830'and r['phase']=='original_new_object_pass')
 assert locals_['stack24_words'][2]=='00000015'and locals_['packets'][1]['size']==21*32
 assert d['stages']==prior['stages']and not any(e.get('service')for q in d['stages'][-4:]for e in q['services'])
 result=dict(schema=1,report_sha256=digest(out/'report.json'),auditor_sha256=digest(__file__),prior_pins=len(FROZEN),
  prior_runtime_correlation_verified=True,calls=len(d['calls']),call_counts=dict(sorted(call_counts.items())),states=len(d['states']),reads=len(d['reads']),ordered_stores=len(d['stores']),
  complete_buffer_intervals=ranges,packet_intervals=packet_ranges,replayed_observations=matches,packet_replayed_observations=packet_matches,
  instruction_files=len(d['instructions']),instruction_rows=rows,original_instruction_bytes_verified=True,
  actual_classification=0,actual_blend_byte=0,original_request12_hex=request['packets'][0]['raw_hex'],
  primary_clip=185,primary_clock_before='00000000',primary_clock_after='3f800000',source_local_sample_count=21,
  changed_local_nodes=changed['110'],changed_current_matrix_nodes=changed['114'],
  source_schedule_compositions=1,full_admitted_frames=0,positive_nonneutral_input_producer=False,new_guest_results=False)
 frozen_write(out/'audit.json',encoded(result));print(json.dumps(result))
if __name__=='__main__':main()
