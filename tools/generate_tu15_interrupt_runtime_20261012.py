#!/usr/bin/env python3
"""Lower both complete owned Complex4627 interrupt-lowering routines.

All context reads, interrupt dispatch and PIC port writes require named owners.
Production neither decodes original bytes nor executes guest instructions.
"""
import argparse,hashlib,json
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
PIN='15b8e78cb768146c1d469cda8c39482f545227b6a3c63d6f0e19ea30de08dbfb'
ENTRIES=(0x80018ba0,0x80014368)
REGS={r:(i,4) for i,r in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))}
REGS.update({r:(i,1) for i,r in enumerate(('al','cl','dl','bl'))})
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for n in ('kernel','output','manifest'):ap.add_argument('--'+n,type=Path,required=True)
 a=ap.parse_args();assert sha(a.kernel)==PIN
 raw=a.kernel.read_bytes();base=0x800102c0;d=Cs(CS_ARCH_X86,CS_MODE_32);d.detail=True;bodies={}
 for entry in ENTRIES:
  todo=[entry];found={}
  while todo:
   p=todo.pop()
   if p in found:continue
   assert entry<=p<entry+0x200
   i=next(d.disasm(raw[p-base:],p,count=1));found[p]=i
   if i.mnemonic=='ret':continue
   if i.mnemonic.startswith('j'):
    target=i.operands[0].imm
    if target not in ENTRIES:todo.append(target)
    if i.mnemonic=='jmp':continue
   todo.append(p+i.size)
  bodies[entry]=found
 def addr(i,o):
  assert o.type==3 and not o.mem.segment
  parts=[hex(o.mem.disp&0xffffffff)+'u']
  for r,mult in ((o.mem.base,1),(o.mem.index,o.mem.scale)):
   if r:parts.append(f's.gp[{REGS[i.reg_name(r)][0]}]*{mult}u')
  return '('+'+'.join(parts)+')'
 def get(i,o):
  if o.type==2:return hex(o.imm&0xffffffff)+'u'
  if o.type==3:
   if not o.mem.base:return f'owned_read(s,0x{i.address:x}u,{addr(i,o)},{o.size})'
   return f'r(s.memory,{addr(i,o)},{o.size})'
  r,n=REGS[i.reg_name(o.reg)];return f's.reg({r},{n})'
 def put(i,o,x):
  if o.type==3:return f'w(s.memory,s.observe,0x{i.address:x}u,{addr(i,o)},{o.size},{x});'
  r,n=REGS[i.reg_name(o.reg)];return f's.set({r},{x},{n});'
 text='''// Generated complete modified Complex4627 interrupt bodies; no runtime decoder.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "halo2/source_tu15_interrupt.hpp"
namespace halo2 { namespace {
using namespace physics_world_runtime_detail;
struct InterruptState:WorldState {
 const SourceTu15TimerDataOwner &data;
 const SourceTu15InterruptPortOwner &ports;
 InterruptState(M &m,const O &o,const SourcePhysicsContactRuntimeServices &v,
  const SourceTu15TimerDataOwner &d,const SourceTu15InterruptPortOwner &p)
  :WorldState(m,o,v,SourcePhysicsContactImage::TitleUpdate15),data(d),ports(p) {}
};
std::uint32_t owned_read(InterruptState &s,std::uint32_t pc,std::uint32_t p,unsigned n) {
 if(!s.data)throw SourceTu15TimerDataUnavailable({pc,p,n});
 s.data(s.memory,{pc,p,n});return r(s.memory,p,n);
}
void run_80018ba0(InterruptState &);
void run_80014368(InterruptState &);
'''
 sites={}
 for entry,body in bodies.items():
  text+=f'void run_{entry:x}(InterruptState &s) {{\n'
  for p,i in sorted(body.items()):
   mn=i.mnemonic;o=list(i.operands)
   sites[f'{p:08x}']=dict(raw=i.bytes.hex(),mnemonic=mn,operand=i.op_str)
   if mn=='mov':v=put(i,o[0],get(i,o[1]))
   elif mn=='lea':v=put(i,o[0],addr(i,o[1]))
   elif mn=='push':v=f's.push(0x{p:x}u,{get(i,o[0])});'
   elif mn=='pop':v=put(i,o[0],'s.pop()')
   elif mn=='pushfd':v=f's.push(0x{p:x}u,s.other_flags|s.flags.bits());'
   elif mn=='popfd':v='{const auto f=s.pop();s.other_flags=f&~0x8d5u;s.flags.load(f);}'
   elif mn=='cli':v='s.other_flags&=~0x200u;'
   elif mn=='ret':v='s.pop();return;'
   elif mn in ('add','sub','cmp','and','xor','or'):
    x,y=get(i,o[0]),get(i,o[1]);n=o[0].size
    if mn in ('add','sub','cmp'):calc=f's.flags.{"plus" if mn=="add" else "minus"}({x},{y},{n})'
    else:calc=f's.flags.logic(({x})'+{'and':'&','xor':'^','or':'|'}[mn]+f'({y}),{n})'
    v=calc+';' if mn=='cmp' else put(i,o[0],calc)
   elif mn=='shl':v=put(i,o[0],f's.flags.shift({get(i,o[0])},{get(i,o[1])},{o[0].size})')
   elif mn=='shr':v=put(i,o[0],f's.flags.right({get(i,o[0])},{get(i,o[1])},{o[0].size},false)')
   elif mn=='bsr':v='{const auto x='+get(i,o[1])+';s.flags.zf=x==0;if(x)'+put(i,o[0],'31u-std::countl_zero(x)')+'}'
   elif mn=='out':v=f'if(!s.ports)throw std::runtime_error("Original PIC output requires its port owner");s.ports({{0x{p:x}u,{get(i,o[0])},std::uint8_t({get(i,o[1])})}});'
   elif mn=='call':
    v=f's.push(0x{p:x}u,0x{p+i.size:x}u);'
    v+=(f'run_{o[0].imm:x}(s);' if o[0].type==2 and o[0].imm in ENTRIES else f's.required(0x{p:x}u,{get(i,o[0])},false);')
   elif mn.startswith('j'):
    target=o[0].imm
    if target in ENTRIES:v=f'run_{target:x}(s);return;'
    else:
     assert target in body
     cond={'jmp':'','jne':'if(!s.flags.zf)','ja':'if(!s.flags.cf&&!s.flags.zf)','jge':'if(s.flags.sf==s.flags.of)'}[mn]
     v=f'{cond}goto L{target:x};'
   else:raise AssertionError((hex(p),mn))
   text+=f'L{p:x}: if(s.services.instruction)s.services.instruction(0x{p:x}u);{v}\n'
  text+='}\n'
 text+='''}
void source_run_tu15_interrupt_runtime(SourcePhysicsContactMemory &m,std::uint32_t entry,
 SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
 const SourcePhysicsContactRuntimeServices &services,const SourcePhysicsContactRecordObserver &observe,
 const SourceTu15TimerDataOwner &data,const SourceTu15InterruptPortOwner &ports) {
 FpScope scope(fp);InterruptState s{m,observe,services,data,ports};s.load(abi);
 try {switch(entry) {
 case 0x80018ba0:run_80018ba0(s);break;
 case 0x80014368:run_80014368(s);break;
 default:throw std::runtime_error("Unverified original interrupt entry");}}
 catch(...) {abi=s.save();throw;}abi=s.save();
}
} // namespace halo2
'''
 a.output.write_text(text)
 result=dict(schema=1,kernel_sha256=PIN,generator_sha256=sha(__file__),generated_sha256=sha(a.output),
  reference='owned modified Complex4627 only',source_bodies={hex(e):sorted(b) for e,b in bodies.items()},
  source_instructions=sites,complete_bodies=len(bodies),instruction_count=len(sites),physical_xbox_proof=False)
 a.manifest.parent.mkdir(parents=True,exist_ok=True);a.manifest.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(bodies=len(bodies),sites=len(sites),sha256=sha(a.output))))
if __name__=='__main__':main()
