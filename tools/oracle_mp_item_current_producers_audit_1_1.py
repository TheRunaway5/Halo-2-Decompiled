#!/usr/bin/env python3
"""Independent read-only original-byte/current-node/registry producer audit."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from oracle_mp_item_current_producers_1_1 import ROOT, OUT, digest, encoded, frozen_write, STATE_OWNERS
from xbe import Xbe

def u(raw,off,n=4):
 return int.from_bytes(bytes.fromhex(raw)[off:off+n],'little')

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output-dir',type=Path,default=OUT);args=ap.parse_args();out=args.output_dir
 d=json.loads((out/'report.json').read_text())
 assert digest(ROOT/'tools/oracle_mp_item_current_producers_1_1.py')==d['oracle_sha256']
 for p,h in d['frozen_pins'].items():assert digest(ROOT/p)==h,p
 assert d['instruction_patches']==[]
 image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
 assert digest(image.path)==d['source_sha256']
 listed=set();rows=0;store_sites=set()
 for r in d['instructions']:
  p=out/r['file'];assert digest(p)==r['sha256'];lines=p.read_text().splitlines();assert len(lines)==r['rows']
  for line in lines:
   a,b,_=line.split(' ',2);a=int(a,16);b=bytes.fromhex(b);assert image.read(a,len(b))==b
   listed.add(a)
  rows+=len(lines)
 for m in d['source_metadata']['selected_source_callback_records']:
  a=int(m['address'],16);assert image.read(a,0x24).hex()==m['raw_original_hex']
  assert m['words']==[f'{int.from_bytes(image.read(a+n,4),"little"):08x}'for n in range(0,0x24,4)]
 stages=states=calls=stores=objects=0
 for case in d['cases']:
  for s in case['current_states']:
   raw={k:v for k,v in s.items()if k not in ('id','sha256')}
   assert hashlib.sha256(json.dumps(raw,sort_keys=True,separators=(',',':')).encode()).hexdigest()==s['sha256']
   states+=1
   for i in s['objects']:
    datum=int(i['datum'],16);header=bytes.fromhex(i['header_hex'])
    assert struct.unpack_from('<H',header)[0]==datum>>16
    assert f'{struct.unpack_from("<I",header,8)[0]:08x}'==i['body_address']
    assert struct.unpack_from('<H',header,6)[0]==len(bytes.fromhex(i['body_hex']))==i['body_size']
    if 'definition' in i:
     raw=i['body_hex'];assert f'{u(raw,0):08x}'==i['definition'];assert f'{u(raw,4):08x}'==i['flags']
     assert bytes.fromhex(raw)[0x28:0x30].hex()==i['location_hex']
     assert bytes.fromhex(raw)[0x30:0x60].hex()==i['bounds_hex']
     assert [f'{u(raw,0x64+n*4):08x}'for n in range(3)]==i['root']['bits']
     for node in i['node_blocks']:
      at=int(node['field'],16);assert u(raw,at,2)==node['size'] and u(raw,at+2,2)==node['offset']
      if node['size']:assert bytes.fromhex(raw)[node['offset']:node['offset']+node['size']].hex()==node['raw_hex']
    objects+=1
  for s in case['current_stages']:
   stages+=1
   assert s['completion'] in ('complete_original_return','complete_original_fragment','blocked_original_instruction')
   if s['completion']=='blocked_original_instruction':assert s['invalid']['instruction']==s['stopped_eip']
  for c in case['current_calls']:
   calls+=1
   if int(c['entry'],16) in STATE_OWNERS:
    assert 0<=c['before_state']<len(case['current_states'])
    if 'returned_eax' in c:assert 0<=c['after_state']<len(case['current_states']) and c['return_order']>c['order']
  for r in case['current_stores']:
   a=int(r['instruction'],16);assert a in listed
   assert len(bytes.fromhex(r['before']))==r['size'];assert len(bytes.fromhex(r['value']))==r['size']
   store_sites.add(a);stores+=1
  assert set(case['executed_original_addresses'])<=listed
 tail=next(c for c in d['cases']if c['case']=='tail_without_prior_object_setup')
 whole=next(c for c in d['cases']if c['case']=='whole_object_initializer')
 assert tail['current_stages'][0]['completion']=='complete_original_fragment'
 assert tail['current_stages'][0]['after']['game_arena_cursor']==0x13735c
 assert tail['current_stages'][1]['stopped_eip']=='000d5e98'
 assert [s['completion']for s in whole['current_stages'][:3]]==['complete_original_return']*3
 assert whole['current_stages'][3]['stopped_eip']=='001cd56d'
 mask=next(s for s in whole['current_stages']if s['stage']=='original_active_cluster_masks_with_source_empty_player_pool')
 assert mask['completion']=='complete_original_return'
 assert bytes.fromhex(mask['after']['active_mask_hex'])==bytes(0x40)
 assert bytes.fromhex(mask['after']['visible_mask_hex'])==bytes(0x40)
 expected=[(0,'e26f0000',9,830,36,72),(1,'e2700001',9,830,36,72),(12,'e2710002',15,1260,60,120)]
 for index,datum,cluster,length,loaded,reserve in expected:
  phase=f'original_authored_weapon_row_{index}_allocation'
  stage=next(s for s in whole['current_stages']if s['stage']==phase)
  item=next(i for i in stage['after']['objects']if i['datum']==datum)
  assert stage['completion']=='complete_original_return'
  assert item['row']==index and item['body_size']==length and item['loaded']==loaded and item['reserve']==reserve
  assert item['owner']==item['parent']==item['physical_component']=='ffffffff' and item['heat_bits']=='00000000'
  assert int(item['flags'],16)&0x100 and int(item['header_hex'][4:6],16)==0x4a
  assert u(item['location_hex'],4,2)==cluster
  assert item['node_blocks'][1]['size']==208 and item['node_blocks'][1]['offset']==612
  creation=next(c for c in whole['current_calls']if c['phase']==phase and c['entry']=='000b8a30')
  assert creation['returned_eax']==datum
  assert [f'{u(creation["creation_descriptor_hex"],0x1c+n*4):08x}'for n in range(3)]==item['root']['bits']
  authored=next(r for r in d['source_metadata']['rows']if r['index']==index)
  assert [f'{u(authored["record_hex"],0x40+n*4):08x}'for n in range(3)]==item['root']['bits']
  owner_calls=[c for c in whole['current_calls']if c['phase']==phase]
  assert sum(c['entry']=='000bdf80'for c in owner_calls)==2
  assert sum(c['entry']=='000bede0'for c in owner_calls)==2
  assert sum(c['entry']=='000b94f0'for c in owner_calls)==1
  for c in owner_calls:
   if c['entry']=='000bdf80':
    before=whole['current_states'][c['before_state']]['objects'][-1];after=whole['current_states'][c['after_state']]['objects'][-1]
    assert before['bounds_hex']==after['bounds_hex'] and before['location_hex']==after['location_hex']
   if c['entry']=='000bede0':
    before=whole['current_states'][c['before_state']]['objects'][-1];after=whole['current_states'][c['after_state']]['objects'][-1]
    assert before['node_blocks']==after['node_blocks']
  assert stage['before']['shared_rng']==stage['after']['shared_rng']=='12345678'
 end=whole['current_stages'][-1]['after'];assert len(end['objects'])==3
 assert [x['object']for x in end['registry_chains'][9]['collidable']]==['e2700001','e26f0000']
 assert [x['object']for x in end['registry_chains'][15]['collidable']]==['e2710002']
 phases=('ordinary_object_pass_before_first_postpass','dirty_object_pass_before_first_postpass','new_object_first_normal_item_tick')
 for phase in phases:
  stage=next(s for s in whole['current_stages']if s['stage']==phase)
  assert stage['before']['objects']==stage['after']['objects'] and stage['before']['registry_chains']==stage['after']['registry_chains']
 assert not any(c['entry'] in ('000bd360','000bd710','000f9b10','000f9ea0','0016b240','00246a50')for c in whole['current_calls'])
 supplied=[r for s in whole['current_stages']for r in s['services']if 'service'in r]
 assert {r['service']for r in supplied}=={'000c10e0'}
 for r in whole['resource_delivery']:
  with (ROOT/r['file']).open('rb')as f:f.seek(r['file_offset']);raw=f.read(r['size'])
  assert hashlib.sha256(raw).hexdigest()==r['raw_sha256'] and r['size']==200
 result=dict(schema=1,report_sha256=digest(out/'report.json'),oracle_sha256=d['oracle_sha256'],auditor_sha256=digest(__file__),
  prior_frozen_pins_verified=len(d['frozen_pins']),cases=len(d['cases']),stages=stages,calls=calls,states=states,stores=stores,
  stored_object_states=objects,instruction_files=len(d['instructions']),opcode_rows=rows,observed_store_sites=len(store_sites),
  original_authored_item_allocations=3,original_current_node_banks=3,original_registry_cluster_order_verified=True,
  source_empty_player_activation_masks_zero=True,scheduled_item_motion_or_contact_calls=0,
  production_edits=False,instruction_patches=[])
 frozen_write(out/'audit.json',encoded(result));print(json.dumps(result,sort_keys=True))
if __name__=='__main__':main()
