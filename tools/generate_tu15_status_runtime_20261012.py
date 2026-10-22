#!/usr/bin/env python3
"""Statically lower complete owned Complex4627 export301 for its TU1.5 caller.

The BIOS is modified; this is its exact immutable routine, not physical Xbox
proof. No opcode fetch/decoder or original code execution is used at runtime.
"""
import argparse,hashlib,json
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
PIN='9473f53e951680c531d3e341264567df38def483706d340f709d0c9e99ad1a26'
REGS={r:(i,4) for i,r in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))}
REGS.update({r:(i,2) for i,r in enumerate(('ax','cx','dx','bx','sp','bp','si','di'))})
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for n in ('function','output','manifest'):ap.add_argument('--'+n,type=Path,required=True)
 a=ap.parse_args();assert sha(a.function)==PIN
 dis=Cs(CS_ARCH_X86,CS_MODE_32);dis.detail=True;ins=list(dis.disasm(a.function.read_bytes(),0x80023653))
 assert sum(i.size for i in ins)==208 and ins[-1].mnemonic=='ret'
 sites={i.address for i in ins};rows={};lines=[]
 def addr(i,o):
  assert o.type==3 and not o.mem.segment
  parts=[hex(o.mem.disp&0xffffffff)+'u']
  for r,mult in ((o.mem.base,1),(o.mem.index,o.mem.scale)):
   if r:parts.append(f's.gp[{REGS[i.reg_name(r)][0]}]*{mult}u')
  return '('+'+'.join(parts)+')'
 def get(i,o):
  if o.type==2:return hex(o.imm&0xffffffff)+'u'
  if o.type==3:return f'r(s.memory,{addr(i,o)},{o.size})'
  r,n=REGS[i.reg_name(o.reg)];return f's.reg({r},{n})'
 def put(i,o,x):
  assert o.type==1;r,n=REGS[i.reg_name(o.reg)];return f's.set({r},{x},{n});'
 for i in ins:
  p=i.address;mn=i.mnemonic;o=list(i.operands);rows[f'{p:08x}']=dict(raw=i.bytes.hex(),mnemonic=mn,operand=i.op_str)
  if mn in ('mov','movzx'):body=put(i,o[0],get(i,o[1]))
  elif mn=='lea':body=put(i,o[0],addr(i,o[1]))
  elif mn=='push':body=f's.push(0x{p:x}u,{get(i,o[0])});'
  elif mn=='pop':body=put(i,o[0],'s.pop()')
  elif mn=='ret':body=f's.pop();s.gp[4]+={o[0].imm}u;return;'
  elif mn in ('test','and','xor','or','add','sub','cmp'):
   x,y=get(i,o[0]),get(i,o[1]);n=o[0].size
   if mn in ('add','sub','cmp'):v=f's.flags.{"plus" if mn=="add" else "minus"}({x},{y},{n})'
   else:v=f's.flags.logic(({x})'+{'and':'&','test':'&','xor':'^','or':'|'}[mn]+f'({y}),{n})'
   body=v+';' if mn in ('test','cmp') else put(i,o[0],v)
  elif mn=='inc':body=put(i,o[0],f's.flags.increment({get(i,o[0])},{o[0].size},false)')
  elif mn=='shl':body=put(i,o[0],f's.flags.shift({get(i,o[0])},{get(i,o[1])},{o[0].size})')
  elif mn=='imul':body=put(i,o[0],f's.flags.product({get(i,o[0])},{get(i,o[1])})')
  elif mn in ('jmp','je','jne','jb'):
   target=o[0].imm;assert target in sites
   prefix={'jmp':'','je':'if(s.flags.zf)','jne':'if(!s.flags.zf)','jb':'if(s.flags.cf)'}[mn]
   body=f'{prefix}goto L{target:x};'
  else:raise AssertionError((hex(p),mn))
  lines.append(f'L{p:x}: if(s.services.instruction)s.services.instruction(0x{p:x}u);{body}')
 text='''// Complete owned modified Complex4627 kernel301, statically translated.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "halo2/source_tu15_status.hpp"
namespace halo2 { namespace {
using namespace physics_world_runtime_detail;
void run_status(WorldState &s) {
'''+ '\n'.join(lines)+'''
throw std::runtime_error("Original owned kernel301 left its complete CFG");
}
}
void source_run_tu15_status_runtime(SourcePhysicsContactMemory &m,
    SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsContactRuntimeServices &services,const SourcePhysicsContactRecordObserver &observe) {
    FpScope scope(fp);WorldState s{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
    try {run_status(s);} catch(...) {abi=s.save();throw;}abi=s.save();
}
}
'''
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(text)
 result=dict(schema=1,parity_target='English TU1.5 caller only',reference='owned modified Complex4627 kernel301',
  function_sha256=PIN,generator_sha256=sha(__file__),generated_sha256=sha(a.output),source_instructions=rows,
  complete_bodies=1,instruction_count=len(ins),physical_xbox_proof=False,production_decoder=False)
 a.manifest.parent.mkdir(parents=True,exist_ok=True);a.manifest.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(sites=len(ins),manifest_sha256=sha(a.manifest))))
if __name__=='__main__':main()
