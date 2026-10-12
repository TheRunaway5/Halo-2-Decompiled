#!/usr/bin/env python3
"""Run unchanged original C12D0/C0A50 over explicit camera/light inputs.

The actual light, tag table, definitions and literal banks come from a saved
retail frame. Only position/radius, camera position and three authored class
inputs vary. All original instructions execute; no callees are intercepted.
This proves CPU view fades, not visibility, selection or rendered appearance.
"""
import argparse,hashlib,json,random,struct
from pathlib import Path
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot,u

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 s=Snapshot(a.snapshot);sha=hashlib.sha256(s.ram).hexdigest();assert sha=='68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'
 datum=0xe9810015;pool=u(s.read(0x4e030c,4));light=u(s.read(pool+0x44,4))+(datum&65535)*0x110
 tag=u(s.read(light+4,4));table=u(s.read(0x4e3b44,4));definition=u(s.read(table+(tag&65535)*16+8,4))
 rng=random.Random(0xc12d0);inputs=[];rows=[]
 position=struct.unpack('<3f',s.read(light+0x28,12))
 for classes in ((3,4,4),(0,0,0),(1,2,3),(2,3,4),(4,4,4)):
  for radius in (.01,.125,.371,1.,2.,7.293,32.):
   for delta in (0.,radius-.001,radius,radius+.001,radius+1,radius+3,radius+5,radius+10,radius+20,radius+30):
    inputs.append((position,radius,(position[0]+delta,position[1],position[2]),classes))
 for _ in range(1000):
  pos=tuple(rng.uniform(-1024,1024) for _ in range(3));radius=rng.uniform(.001,32)
  eye=tuple(v+rng.uniform(-30,30) for v in pos);classes=tuple(rng.randrange(5) for _ in range(3));inputs.append((pos,radius,eye,classes))
 for position,radius,eye,classes in inputs:
  g=s.guest();g.mem_map(0x70000000,0x10000);stack=0x7000e000;stop=0x7000ff00
  for at,n in ((light,0x110),(definition,0xe4),(0x4b9da0,12)):
   for page in range(at&~4095,(at+n+4095)&~4095,4096):
    g.mem_map(page,4096);g.mem_write(page,s.read(page,4096))
  raw=struct.pack('<7f3h',*position,radius,*eye,*classes)
  g.mem_write(light+0x28,raw[:16]);g.mem_write(0x4b9da0,raw[16:28]);g.mem_write(definition+0xb8,raw[28:])
  g.mem_write(stack,struct.pack('<I',stop));g.reg_write(R.UC_X86_REG_ESP,stack);g.reg_write(R.UC_X86_REG_EDX,datum);g.reg_write(R.UC_X86_REG_FPCW,0x023f);g.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
  g.emu_start(0xc12d0,stop,count=100000);assert g.reg_read(R.UC_X86_REG_EIP)==stop
  rows.append(dict(input=raw.hex(),output=bytes(g.mem_read(light+0xf8,20)).hex()))
 report=dict(boundary=__doc__,snapshot_sha256=sha,entry='000c12d0',intercepted_instructions=0,definition=s.read(definition,0xe4).hex(),cases=rows,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 data=bytearray(struct.pack('<II',0x31464548,len(rows)))
 for row in rows:data+=bytes.fromhex(row['input'])+bytes.fromhex(row['output'])
 (a.output/'fixture.bin').write_bytes(data);print('PASS',len(rows),'whole original view-fade calls')
if __name__=='__main__':main()
