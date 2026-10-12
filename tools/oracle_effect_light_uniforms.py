#!/usr/bin/env python3
"""Execute unchanged retail497A0 over retained original grenade light inputs.

The shape, light frame and tag are actual original CPU inputs. This bounded
replay varies the spherical position, radius, brightness and diffuse/specular
inputs, retaining all source instructions and literal data. It does not execute
light admission, draw selection, graphics calls or the GPU.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 s=Snapshot(a.snapshot);assert hashlib.sha256(s.ram).hexdigest()=='68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'
 capture=a.snapshot/'bindings-entry';abi=json.loads((capture/'abi.json').read_text());shape=(capture/'shape.bin').read_bytes();frame=(capture/'light-frame.bin').read_bytes();definition=s.read(abi['stack'][1],228)
 assert struct.unpack_from('<I',shape)[0]==0
 rows=[]
 for position in (struct.unpack_from('<3f',shape,0x2c),(0.,0.,0.),(-3.25,4.5,10.75)):
  for radius in (.125,1.,5.,32.):
   for diffuse,specular in (((1.,.5,.125),(0.,0.,0.)),((.25,.75,1.),(.125,.25,.5)),((-.5,0.,1.5),(1.5,-1.,0.))):
    for brightness in (0.,.5,1.,2.,4.):
     g=s.guest();g.mem_map(0x70000000,0x10000)
     at=dict(stack=0x7000e000,stop=0x7000ff00,shape=0x70001000,frame=0x70002000,definition=0x70003000,diffuse=0x70004000,specular=0x70004100,output=0x70005000,shift=0x70006000,basis_a=0x70006100,basis_b=0x70006200)
     sh=bytearray(shape);struct.pack_into('<3f',sh,0x2c,*position);struct.pack_into('<f',sh,0x44,radius)
     g.mem_write(at['shape'],bytes(sh));g.mem_write(at['frame'],frame);g.mem_write(at['definition'],definition);g.mem_write(at['diffuse'],struct.pack('<3f',*diffuse));g.mem_write(at['specular'],struct.pack('<3f',*specular))
     # C37/38.w and C39 are retained by497A0; initialize to the captured bank.
     initial=s.read(0x48597c,9*16);g.mem_write(at['output'],initial)
     args=[at['stop'],at['frame'],at['definition'],at['diffuse'],at['specular'],struct.unpack('<I',struct.pack('<f',brightness))[0],at['shift'],at['basis_a'],at['basis_b']]
     g.mem_write(at['stack'],struct.pack('<9I',*args));g.reg_write(R.UC_X86_REG_ESP,at['stack']);g.reg_write(R.UC_X86_REG_ECX,at['shape']);g.reg_write(R.UC_X86_REG_EAX,at['output']);g.reg_write(R.UC_X86_REG_FPCW,0x023f);g.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
     g.emu_start(0x497a0,at['stop'],count=1000000);assert g.reg_read(R.UC_X86_REG_EIP)==at['stop']
     rows.append(dict(position=position,radius=radius,diffuse=diffuse,specular=specular,brightness=brightness,shape=bytes(sh).hex(),frame=frame.hex(),initial=initial.hex(),output=bytes(g.mem_read(at['output'],9*16)).hex(),shift=struct.unpack('<I',g.mem_read(at['shift'],4))[0],basis_a=bytes(g.mem_read(at['basis_a'],16)).hex(),basis_b=bytes(g.mem_read(at['basis_b'],16)).hex()))
 (a.output/'report.json').write_text(json.dumps(dict(boundary=__doc__,entry='000497a0',snapshot_sha256=hashlib.sha256(s.ram).hexdigest(),definition=definition.hex(),intercepted_instructions=0,cases=rows,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()),indent=2)+'\n');print('PASS',len(rows),'whole original light uniform calls')
if __name__=='__main__':main()
