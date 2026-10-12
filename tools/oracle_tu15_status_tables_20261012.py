#!/usr/bin/env python3
"""Execute unchanged owned kernel301 for every mapped status and range edges.

Uses actual captured immutable code/tables, fresh patterned stacks and complete
original CPU effects. This is offline original-code evidence for the modified
BIOS, not physical Xbox execution or live physics proof.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_tu15_startup_records_20261011 import abi,load,cpu,STACK,SP,STOP
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--kernel',type=Path,required=True);ap.add_argument('--output',type=Path,required=True)
 a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 report=json.loads((a.kernel/'report.json').read_text());banks=report['status301_tables']['records']
 for bank in banks:assert sha(a.kernel/(bank['name']+'.bin'))==bank['sha256']
 function=(a.kernel/'function.bin').read_bytes();ranges=(a.kernel/'ranges.bin').read_bytes()
 assert sha(a.kernel/'function.bin')=='9473f53e951680c531d3e341264567df38def483706d340f709d0c9e99ad1a26'
 sites={i.address:dict(raw=i.bytes.hex(),mnemonic=i.mnemonic,operand=i.op_str) for i in Cs(CS_ARCH_X86,CS_MODE_32).disasm(function,0x80023653)}
 statuses={0,0xffffffff,0x20000000,0x20000001,0xa000009c,0x80070000,0x80070017,0x8007ffff,0xc0010000,0xc0010017,0xc001ffff,0xd000009c,0xd0000000}
 for i in range(103):
  start,count,width=struct.unpack_from('<IHH',ranges,i*8)
  statuses.update((start-1&0xffffffff,start,start+count&0xffffffff))
  statuses.update((start+j)&0xffffffff for j in range(count))
 stack=bytes((i*17+11)&255 for i in range(0x10000));cases=[];covered=set()
 for index,status in enumerate(sorted(statuses)):
  g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(STACK,0x10000);g.mem_write(STACK,stack)
  g.mem_map(STOP,0x1000);g.mem_map(0x80011000,0x2000);g.mem_map(0x80023000,0x1000)
  for bank in banks:g.mem_write(bank['address'],(a.kernel/(bank['name']+'.bin')).read_bytes())
  sp=SP+(index%3)*4;g.mem_write(sp,struct.pack('<2I',STOP,status));load(g,cpu(sp,2 if index&1 else 0xcd7,0 if index&2 else 3))
  before=abi(g);trace=[];stores=[];active=0
  def hook(uc,p,n,user):
   nonlocal active
   assert p in sites and bytes(uc.mem_read(p,n)).hex()==sites[p]['raw']
   active=p;trace.append(p);covered.add(p)
  def write(uc,access,p,n,value,user):
   assert STACK<=p<p+n<=STACK+0x10000
   stores.append(dict(instruction=active,trace_index=len(trace),address=p,before=bytes(uc.mem_read(p,n)).hex(),written=(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()))
  g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write);g.emu_start(0x80023653,STOP,count=2000)
  assert g.reg_read(R.UC_X86_REG_EIP)==STOP and g.reg_read(R.UC_X86_REG_ESP)==sp+8
  for bank in banks:assert bytes(g.mem_read(bank['address'],bank['bytes']))==(a.kernel/(bank['name']+'.bin')).read_bytes()
  cases.append(dict(status=status,stack_pointer=sp,before=before,after=abi(g),instructions=trace,stores=stores,
   final_bank=dict(address=sp-32,hex=bytes(g.mem_read(sp-32,64)).hex())))
 result=dict(schema=1,tool_sha256=sha(__file__),kernel_report_sha256=sha(a.kernel/'report.json'),records=banks,
  source_instructions=sites,covered_instructions=sorted(covered),stack_address=STACK,initial_stack_hex=stack.hex(),cases=cases,
  original_code_executed=True,code_patches=[],native_expected_data_used=False,physical_xbox_proof=False,live_physics_parity=False)
 (a.output/'fixture.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(cases=len(cases),instructions=sum(len(c['instructions']) for c in cases),covered=len(covered),sites=len(sites),sha256=sha(a.output/'fixture.json'))))
if __name__=='__main__':main()
