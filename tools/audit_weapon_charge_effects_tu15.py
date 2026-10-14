#!/usr/bin/env python3
"""Inspect actual charge-effect graphs across the complete authored weapon scope.

References, graph bytes, part/system counts and authored clocks are metadata.
The complete original1030F0 idle-trigger dispatch can reach108DB0 only from
behavior3 or behavior2 with charging/discharging type1. This static route
predicate does not prove input or barrel admission, allocation, visibility,
damage/player feedback, event lifetime or presentation for a weapon/map.
"""
import argparse,hashlib,json,struct
from collections import Counter
from pathlib import Path
from audit_rendering import Cache,file_sha
ROOT=Path(__file__).resolve().parents[1]
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--scope',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--trigger-correspondence',type=Path,required=True);a=ap.parse_args();assert not a.output.exists();scope=json.loads(a.scope.read_text());references=[];graphs={}
 correspondence=json.loads(a.trigger_correspondence.read_text());owners={x['old']:x for x in correspondence['owners']}
 update,start=owners['001021d0'],owners['00107e90'];assert update['u5']=='001030f0' and start['u5']=='00108db0'
 assert update['normalized_complete_body_and_internal_branch_equal'] and start['normalized_complete_body_and_internal_branch_equal']
 calls=[c for c in update['calls'] if c['old_target']=='00107e90'];assert len(calls)==4 and all(c['new_target']=='00108db0' for c in calls)
 for map_ in scope['maps']:
  path=Path(map_['path']);assert file_sha(path)==map_['sha256']
  with_cache=Cache(path)
  try:
   for weapon in map_['weapons']:
    if not weapon['resident']:continue
    for index,trigger in enumerate(weapon['triggers']):
     raw=bytes.fromhex(trigger['raw_hex']);assert len(raw)==0x40
     at=trigger['file_offset'];with_cache.require(at,0x40);assert raw==bytes(with_cache.data[at:at+0x40])
     datum=struct.unpack_from('<I',raw,0x34)[0]
     if datum==0xffffffff:continue
     tag=with_cache.tags[datum];root=tag['address']-with_cache.magic;with_cache.require(root,tag['size']);body=bytes(with_cache.data[root:root+tag['size']]);key=tag['name']+'|'+hashlib.sha256(body).hexdigest()
     behavior=struct.unpack_from('<h',raw,6)[0];charging,discharging=struct.unpack_from('<2h',raw,0x18);assert behavior==trigger['behavior']
     route=behavior==3 or (behavior==2 and (charging==1 or discharging==1))
     row=dict(map=str(path),weapon=weapon['tag']['name'],weapon_datum=weapon['tag']['datum'],trigger=index,trigger_raw_hex=raw.hex(),trigger_behavior=behavior,charging_type=charging,discharging_type=discharging,idle_trigger_has_start_charge_route=route,definition=tag,root_bytes_sha256=hashlib.sha256(body).hexdigest())
     if tag['group']=='effe':
      events=[]
      for e in with_cache.block(root+0x14,0x38):
       parts=[]
       for part in with_cache.block(e+0x18,0x38):
        reference=with_cache.tags.get(with_cache.u32(part+0x10));parts.append(dict(raw_hex=bytes(with_cache.data[part:part+0x38]).hex(),reference=reference))
       events.append(dict(raw_hex=bytes(with_cache.data[e:e+0x38]).hex(),clocks=struct.unpack_from('<5f',with_cache.data,e+4),parts=parts,particle_systems=len(with_cache.block(e+0x30,0x38)),beams=with_cache.u32(e+0x20),accelerations=with_cache.u32(e+0x28)))
      row.update(flags=with_cache.u32(root),loop_start=struct.unpack_from('<h',with_cache.data,root+4)[0],events=events)
     references.append(row);graphs.setdefault(key,[]).append(len(references)-1)
  finally:with_cache.close()
 counts=Counter((r['definition']['name'],r['trigger_behavior'],r['idle_trigger_has_start_charge_route']) for r in references)
 report=dict(scope=__doc__,source_scope_sha256=file_sha(a.scope),tool_sha256=file_sha(Path(__file__)),trigger_route_evidence=dict(correspondence=str(a.trigger_correspondence.resolve()),sha256=file_sha(a.trigger_correspondence),update=update['u5'],start_charge=start['u5'],call_sites=calls),maps_verified=len(scope['maps']),references=references,graph_groups=graphs,unique_graph_name_and_raw_bytes=len(graphs),route_summary=[dict(effect=name,behavior=behavior,idle_trigger_has_start_charge_route=route,references=count) for (name,behavior,route),count in sorted(counts.items())],references_with_idle_charge_route=sum(r['idle_trigger_has_start_charge_route'] for r in references),runtime_verified=False)
 a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print('charge references',len(references),'distinct raw graphs',len(graphs),'maps',len(scope['maps']))
if __name__=='__main__':main()
