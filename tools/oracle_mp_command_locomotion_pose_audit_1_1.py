#!/usr/bin/env python3
"""Audit unchanged command/control/request/channel/pose buffers and original bytes."""
import argparse,bisect,collections,hashlib,json
from pathlib import Path
from oracle_mp_command_locomotion_pose_1_1 import ROOT,OUT,FROZEN,digest,encoded,frozen_write
from xbe import Xbe

def buffers(s):
 out=[]
 for o in s['objects']:
  out.extend([(int(o['body_address'],16),bytes.fromhex(o['body_hex']),'object.'+o['datum']),
   (int(o['header_address'],16),bytes.fromhex(o['header_hex']),'header.'+o['datum'])])
 for kind,q,n in [('clock',s['clock'],'raw24_hex'),('seed',s['seed'],'raw4_hex'),('player',s['player'],'raw21c_hex')]:out.append((int(q['pointer'],16),bytes.fromhex(q[n]),kind))
 return out

def unit(d,ind):return next(o for o in d['states'][ind]['objects']if o['datum']=='e26f0000')

def selected(o):
 b=bytes.fromhex(o['body_hex']);size,offset=(int.from_bytes(b[n:n+2],'little')for n in(0x128,0x12a))
 assert size==0x90 and offset+size<=len(b)
 return dict(graph=f'{int.from_bytes(b[offset:offset+4],"little"):08x}',packed=f'{int.from_bytes(b[offset+4:offset+8],"little"):08x}',
  clip=int.from_bytes(b[offset+6:offset+8],'little'),frame_bits=f'{int.from_bytes(b[offset+0x1c:offset+0x20],"little"):08x}',
  activity=f'{int.from_bytes(b[offset+0x7c:offset+0x80],"little"):08x}')

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--directory',type=Path,default=OUT/'measured');out=ap.parse_args().directory
 d=json.loads((out/'report.json').read_text());assert digest(ROOT/'tools/oracle_mp_command_locomotion_pose_1_1.py')==d['oracle_sha256']
 assert d['source_pins']==FROZEN
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 assert not d['instruction_patches']and not d['new_supplied_classification_pose_clock_physical_mode_contact_admission_or_success']
 corr=d['prior_runtime_correlation'];p=ROOT/corr['report'];assert digest(p)==corr['report_sha256'];prior=json.loads(p.read_text())
 old=next(c for c in prior['cases']if c['case']==corr['case']);assert hashlib.sha256(encoded({k:old[k]for k in corr['keys']})).hexdigest()==corr['runtime_sha256']and corr['byte_identical']
 x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');sites={};rows=0
 for q in d['instructions']:
  p=out/q['file'];assert digest(p)==q['sha256'];lines=p.read_text().splitlines();assert len(lines)==q['rows'];rows+=len(lines)
  for line in lines:
   a,b,*_=line.split();a=int(a,16);v=bytes.fromhex(b);assert x.read(a,len(v))==v;sites[a]=v
 for s in d['states']:
  v={k:q for k,q in s.items()if k not in('id','sha256')};assert hashlib.sha256(encoded(v)).hexdigest()==s['sha256']
  for o in s['objects']:
   assert len(bytes.fromhex(o['body_hex']))==o['body_size']
   for n in o['node_blocks']:
    if n['size']:assert n['offset']+n['size']<=o['body_size'] and len(bytes.fromhex(n['raw_hex']))==n['size']
 events=sorted([(r['order'],'read',r)for r in d['reads']]+[(r['order'],'store',r)for r in d['stores']]);orders=[q[0]for q in events]
 for _,kind,r in events:
  assert int(r['instruction'],16)in sites
  assert len(bytes.fromhex(r['raw_hex'if kind=='read'else'before_hex']))==r['size']
  if kind=='store':assert len(bytes.fromhex(r['written_hex']))==r['size']
 intervals=packets=matches=0
 def replay(p,initial,expected,label,lo,hi):
  nonlocal matches
  value=bytearray(initial)
  for order,kind,r in events[bisect.bisect_right(orders,lo):bisect.bisect_left(orders,hi)]:
   a=int(r['address'],16);left,right=max(p,a),min(p+len(value),a+r['size'])
   if left>=right:continue
   off,start,length=left-p,left-a,right-left
   raw=bytes.fromhex(r['raw_hex'if kind=='read'else'before_hex'])
   assert value[off:off+length]==raw[start:start+length],(label,r)
   if kind=='store':value[off:off+length]=bytes.fromhex(r['written_hex'])[start:start+length]
   matches+=1
  assert bytes(value)==expected,(label,lo,hi)
 for r in d['calls']:
  assert len(r['stack24_words'])==24 and r['return_order']>r['order']
  if 'before_state'in r:
   before,after=d['states'][r['before_state']],d['states'][r['after_state']];ends={(p,n):v for p,v,n in buffers(after)}
   for p,v,n in buffers(before):
    if (p,n)in ends:replay(p,v,ends[p,n],(r['id'],n),r['order'],r['return_order']);intervals+=1
  for p,q in zip(r['packets'],r['after_packets']):
   if int(p['pointer'],16):replay(int(p['pointer'],16),bytes.fromhex(p['raw_hex']),bytes.fromhex(q['raw_hex']),(r['id'],p['field']),r['order'],r['return_order']);packets+=1
 actual=[];cache=(ROOT/'assets/disc/maps/lockout.map').read_bytes();animations={a['index']:a for a in d['source_metadata']['animations']}
 for a in animations.values():
  p=int(a['file_offset'],16);assert cache[p:p+0x6c].hex()==a['raw6c_hex']
 for c in d['cases']:
  name=c['case'];calls=d['calls'][slice(*c['ranges']['calls'])];assert all(r['probe']==name for r in calls)
  assert not c['unreturned_original_calls'] and all(r['completion']=='complete_original_return'for r in c['stages'])
  assert not any(e.get('service')for stage in c['stages']for e in stage['services'])
  def call(e):return next(r for r in calls if r['entry']==e)
  submitted=call('000852a0')['packets'][1]['raw_hex'];axis=c['declared_command_axis_write'];assert submitted[0xc*2:0x14*2]==axis['after_hex']
  heading=c['declared_authored_heading_delivery']
  if heading:
   p=int(heading['file_offset'],16);assert cache[p:p+0x34].hex()==heading['raw34_hex']
   assert heading['after_command_heading_bits']==f'{int.from_bytes(cache[p+12:p+16],"little"):08x}'
   assert submitted[8:16]==cache[p+12:p+16].hex()
  admitted=call('00151a20');assert int(admitted['return_cpu']['registers']['eax'],16)&255==1
  cp=call('000c8140');after=bytes.fromhex(unit(d,cp['after_state'])['body_hex']);control=bytes.fromhex(cp['packets'][0]['raw_hex'])
  assert after[0x1b0:0x1bc]==control[0x14:0x20]
  classifier=call('001f52f0');request=bytes.fromhex(classifier['packets'][0]['raw_hex']);result=bytes.fromhex(classifier['after_packets'][1]['raw_hex'])
  assert bytes.fromhex(unit(d,classifier['before_state'])['body_hex'])[0x1b0:0x1bc]==request[0xdc:0xe8]
  mode=int.from_bytes(result[4:8],'little');activity=call('000e44c0');assert int(activity['stack24_words'][0],16)==mode
  animation=call('00112e10');selection=selected(unit(d,animation['after_state']));selection['authored_name']=animations[selection['clip']]['name']
  pose=next(r for r in calls if r['entry']=='000bdf80'and r['stack24_words'][0]=='e26f0000');before,after=unit(d,pose['before_state']),unit(d,pose['after_state']);changed={}
  for a,b in zip(before['node_blocks'],after['node_blocks']):
   if a['field']not in('110','114'):continue
   stride=32 if a['field']=='110'else 52;v,w=bytes.fromhex(a['raw_hex']),bytes.fromhex(b['raw_hex'])
   changed[a['field']]=[i//stride for i in range(0,len(v),stride)if v[i:i+stride]!=w[i:i+stride]]
  locals_=call('00113830');assert locals_['stack24_words'][2]=='00000015'
  actual.append(dict(case=name,submitted_command5c_hex=submitted,delivered_command5c_hex=call('00151c90')['packets'][0]['raw_hex'],
   C8140_motion12_hex=control[0x14:0x20].hex(),classification_request_motion12_hex=request[0xdc:0xe8].hex(),source_classification=mode,
   E1800_request12_hex=call('000e1800')['after_packets'][0]['raw_hex'],animation_request12_hex=animation['packets'][0]['raw_hex'],
   selected=selection,changed_local_nodes=changed['110'],changed_current_nodes=changed['114'],sampled_local_prefix_count=21))
 assert len(actual)==15
 result=dict(schema=1,report_sha256=digest(out/'report.json'),auditor_sha256=digest(__file__),prior_pins=len(FROZEN),
  prior_runtime_byte_identical=True,cases=actual,calls=len(d['calls']),states=len(d['states']),reads=len(d['reads']),ordered_stores=len(d['stores']),
  arithmetic_and_branch_cpu_points=len(d['points']),complete_buffer_intervals=intervals,packet_intervals=packets,replayed_observations=matches,
  instruction_files=len(d['instructions']),original_instruction_rows=rows,original_instruction_bytes_verified=True,
  natural_device_or_full_frame_admission=False,original1_5_dynamic_execution=False,new_pose_class_contact_clock_admission_or_success_inputs=False)
 frozen_write(out/'audit.json',encoded(result));print(json.dumps({k:v for k,v in result.items()if k!='cases'}))
if __name__=='__main__':main()
