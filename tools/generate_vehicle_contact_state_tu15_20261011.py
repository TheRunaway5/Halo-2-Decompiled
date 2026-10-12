#!/usr/bin/env python3
"""Statically lower the complete owned TU1.5 contact-state routine."""
import argparse,csv,json
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from research_vehicle_contact_state_tu15_20261011 import sha,ENTRY_ID
REGS={}
for index,(full,word,lo,hi) in enumerate((('eax','ax','al','ah'),('ecx','cx','cl','ch'),('edx','dx','dl','dh'),
 ('ebx','bx','bl','bh'),('esp','sp',None,None),('ebp','bp',None,None),('esi','si',None,None),('edi','di',None,None))):
 for name,n,shift in ((full,4,0),(word,2,0),(lo,1,0),(hi,1,8)):
  if name:REGS[name]=(index,n,shift)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--research',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--manifest',type=Path,required=True);a=p.parse_args()
 report=json.loads(a.research.read_text());assert sha(NEW_DEFAULT)==NEW_PIN
 assert report['extension_tool_sha256']==sha(ROOT/'tools/research_vehicle_contact_state_tu15_20261011.py')
 image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');entries={r['entry']:r['parent_entry_id'] for r in report['tu15_new_owners']}
 starts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(),delimiter='\t')}
 instructions=[code.at(p) for entry in sorted(entries) for p in sorted(_trace(code,entry,starts).code)]
 assert len(instructions)==160
 def addr(i,op):
  assert op.type==3 and not op.mem.segment
  parts=[f'0x{op.mem.disp&0xffffffff:x}u']
  if op.mem.base:parts.append(f's.gp[{REGS[i.reg_name(op.mem.base)][0]}]')
  if op.mem.index:parts.append(f's.gp[{REGS[i.reg_name(op.mem.index)][0]}]*{op.mem.scale}u')
  return '('+'+'.join(parts)+')'
 def get(i,op):
  if op.type==2:return f'0x{op.imm&0xffffffff:x}u'
  if op.type==3:return f'r(s.memory,{addr(i,op)},{op.size})'
  name=i.reg_name(op.reg)
  if name.startswith('xmm'):return f's.xmm[{name[3:]}]'
  index,n,shift=REGS[name];return f's.reg({index},{n},{shift})'
 def put(i,op,value):
  if op.type==3:return f'w(s.memory,s.observe,0x{i.address:x}u,{addr(i,op)},{op.size},{value});'
  name=i.reg_name(op.reg)
  if name.startswith('xmm'):return f's.xmm[{name[3:]}]={value};'
  index,n,shift=REGS[name];return f's.set({index},{value},{n},{shift});'
 def scalar(i,op):return f'_mm_set_ss(f({get(i,op)}))' if op.type==3 else get(i,op)
 def emit(i):
  op=i.operands;mn=i.mnemonic
  if mn=='push':return f's.push(0x{i.address:x}u,{get(i,op[0])});'
  if mn=='pop':return put(i,op[0],'s.pop()')
  if mn=='ret':return f's.pop();s.gp[4]+={op[0].imm if op else 0}u;return;'
  if mn in ('mov','movsx'):
   value=get(i,op[1])
   if mn=='movsx':value=f'sign_extend({value},{op[1].size})'
   return put(i,op[0],value)
  if mn=='lea':return put(i,op[0],addr(i,op[1]))
  if mn=='imul':return put(i,op[0],f's.flags.product({get(i,op[1])},{get(i,op[2])})')
  if mn in ('inc','dec'):return put(i,op[0],f's.flags.increment({get(i,op[0])},{op[0].size},{str(mn=="dec").lower()})')
  if mn=='shl':return put(i,op[0],f's.flags.shift({get(i,op[0])},{get(i,op[1])},{op[0].size})')
  if mn in ('and','test','xor'):
   value=f's.flags.logic(({get(i,op[0])}){"^" if mn=="xor" else "&"}({get(i,op[1])}),{op[0].size})'
   return value+';' if mn=='test' else put(i,op[0],value)
  if mn in ('add','sub','cmp'):
   value=f's.flags.{"plus" if mn=="add" else "minus"}({get(i,op[0])},{get(i,op[1])},{op[0].size})'
   return value+';' if mn=='cmp' else put(i,op[0],value)
  if mn=='xorps':return put(i,op[0],f'_mm_xor_ps({get(i,op[0])},{get(i,op[1])})')
  if mn=='movaps':return put(i,op[0],get(i,op[1]))
  if mn=='movss':
   if op[0].type==3:return put(i,op[0],f'u(_mm_cvtss_f32({get(i,op[1])}))')
   return put(i,op[0],scalar(i,op[1]) if op[1].type==3 else f'_mm_move_ss({get(i,op[0])},{get(i,op[1])})')
  if mn=='subss':return put(i,op[0],f'_mm_sub_ss({get(i,op[0])},{scalar(i,op[1])})')
  if mn=='comiss':return f's.flags.scalar_compare({get(i,op[0])},{scalar(i,op[1])},false);'
  if mn in ('je','jne','jbe','jge','jl','jle','jg'):
   condition={'je':'s.flags.zf','jne':'!s.flags.zf','jbe':'s.flags.cf||s.flags.zf','jge':'s.flags.sf==s.flags.of','jl':'s.flags.sf!=s.flags.of','jle':'s.flags.zf||(s.flags.sf!=s.flags.of)','jg':'!s.flags.zf&&(s.flags.sf==s.flags.of)'}[mn]
   return f'if({condition})goto L{op[0].imm:x};'
  if mn=='jmp':return f'goto L{op[0].imm:x};'
  if mn=='call':
   assert op[0].type==2;target=op[0].imm
   call=f'run_{target:x}(s);' if target in entries else f's.required(0x{i.address:x}u,0x{target:x}u,false);'
   return f's.push(0x{i.address:x}u,0x{i.address+i.size:x}u);'+call
  raise AssertionError((hex(i.address),mn,i.op_str))
 bodies=[]
 for entry in sorted(entries):
  body=[code.at(p) for p in sorted(_trace(code,entry,starts).code)]
  targets={i.operands[0].imm for i in body if i.mnemonic.startswith('j')}
  lines=[]
  for i in body:
   label=f'L{i.address:x}: ' if i.address in targets else ''
   lines.append(label+f'if(s.services.instruction)s.services.instruction(0x{i.address:x}u); '+emit(i))
  bodies.append(f'void run_{entry:x}(MathState &s) {{\n'+'\n'.join(lines)+'\nthrow std::runtime_error("TU1.5 contact state fell outside authenticated CFG");\n}')
 text='''// Generated from complete owned TU1.5 instructions. No runtime decoder.
#include "halo2/physics_vehicle_contact_state.hpp"
#include "physics_vehicle_motion_math_internal.hpp"
namespace halo2 {
namespace {
using namespace physics_vehicle_motion_math_detail;
std::uint32_t sign_extend(std::uint32_t x,unsigned n) {
 if(n==1)return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(x))));
 if(n==2)return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(x))));
 throw std::runtime_error("Unverified TU1.5 sign-extension width");
}
'''+''.join(f'void run_{entry:x}(MathState &);\n' for entry in sorted(entries))+'\n\n'.join(bodies)+'''
} // namespace
void source_run_physics_vehicle_contact_state_runtime(M &m,std::uint32_t entry,
 SourcePhysicsContactRuntimeAbi &abi,FP &fp,const SourcePhysicsContactRuntimeServices &services,const O &observe) {
 auto bound=services;
 bound.required_owner=[&](M &memory,const SourcePhysicsContactRuntimeCall &call,
  SourcePhysicsContactRuntimeAbi &state,const O &observer) {
'''+''.join(f'if(call.target==0x{entry:x}u){{source_run_physics_vehicle_contact_state_runtime(memory,0x{entry_id:x}u,state,fp,services,observer);return;}}\n' for entry,entry_id in sorted(entries.items()))+'''
  if(!services.required_owner)throw std::runtime_error("Required original contact runtime callee unavailable");
  services.required_owner(memory,call,state,observer);
 };
 switch(entry) {
'''+''.join(f'case 0x{x:x}u:\n' for x in report['parent_native_entries'])+'''
  try {source_run_physics_vehicle_motion_math_runtime(m,entry,abi,SourcePhysicsContactImage::TitleUpdate15,fp,bound,observe);}
  catch(...) {if(abi.floating_point)fp=*abi.floating_point;throw;}
  if(abi.floating_point)fp=*abi.floating_point;return;
'''+''.join(f'case 0x{entry_id:x}u:break;\n' for entry_id in sorted(entries.values()))+'''
 default:throw std::runtime_error("Required TU1.5 vehicle entry unavailable");
 }
 FpScope scope(fp);MathState s{m,observe,bound,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
 try {switch(entry) {
'''+''.join(f'case 0x{entry_id:x}u:run_{entry:x}(s);break;\n' for entry,entry_id in sorted(entries.items()))+'''
 }}catch(...) {abi=s.save();fp={control_word(),_mm_getcsr()};throw;}
 abi=s.save();fp={control_word(),_mm_getcsr()};
}
} // namespace halo2
'''
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(text)
 manifest=dict(schema=1,parity_target='English TU1.5 only',generator_sha256=sha(__file__),research_sha256=sha(a.research),
  original_sha256=NEW_PIN,generated_cpp_sha256=sha(a.output),native_entries={f'{p:08x}':q for p,q in entries.items()},
  reused_parent_native_entries=report['parent_native_entries'],lowered_instructions={f'{i.address:08x}':dict(bytes=i.bytes.hex(),mnemonic=i.mnemonic) for i in instructions},
  production_opcode_decoder=False,production_original_code_execution=False,new_recovery_requires_older_image=False)
 a.manifest.parent.mkdir(parents=True,exist_ok=True);a.manifest.write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(instructions=160,generated_cpp_sha256=sha(a.output),manifest_sha256=sha(a.manifest))))
if __name__=='__main__':main()
