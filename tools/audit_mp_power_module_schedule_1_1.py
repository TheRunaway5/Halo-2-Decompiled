#!/usr/bin/env python3
"""Audit original power-module phases, contact banks and source byte authority."""
import argparse, hashlib, json, struct
from pathlib import Path
from oracle_mp_power_module_schedule_1_1 import ROOT, OUT, FROZEN
from oracle_mp_item_current_producers_1_1 import frozen_write, encoded
from oracle_mp_sandbox_owner_1_1 import digest
from xbe import Xbe

BOOT_MANIFESTS={
 'analysis/multiplayer/integration/inventory-source/smg-effect-dsp-owner/manifest.json':'51d914f8f892ceb538e831e11c6fec3fe68a9bf7998c6a40ec3860b2a9c8c80d',
 'analysis/multiplayer/integration/inventory-source/smg-effect-title-arena/manifest.json':'cf80069fe114cde529132e6a474796f68b0c05aa2d3336ad82b53769376349d8'}

def buffers(s,component_base):
 result=[]
 def add(p,h):
  if p is not None and h is not None:result.append((int(p,16),bytes.fromhex(h)))
 w=s['world'];p=s['player'];c=s['current'];hp=s['header_pool']
 for q in [hp,*s['impact_pools']]:
  add(q['pointer'],q['header4c_hex']);add(q['data_pointer'],q['data_hex']);add(q['bitmap_pointer'],q['bitmap_hex'])
 for a,b in [('object_world','object_world_hex'),('world','world260_hex'),('physics_constants','physics_constants_hex')]:add(w[a],w[b])
 add(w['physics_counter'],int(w['physics_counter_value']).to_bytes(4,'little').hex())
 for a,b in [('globals','globals130_hex'),('engine','engine_c9c_hex'),('unit_queue','unit_queue1d88_hex'),('trigger_bits','trigger24_hex')]:add(p[a],p[b])
 add(p['pool'],p['header_hex'])
 for q in p['records']:add(q['pointer'],q['raw_hex'])
 add(s['clock_pointer'],s['clock0c_hex']);add(c['body_heap'],c['body_heap_header_hex']);add(c['AI_global'],c['AI_global_hex'])
 for o in w['objects']:
  add(o['pointer'],o['raw_hex'])
  if o['component']!='ffffffff':
   # The source1D7EE0 stack packet below supplies this actual pool record
   # address; its low datum index and originalA0 stride identify the base.
   component=int(o['component'],16)&0xffff
   base=component_base+component*0xa0
   add(f'{base:08x}',o['component_hex'])
   aux=o['physical_auxiliary'];add(aux['contact_bank_pointer'],aux['allocated_capacity_hex'])
   add(aux['phantom_pointer'],aux['phantom_header0c_hex'])
  for b in o['bodies']:
   add(b['slot'],b['slot_hex']);add(b['body'],b['body_hex']);add(b['motion'],b.get('motion_consumed_prefix_c0_hex'))
   # The stored transform pointer is read from the genuine body record.
   raw=bytes.fromhex(b['body_hex']);add(f'{struct.unpack_from("<I",raw,0x14)[0]:08x}',b['transform30_hex'])
 for reg in c['cluster_registries'].values():
  add(reg['head'],reg['head_hex'])
  for q in reg['pools']:add(q['pointer'],q['header_hex'])
 for reg in c['registry_chains']:
  for kind in ('collidable','noncollidable'):
   # Registry entries lack a literal backing pointer in this frozen schema;
   # their raw/next order is separately audited below.
   pass
 return result

def replay(r):
 count=0;compared=0
 bounds=next(c for c in r['calls']if c['entry']=='001d7ee0')
 obj=r['states'][bounds['before_state']]['world']['objects'][0]
 component_base=int(bounds['stack'][0],16)-(int(obj['component'],16)&0xffff)*0xa0
 for call in r['calls']:
  if 'before_state'not in call:continue
  before=r['states'][call['before_state']];after=r['states'][call['after_state']]
  memory={}
  for address,raw in buffers(before,component_base):
   for n,v in enumerate(raw):
    if address+n in memory:assert memory[address+n]==v
    memory[address+n]=v
  for st in r['stores']:
   if not call['order']<st['order']<call['return_order']:continue
   a=int(st['address'],16);old=bytes.fromhex(st['before_hex']);new=bytes.fromhex(st['written_hex'])
   for n,v in enumerate(old):
    if a+n in memory:assert memory[a+n]==v,(call['entry'],st)
   for n,v in enumerate(new):memory[a+n]=v
  for address,raw in buffers(after,component_base):
   for n,v in enumerate(raw):
    if address+n in memory:assert memory[address+n]==v,(call['entry'],f'{address+n:08x}',memory[address+n],v)
    compared+=address+n in memory
  count+=1
 return dict(owning_call_buffer_replays=count,known_before_or_written_output_bytes_verified=compared)

def audit(out,repeat):
 path=out/'report.json';r=json.loads(path.read_text());assert digest(path)==digest(repeat/'report.json')
 assert r['oracle_sha256']==digest(ROOT/'tools/oracle_mp_power_module_schedule_1_1.py')
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 assert r['prior_pins']==FROZEN and not r['new_supplied_game_or_Havok_fields'] and not r['instruction_patches']
 assert not r['native_gameplay_update_applied']
 image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');sites=set();rows=0
 for listing in r['instructions']:
  p=out/listing['file'];assert digest(p)==listing['sha256']==digest(repeat/listing['file'])
  lines=p.read_text().splitlines();assert len(lines)==listing['rows']
  for line in lines:
   a,h,*_=line.split();a=int(a,16);raw=bytes.fromhex(h);assert image.read(a,len(raw))==raw;sites.add(a)
  rows+=len(lines)
 assert {int(a,16)for a in r['full_setup_and_schedule_executed_addresses']}<=sites
 stages=r['stages'];assert len(stages)==6 and all(s['completed']and s['invalid']is None and s['stopped_eip']=='03000000'and not s['used_services']for s in stages)
 source_root=bytes.fromhex(r['source_factory']['authored_placement_hex'])[8:20]
 for s in r['states']:
  assert hashlib.sha256(json.dumps({k:v for k,v in s.items()if k not in ('id','sha256')},sort_keys=True,separators=(',',':')).encode()).hexdigest()==s['sha256']
  hp=s['header_pool'];assert hp['capacity']==2048 and hp['stride']==12 and len(bytes.fromhex(hp['data_hex']))==2048*12
  objects={o['datum']:o for o in s['world']['objects']}
  assert objects['e26f0000']['definition']=='e6f80583'
  box=objects['e26f0000'];raw=bytes.fromhex(box['raw_hex']);assert raw[0x64:0x70]==source_root
  assert raw[0xe4:0xf4]==bytes.fromhex('0000f041000000000000803f00000000')
  size,off=struct.unpack_from('<Hh',raw,0x120);assert size==16 and raw[off:off+size]==bytes(16)
  for o in objects.values():
   h=bytes.fromhex(o['header_hex']);b=bytes.fromhex(o['raw_hex']);idx=int(o['datum'],16)&0xffff
   assert bytes.fromhex(hp['data_hex'])[idx*12:idx*12+12]==h and len(b)==struct.unpack_from('<H',h,6)[0]
   assert int(o['pointer'],16)==struct.unpack_from('<I',h,8)[0]
  for q in s['impact_pools']:
   assert q['capacity']==32 and len(bytes.fromhex(q['data_hex']))==32*q['stride']
  for reg in s['current']['registry_chains']:
   for kind in ('collidable','noncollidable'):
    chain=reg[kind]
    for n,c in enumerate(chain):
     assert c['object']in objects
     assert c['next']==(chain[n+1]['link']if n+1<len(chain)else'ffffffff')
     assert struct.unpack_from('<I',bytes.fromhex(c['raw_hex']),4)[0]==int(c['object'],16)
 for c in r['calls']:
  assert 'returned_eax'in c and c['order']<c['return_order']
 for s in r['stores']:
  assert int(s['instruction'],16)in sites and s['size']in(1,2,4,8)
  assert len(bytes.fromhex(s['written_hex']))==len(bytes.fromhex(s['before_hex']))==s['size']
 calls={a:[c for c in r['calls']if c['entry']==a]for a in ['001d3df0','001d2df0','001d2f10','001d3fc0','001d40b0','0011d2b0','000d73e0','001d7ee0']}
 active=bool(r['states'][stages[0]['after_state']]['player']['records'])
 assert len(calls['001d7ee0'])==1 and int(calls['001d7ee0'][0]['returned_eax'],16)&255==1
 assert len(r['actual_broadphase_calls'])==1
 broad=r['actual_broadphase_calls'][0];assert broad['entry']=='002e7610'and broad['owning_indirect_site']=='001d80ef'
 assert broad['max_authored_margin_word']=='00000000'and len(bytes.fromhex(broad['broadphase_output20_after_hex']))==32
 assert all(not[c for c in r['calls']if c['entry']==f'{a:08x}']for a in (0xd9180,0xdc0a0,0xdb710,0xdb470,0xd8280,0x1d5830,0x1d5e90))
 if active:
  assert [len(calls[a])for a in ('001d3df0','001d2df0','001d2f10','001d3fc0','001d40b0','0011d2b0')]==[1,1,2,1,1,1]
  assert calls['0011d2b0'][0]['stack'][0]=='e26f0000'and int(calls['0011d2b0'][0]['returned_eax'],16)&255==0
  for c in calls['001d2f10']:
   reads=c['actual_contact_manager_reads'];assert [x['kind']for x in reads]==['point_distance_normal_packet','source_scalar_word']
   assert reads[1]['raw_hex']=='00000000'
  assert max(s['world']['objects'][0]['physical_auxiliary']['count']for s in r['states'])==1
 else:
  assert all(not calls[a]for a in ('001d3df0','001d2df0','001d2f10','001d3fc0','001d40b0','0011d2b0','000d73e0'))
 result=dict(report_sha256=digest(path),active_player_source_case=active,stages=len(stages),calls=len(r['calls']),
  states=len(r['states']),stores=len(r['stores']),source_instruction_files=len(r['instructions']),source_instruction_rows=rows,
  original_executed_schedule_addresses=len(r['source_executed_schedule_addresses']),
  original_full_setup_executed_addresses=len(r['full_setup_and_schedule_executed_addresses']),
  real_contact_creations=len(calls['001d3df0']),real_contact_refreshes=len(calls['001d2f10']),real_contact_removals=len(calls['001d40b0']),
  genuine_backend_bounds_calls=1,original_game_or_Havok_supplied_returns_in_selected_phases=0,
  destruction_or_projectile_damage_injected=False,**replay(r))
 return result

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'frozen/audit.json');args=ap.parse_args()
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 prior=json.loads((ROOT/'analysis/multiplayer/sandbox-source/owner-creation/frozen/suite.json').read_text())
 for p,h in prior['files'].items():assert digest(ROOT/p)==h,p
 boot_files=0
 for p,h in BOOT_MANIFESTS.items():
  assert digest(ROOT/p)==h,p
  for q,v in json.loads((ROOT/p).read_text())['files'].items():assert digest(ROOT/q)==v,q;boot_files+=1
 results=[audit(OUT/'frozen'/a,OUT/'frozen'/b)for a,b in [('measured','repeat'),('inactive-measured','inactive-repeat')]]
 frozen_write(args.output,encoded(dict(schema=1,auditor_sha256=digest(__file__),prior_pins=len(FROZEN),
  prior_complete_factory_manifest_files=len(prior['files']),preserved_title_and_DSP_manifests=BOOT_MANIFESTS,
  preserved_title_and_DSP_artifact_count=boot_files,cases=results)))
 print(json.dumps(results))

if __name__=='__main__':main()
