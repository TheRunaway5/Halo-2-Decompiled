#!/usr/bin/env python3
"""Unchanged retailC0C80 update over an actual frag-light return snapshot.

Physical RAM and its retained page tables supply all original light, spatial,
metadata and clock banks. Only the declared call stack, light tag/control and
elapsed ticks vary. No callee or instruction is replaced. This is a CPU replay,
not a rendered-frame or original allocation/admission equivalence claim.
"""
import argparse,hashlib,json,re,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_MEM_UNMAPPED,UC_HOOK_CODE
from unicorn import x86_const as R
ROOT=Path(__file__).resolve().parents[1]
def u(b,a=0):return struct.unpack_from('<I',b,a)[0]
class Snapshot:
 def __init__(self,p):self.ram=(p/'physical-ram.bin').read_bytes();self.cr3=0xf000
 def physical(self,a):
  pde=u(self.ram,self.cr3+(a>>22)*4);assert pde&1,hex(a)
  if pde&128:return (pde&0xffc00000)+(a&0x3fffff)
  pte=u(self.ram,(pde&0xfffff000)+((a>>12)&1023)*4);assert pte&1,hex(a)
  return (pte&0xfffff000)+(a&4095)
 def read(self,a,n):
  out=b''
  while n:
   count=min(n,4096-(a&4095));p=self.physical(a);assert p+count<=len(self.ram),(hex(a),hex(p))
   out+=self.ram[p:p+count];a+=count;n-=count
  return out
 def guest(self):
  g=Uc(UC_ARCH_X86,UC_MODE_32)
  def unmapped(guest,access,a,size,value,data):
   at=a&~4095
   try:raw=self.read(at,4096)
   except Exception as e:print('Unmapped original data',hex(a),str(e));return False
   guest.mem_map(at,4096);guest.mem_write(at,raw);return True
  g.hook_add(UC_HOOK_MEM_UNMAPPED,unmapped)
  return g

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 snap=Snapshot(a.snapshot);assert len(snap.ram)==67108864
 assert hashlib.sha256(snap.ram).hexdigest()=='9819df450ed6620ab664560d00755131aa8f5416afac40a6f1245d7df05f46f1'
 datum=0xe9810015;pool=u(snap.read(0x4e030c,4));light=u(snap.read(pool+0x44,4))+(datum&65535)*0x110;clock=u(snap.read(0x510c54,4));birth=u(snap.read(light+0x14,4));table=u(snap.read(0x4e3b44,4))
 print({'light':hex(light),'clock':hex(clock),'birth':birth})
 rows=[];definitions={};calls=set()
 for tag in (0xe63a04c5,0xe694051f):
  definition=u(snap.read(table+(tag&65535)*16+8,4));definitions[f'{tag:08x}']=snap.read(definition,0xe4).hex()
  for control in (0.,.5,1.,2.):
   for elapsed in range(0,31):
    g=snap.guest();stack,stop=0x7000f000,0x7000ff00;g.mem_map(0x70000000,0x10000)
    # Materialize original data pages before explicitly writing declared inputs.
    for at in (light,clock):
     for page in range(at&~4095,(at+0x110+4095)&~4095,4096):
      try:g.mem_map(page,4096);g.mem_write(page,snap.read(page,4096))
      except Exception:raise
    g.mem_write(light+4,struct.pack('<I',tag));g.mem_write(light+0xcc,struct.pack('<f',control));g.mem_write(clock+8,struct.pack('<I',birth+elapsed))
    before=bytes(g.mem_read(light,0x110));g.mem_write(stack,struct.pack('<II',stop,datum));g.reg_write(R.UC_X86_REG_ESP,stack);g.reg_write(R.UC_X86_REG_FPCW,0x023f);g.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
    count=[0]
    def trace(guest,address,size,data):count[0]+=1;calls.add(address)
    g.hook_add(UC_HOOK_CODE,trace)
    g.emu_start(0xc0c80,stop,count=1000000)
    assert g.reg_read(R.UC_X86_REG_EIP)==stop,(elapsed,hex(g.reg_read(R.UC_X86_REG_EIP)))
    after=bytes(g.mem_read(light,0x110));rows.append(dict(tag=f'{tag:08x}',control=struct.unpack('<I',struct.pack('<f',control))[0],elapsed=elapsed,before=before.hex(),after=after.hex(),instructions=count[0]))
 report=dict(boundary=__doc__,snapshot_sha256=hashlib.sha256(snap.ram).hexdigest(),cr3='0000f000',entry='000c0c80',light_datum=f'{datum:08x}',light_address=f'{light:08x}',clock_address=f'{clock:08x}',tick_rate=struct.unpack('<h',snap.read(clock+2,2))[0],fpcw='023F',mxcsr='1FA0',definitions=definitions,cases=rows,intercepted_instructions=0,executed_addresses=[f'{v:08x}' for v in sorted(calls)],tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS',len(rows),'whole original light updates')
if __name__=='__main__':main()
