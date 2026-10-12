#!/usr/bin/env python3
"""Independent original-byte, complete stored-state and frame audit."""
import argparse,collections,hashlib,json,struct
from pathlib import Path
from oracle_mp_item_tick_owner_1_1 import ROOT,OUT,FROZEN,digest,encoded,frozen_write
from oracle_mp_item_contact_producers_audit_1_1 import buffers as item_buffers
from xbe import Xbe

def buffers(s):
 result=item_buffers(s['item'])
 def add(p,h,name):
  if p and h:result.append((int(p,16),bytes.fromhex(h),name))
 for name in ('clock','camera','game','simulation','watcher'):
  q=s[name];add(q['pointer'],q['raw_hex'],name)
 for name in ('global_slots_hex','original_globals','kernel_time_slots'):
  for p,h in s[name].items():add(p,h,name+'.'+p)
 for name,key in (('arena_owner','raw10_hex'),('simulation_flags','raw18_hex')):
  q=s[name];add(q['address'],q[key],name)
 return result

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output-dir',type=Path,default=OUT);out=ap.parse_args().output_dir
 d=json.loads((out/'report.json').read_text());assert digest(ROOT/'tools/oracle_mp_item_tick_owner_1_1.py')==d['oracle_sha256']
 for p,h in d['source_pins'].items():assert digest(ROOT/p)==h,p
 assert d['source_pins']==FROZEN and not d['new_guest_inputs']and not d['new_supplied_service_results']and not d['instruction_patches']
 x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');sites={};rows=0
 for r in d['instructions']:
  p=out/r['file'];assert digest(p)==r['sha256'];lines=p.read_text().splitlines();assert len(lines)==r['rows'];rows+=len(lines)
  for line in lines:
   a,b,*_=line.split();raw=bytes.fromhex(b);a=int(a,16);assert x.read(a,len(raw))==raw;sites[a]=raw
 replay_buffers=0;replay_events=0;read_counts=collections.Counter();store_counts=collections.Counter();call_counts=collections.Counter();frames=[]
 for case in d['cases']:
  for correlation in case['prior_runtime_correlations'].values():
   p=ROOT/correlation['report'];prior=json.loads(p.read_text());assert digest(p)==correlation['report_sha256']
   assert correlation['byte_identical']and hashlib.sha256(encoded({k:prior[k]for k in correlation['keys']})).hexdigest()==correlation['runtime_sha256']
  for s in case['states']:
   v={k:q for k,q in s.items()if k not in('id','sha256')};assert hashlib.sha256(json.dumps(v,sort_keys=True,separators=(',',':')).encode()).hexdigest()==s['sha256']
   for name in ('clock','camera','game','simulation','watcher'):
    q=s[name]
    if q['raw_hex']is not None:assert len(bytes.fromhex(q['raw_hex']))==q['size']
  for r in case['reads']:
   assert int(r['instruction'],16)in sites and len(bytes.fromhex(r['raw_hex']))==r['size'];read_counts[r['instruction']]+=1
  for r in case['stores']:
   assert int(r['instruction'],16)in sites and len(bytes.fromhex(r['written_hex']))==r['size']==len(bytes.fromhex(r['before_hex']));store_counts[r['instruction']]+=1
  for r in case['calls']:
   call_counts[r['entry']]+=1;assert len(r['stack16_words'])==16
   assert len(r['cpu']['fp80'])==8 and len(r['cpu']['xmm128'])==8
   if 'return_order'in r:assert r['return_order']>r['order']
   if r['entry']=='00326af0':assert r['return_esp_delta']==-0x4044
   if 'frame_after_hex'in r:
    before=bytearray.fromhex(r['frame_before_hex']);after=bytes.fromhex(r['frame_after_hex']);base=int(r['frame_pointer'],16)
    assert len(before)==len(after)==0x4048
    for e in sorted(case['stores'],key=lambda q:q['order']):
     if r['order']<e['order']<r['return_order']:
      a=int(e['address'],16);n=e['size']
      if base<=a and a+n<=base+len(before):
       off=a-base;assert before[off:off+n].hex()==e['before_hex'];before[off:off+n]=bytes.fromhex(e['written_hex'])
    assert bytes(before)==after and after[4]==0 and struct.unpack_from('<I',after,8)[0]==0
    frames.append(dict(case=case['name'],frame_pointer=r['frame_pointer'],active_byte=after[4],command_mask=struct.unpack_from('<I',after,8)[0],sha256=hashlib.sha256(after).hexdigest()))
  for r in case['stages']:
   assert not r['arguments']and not r['input_registers']and not r['services']
   before=case['states'][r['before_state']];after=case['states'][r['after_state']]
   start=buffers(before);end={(p,name):v for p,v,name in buffers(after)}
   events=[(e['order'],'store',e)for e in case['stores']if e['phase']==r['name']]
   # Read records have no duplicated phase: use the owning root's precise
   # global order interval, including the measured stop's incomplete calls.
   root=next(c for c in case['calls']if c['phase']==r['name']and c['entry']==r['entry']and c['caller']=='03000000')
   next_roots=[c['order']for c in case['calls']if c['caller']=='03000000'and c['phase']!=r['name']and c['order']>root['order']]
   hi=root.get('return_order',min(next_roots)if next_roots else 1<<60)
   events += [(e['order'],'read',e)for e in case['reads']if root['order']<e['order']<hi]
   events.sort()
   for p,raw,name in start:
    if (p,name)not in end:continue
    value=bytearray(raw)
    for _,kind,e in events:
     a=int(e['address'],16);n=e['size']
     if p<=a and a+n<=p+len(value):
      off=a-p;assert value[off:off+n].hex()==e['raw_hex'if kind=='read'else'before_hex'],(case['name'],r['name'],name,e)
      if kind=='store':value[off:off+n]=bytes.fromhex(e['written_hex'])
      replay_events+=1
    assert bytes(value)==end[(p,name)],(case['name'],r['name'],name);replay_buffers+=1
   assert int.from_bytes(bytes.fromhex(after['clock']['raw_hex'])[8:12],'little')==0
   # No admitted world-item phase may be dispatched after replacing the
   # separately declared prior fixture clock with the actual zero-rate reset.
  assert not any(c['entry']in('000b7e00','001c7660','000b7f50','000b8040','000f9b10','000f9ea0')for c in case['calls'])
  constructor=case['stages'][0];before=case['states'][constructor['before_state']];after=case['states'][constructor['after_state']]
  owner=bytes.fromhex(before['arena_owner']['raw10_hex']);base,cursor=struct.unpack_from('<II',owner)
  assert int(after['clock']['pointer'],16)==base+cursor
  assert struct.unpack_from('<I',bytes.fromhex(after['arena_owner']['raw10_hex']),4)[0]==cursor+0x24
  assert bytes.fromhex(after['clock']['raw_hex'])==bytes(0x24)
  reset=case['states'][case['stages'][1]['after_state']];clock=bytes.fromhex(reset['clock']['raw_hex']);game=bytes.fromhex(reset['game']['raw_hex'])
  assert clock[0]==1 and clock[2:4]==game[14:16]==b'\0\0'and clock[4:8]==bytes.fromhex('0000807f')and clock[8:12]==bytes(4)and clock[12:16]==bytes.fromhex('0000803f')
  assert int('00139868',16)not in set(int(a,16)for a in case['executed_in_extension'])
  if case['name']=='missing_watcher_whole_tick_stop':
   r=case['stages'][-1];assert r['stopped_eip']=='00068570'and r['invalid']['address']=='00000084'
  else:
   r=case['stages'][-1];assert r['stopped_eip']=='00337a45'and r['invalid']['kernel_ordinal']==156 and r['invalid']['instruction_unexecuted']
   q=r['invalid']['actual_backing_cache'];assert r['invalid']['aliased_cache4_hex']==q['raw_hex']=='fdffbf2f'
   cache=(ROOT/'assets/disc/maps/lockout.map').read_bytes();off=int(q['file_offset'],16);assert off==0x1bd389c and cache[off:off+4].hex()==q['raw_hex']
   assert '00337a45'not in case['executed_in_extension']and not any(c['entry']=='00336983'for c in case['calls'])
   whole=next(r for r in case['stages']if r['entry']=='00139750');assert whole['completion']=='complete_original_return'
   assert whole['after_cpu']['fpcw']=='023f'
 result=dict(schema=1,report_sha256=digest(out/'report.json'),auditor_sha256=digest(__file__),prior_pins=len(FROZEN),prior_runtime_sections_per_case=4,
  cases=len(d['cases']),calls=sum(call_counts.values()),call_counts=dict(sorted(call_counts.items())),states=sum(len(c['states'])for c in d['cases']),
  actual_reads=sum(read_counts.values()),ordered_stores=sum(store_counts.values()),full_buffer_replay_ranges=replay_buffers,full_buffer_replay_events=replay_events,
  full_frame_store_replay=frames,instruction_files=len(d['instructions']),instruction_rows=rows,original_instruction_bytes_verified=True,
  game_tick_advances=0,admitted_item_phases=0,continuous_settling=False,first_missing_watcher_instruction='00068570',owning_newmap_stop='00337a45',
  encoded_kernel_data_import_cache_alias_consumed=False,new_guest_inputs_or_results=False)
 frozen_write(out/'audit.json',encoded(result));print(json.dumps(result))
if __name__=='__main__':main()
