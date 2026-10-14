#!/usr/bin/env python3
"""Record actual plasma-pistol visual references before implementing their owners.

This is authored-byte evidence only. No visible effect, attachment lifetime or
rendered parity follows from a reference existing in the cache.
"""
import argparse,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import Cache,ROOT
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 cache_path=ROOT/'assets/disc/maps/lockout.map';c=Cache(cache_path)
 weapon=next(t for t in c.tags.values() if t['group']=='weap' and t['name'].endswith('\\plasma_pistol'))
 referenced={};barrels=[]
 def tag(datum):
  if datum==0xffffffff:return None
  t=c.tags[datum];referenced[t['datum']]=dict(t,root_bytes_sha256=hashlib.sha256(c.data[t['offset']:t['offset']+t['size']]).hexdigest());return t
 for b in c.block(weapon['offset']+0x2d0,0xec):
  projectile=tag(c.u(b+0x90));q=projectile['offset'];attachments=[]
  for at in c.block(q+0x94,0x18):
   definition=tag(c.u(at+4));item=dict(raw_hex=bytes(c.data[at:at+0x18]).hex(),definition=definition)
   if definition and definition['group']=='MGS2':
    item['volumes']=[dict(flags=f'{c.u(v):08x}',sprite_count=c.u(v+12),runtime_flags=f'{c.u(v+0x5c):08x}',
      raw_hex=bytes(c.data[v:v+0x98]).hex()) for v in c.block(definition['offset']+8,0x98)]
   attachments.append(item)
  effects=[]
  for e in c.block(b+0xe4,0x34):
   effects.append(dict(raw_hex=bytes(c.data[e:e+0x34]).hex(),normal=tag(c.u(e+8)),misfire=tag(c.u(e+16)),empty=tag(c.u(e+24))))
  barrels.append(dict(raw_hex=bytes(c.data[b:b+0xec]).hex(),projectile=projectile,projectile_model=tag(c.u(q+0x38)),attachments=attachments,effects=effects))
 triggers=[dict(raw_hex=bytes(c.data[t:t+0x40]).hex(),charging_effect=tag(c.u(t+0x34)),discharging_effect=tag(c.u(t+0x3c))) for t in c.block(weapon['offset']+0x2c8,0x40)]
 d=dict(scope=__doc__,weapon=weapon,triggers=triggers,barrels=barrels,referenced_tags=referenced,
  cache_path=str(cache_path),cache_sha256=hashlib.sha256(c.data).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
 a.output.write_text(json.dumps(d,indent=2,sort_keys=True)+'\n');print('barrels',len(barrels),'referenced tags',len(referenced),'authored identity only',flush=True)
if __name__=='__main__':main()
