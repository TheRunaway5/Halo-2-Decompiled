#!/usr/bin/env python3
"""Statically lower complete owned Complex4627 timer bodies for TU1.5 callers.

The BIOS is modified; this is its exact immutable routine, not physical Xbox
proof. No opcode fetch/decoder or original code execution is used at runtime.
"""
import argparse,hashlib,json
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
PIN='15b8e78cb768146c1d469cda8c39482f545227b6a3c63d6f0e19ea30de08dbfb'
ENTRIES=(0x800195d6,0x8001951c,0x80014348,0x8001cd34,0x8001a384,0x800194c0,0x8001c4cf,0x8001c3cf,0x8001ccac)
REGS={r:(i,4) for i,r in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))}
REGS.update({r:(i,2) for i,r in enumerate(('ax','cx','dx','bx','sp','bp','si','di'))})
REGS.update({r:(i,1) for i,r in enumerate(('al','cl','dl','bl'))})
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for n in ('function','output','manifest'):ap.add_argument('--'+n,type=Path,required=True)
 a=ap.parse_args();assert sha(a.function)==PIN
 dis=Cs(CS_ARCH_X86,CS_MODE_32);dis.detail=True;raw=a.function.read_bytes();base=0x800102c0;bodies={};rows={};lines=[]
 for entry in ENTRIES:
  todo=[entry];found={}
  while todo:
   p=todo.pop()
   if p in found:continue
   assert base<=p<base+len(raw)
   i=next(dis.disasm(raw[p-base:],p,count=1));found[p]=i
   if i.mnemonic=='ret':continue
   if i.mnemonic.startswith('j'):
    assert i.operands[0].type==2;todo.append(i.operands[0].imm)
    if i.mnemonic=='jmp':continue
   todo.append(p+i.size)
  bodies[entry]=[found[p] for p in sorted(found)]
 ins=[i for b in bodies.values() for i in b];sites={i.address for i in ins}
 def addr(i,o):
  assert o.type==3 and not o.mem.segment
  parts=[hex(o.mem.disp&0xffffffff)+'u']
  for r,mult in ((o.mem.base,1),(o.mem.index,o.mem.scale)):
   if r:parts.append(f's.gp[{REGS[i.reg_name(r)][0]}]*{mult}u')
  return '('+'+'.join(parts)+')'
 def get(i,o):
  if o.type==2:return hex(o.imm&0xffffffff)+'u'
  if o.type==3:
   value=f'r(s.memory,{addr(i,o)},{o.size})'
   if not o.mem.base and not o.mem.index:
    return f'kernel_read(s,0x{i.address:x}u,{addr(i,o)},{o.size})'
   return value
  r,n=REGS[i.reg_name(o.reg)];return f's.reg({r},{n})'
 def put(i,o,x):
  if o.type==3:return f'w(s.memory,s.observe,0x{i.address:x}u,{addr(i,o)},{o.size},{x});'
  assert o.type==1;r,n=REGS[i.reg_name(o.reg)];return f's.set({r},{x},{n});'
 for i in ins:
  p=i.address;mn=i.mnemonic;o=list(i.operands);rows[f'{p:08x}']=dict(raw=i.bytes.hex(),mnemonic=mn,operand=i.op_str)
  if mn in ('mov','movzx'):body=put(i,o[0],get(i,o[1]))
  elif mn=='lea':body=put(i,o[0],addr(i,o[1]))
  elif mn=='push':body=f's.push(0x{p:x}u,{get(i,o[0])});'
  elif mn=='pop':body=put(i,o[0],'s.pop()')
  elif mn=='ret':body=f's.pop();s.gp[4]+={o[0].imm if o else 0}u;return;'
  elif mn=='leave':body='s.gp[4]=s.gp[5];s.gp[5]=s.pop();'
  elif mn=='pause':body=';'
  elif mn=='cdq':body='s.gp[2]=si(s.gp[0])<0?0xffffffffu:0u;'
  elif mn=='call':
   target=o[0].imm;assert o[0].type==2
   body=f's.push(0x{p:x}u,0x{p+i.size:x}u);'+(f'run_{target:x}(s);' if target in ENTRIES else f's.required(0x{p:x}u,0x{target:x}u,false);')
  elif mn in ('test','and','xor','or','add','sub','cmp','sbb','adc'):
   x,y=get(i,o[0]),get(i,o[1]);n=o[0].size
   if mn=='sbb':v=f's.flags.borrow({x},{y},{n})'
   elif mn=='adc':v=f'carry_add(s,{x},{y},{n})'
   elif mn in ('add','sub','cmp'):v=f's.flags.{"plus" if mn=="add" else "minus"}({x},{y},{n})'
   else:v=f's.flags.logic(({x})'+{'and':'&','test':'&','xor':'^','or':'|'}[mn]+f'({y}),{n})'
   body=v+';' if mn in ('test','cmp') else put(i,o[0],v)
  elif mn in ('inc','dec'):body=put(i,o[0],f's.flags.increment({get(i,o[0])},{o[0].size},{str(mn=="dec").lower()})')
  elif mn=='shl':body=put(i,o[0],f's.flags.shift({get(i,o[0])},{get(i,o[1])},{o[0].size})')
  elif mn=='imul':body=put(i,o[0],f's.flags.product({get(i,o[0])},{get(i,o[1])})')
  elif mn=='mul':body='{const auto product=std::uint64_t(s.gp[0])*'+get(i,o[0])+';s.gp[0]=std::uint32_t(product);s.gp[2]=std::uint32_t(product>>32u);s.flags.cf=s.flags.of=s.gp[2]!=0;}'
  elif mn=='shrd':body=put(i,o[0],f'double_right(s,{get(i,o[0])},{get(i,o[1])},{get(i,o[2])})')
  elif mn in ('jmp','je','jne','jb','ja','jae','jbe','jl','js'):
   target=o[0].imm;assert target in sites
   prefix={'jmp':'','je':'if(s.flags.zf)','jne':'if(!s.flags.zf)','jb':'if(s.flags.cf)','ja':'if(!s.flags.cf&&!s.flags.zf)','jae':'if(!s.flags.cf)','jbe':'if(s.flags.cf||s.flags.zf)','jl':'if(s.flags.sf!=s.flags.of)','js':'if(s.flags.sf)'}[mn]
   body=f'{prefix}goto L{target:x};'
  else:raise AssertionError((hex(p),mn))
  lines.append((f'void run_{p:x}(TimerState &s) {{\n' if p in ENTRIES else '')+f'L{p:x}: if(s.services.instruction)s.services.instruction(0x{p:x}u);{body}'+('\n}' if i is bodies[next(e for e,b in bodies.items() if i in b)][-1] else ''))
 text='// Generated complete owned modified Complex4627 timer bodies; no runtime decoder.\n#include "physics/runtime/physics_world_runtime_internal.hpp"\n#include "halo2/source_tu15_timer.hpp"\nnamespace halo2 { namespace {\nusing namespace physics_world_runtime_detail;\n'
 text+='''struct TimerState:WorldState {
 const SourceTu15TimerDataOwner &data_owner;
 TimerState(M &m,const O &o,const SourcePhysicsContactRuntimeServices &v,const SourceTu15TimerDataOwner &d)
  :WorldState(m,o,v,SourcePhysicsContactImage::TitleUpdate15),data_owner(d) {}
};
std::uint32_t kernel_read(TimerState &s,std::uint32_t pc,std::uint32_t p,unsigned n) {
 if(!s.data_owner)throw SourceTu15TimerDataUnavailable({pc,p,n});
 s.data_owner(s.memory,{pc,p,n});return r(s.memory,p,n);
}
std::uint32_t carry_add(TimerState &s,std::uint32_t a,std::uint32_t b,unsigned n) {
 const auto carry=unsigned(s.flags.cf),mask=Flags::mask(n),sign=1u<<(n*8-1);a&=mask;b&=mask;
 const auto full=std::uint64_t(a)+b+carry;const auto x=std::uint32_t(full)&mask;
 s.flags.cf=full>mask;s.flags.of=(~(a^b)&(a^x)&sign)!=0;s.flags.af=((a^b^x)&16u)!=0;s.flags.result(x,n);return x;
}
std::uint32_t double_right(TimerState &s,std::uint32_t a,std::uint32_t b,unsigned count) {
 count&=31;if(!count)return a;
 const auto x=(a>>count)|(b<<(32-count));s.flags.cf=((a>>(count-1))&1)!=0;
 if(count==1)s.flags.of=((a^x)>>31)!=0;s.flags.result(x,4);return x;
}
'''
 text+=''.join(f'void run_{e:x}(TimerState &);\n' for e in ENTRIES)+'\n'.join(lines)+'\n}\n'
 text+='''void source_run_tu15_timer_runtime(SourcePhysicsContactMemory &m,std::uint32_t entry,
 SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
 const SourcePhysicsContactRuntimeServices &services,const SourcePhysicsContactRecordObserver &observe,
 const SourceTu15TimerDataOwner &data_owner) {
 FpScope scope(fp);TimerState s{m,observe,services,data_owner};s.load(abi);
 try {switch(entry) {
'''+''.join(f'case 0x{e:x}u:run_{e:x}(s);break;\n' for e in ENTRIES)+'''default:throw std::runtime_error("Unverified original timer entry");}}
 catch(...) {abi=s.save();throw;}abi=s.save();
}
} // namespace halo2
'''
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(text)
 result=dict(schema=1,parity_target='English TU1.5 caller only',reference='owned modified Complex4627 timer dependencies',
  function_sha256=PIN,generator_sha256=sha(__file__),generated_sha256=sha(a.output),source_instructions=rows,
  complete_bodies=len(bodies),source_bodies={hex(e):[i.address for i in b] for e,b in bodies.items()},instruction_count=len(ins),physical_xbox_proof=False,production_decoder=False)
 a.manifest.parent.mkdir(parents=True,exist_ok=True);a.manifest.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(sites=len(ins),manifest_sha256=sha(a.manifest))))
if __name__=='__main__':main()
