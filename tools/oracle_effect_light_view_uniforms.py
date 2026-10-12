#!/usr/bin/env python3
"""Original4A1E0 color/fade prefix through the complete497A0 call.

Replay stops at4A3D7 before scissor projection and GPU submission. Shapes are
created by unchangedC1930/C1A80; all color/fade and497A0 instructions run without
interception. This is the binding seam, not whole4A1E0 or rendered parity.
"""
import argparse,hashlib,json,random,struct
from pathlib import Path
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot,u

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 s=Snapshot(a.snapshot);sha=hashlib.sha256(s.ram).hexdigest();assert sha=='68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'
 datum=0xe9810015;pool=u(s.read(0x4e030c,4));light=u(s.read(pool+0x44,4))+(datum&65535)*0x110
 tag=u(s.read(light+4,4));table=u(s.read(0x4e3b44,4));definition=u(s.read(table+(tag&65535)*16+8,4));raw=s.read(definition,228);previous=s.read(0x48597c,144)
 rng=random.Random(0x4a1e0);rows=[]
 for n in range(512):
  g=s.guest();g.mem_map(0x70000000,0x10000);stack,stop,shape=0x7000e000,0x7000ff00,0x70001000
  for page in range(light&~4095,(light+0x110+4095)&~4095,4096):g.mem_map(page,4096);g.mem_write(page,s.read(page,4096))
  position=[rng.uniform(-32,32)for _ in range(3)];forward=(1.,0.,0.);up=(0.,0.,1.);size=rng.choice([.125,.371,1.,2.,7.293]);colors=[rng.uniform(0,1)for _ in range(6)];brightness=rng.choice([0.,.5,1.,2.,4.]);fades=[rng.choice([0.,.125,.371,.5,1.])for _ in range(5)];selected=rng.choice([0.,.125,.5,1.])
  inputs=struct.pack('<23f',*position,*forward,*up,size,*colors,brightness,*fades,selected)
  g.mem_write(light+0x84,inputs[:12]);g.mem_write(light+0xac,inputs[12:36]);g.mem_write(light+0xc4,inputs[36:40]);g.mem_write(light+0xd4,inputs[40:68]);g.mem_write(light+0xf8,struct.pack('<5f',fades[0],fades[1],fades[3],fades[2],fades[4]))
  def initialize(regs,args):
   g.mem_write(stack,struct.pack('<'+'I'*len(args),*args));g.reg_write(R.UC_X86_REG_ESP,stack);g.reg_write(R.UC_X86_REG_FPCW,0x023f);g.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
   for reg,val in regs.items():g.reg_write(reg,val)
  initialize({R.UC_X86_REG_EAX:definition,R.UC_X86_REG_EDX:shape},[stop]);g.emu_start(0xc1930,stop,count=1000000);assert g.reg_read(R.UC_X86_REG_EIP)==stop
  initialize({R.UC_X86_REG_ECX:light+0x84,R.UC_X86_REG_EDX:shape},[stop,definition]);g.emu_start(0xc1a80,stop,count=1000000);assert g.reg_read(R.UC_X86_REG_EIP)==stop
  initialize({R.UC_X86_REG_ECX:shape,R.UC_X86_REG_EAX:light+0x84,R.UC_X86_REG_EDX:datum},[stop,definition,0,0,struct.unpack('<I',struct.pack('<f',selected))[0]])
  g.emu_start(0x4a1e0,0x4a3d7,count=1000000);assert g.reg_read(R.UC_X86_REG_EIP)==0x4a3d7
  rows.append(dict(input=inputs.hex(),output=bytes(g.mem_read(0x48597c,144)).hex(),shift=u(g.mem_read(0x485a2c,4)),basis_a=bytes(g.mem_read(0x485a0c,16)).hex(),basis_b=bytes(g.mem_read(0x485a1c,16)).hex()))
 report=dict(boundary=__doc__,snapshot_sha256=sha,start='0004a1e0',stop='0004a3d7',definition=raw.hex(),previous=previous.hex(),intercepted_instructions=0,cases=rows,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');data=bytearray(struct.pack('<II',0x31564548,len(rows))+raw+previous)
 for row in rows:data+=bytes.fromhex(row['input'])+bytes.fromhex(row['output'])+bytes.fromhex(row['basis_a'])+bytes.fromhex(row['basis_b'])+struct.pack('<I',row['shift'])
 (a.output/'fixture.bin').write_bytes(data);print('PASS',len(rows),'original4A1E0 binding-prefix/497A0 calls')
if __name__=='__main__':main()
