#!/usr/bin/env python3
"""Execute original kernel timer constructors/due/index/insertion procedures.

Fresh patterned storage and declared procedure clock inputs. Queue and timer
records are constructed by the original producers before each dependent call.
No captured scheduler/world, native expected values, patches or guest return
substitutions. Modified BIOS procedure evidence, not live timer scheduling.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_tu15_startup_records_20261011 import abi,load,cpu,STACK,SP,STOP
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
ENTRIES=(0x800195d6,0x8001951c,0x80014348,0x8001cd34,0x8001a384,0x800194c0,0x8001c4cf,0x8001c3cf,0x8001ccac)
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--kernel',type=Path,required=True);ap.add_argument('--output',type=Path,required=True)
 a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 report=json.loads((a.kernel/'report.json').read_text());raw=(a.kernel/'kernel-text.bin').read_bytes();base=report['code_section']['address']
 assert sha(a.kernel/'kernel-text.bin')==report['code_section']['sha256']=='15b8e78cb768146c1d469cda8c39482f545227b6a3c63d6f0e19ea30de08dbfb'
 assert not report['mutable_kernel_state_captured'] and not report['guest_memory_writes']
 dis=Cs(CS_ARCH_X86,CS_MODE_32);dis.detail=True;sites={};bodies={}
 for entry in ENTRIES:
  todo=[entry];body={}
  while todo:
   p=todo.pop()
   if p in body:continue
   assert base<=p<base+len(raw);i=next(dis.disasm(raw[p-base:],p,count=1));body[p]=i
   if i.mnemonic=='ret':continue
   if i.mnemonic.startswith('j'):
    assert i.operands[0].type==2;todo.append(i.operands[0].imm)
    if i.mnemonic=='jmp':continue
   todo.append(p+i.size)
  bodies[entry]=sorted(body)
  for p,i in body.items():sites[p]=dict(raw=i.bytes.hex(),mnemonic=i.mnemonic,operand=i.op_str)
 timer0=0x56ede0;timer1=0x56ee20;mask=(1<<64)-1
 def clock(v):return struct.pack('<III',v&0xffffffff,v>>32,v>>32)
 stack=bytes((i*17+11)&255 for i in range(0x10000))
 initial=[dict(address=STACK,hex=stack.hex()),dict(address=0x56ed00,hex=bytes((i*7+3)&255 for i in range(0x300)).hex()),
  dict(address=0x8003a8a8,hex=bytes((i*13+5)&255 for i in range(0x278)).hex())]
 cases=[];covered=set()
 specs=[]
 for flag in (2,0xcd7):
  for shift in (0,4,8):
   for top in (0,3):specs.append(dict(kind='constructors',flags=flag,shift=shift,top=top,requests=[]))
 for interval in (0,mask,mask-9999,(-216000000000)&mask,1<<63):
  for now in (0,(1<<32)-1,1<<32,mask):
   specs.append(dict(kind='index',flags=2,shift=0,top=3,requests=[(0x8001ccac,0,[interval&0xffffffff,interval>>32,now&0xffffffff,now>>32,timer0])]))
 for index,(due,system,interrupt) in enumerate(((0,1000000,12345),(2000000,1000000,12345),
  ((-216000000000)&mask,1000000,12345),(mask,1000000,(1<<32)-1),(1<<63,1000000,12345))):
  specs.append(dict(kind='due_and_queue',flags=2 if index&1 else 0xcd7,shift=(index%3)*4,top=3,
   system=system,interrupt=interrupt,requests=[(0x8001c4cf,timer0,[due&0xffffffff,due>>32]),
    (0x8001c4cf,timer1,[(-432000000000)&0xffffffff,((-432000000000)&mask)>>32])]))
 for spec in specs:
  g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(STACK,0x10000);g.mem_map(STOP,0x1000);g.mem_map(0x56e000,0x2000)
  g.mem_map(0x80010000,0x26000);g.mem_write(base,raw);g.mem_map(0x8003a000,0x1000)
  for bank in initial:g.mem_write(bank['address'],bytes.fromhex(bank['hex']))
  clocks=clock(spec.get('system',1000000))+b'\xcc'*4+clock(spec.get('interrupt',12345));g.mem_write(0x8003a880,clocks)
  phases=[];requests=[(0x8001cd34,0,[]),(0x800194c0,0,[timer0,0]),(0x800194c0,0,[timer1,1])]+spec['requests']
  for entry,ecx,args in requests:
   sp=SP+spec['shift'];frame=struct.pack('<'+'I'*(len(args)+1),STOP,*args);g.mem_write(sp,frame)
   state=cpu(sp,spec['flags'],spec['top']);state['gp'][1]=ecx;load(g,state);before=abi(g);trace=[];stores=[];active=0
   def hook(uc,p,n,user):
    nonlocal active
    assert p in sites and bytes(uc.mem_read(p,n)).hex()==sites[p]['raw'],hex(p)
    active=p;trace.append(p);covered.add(p)
   def write(uc,access,p,n,value,user):
    assert STACK<=p<p+n<=STACK+0x10000 or 0x56ed00<=p<p+n<=0x56f000 or 0x8003a8a8<=p<p+n<=0x8003ab20
    stores.append(dict(instruction=active,trace_index=len(trace),address=p,before=bytes(uc.mem_read(p,n)).hex(),written=(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()))
   hc=g.hook_add(UC_HOOK_CODE,hook);hw=g.hook_add(UC_HOOK_MEM_WRITE,write)
   g.emu_start(entry,STOP,count=10000);g.hook_del(hc);g.hook_del(hw)
   assert g.reg_read(R.UC_X86_REG_EIP)==STOP and g.reg_read(R.UC_X86_REG_ESP)==sp+len(frame)
   phases.append(dict(entry=entry,frame=frame.hex(),before=before,after=abi(g),instructions=trace,stores=stores))
  final=[dict(address=p,hex=bytes(g.mem_read(p,n)).hex()) for p,n in ((SP-0x100,0x200),(0x56ed00,0x300),(0x8003a8a8,0x278),(0x8003a880,28))]
  cases.append(dict(spec=spec,clocks_hex=clocks.hex(),phases=phases,final_banks=final))
 result=dict(schema=1,tool_sha256=sha(__file__),kernel_report_sha256=sha(a.kernel/'report.json'),kernel_text_sha256=sha(a.kernel/'kernel-text.bin'),
  source_instructions=sites,source_bodies=bodies,covered_instructions=sorted(covered),initial_banks=initial,cases=cases,
  original_producers_executed=True,procedure_clock_inputs=True,code_patches=[],native_expected_data_used=False,
  captured_scheduler_state_used=False,physical_xbox_proof=False,live_timer_scheduling=False,live_physics_parity=False)
 (a.output/'fixture.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(cases=len(cases),instructions=sum(len(p['instructions']) for c in cases for p in c['phases']),
  writes=sum(len(p['stores']) for c in cases for p in c['phases']),covered=len(covered),sha256=sha(a.output/'fixture.json'))))
if __name__=='__main__':main()
