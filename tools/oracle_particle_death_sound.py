#!/usr/bin/env python3
"""Original particle-death snd! dispatch and final-position/control packet.

Unchanged retail246E60 ->247000 ->246EB0 executes on explicit particle
position/velocity inputs.188DD0/11BED0 spatial producers and1895F0 sound
admission are recorded boundaries; this is not original sound admission,
audio output or a native world-position/region producer equivalence claim.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

def sha(b):return hashlib.sha256(b).hexdigest()
def pack(*v):return struct.pack('<'+'I'*len(v),*v)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
 source=ROOT/'tests/fixtures/particle_collision_sound_gain.original.bin';raw=source.read_bytes();assert raw[:8]==b'PCG1'+pack(2048)
 vm=Uc(UC_ARCH_X86,UC_MODE_32);vm.mem_map(0x10000,0x600000)
 for s in x.sections:vm.mem_write(s.va,x.section_bytes(s))
 vm.mem_map(0x70000000,0x10000);stack,stop,particle=0x7000e000,0x7000ff00,0x70001000;executed={};events=[];outputs=[];records=[]
 def word(at):return struct.unpack('<I',vm.mem_read(at,4))[0]
 def code(g,pc,size,_):
  if pc not in executed:
   b=bytes(g.mem_read(pc,size));assert b==x.read(pc,size);executed[pc]=b
  if pc not in [0x188dd0,0x11bed0,0x1895f0]:return
  sp=g.reg_read(R.UC_X86_REG_ESP);ret=word(sp);extra=0
  if pc==0x188dd0:events.append({'call':'188DD0','input':g.reg_read(R.UC_X86_REG_EAX),'declared_result':0x1234});g.reg_write(R.UC_X86_REG_EAX,0x1234)
  elif pc==0x11bed0:
   position=bytes(g.mem_read(g.reg_read(R.UC_X86_REG_EDX),12));g.mem_write(g.reg_read(R.UC_X86_REG_ESI),pack(0x00020001,0x44556677));events.append({'call':'11BED0','position_hex':position.hex(),'declared_result_hex':'0100020077665544'})
  else:
   out={'sound':word(sp+4),'position_hex':bytes(g.mem_read(g.reg_read(R.UC_X86_REG_EDX),12)).hex(),'control_word':g.reg_read(R.UC_X86_REG_XMM0)&0xffffffff,'raw_location_packet_hex':bytes(g.mem_read(g.reg_read(R.UC_X86_REG_EDX),36)).hex()};events.append({'call':'1895F0',**out});outputs.append(out);extra=4
  g.reg_write(R.UC_X86_REG_ESP,sp+4+extra);g.reg_write(R.UC_X86_REG_EIP,ret)
 vm.hook_add(UC_HOOK_CODE,code)
 for i in range(2048):
  velocity=raw[8+i*16:20+i*16];gain=struct.unpack_from('<I',raw,20+i*16)[0];position=struct.pack('<3f',float(i)-1024,float(i%7)*.25,-7.5);tag=[0xe24300ce,0xe9d90854][i&1];vm.mem_write(particle,bytes(64));vm.mem_write(particle+0x1c,position);vm.mem_write(particle+0x28,velocity);vm.mem_write(stack,pack(stop,particle,0x88776655));vm.reg_write(R.UC_X86_REG_ESP,stack);vm.reg_write(R.UC_X86_REG_EAX,tag);vm.reg_write(R.UC_X86_REG_ECX,0);vm.reg_write(R.UC_X86_REG_EDX,0x736e6421);vm.reg_write(R.UC_X86_REG_FPCW,0x023f);vm.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
  before=len(events);vm.emu_start(0x246e60,stop,count=10000);assert vm.reg_read(R.UC_X86_REG_EIP)==stop;out=outputs[-1];assert out['sound']==tag and out['position_hex']==position.hex() and out['control_word']==gain;records.append({'velocity_hex':velocity.hex(),'events':events[before:]})
 report={'scope':__doc__,'cases':len(records),'original_xbe_sha256':sha(x.data),'input_fixture_sha256':sha(raw),'tool_sha256':sha(Path(__file__).read_bytes()),'executed_instructions':len(executed),'executed_instruction_bank_sha256':sha(b''.join(pack(pc)+b for pc,b in sorted(executed.items()))),'instruction_patches':[],'native_gain_fixture_matches':True,'source_sound_and_position_match':True,'records':records};(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='records'},indent=2))
if __name__=='__main__':main()
