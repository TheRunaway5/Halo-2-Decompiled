#!/usr/bin/env python3
"""Audit only actual source melee packets, state, event bytes and owner mapping."""
import argparse,hashlib,json,struct
from pathlib import Path
from inspect_multiplayer import Cache
from oracle_mp_title_update_display import Xbe,OLD,NEW
ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'analysis/multiplayer/integration/inventory-source/melee-channel-owner'
MEASURED=ROOT/'analysis/multiplayer/integration/inventory-source/melee-admission-measured'
MAPPING=ROOT/'analysis/multiplayer/integration/inventory-source/melee-admission-u5'
PINS={SOURCE/'report.json':'de6cb6605b7edf15fc699c17232b56d530a9c1d1c263729e40b68120f0ab31b7',
 MEASURED/'report.json':'4891168c3b4bc47da9f793dde25a07b10b6772871ea74f0e4abcc36a1dfe06bc',
 MAPPING/'report.json':'fa820057bca73cd9b96f944a3bec1c634e1f2c8d382710ac9e43168994d0171e'}
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p,data):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==data,p
 else:p.write_bytes(data)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=SOURCE/'audit.json');a=p.parse_args()
 for path,pin in PINS.items():assert sha(path)==pin,path
 reports={base:json.loads((base/'report.json').read_text())for base in(SOURCE,MEASURED,MAPPING)}
 repeats=[]
 for base,r in reports.items():
  files=[base/'report.json',*(base/'instructions').glob('*.asm')]
  for f in files:assert f.read_bytes()==(base/'reexecution'/f.relative_to(base)).read_bytes(),f
  for row in r.get('listings',[]):assert sha(base/row['file'])==row['sha256']
  repeats.append(dict(directory=str(base.relative_to(ROOT)),files=len(files),byte_identical=True))
 r=reports[SOURCE];mapping=reports[MAPPING]
 for path,pin in r['frozen_pins'].items():assert sha(ROOT/path)==pin,path
 old,new=Xbe(OLD),Xbe(NEW)
 for row in r['listings']:
  for line in (SOURCE/row['file']).read_text().splitlines():
   addr,raw,*_=line.split();assert old.read(int(addr,16),len(raw)//2)==bytes.fromhex(raw)
 for c in r['cases']:assert c['stages'][-1]['completion']=='complete_original_return' and not c['unreturned_original_calls']
 calls=[c for c in r['calls']if c['probe']=='declared_flag20'];roots={e:[c for c in calls if c['entry']==e]for e in('000eb530','000eb080','000eb700','00115650','000eade0','000eb000')}
 for e in roots:assert len(roots[e])==1,e
 for e in('000eb530','000eb080','00115650','000eade0','000eb700'):assert int(roots[e][0]['return_cpu']['registers']['eax'],16)&255==1,e
 assert not any(c['entry']=='000d0830'for c in r['calls'])
 assert not any(v.get('service')=='00115650'for c in r['cases']for s in c['stages']for v in s['services'])
 request=next(c for c in calls if c['entry']=='000e7f30')
 packet=next(p for p in request['packets']if p['field']=='edi_actual_action_request32')['raw_hex']
 assert packet=='1b00000001000000ffffffff0000000000000000000000000000000000000000'
 cache=Cache(ROOT/'assets/disc/maps/lockout.map');assert sha(cache.path)==r['cache_sha256']
 def unit(state):return next(o for o in r['states'][state]['objects']if o['datum']=='e26f0000')
 def body(state):return bytes.fromhex(unit(state)['body_hex'])
 def action(c,after=False):return next(p for p in c['after_packets'if after else'packets']if p['field']=='actual_signed346_action44')
 def channel(state):
  b=body(state);offset=struct.unpack_from('<h',b,0x33e)[0]+0x9c
  return dict(source_unit_offset=f'{offset:04x}',raw32_hex=b[offset:offset+0x20].hex())
 selected=channel(roots['00115650'][0]['after_state']);b=bytes.fromhex(selected['raw32_hex']);graph,index=struct.unpack_from('<I',b)[0],struct.unpack_from('<h',b,6)[0]
 graph_tag=cache.tags[graph];at=cache.block(graph_tag['offset']+0x2c,0x6c)[index]
 sid=cache.u(at);name=cache.text(cache.u(368)+cache.u(cache.u(364)+(sid&0xffff)*4))
 authored=dict(graph=graph_tag,index=index,name=name,file_offset=f'{at:08x}',source_address=f'{at+cache.magic:08x}',raw6c_hex=bytes(cache.data[at:at+0x6c]).hex())
 selected.update(observed_graph_datum=f'{graph:08x}',observed_signed_animation_index=index,authored_record=authored)
 effects=[c['actual_returned_animation_record']for c in calls if c['entry']=='00169f3b'and 'actual_returned_animation_record'in c]
 assert len(effects)==2 and effects[0]==effects[1]
 for q in effects:
  at=int(q['pointer'],16)-cache.magic;assert bytes(cache.data[at:at+0x6c]).hex()==q['raw6c_hex']
  event=int(q['event_pointer'],16)-cache.magic;assert bytes(cache.data[event:event+q['event_count']*4]).hex()==q['event_raw_hex']
 handler_map={c['old_entry']:c['u5_entry']for c in mapping['owners']}
 old_table,new_table=0x46ec68,0x473d28
 descriptors=[]
 for image,table,labels in((old,old_table,['000eb530','000eb700',None,'000eb830']),
  (new,new_table,[handler_map['000eb530'],handler_map['000eb700'],None,handler_map['000eb830']])):
  ptr=struct.unpack('<I',image.read(table+27*4,4))[0];raw=image.read(ptr,16)
  assert struct.unpack('<4I',raw)==tuple(int(e,16)if e else 0 for e in labels)
  descriptors.append(dict(table=f'{table:08x}',type=27,descriptor=f'{ptr:08x}',raw16_hex=raw.hex(),handlers=labels))
 result=dict(schema=1,tool_sha256=sha(__file__),report_pins={str(p.relative_to(ROOT)):h for p,h in PINS.items()},independent_repeats=repeats,
  source_instruction_bytes_checked=True,prior_pins_intact=len(r['frozen_pins']),cases=len(r['cases']),calls=len(r['calls']),states=len(r['states']),
  actual_request32_hex=packet,actual_action_owner={e:dict(call_id=c[0]['id'],original_AL=int(c[0]['return_cpu']['registers']['eax'],16)&255,before_state=c[0]['before_state'],after_state=c[0]['after_state'])for e,c in roots.items()},
  begin_action=dict(before=action(roots['000eb080'][0]),after=action(roots['000eb080'][0],True)),
  first_update=dict(before=action(roots['000eb700'][0]),after=action(roots['000eb700'][0],True)),
  actual_secondary_channel=selected,actual_weapon_animation_record=effects[0],actual_shared_rng_before=r['states'][roots['000eb000'][0]['before_state']]['seed'],actual_shared_rng_after=r['states'][roots['000eb000'][0]['after_state']]['seed'],
  source_descriptor_type27=descriptors,source_u5_owner_map=handler_map,
  impact_execution=False,channel_success_supplied=False,clock_increment_supplied=False,target_contact_pose_supplied=False,
  retained_supplied_outputs=[dict(case=c['case'],stage=s['name'],entry=v['service'],description=v['description'],result=v['return_value'])for c in r['cases']for s in c['stages']for v in s['services']if v.get('supplied')],
  boundary='Original first declared-input melee channel/action/ordinary update only. Source metadata is read at the actual selected graph/index/record pointer; no clip or event is selected by this audit. EFE30 feedback remains explicit. D0830 is never invoked, no target/hit/timing supplied. TU1.5 is complete static correspondence, not dynamic source-owned runtime execution.')
 save(a.output,(json.dumps(result,sort_keys=True,indent=2)+'\n').encode());print(json.dumps(dict(output=str(a.output),sha256=sha(a.output),selected_graph=f'{graph:08x}',animation_index=index,name=name,repeats=repeats)))
if __name__=='__main__':main()
