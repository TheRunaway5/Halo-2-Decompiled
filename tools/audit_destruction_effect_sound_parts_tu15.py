#!/usr/bin/env python3
"""Read actual MCC effe parts referencing the vehicle destruction sound inventory.

This is a literal metadata census. A matching sound reference does not prove
that the effect is spawned by a particular destruction event or heard in game.
Particle, material, loop and other sound dispatchers remain separate owners.
"""
import argparse,hashlib,json,os,struct
from pathlib import Path
from mcc_content import MccCache


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--inventory',type=Path,required=True);p.add_argument('--maps',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
 if a.output.exists():p.error('Preserve prior metadata census')
 inventory=json.loads(a.inventory.read_text());assert inventory['passed']and inventory['vehicle_paths']==37
 sounds={s for owner in inventory['owners']for s in owner['sounds']};names=['shared.map','single_player_shared.map']+[Path(m['mcc_path']).name for m in inventory['maps']]
 cases={};inputs=[]
 for name in dict.fromkeys(names):
  path=a.maps/name;cache=MccCache(path)
  try:
   inputs.append(dict(map=name,sha256=hashlib.file_digest(path.open('rb'),'sha256').hexdigest()))
   for tag in cache.tags.values():
    if tag['group']!='effe'or tag['offset']is None or tag['size']<0x1c:continue
    root=tag['offset'];locations=cache.u32(root+12)
    for event,at in enumerate(cache.block(root+0x14,0x38)):
     for part,row in enumerate(cache.block(at+0x18,0x38)):
      if cache.u32(row+8)!=0x736e6421:continue
      ref=cache.u32(row+0x10);sound=cache.tags.get(ref)
      if not sound or sound['group']!='snd!'or sound['name']not in sounds:continue
      raw=bytes(cache.data[row:row+0x38]);location=struct.unpack_from('<h',raw,4)[0]
      key=(tag['name'],event,part,locations,raw.hex())
      if key not in cases:cases[key]=dict(effect=tag['name'],event=event,part=part,sound=sound['name'],location_count=locations,location=location,
        environment=cache.u16(row),selector=cache.u16(row+2),part_flags=cache.u16(row+6),part_hex=raw.hex(),maps=[])
      cases[key]['maps'].append(name)
      assert len(cases)<=1024,'Bounded part census exceeded'
  finally:cache.close()
 rows=list(cases.values());conditional=[r for r in rows if r['selector']in(1,2)];invalid=[r for r in rows if r['location']<0 or r['location']>=r['location_count']]
 out=dict(passed=True,scope=__doc__,inventory=str(a.inventory.resolve()),inventory_sha256=hashlib.sha256(a.inventory.read_bytes()).hexdigest(),maps=inputs,
  destruction_sound_inventory_count=len(sounds),matched_sound_names=sorted({r['sound']for r in rows}),cases=rows,conditional_selector_parts=len(conditional),invalid_location_parts=len(invalid),
  actual_effect_flag_or_event_timing_observed=False,audible_parity_verified=False,retained_large_artifacts=[],tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
 raw=json.dumps(out,indent=2)+'\n';assert len(raw)<1024*1024;a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(raw)
 print(json.dumps(dict(passed=True,maps=len(inputs),parts=len(rows),matched_sounds=len(out['matched_sound_names']),conditional_selector_parts=len(conditional),invalid_location_parts=len(invalid))))


if __name__=='__main__':main()
