#!/usr/bin/env python3
"""Lower complete TU1.5 DSP image and scratch-table CPU bodies.

Original allocation, address relocation, request publication and polling remain
in their real callers. Native resources and GP acknowledgement are required;
no device result or completed request is supplied by the generator.
"""
import argparse,json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha
ENTRIES=(0x3879b3,0x388275,0x388294,0x388309,0x38850a,0x388576,
         0x38dd0c,0x38dd26,0x38dd54,0x38de07,0x38de59,
         0x38de81,0x38df59,0x38dfd2,0x38dfff,0x38e668,0x38e6df,0x38f544,0x38f5f3)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--directory',type=Path,required=True);a=p.parse_args()
 if a.output.exists()or a.directory.exists():raise ValueError('Preserve previous DSP decoder evidence')
 assert sha(NEW_DEFAULT)==NEW_PIN;x=Xbe(NEW_DEFAULT);code=_Code(x,'DSOUND');a.directory.mkdir(parents=True)
 sites={};instructions={};addresses={};routines=[]
 for entry in ENTRIES:
  body=_trace(code,entry,set(ENTRIES));assert body.code and not body.tables;sites[f'{entry:08x}']=sorted(body.code);routines.append(dict(entry=entry,tables=[],byte_tables=[]));lines=[]
  for pc in sorted(body.code):
   i=code.at(pc);assert bytes(i.bytes)==x.read(pc,i.size);lines.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
   for op in i.operands:
    if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
    elif op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
  (a.directory/f'{entry:08x}.asm').write_text(''.join(lines))
 packet=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{e:08x}':e for e in ENTRIES},body_sites=sites,instruction_map=instructions,address_map={f'{v:08x}':v for v in addresses},routines=routines)
 path=a.directory/'bodies.json';path.write_text(json.dumps(packet,indent=2,sort_keys=True)+'\n')
 generate(SimpleNamespace(output=a.output,manifest=a.directory/'generation.json',correspondence=path,additional_entries=Path(__file__)),packet,code_override=code)
 s=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_sound_dsp_image_tu15.py').replace('halo2/physics_live_support_tu15.hpp','halo2/source_sound_dsp_image_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_sound_dsp_image_tu15').replace('source_physics_live_support_tu15_owns_entry','source_sound_dsp_image_tu15_owns_entry')
 # A real native resource store has no byte-backed MMIO preimage. Preserve
 # the original store itself without the generic observer's extra read.
 # Declared offline byte banks retain their existing before/after observer.
 helper='''
static void dsp_store(M& memory,const O& observe,std::uint32_t site,
    std::uint32_t address,unsigned bytes,std::uint64_t value){
    if(auto* owner=dynamic_cast<SourceSoundDspImageMmioTu15*>(&memory)){
        if(bytes!=4||value>0xffffffffu)throw std::runtime_error("Invalid original GP resource store width");
        owner->write_sound_dsp_mmio(site,address,std::uint32_t(value));return;
    }
    w(memory,observe,site,address,bytes,value);
}
'''
 marker='using namespace physics_world_runtime_detail;';assert s.count(marker)==1;s=s.replace(marker,marker+helper,1)
 for site in (0x38dde3,0x38dded,0x38ddf4,0x38ddfb,0x38de4d,0x38df31):
  old=f'w(s.memory,s.observe,0x{site:x}u,';assert s.count(old)==1;s=s.replace(old,f'dsp_store(s.memory,s.observe,0x{site:x}u,',1)
 a.output.write_text(s)
 report=json.loads((a.directory/'generation.json').read_text());report.update(generated_cpp_sha256=sha(a.output),module_generator_sha256=sha(Path(__file__)),scope=__doc__);(a.directory/'generation.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(f'{len(ENTRIES)} bodies, {len(instructions)} original instructions')
if __name__=='__main__':main()
