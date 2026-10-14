#!/usr/bin/env python3
"""Whole TU1.5 1791C0 and 17B580 with declared node/origin inputs.

Actual object attachment call C0E2E ->179540 and seed call1795E3 ->1791C0
authenticate the two original seed records. No instructions/services are
replaced. Node matrices and completed BBE00 world origin are declared inputs;
this does not execute the complete constructor, object-parent origin producer
or original attachment eligibility and scheduling.
"""
import argparse,hashlib,json,math,random,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 x=Xbe(NEW_DEFAULT);assert hashlib.sha256(x.data).hexdigest()==NEW_PIN;c=_Code(x,'.text')
 for site,target in [(0xc0e2e,0x179540),(0x1795e3,0x1791c0),(0x1795fb,0x178e30),(0x17b772,0x11f9c0)]:
  i=c.at(site);assert i.mnemonic=='call'and i.operands[0].imm==target
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
 for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
 state,stack,stop=0x2400000,0x241f000,0x3000000;u.mem_map(state,0x20000);u.mem_map(stop,4096)
 def call(entry,regs):
  u.mem_write(stack,struct.pack('<I',stop))
  for reg,value in [(R.UC_X86_REG_ESP,stack),(R.UC_X86_REG_EFLAGS,2),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80),*regs]:u.reg_write(reg,value)
  u.emu_start(entry,stop,count=100000);assert u.reg_read(R.UC_X86_REG_EIP)==stop
 rows=[];rng=random.Random(0x17b580)
 for n in range(1024):
  # Finite declared node transforms exercise arbitrary rotations, translations,
  # zero/unit/scaled nodes, both actual seed directions and nullable ownership.
  yaw,pitch,roll=[rng.uniform(-math.pi,math.pi)for _ in range(3)]
  cy,sy,cp,sp,cr,sr=math.cos(yaw),math.sin(yaw),math.cos(pitch),math.sin(pitch),math.cos(roll),math.sin(roll)
  node=[(0.,1.,.25,2.)[n%4],cy*cp,sy*cp,sp,-cy*sr*sp-sy*cr,cy*cr-sy*sr*sp,cp*sr,sy*sr-cy*cr*sp,-sy*cr*sp-cy*sr,cp*cr,*[rng.uniform(-100,100)for _ in range(3)]]
  origin=struct.pack('<3f',*[rng.uniform(-100,100)for _ in range(3)]);node=struct.pack('<13f',*node)
  u.mem_write(state+0x100,origin);call(0x1791c0,[(R.UC_X86_REG_EAX,state+0x200),(R.UC_X86_REG_ECX,state+0x100)])
  packet=bytes(u.mem_read(state+0x200,56))
  assert struct.unpack_from('<I',packet,24)[0]==0x070000c0 and struct.unpack_from('<I',packet,52)[0]==0x020000ca
  for ordinal in range(2):
   present=n%8!=0;u.mem_write(state+0x300,node);u.mem_write(state+0x400,struct.pack('<h2xIII',0,state+0x300 if present else 0,2,state+0x200));u.mem_write(state+0x500,b'\xcd'*112)
   call(0x17b580,[(R.UC_X86_REG_EAX,state+0x400),(R.UC_X86_REG_ESI,state+0x500),(R.UC_X86_REG_EDX,ordinal)])
   seed=packet[ordinal*28:ordinal*28+24];rows.append((struct.pack('<I',present)+node+seed,bytes(u.mem_read(state+0x504,52))))
 a.output.mkdir(parents=True,exist_ok=True);inputs=struct.pack('<I',len(rows))+b''.join(v[0]for v in rows);outputs=b''.join(v[1]for v in rows)
 (a.output/'inputs.bin').write_bytes(inputs);(a.output/'original.bin').write_bytes(outputs)
 report=dict(original_sha256=NEW_PIN,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),cases=len(rows),entry='0017b580',seed_entry='001791c0',constructor_call='000c0e2e',seed_call='001795e3',instruction_patches=[],replaced_services=[],input_sha256=hashlib.sha256(inputs).hexdigest(),output_sha256=hashlib.sha256(outputs).hexdigest(),boundary=__doc__,full_goal_complete=False)
 (a.output/'provenance.json').write_text(json.dumps(report,indent=2)+'\n')
 for entry in [0xc0cd0,0x179540,0x1791c0,0x17b580,0x11f9c0]:
  (a.output/(f'{entry:08x}.asm')).write_text('\n'.join(f'{pc:08x} {bytes(c.at(pc).bytes).hex()} {c.at(pc).mnemonic} {c.at(pc).op_str}'for pc in sorted(_trace(c,entry,{entry}).code))+'\n')
 print(len(rows),'whole original seed location matrices')
if __name__=='__main__':main()
