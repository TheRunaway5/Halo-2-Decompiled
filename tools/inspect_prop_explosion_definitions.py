#!/usr/bin/env python3
"""Read actual Lockout explosion records; no runtime state or result is supplied."""
import argparse,hashlib,json,struct
from pathlib import Path
from inspect_multiplayer import Cache,ROOT

def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);args=ap.parse_args()
 path=ROOT/'assets/disc/maps/lockout.map';c=Cache(path)
 def record(p,n):return dict(file_offset=p,source_address=f'{p+c.magic:08x}',raw_hex=c.data[p:p+n].hex())
 def bits(p):return f'{c.u(p):08x}'
 def tag(d):return c.reference(d)
 effect=0xe70a0595;root=c.tags[effect]['offset'];events=[]
 for ei,e in enumerate(c.block(root+0x14,0x38)):
  systems=[]
  for si,s in enumerate(c.block(e+0x30,0x38)):
   emitters=[]
   for i,p in enumerate(c.block(s+0x30,0xb8)):
    movement=c.u(p+4);physics=None
    if movement!=0xffffffff:
     m=c.tags[movement]['offset'];controllers=[]
     for ci,q in enumerate(c.block(m+0xc,0x14)):
      params=[]
      for pi,v in enumerate(c.block(q+4,0x14)):
       size=c.u(v+0xc);address=c.u(v+0x10)
       params.append(dict(index=pi,record=record(v,0x14),function_hex=c.data[address-c.magic:address-c.magic+size].hex() if size else ''))
      controllers.append(dict(index=ci,record=record(q,0x14),type=c.h(q),parameters=params))
     physics=dict(tag=tag(movement),root=record(m,0x14),flags08=bits(m+8),controllers=controllers)
    emitters.append(dict(index=i,record=record(p,0xb8),shape=c.h(p+0x78),movement=physics))
   systems.append(dict(index=si,record=record(s,0x38),particle=tag(c.u(s+4)),coordinates=c.h(s+0xc),emitters=emitters))
  parts=[dict(index=i,record=record(p,0x38),type=tag(c.u(p+0x10)))for i,p in enumerate(c.block(e+0x18,0x38))]
  events.append(dict(index=ei,record=record(e,0x38),delay_bits=[bits(e+8),bits(e+12)],duration_bits=[bits(e+16),bits(e+20)],parts=parts,systems=systems))
 report=dict(schema=1,cache_sha256=digest(path),tool_sha256=digest(Path(__file__)),effect=tag(effect),root=record(root,0x30),
  locations=[dict(index=i,record=record(p,4),marker=bits(p))for i,p in enumerate(c.block(root+0xc,4))],events=events,
  boundaries=['Read-only actual authored records. Assembly layouts guide navigation, not controller semantics.',
   'No event clock, movement, particle birth, contact or visible draw is executed by this inventory.'])
 raw=(json.dumps(report,indent=2,sort_keys=True)+'\n').encode();args.output.parent.mkdir(parents=True,exist_ok=True)
 if args.output.exists():assert args.output.read_bytes()==raw
 else:args.output.write_bytes(raw)
 print(hashlib.sha256(raw).hexdigest())
if __name__=='__main__':main()
