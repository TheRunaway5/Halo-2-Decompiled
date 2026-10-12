#!/usr/bin/env python3
"""Original-only occupancy check for the synthetic native King arena.

Execute whole initialization/start/containment/player callbacks in unchanged
retail/TU1.1/TU1.5 images. The synthetic rows, pool and unparented position are
declared inputs. No original instruction or service result is replaced.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_king_engine import KingTrace,BASE,K,G,C,R,H,U,UC_X86_REG_EAX,UC_X86_REG_EDI

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 rows=[]
 for hill in range(2):
  for x,y in ((-8,-8),(-8,8),(8,8),(8,-8)):
   rows.append(struct.pack('<4f3h10s',x+hill*30,y,0,0,11+hill,0,0,bytes(10)))
  for z in (-2,4):rows.append(struct.pack('<4f3h10s',hill*30,0,z,0,11+hill,0,1,bytes(10)))
 raw=b''.join(rows);versions={}
 for name in BASE:
  t=KingTrace(name);t.setup(raw);t.h(G+0x230,60);t.h(C+2,30)
  t.run('initialize');t.run('start');state=t.raw();observed=[]
  for point in ((0,0,0),(0,.25,0),(-1,.25,0),(1,.25,0),(30,.25,0)):
   t.w(R+0x2c,0x8aaa0000);t.h(R+0x1b8,0);t.w(H+8,U);t.w(U+0x14,-1)
   t.g.mem_write(U+0x64,struct.pack('<3f',*point));t.g.mem_write(G+0xc,b'\x01');t.g.mem_write(G+0x1128,b'\0')
   contains=t.run('contains',regs=((UC_X86_REG_EAX,0x9abc0000),(UC_X86_REG_EDI,K)))&255
   t.run('player',(0x9abc0000,));inside=struct.unpack('<h',t.g.mem_read(R+0x1b8,2))[0]
   assert t.raw()==state
   observed.append(dict(point=point,contains=contains,inside_ticks=inside))
  versions[name]=dict(image_sha256=t.spec['pin'],state_hex=state.hex(),points=observed,whole_calls=t.steps)
 baseline=versions['retail']
 for version in versions.values():assert version['state_hex']==baseline['state_hex'] and version['points']==baseline['points']
 result=dict(boundary=__doc__,rows_hex=raw.hex(),versions=versions,instruction_patches=[],supplied_returns=[],
  oracle_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
 print(json.dumps(baseline['points']))
if __name__=='__main__':main()
