#!/usr/bin/env python3
"""Static full-ABI lowering of the 22-site raw heap producer only.

Existing constructor/allocator bodies remain in the shared world graph. No
production instruction decoding, original execution or captured storage.
"""
import argparse, hashlib, json
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
REPORT_PIN='579a526dba8c9fb6e090b78c79c906487935b1d74b9c4b8ef601b22d71277de4'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for s in ('correspondence','output','manifest'):ap.add_argument('--'+s,type=Path,required=True)
 a=ap.parse_args();assert sha(a.correspondence)==REPORT_PIN
 d=json.loads(a.correspondence.read_text());old=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
 assert sha(old)==PIN and sha(NEW_DEFAULT)==NEW_PIN
 c=_Code(Xbe(old),'.text');ins=[c.at(p)for p in sorted(_trace(c,0x1482a0,{0x1482a0}).code)]
 assert len(ins)==22 and [i.bytes.hex()for i in ins]==[r['old_bytes']for r in d['instructions']]
 mapping={int(p,16):q for p,q in d['instruction_map'].items()};mapping.update({int(p,16):q for p,q in d['address_map'].items()})
 registers={'eax':0,'ecx':1,'edx':2,'esi':6};used=set()
 def imm(p):
  p&=0xffffffff
  if p in mapping:used.add(p);return f'selected(0x{p:x}u,s.image)'
  return f'0x{p:x}u'
 def address(i,o):
  assert o.type==3 and not o.mem.index and not o.mem.segment
  return '('+imm(o.mem.disp)+(f'+s.gp[{registers[i.reg_name(o.mem.base)]}]'if o.mem.base else '')+')'
 def get(i,o):
  if o.type==2:return imm(o.imm)
  if o.type==1:return f's.gp[{registers[i.reg_name(o.reg)]}]'
  return f'r(s.memory,{address(i,o)},4)'
 def put(i,o,value):
  if o.type==1:return get(i,o)+'='+value+';'
  return f'w(s.memory,s.observe,0x{i.address:x}u,{address(i,o)},4,{value});'
 lines=[]
 for i in ins:
  p,m,o=i.address,i.mnemonic,list(i.operands)
  if m=='push':s=f's.push(0x{p:x}u,{get(i,o[0])});'
  elif m=='pop':s=put(i,o[0],'s.pop()')
  elif m=='mov':s=put(i,o[0],get(i,o[1]))
  elif m=='lea':s=put(i,o[0],address(i,o[1]))
  elif m=='xor':s=put(i,o[0],f's.flags.logic({get(i,o[0])}^{get(i,o[1])},4)')
  elif m=='and':s=put(i,o[0],f's.flags.logic({get(i,o[0])}&{get(i,o[1])},4)')
  elif m=='sub':s=put(i,o[0],f's.flags.minus({get(i,o[0])},{get(i,o[1])},4)')
  elif m=='test':s=f's.flags.logic({get(i,o[0])}&{get(i,o[1])},4);'
  elif m=='jne':s=f'if(!s.flags.zf)goto L{o[0].imm:x};'
  elif m=='ret':s='s.pop();return;'
  elif m=='call':s=f's.push(0x{p:x}u,{imm(p+i.size)});s.required(0x{p:x}u,{imm(o[0].imm)},false);'
  else:raise AssertionError(m)
  label=f'L{p:x}: 'if p==0x1482c6 else ''
  lines.append(label+f'if(s.services.instruction)s.services.instruction(0x{p:x}u);'+s)
 pre='''// Generated from complete authenticated source; no production decoder.
#include "physics_world_runtime_internal.hpp"
#include "halo2/physics_main_heap_runtime.hpp"
namespace halo2 { namespace {
using namespace physics_world_runtime_detail;
'''
 pre+='std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image){if(image==SourcePhysicsContactImage::MapPack11)return p;switch(p){'+''.join(f'case 0x{p:x}u:return 0x{mapping[p]:x}u;'for p in sorted(used))+'default:throw std::runtime_error("Unbound original main heap operand");}}\n'
 pre+='void producer(WorldState &s){\n'+'\n'.join(lines)+'\n}\n} // namespace\n'
 pre+='''void source_run_physics_main_heap_runtime(SourcePhysicsContactMemory &m,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,SourcePhysicsContactImage image,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
  if(entry!=0x1482a0u || (image!=SourcePhysicsContactImage::MapPack11 && image!=SourcePhysicsContactImage::TitleUpdate15))
    throw std::runtime_error("Unverified original main heap producer entry/image");
  FpScope scope(fp);WorldState s{m,observe,services,image};s.load(abi);
  try {producer(s);} catch(...) {abi=s.save();throw;}
  abi=s.save();
}
} // namespace halo2
'''
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(pre)
 report=dict(schema=1,generator_sha256=sha(__file__),correspondence_sha256=sha(a.correspondence),generated_cpp_sha256=sha(a.output),
  instructions=22,complete_bodies=1,selected_operands={f'{p:08x}':mapping[p]for p in sorted(used)},production_decoder=False)
 a.manifest.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(sha(a.manifest))
if __name__=='__main__':main()
