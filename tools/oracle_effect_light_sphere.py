#!/usr/bin/env python3
"""Execute unchanged C1930/C1A80 shape and C28B0/C2C50 spatial sphere owners.

Original retained light/tag/scenario banks supply every external read. Explicit
sphere definition radii/size bounds, position/orientation and tick size vary.
These are whole original calls without interception; graphics are not run.
"""
import argparse,hashlib,json,random,struct
from pathlib import Path
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot,u

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 s=Snapshot(a.snapshot);sha=hashlib.sha256(s.ram).hexdigest();assert sha=='68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'
 datum=0xe9810015;pool=u(s.read(0x4e030c,4));light=u(s.read(pool+0x44,4))+(datum&65535)*0x110
 tag=u(s.read(light+4,4));table=u(s.read(0x4e3b44,4));definition=u(s.read(table+(tag&65535)*16+8,4));raw=s.read(definition,228)
 rng=random.Random(0xc1a80);rows=[]
 for n in range(384):
  g=s.guest();g.mem_map(0x70000000,0x10000);stack,stop,shape=0x7000e000,0x7000ff00,0x70001000
  for at,size in ((light,0x110),(definition,228)):
   for page in range(at&~4095,(at+size+4095)&~4095,4096):
    g.mem_map(page,4096);g.mem_write(page,s.read(page,4096))
  d=bytearray(raw);flags=(u(d)&~2)|(2 if n%2 else 0);radii=[rng.uniform(0,16),rng.uniform(0,16)];maximum=rng.choice([0.,.25,1.,4.]);minimum=rng.choice([0.,.125,1.,32.]);size=rng.choice([0.,.125,.371,1.,2.,7.293])
  position=[rng.uniform(-32,32)for _ in range(3)]
  forward,up=((1.,0.,0.),(0.,0.,1.))if n<24 else (tuple(rng.uniform(-1,1)for _ in range(3)),tuple(rng.uniform(-1,1)for _ in range(3)))
  struct.pack_into('<I',d,0,flags);struct.pack_into('<f',d,12,maximum);struct.pack_into('<2f',d,24,*radii);struct.pack_into('<f',d,0x98,minimum);g.mem_write(definition,bytes(d))
  g.mem_write(light+0x84,struct.pack('<3f',*position));g.mem_write(light+0xac,struct.pack('<6f',*forward,*up));g.mem_write(light+0xc4,struct.pack('<f',size));g.mem_write(light+0x9c,struct.pack('<f',0.))
  def run(entry,regs):
   g.mem_write(stack,struct.pack('<II',stop,definition));g.reg_write(R.UC_X86_REG_ESP,stack);g.reg_write(R.UC_X86_REG_FPCW,0x023f);g.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
   for register,value in regs.items():g.reg_write(register,value)
   g.emu_start(entry,stop,count=1000000);assert g.reg_read(R.UC_X86_REG_EIP)==stop
  run(0xc1930,{R.UC_X86_REG_EAX:definition,R.UC_X86_REG_EDX:shape})
  run(0xc1a80,{R.UC_X86_REG_ECX:light+0x84,R.UC_X86_REG_EDX:shape})
  completed=bytes(g.mem_read(shape,124))
  run(0xc28b0,{R.UC_X86_REG_EAX:datum})
  bound=bytes(g.mem_read(light+0x18,32))
  row=dict(definition=bytes(d).hex(),input=struct.pack('<10f',*position,*forward,*up,size).hex(),shape=completed.hex(),spatial=bound.hex());rows.append(row)
 report=dict(boundary=__doc__,snapshot_sha256=sha,entry=['000c1930','000c1a80','000c28b0'],intercepted_instructions=0,cases=rows,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');data=bytearray(struct.pack('<II',0x31534548,len(rows)))
 for row in rows:
  for key in ('definition','input','shape','spatial'):data+=bytes.fromhex(row[key])
 (a.output/'fixture.bin').write_bytes(data);print('PASS',len(rows),'whole original sphere shape/spatial calls')
if __name__=='__main__':main()
