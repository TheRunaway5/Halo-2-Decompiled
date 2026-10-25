#!/usr/bin/env python3
"""Execute the unchanged12-instruction cloth shader getter over declared data."""
import argparse,hashlib,itertools,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import *
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert os.environ.get('HALO2_TEST_SCRATCH')
 assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
 image=Xbe(NEW_DEFAULT);u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000);u.mem_map(0x4000000,0x80000)
 for s in image.sections:
  if s.va+len(image.section_bytes(s))<=0x610000:u.mem_write(s.va,image.section_bytes(s))
 def w(p,v):u.mem_write(p,struct.pack('<I',v))
 regs=[UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI];rows=[]
 for slot,index,flags in itertools.product((0,1,15,63),(0,1,4,7),(0x202,0xad7)):
  gp=[0x11111111*i for i in range(8)];gp[4]=0x4078000
  for r,v in zip(regs,gp):u.reg_write(r,v)
  u.reg_write(UC_X86_REG_EFLAGS,flags);w(gp[4],0x10000);w(gp[4]+4,0xcafe0000|slot)
  w(0x4ecc08,0x4001000);w(0x4001044,0x4002000);w(0x4f0414,0x4060000)
  w(0x4002004+slot*0x1258,0xbeef0000|index);w(0x4060008+index*16,0x4065000);w(0x406500c,0xdada0000|index)
  u.emu_start(0x1191a0,0x10000,count=32)
  rows.append(dict(slot=slot,index=index,flags=flags,gp=gp,result=[u.reg_read(r)for r in regs],result_flags=u.reg_read(UC_X86_REG_EFLAGS)))
 a.output.write_text(json.dumps(dict(original_sha256=NEW_PIN,supplied_service_results=[],cases=rows),separators=(',',':'))+'\n');print(len(rows),'original complete cloth shader lookups')
if __name__=='__main__':main()
