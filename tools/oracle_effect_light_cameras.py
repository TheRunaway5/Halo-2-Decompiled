#!/usr/bin/env python3
"""Unchanged retailC1D90 six-camera spherical-light branch over original inputs.

Only the declared sphere position/radius vary. The retained shape, frame,
literal tables and every callee instruction remain original. This constructs
secondary cameras; light visibility, selection and geometry admission are
separate owners, and no rendering is executed here.
"""
import argparse,hashlib,json,struct,random
from pathlib import Path
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 s=Snapshot(a.snapshot);assert hashlib.sha256(s.ram).hexdigest()=='68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'
 shape=(a.snapshot/'bindings-entry/shape.bin').read_bytes();frame=(a.snapshot/'bindings-entry/light-frame.bin').read_bytes();rows=[]
 rng=random.Random(0xc1d90)
 positions=[struct.unpack_from('<3f',shape,0x2c),(0.,0.,0.),(-3.25,4.5,10.75),(100.,-200.,300.)]
 positions += [tuple(rng.uniform(-1024,1024) for _ in range(3)) for _ in range(64)]
 for position in positions:
  for radius in (.125,1.,2.,5.,32.,.371,7.293):
   g=s.guest();g.mem_map(0x70000000,0x10000);stack=0x7000e000;stop=0x7000ff00;sh=bytearray(shape);struct.pack_into('<3f',sh,0x2c,*position);struct.pack_into('<f',sh,0x44,radius)
   g.mem_write(0x70001000,frame);g.mem_write(0x70002000,bytes(sh));g.mem_write(stack,struct.pack('<5I',stop,0x70001000,0x70002000,0x70003000,0x70004000));g.reg_write(R.UC_X86_REG_ESP,stack);g.reg_write(R.UC_X86_REG_FPCW,0x023f);g.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
   g.emu_start(0xc1d90,stop,count=2000000);assert g.reg_read(R.UC_X86_REG_EIP)==stop,hex(g.reg_read(R.UC_X86_REG_EIP))
   count=struct.unpack('<h',g.mem_read(0x70004000,2))[0];assert count==6
   rows.append(dict(position=struct.unpack_from('<3f',sh,0x2c),radius=struct.unpack_from('<f',sh,0x44)[0],shape=bytes(sh).hex(),cameras=bytes(g.mem_read(0x70003000,6*0x1bc)).hex()))
 (a.output/'report.json').write_text(json.dumps(dict(boundary=__doc__,entry='000c1d90',snapshot_sha256=hashlib.sha256(s.ram).hexdigest(),intercepted_instructions=0,cases=rows,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()),indent=2)+'\n');print('PASS',len(rows),'whole original six-camera calls')
if __name__=='__main__':main()
