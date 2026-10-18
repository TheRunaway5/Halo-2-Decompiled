#!/usr/bin/env python3
"""Recover complete original TU1.5 audio frame and listener update callers.

Readiness, registered players, observer state and real SDK owners are required.
Recovery never creates availability, listener activation or a platform result.
"""
import argparse,json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha
DEPENDENCIES=((0x12a0d1,0x120060),(0x12a0e3,0x120310),
 (0x128715,0x129500),(0x12874e,0x12ab30),(0x128753,0x12ade0),
 (0x128749,0x12bbe0),(0x128792,0x12bc10),(0x1287f4,0x12be90),
 (0x1287f9,0x12bf00),(0x1286d2,0x12cbc0),(0x128797,0x150900),
 (0x128733,0x2206d0),(0x1286e9,0x2207a0),(0x128744,0x221af0),
 (0x12868b,0x225610),(0x12880d,0x225890),(0x128709,0x225d70),
 (0x1286ee,0x225fb0),(0x12a7f3,0x2271a0),(0x128728,0x228310),
 (0x1287ac,0x228480),(0x12964f,0x12e9a0))
ROUTING_ENTRIES=(0x127ad0,0x127b70,0x12c5b0,0x12ccc0,0x12d6f0,
 0x12d730,0x12d840,0x12db00,0x224c40,0x226490,0x226c00,
 0x2b8650,0x2b8b10,0x2b8f80,0x2b92e0,0x2b9330)
ROUTING_CALLS=((0x12c649,0x12d6f0),(0x12c662,0x127b70),
 (0x12c775,0x12d840),(0x12d88c,0x12d730),(0x12d79b,0x127ad0),
 (0x12d80b,0x12db00),(0x2265a9,0x224c40),(0x226bec,0x226c00),
 (0x2b9307,0x2b8b10),(0x2b9317,0x2b8f80))
ENTRIES=(0x128650,0x129f10)+tuple(target for _,target in DEPENDENCIES)+ROUTING_ENTRIES
CALLS=((0x12e16e,0x128650),(0x12e18c,0x128650),(0x12872e,0x129f10))+DEPENDENCIES+ROUTING_CALLS
INTEGER_ADD_HELPER=r'''
long double native_audio_integer_add(long double value,std::int32_t integer,std::uint16_t &status) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double result;
    asm volatile("fiaddl %3\n\tfnstsw %1":"=t"(result),"=m"(status):"0"(value),"m"(integer));
    return result;
#else
    throw std::runtime_error("Required original audio FIADD32 backend unavailable");
#endif
}
'''
def audio_instruction_factory(*,get,**unused):
 def emit(i):
  if i.address!=0x2221bb:return None
  assert i.mnemonic=='fiadd' and i.bytes.hex()=='da8284000000'
  assert len(i.operands)==1 and i.operands[0].type==3 and i.operands[0].size==4
  return ('{std::uint16_t status;s.fp.st(0)=native_audio_integer_add(s.fp.st(0),'
   f'si({get(i,i.operands[0])}),status);s.fp.status=(s.fp.status&~0x200u)|(status&0x23fu);}}')
 return emit
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--directory',type=Path,required=True);a=p.parse_args()
 if a.output.exists() or a.directory.exists():raise ValueError('Preserve previous audio recovery')
 if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Expected original English TU1.5 executable')
 x=Xbe(NEW_DEFAULT);code=_Code(x,'.text')
 for pc,target in CALLS:
  i=code.at(pc)
  if i.mnemonic!='call' or i.operands[0].imm!=target:raise ValueError('Original audio caller differs')
 sites,instructions,addresses,routines={},{},{},[];a.directory.mkdir(parents=True)
 for entry in ENTRIES:
  body=_trace(code,entry,set(ENTRIES));assert body.code and all(width==4 for _,width,_ in body.tables)
  sites[f'{entry:08x}']=sorted(body.code);routines.append(dict(entry=entry,tables=[dict(disc=q,count=n)for q,_,n in body.tables],byte_tables=[]));lines=[]
  for pc in sorted(body.code):
   i=code.at(pc);assert x.read(pc,i.size)==bytes(i.bytes);lines.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
   for op in i.operands:
    if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
    elif op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
  (a.directory/f'{entry:08x}.asm').write_text(''.join(lines))
 paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{q:08x}':q for q in ENTRIES},body_sites=sites,instruction_map=instructions,address_map={f'{q:08x}':v for q,v in addresses.items()},routines=routines);f=a.directory/'bodies.json';f.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
 generate(SimpleNamespace(output=a.output,manifest=a.directory/'generation.json',correspondence=f,additional_entries=Path(__file__)),paired,
  extra_instruction_factory=audio_instruction_factory,extra_preamble=INTEGER_ADD_HELPER)
 s=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_campaign_audio_tu15.py').replace('halo2/physics_live_support_tu15.hpp','halo2/source_campaign_audio_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_campaign_audio_tu15').replace('source_physics_live_support_tu15_owns_entry','source_campaign_audio_tu15_owns_entry');a.output.write_text(s)
 vtable=0x4633fc;methods={0:0x2b8650,0xc:0x2b92e0,0x10:0x2b9330}
 assert x.read(0x48b8a0,4)==vtable.to_bytes(4,'little')
 for offset,entry in methods.items():assert x.read(vtable+offset,4)==entry.to_bytes(4,'little')
 report=json.loads((a.directory/'generation.json').read_text());report.update(generated_cpp_sha256=sha(a.output),module_generator_sha256=sha(Path(__file__)),original_call_sites={f'{p:08x}':f'{q:08x}'for p,q in CALLS},original_routing_vtable={f'{vtable+offset:08x}':f'{entry:08x}'for offset,entry in methods.items()},scope=__doc__);(a.directory/'generation.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(len(ENTRIES),len(instructions))
if __name__=='__main__':main()
