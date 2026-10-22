#!/usr/bin/env python3
"""Compile all authenticated physical-stage/partition CFG sites statically."""
import argparse,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
REPORT_PIN='4e81e7a5fd05edae4d24a35152896df4191aed827da55ba1d87b559bcb8e540f'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
REGS={}
for k,(full,lo)in enumerate((('eax','al'),('ecx','cl'),('edx','dl'),('ebx','bl'),('esp',None),('ebp',None),('esi',None),('edi',None))):
 REGS[full]=(k,4)
 if lo:REGS[lo]=(k,1)
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for s in ('correspondence','output','manifest'):ap.add_argument('--'+s,type=Path,required=True)
 a=ap.parse_args();assert sha(a.correspondence)==REPORT_PIN;d=json.loads(a.correspondence.read_text())
 oldpath=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03';assert sha(oldpath)==PIN and sha(NEW_DEFAULT)==NEW_PIN
 code=_Code(Xbe(oldpath),'.text');entries={int(p,16):q for p,q in d['entries'].items()}
 mapping={int(p,16):q for p,q in d['instruction_map'].items()};mapping.update({int(p,16):q for p,q in d['address_map'].items()});used=set();bodies=[]
 def imm(p):
  p&=0xffffffff
  if p in mapping:used.add(p);return f'selected(0x{p:x}u,s.image)'
  return f'0x{p:x}u'
 def addr(i,o):
  assert o.type==3 and not o.mem.segment
  parts=[imm(o.mem.disp)]
  if o.mem.base:parts.append(f's.gp[{REGS[i.reg_name(o.mem.base)][0]}]')
  if o.mem.index:parts.append(f's.gp[{REGS[i.reg_name(o.mem.index)][0]}]*{o.mem.scale}u')
  return '('+'+'.join(parts)+')'
 def get(i,o):
  if o.type==2:return imm(o.imm)
  if o.type==3:return f'r(s.memory,{addr(i,o)},{o.size})'
  k,n=REGS[i.reg_name(o.reg)];return f's.reg({k},{n},0)'
 def put(i,o,value):
  if o.type==3:return f'w(s.memory,s.observe,0x{i.address:x}u,{addr(i,o)},{o.size},{value});'
  k,n=REGS[i.reg_name(o.reg)];return f's.set({k},{value},{n},0);'
 for entry in sorted(entries):
  ins=[code.at(p)for p in sorted(_trace(code,entry,set(entries)).code)]
  targets={i.operands[0].imm for i in ins if i.mnemonic.startswith('j')and i.operands[0].type==2};lines=[]
  for i in ins:
   p,m,o=i.address,i.mnemonic,list(i.operands)
   if m=='push':s=f's.push(0x{p:x}u,{get(i,o[0])});'
   elif m=='pop':s=put(i,o[0],'s.pop()')
   elif m=='mov':s=put(i,o[0],get(i,o[1]))
   elif m=='lea':s=put(i,o[0],addr(i,o[1]))
   elif m in ('xor','and','or'):s=put(i,o[0],f's.flags.logic(({get(i,o[0])})'+dict(xor='^',and_='&',or_='|')[m if m=='xor'else m+'_']+f'({get(i,o[1])}),{o[0].size})')
   elif m in ('cmp','sub','add'):
    value=f's.flags.{"plus"if m=="add"else"minus"}({get(i,o[0])},{get(i,o[1])},{o[0].size})'
    s=value+';'if m=='cmp'else put(i,o[0],value)
   elif m=='test':s=f's.flags.logic({get(i,o[0])}&{get(i,o[1])},{o[0].size});'
   elif m in ('je','jne','jl'):
    cond=dict(je='s.flags.zf',jne='!s.flags.zf',jl='s.flags.sf!=s.flags.of')[m];s=f'if({cond})goto L{o[0].imm:x};'
   elif m=='jmp':
    s=f'goto L{o[0].imm:x};'if o[0].type==2 else f's.required(0x{p:x}u,{get(i,o[0])},true);return;'
   elif m=='ret':s=f's.pop();s.gp[4]+={o[0].imm if o else 0}u;return;'
   elif m=='call':
    s=f's.push(0x{p:x}u,{imm(p+i.size)});'
    s+=f'run_{o[0].imm:x}(s);'if o[0].type==2 and o[0].imm in entries else f's.required(0x{p:x}u,{get(i,o[0])},false);'
   elif m=='rep stosd':s=f'while(s.gp[1]){{w(s.memory,s.observe,0x{p:x}u,s.gp[7],4,s.gp[0]);s.gp[7]+=(s.other_flags&1024u)?0xfffffffcu:4u;--s.gp[1];if(s.services.instruction)s.services.instruction(0x{p:x}u);}}'
   else:raise AssertionError((hex(p),m))
   lines.append((f'L{p:x}: 'if p in targets else '')+f'if(s.services.instruction)s.services.instruction(0x{p:x}u);'+s)
  bodies.append(f'static void run_{entry:x}(WorldState &s){{\n'+'\n'.join(lines)+'\n}\n')
 pre='''// Generated complete physical stage and partition source bodies.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "halo2/physics_physical_storage_runtime.hpp"
namespace halo2 {namespace {using namespace physics_world_runtime_detail;
'''
 pre+='std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image){if(image==SourcePhysicsContactImage::MapPack11)return p;switch(p){'+''.join(f'case 0x{p:x}u:return 0x{mapping[p]:x}u;'for p in sorted(used))+'default:throw std::runtime_error("Unbound physical stage operand");}}\n'
 pre+=''.join(f'static void run_{p:x}(WorldState &);\n'for p in entries)+'\n'.join(bodies)+'}\n'
 pre+='''void source_run_physics_physical_storage_runtime(SourcePhysicsContactMemory &m,
 std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,SourcePhysicsContactImage image,
 SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices &services,
 const SourcePhysicsContactRecordObserver &observe){
 if(image!=SourcePhysicsContactImage::MapPack11 && image!=SourcePhysicsContactImage::TitleUpdate15)
  throw std::runtime_error("Unverified physical stage image");
 FpScope scope(fp);WorldState s{m,observe,services,image};s.load(abi);
 try{switch(entry){
'''+''.join(f'case 0x{p:x}u:run_{p:x}(s);break;\n'for p in entries)+'''default:throw std::runtime_error("Unverified physical stage entry");}}
 catch(...){abi=s.save();throw;}abi=s.save();}
} // namespace halo2
'''
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(pre)
 report=dict(schema=1,generator_sha256=sha(__file__),correspondence_sha256=REPORT_PIN,generated_cpp_sha256=sha(a.output),
  complete_bodies=4,instructions=86,selected_operands={f'{p:08x}':mapping[p]for p in sorted(used)},production_decoder=False)
 a.manifest.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(sha(a.manifest))
if __name__=='__main__':main()
