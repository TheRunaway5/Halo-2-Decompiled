#!/usr/bin/env python3
"""Unchanged original observer branch contracts on explicit caller-owned banks.

Packet contents, retained observer derivatives, durations and matrices are ABI
inputs to this experiment. These controls test the complete owners; they are
not claimed as naturally produced animation, parent, damage or clock state.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_mp_death_camera_runtime import Trace,h,sha
from oracle_mp_scope_camera_1_1 import INITIALIZERS,OBSERVER,PACKET
from oracle_smg_firing_1_1 import PIN,CACHE_PIN
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESI,UC_X86_REG_EDI

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True)
 ap.add_argument('--fixture',type=Path,required=True);a=ap.parse_args();t=Trace();rows=[];stages=[]
 for entry in (*INITIALIZERS,0x37d540,0x37d560):stages.append(t.measure('initializer_'+h(entry),entry))
 bsp=t.u(0x46fb64,2)
 for name,raw in [('fresh',bytes(0x358)),('retained',struct.pack('<214I',*[0x3f800000+i for i in range(214)]))]:
  t.g.mem_write(OBSERVER,raw);t.measure('reset_'+name,0x170f30,regs=((UC_X86_REG_ESI,OBSERVER),))
  rows.append('R '+raw.hex()+' '+str(bsp)+' '+bytes(t.g.mem_read(OBSERVER,0x358)).hex())
 #170EE0's actual EDX ABI owns the caller's AC-byte packet.
 from unicorn.x86_const import UC_X86_REG_EDX
 t.measure('packet_default',0x170ee0,regs=((UC_X86_REG_EDX,PACKET),))
 baseline=bytearray(t.g.mem_read(PACKET,0xac));struct.pack_into('<I',baseline,0,1)
 cases=[('valid',{}),('inactive',{0:0,0x2c:0x7fc00000}),('orientation',{0x2c:0}),
  ('parallel',{0x38:0x3f800000,0x40:0}),('positive_coordinate',{4:0x7f800000}),
  ('nan_coordinate',{8:0x7fc00000}),('negative_coordinate',{12:0xff800000}),
  ('nonfinite_vector',{0x44:0x7fc00000}),('fov_small',{0x28:0}),('fov_large',{0x28:0x41200000}),
  ('nan_fov',{0x28:0x7fc00000}),('negative_distance',{0x24:0xbf800000}),
  ('huge_distance',{0x24:0x4b800000}),('negative_duration',{0x88:0xbf800000}),
  ('nan_duration',{0x88:0x7fc00000})]
 for name,changes in cases:
  packet=bytearray(baseline)
  for at,value in changes.items():struct.pack_into('<I',packet,at,value)
  t.g.mem_write(PACKET,bytes(packet));t.measure('repair_'+name,0x173fa0,regs=((UC_X86_REG_EAX,PACKET),))
  rows.append('P '+packet.hex()+' '+bytes(t.g.mem_read(PACKET,0xac)).hex())
 # Complete angular and coupled acceleration routes with an already registered
 # observer. All other current state comes from the actual reset/adopt owner.
 for name,forward,up,seconds in [('quarter',(0,1,0),(0,0,1),.18),('half',(-1,0,0),(0,0,1),3),
   ('pitch',(0,0,1),(-1,0,0),.18),('roll',(1,0,0),(0,-1,0),.18),
   ('coupled_cap',(0,1,0),(0,0,1),1e-5)]:
  t.g.mem_write(OBSERVER,bytes(0x358));t.measure('reset_'+name,0x170f30,regs=((UC_X86_REG_ESI,OBSERVER),))
  t.g.mem_write(PACKET,bytes(baseline));t.measure('register_initial',0x170c10,regs=((UC_X86_REG_EAX,PACKET),(UC_X86_REG_ECX,0)))
  t.measure('adopt_initial',0x170ff0,(0,))
  packet=bytearray(baseline);struct.pack_into('<3f',packet,0x2c,*forward);struct.pack_into('<3f',packet,0x38,*up)
  struct.pack_into('<f',packet,0x88,seconds)
  if name=='coupled_cap':struct.pack_into('<3f',packet,4,100,200,300)
  for step in range(7):
   before=bytes(t.g.mem_read(OBSERVER,0x358));t.g.mem_write(PACKET,bytes(packet))
   t.measure('register_'+name,0x170c10,regs=((UC_X86_REG_EAX,PACKET),(UC_X86_REG_ECX,0)))
   t.measure('adopt_'+name,0x170ff0,(0,));adopted=bytes(t.g.mem_read(OBSERVER,0x358))
   adopted_packet=bytes(t.g.mem_read(PACKET,0xac));t.f(0x4f11a0,1/30)
   t.measure('motion_'+name,0x171910,regs=((UC_X86_REG_EDI,0),))
   rows.append('T '+before.hex()+' '+packet.hex()+' '+adopted_packet.hex()+' '+adopted.hex()+' '+h(t.u(0x4f11a0))+' '+bytes(t.g.mem_read(OBSERVER,0x358)).hex())
 a.fixture.parent.mkdir(parents=True,exist_ok=True);a.output.parent.mkdir(parents=True,exist_ok=True)
 a.fixture.write_text('\n'.join(rows)+'\n');a.output.write_text(json.dumps(dict(tool_sha256=sha(__file__),source_sha256=PIN,
  cache_sha256=CACHE_PIN,stages=stages,fixture_sha256=sha(a.fixture),boundaries=__doc__,instruction_patches=[]),indent=2)+'\n')
 print(json.dumps(dict(cases=len(rows),report=str(a.output))))
if __name__=='__main__':main()
