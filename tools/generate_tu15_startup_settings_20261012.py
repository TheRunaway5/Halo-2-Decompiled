#!/usr/bin/env python3
"""Lower the complete English TU1.5 settings wrapper and import thunk only."""
import argparse,hashlib,json
from pathlib import Path
from oracle_tu15_startup_records_20261011 import ROOT,NEW_DEFAULT,NEW_PIN,Xbe,_Code,_trace
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for name in ('output','manifest'):ap.add_argument('--'+name,type=Path,required=True)
 a=ap.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
 image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');entries={0x2e0fe2,0x2e101c};bodies=[];rows={}
 regs={'eax':0,'esp':4,'ebp':5}
 def get(i,o):
  if o.type==1:return 's.gp['+str(regs[i.reg_name(o.reg)])+']'
  if o.type==2:return hex(o.imm&0xffffffff)+'u'
  assert o.type==3 and not o.mem.segment and not o.mem.index and o.size==4
  base='s.gp['+str(regs[i.reg_name(o.mem.base)])+']+' if o.mem.base else ''
  return 'r(s.memory,('+base+hex(o.mem.disp&0xffffffff)+'u),4)'
 for entry in sorted(entries):
  ins=[code.at(p) for p in sorted(_trace(code,entry,entries).code)]
  lines=['static void run_'+hex(entry)[2:]+'(WorldState &s) {']
  for i in ins:
   p=i.address;o=list(i.operands);assert image.read(p,i.size)==i.bytes
   rows[f'{p:08x}']=dict(raw=i.bytes.hex(),mnemonic=i.mnemonic,operand=i.op_str)
   if i.mnemonic=='push':body=f's.push(0x{p:x}u,{get(i,o[0])});'
   elif i.mnemonic=='pop':body=get(i,o[0])+'=s.pop();'
   elif i.mnemonic=='mov':body=get(i,o[0])+'='+get(i,o[1])+';'
   elif i.mnemonic=='ret':body='s.pop();s.gp[4]+='+str(o[0].imm if o else 0)+'u;return;'
   elif i.mnemonic=='jmp':body=f's.required(0x{p:x}u,{get(i,o[0])},true);return;'
   elif i.mnemonic=='call':
    body=f's.push(0x{p:x}u,0x{p+i.size:x}u);'
    body+='run_2e101c(s);' if o[0].type==2 and o[0].imm==0x2e101c else f's.required(0x{p:x}u,{get(i,o[0])},false);'
   else:raise AssertionError((hex(p),i.mnemonic))
   lines.append(f'if(s.services.instruction)s.services.instruction(0x{p:x}u);'+body)
  lines.append('throw std::runtime_error("Original TU1.5 settings fell outside complete CFG");\n}')
  bodies.append('\n'.join(lines))
 assert len(rows)==13
 text='''// Generated from unchanged English TU1.5 only; no production decoder.
#include "physics_world_runtime_internal.hpp"
#include "halo2/source_tu15_settings.hpp"
namespace halo2 { namespace {
using namespace physics_world_runtime_detail;
static void run_2e101c(WorldState &);
'''+ '\n'.join(bodies)+'''
} // namespace
void source_run_tu15_settings_runtime(SourcePhysicsContactMemory &m,
    SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsContactRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
    FpScope scope(fp);WorldState s{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
    try {run_2e0fe2(s);} catch(...) {abi=s.save();throw;}
    abi=s.save();
}
} // namespace halo2
'''
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(text)
 report=dict(schema=1,parity_target='English TU1.5 only',original_sha256=NEW_PIN,
  generator_sha256=sha(__file__),generated_cpp_sha256=sha(a.output),source_instructions=rows,
  complete_bodies=2,instruction_count=13,older_image_used=False,production_decoder=False)
 a.manifest.parent.mkdir(parents=True,exist_ok=True);a.manifest.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(instructions=13,generated_sha256=sha(a.output),manifest_sha256=sha(a.manifest))))
if __name__=='__main__':main()
