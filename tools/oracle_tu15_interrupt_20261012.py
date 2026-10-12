#!/usr/bin/env python3
"""Execute unchanged owned kernel lowering and connected timer procedures.

Contexts/clocks/dispatch tokens are declared procedure inputs, never captured
running scheduler state. Missing scheduler/interrupt callees stop before their
first instruction. No successful callee returns or code patches are supplied.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_INSN
from unicorn import x86_const as R
from oracle_tu15_startup_records_20261011 import abi,load,cpu,STACK,SP,STOP
from oracle_tu15_timer_procedures_20261012 import ENTRIES as TIMERS
PIN='15b8e78cb768146c1d469cda8c39482f545227b6a3c63d6f0e19ea30de08dbfb'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--kernel',type=Path,required=True);ap.add_argument('--output',type=Path,required=True)
 a=ap.parse_args();assert sha(a.kernel)==PIN;a.output.mkdir(parents=True,exist_ok=False)
 raw=a.kernel.read_bytes();base=0x800102c0;d=Cs(CS_ARCH_X86,CS_MODE_32);d.detail=True;sites={};bodies={}
 entries=TIMERS+(0x80018ba0,0x80014368)
 for entry in entries:
  todo=[entry];body={}
  while todo:
   p=todo.pop()
   if p in body:continue
   i=next(d.disasm(raw[p-base:],p,count=1));body[p]=i
   if i.mnemonic=='ret':continue
   if i.mnemonic.startswith('j'):
    target=i.operands[0].imm
    if target not in entries:todo.append(target)
    if i.mnemonic=='jmp':continue
   todo.append(p+i.size)
  bodies[entry]=sorted(body)
  for p,i in body.items():sites[p]=dict(raw=i.bytes.hex(),mnemonic=i.mnemonic,operand=i.op_str)
 def word(v):return struct.pack('<I',v)
 specs=[]
 for entry in (0x80018ba0,0x80014368):
  for level in (0,1,2,31):
   for pending in (0,2,4,8,0x80000000):
    specs.append(dict(kind='lower',entry=entry,level=level,pending=pending,policy=0))
 for level in (0,1,2,3,31,0x80):
  specs.append(dict(kind='lower',entry=0x80018ba0,level=level,pending=0,policy=1))
 for flags in (2,0x202,0xcd7):
  for shift in (0,4,8):
   specs.append(dict(kind='timer',entry=0x800195d6,level=0,pending=0,policy=0,flags=flags,shift=shift))
 stack=bytes((i*17+11)&255 for i in range(0x10000));cases=[];covered=set()
 for index,spec in enumerate(specs):
  g=Uc(UC_ARCH_X86,UC_MODE_32)
  for p,n in ((STACK,0x10000),(STOP,0x1000),(0x56e000,0x2000),(0x80010000,0x26000),(0x8003a000,0x1000)):
   g.mem_map(p,n)
  g.mem_write(base,raw)
  context=bytearray(0x240)
  context[0x1c:0x20]=word(spec['pending']);context[0x24:0x28]=word(0x12345678)
  # Declared dispatch tokens lead to missing owners, in their own input bank.
  dispatch=b''.join(word(STOP+0x100+i*4) for i in range(32))
  context[0x200:0x204]=word(0);context[0x204:0x208]=word(0x56ee80)
  context[0x208:0x20c]=word(spec['policy']);context[0x20c:0x210]=word(0x56ef00)
  # No later scheduler state is invented to run scheduler callees.
  clocks=bytes(28)
  initial=[dict(address=STACK,hex=stack.hex()),dict(address=0x56ed00,hex=bytes((i*7+3)&255 for i in range(0x300)).hex()),
   dict(address=0x80035938,hex=dispatch.hex()),dict(address=0x80035a00,hex=context.hex()),
   dict(address=0x8003a880,hex=clocks.hex()),dict(address=0x8003a8a8,hex=bytes((i*13+5)&255 for i in range(0x278)).hex())]
  for bank in initial:g.mem_write(bank['address'],bytes.fromhex(bank['hex']))
  requests=[(spec['entry'],spec['level'],[])]
  if spec['kind']=='timer':requests=[(0x8001cd34,0,[]),(0x800194c0,0,[0x56ede0,0]),
   (0x800195d6,0,[0x56ede0,0xb5659000,0xffffffcd,0x56edc0])]
  phases=[]
  for entry,ecx,args in requests:
   sp=SP+spec.get('shift',0);frame=struct.pack('<'+'I'*(len(args)+1),STOP,*args);g.mem_write(sp,frame)
   state=cpu(sp,spec.get('flags',0x202 if index&1 else 2),3);state['gp'][1]=ecx;load(g,state)
   before=abi(g);trace=[];stores=[];ports=[];active=0;boundary=None
   def hook(uc,p,n,user):
    nonlocal active,boundary
    if p not in sites:
     boundary=dict(target=p,instruction=active,tail=sites[active]['mnemonic']=='jmp')
     uc.emu_stop();return
    assert bytes(uc.mem_read(p,n)).hex()==sites[p]['raw']
    active=p;trace.append(p);covered.add(p)
   def write(uc,access,p,n,value,user):
    assert any(bank['address']<=p<p+n<=bank['address']+len(bytes.fromhex(bank['hex'])) for bank in initial),hex(p)
    stores.append(dict(instruction=active,trace_index=len(trace),address=p,before=bytes(uc.mem_read(p,n)).hex(),written=(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()))
   def port(uc,p,n,value,user):
    assert n==1;ports.append(dict(instruction=active,trace_index=len(trace),port=p,value=value))
   hc=g.hook_add(UC_HOOK_CODE,hook);hw=g.hook_add(UC_HOOK_MEM_WRITE,write);hp=g.hook_add(UC_HOOK_INSN,port,None,1,0,R.UC_X86_INS_OUT)
   g.emu_start(entry,STOP,count=10000)
   for h in (hc,hw,hp):g.hook_del(h)
   assert boundary is not None or g.reg_read(R.UC_X86_REG_EIP)==STOP
   phases.append(dict(entry=entry,frame=frame.hex(),before=before,after=abi(g),instructions=trace,stores=stores,ports=ports,boundary=boundary))
  assert bytes(g.mem_read(base,len(raw)))==raw
  final=[dict(address=b['address'],hex=bytes(g.mem_read(b['address'],len(bytes.fromhex(b['hex'])))).hex()) for b in initial]
  cases.append(dict(spec=spec,initial_banks=initial,phases=phases,final_banks=final))
 result=dict(schema=1,tool_sha256=sha(__file__),kernel_sha256=PIN,source_bodies=bodies,source_instructions=sites,
  cases=cases,covered_instructions=sorted(covered),procedure_context_inputs=True,procedure_clock_inputs=True,
  captured_scheduler_state_used=False,callee_returns_supplied=False,code_patches=[],physical_xbox_proof=False,live_physics_parity=False)
 (a.output/'fixture.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(cases=len(cases),instructions=sum(len(p['instructions']) for c in cases for p in c['phases']),
  stores=sum(len(p['stores']) for c in cases for p in c['phases']),ports=sum(len(p['ports']) for c in cases for p in c['phases']),
  covered=len(covered),sha256=sha(a.output/'fixture.json'))))
if __name__=='__main__':main()
