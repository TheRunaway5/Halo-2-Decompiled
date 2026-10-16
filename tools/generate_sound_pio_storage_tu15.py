#!/usr/bin/env python3
"""Lower unchanged38E0CE and declared804/808 component suffix CPU code.

Native owners must suspend before unsupported methods. This generator does
not supply FIFO space, hardware preimages, processing or a constructor result.
"""
import argparse,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--manifest',type=Path,required=True);a=p.parse_args()
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
 code=_Code(image,'DSOUND');entries=[0x38e0ce,0x38e159];bodies={e:_trace(code,e,set(entries)) for e in entries}
 instructions,addresses={},{}
 for pc in sorted(set().union(*(b.code for b in bodies.values()))):
  op=code.at(pc);assert bytes(op.bytes)==image.read(pc,op.size);instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+op.size]=pc+op.size
  for operand in op.operands:
   if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
   if operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
 paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{e:08x}':e for e in entries},body_sites={f'{e:08x}':sorted(b.code) for e,b in bodies.items()},instruction_map=instructions,address_map={f'{p:08x}':v for p,v in addresses.items()},routines=[dict(entry=e,tables=[],byte_tables=[]) for e in entries])
 a.correspondence=a.manifest.with_name('pio-source-bodies.json');a.correspondence.parent.mkdir(parents=True,exist_ok=True);a.correspondence.write_text(json.dumps(paired,indent=2)+'\n');a.additional_entries=Path(__file__)
 def factory(**helpers):
  def action(i):
   op=list(i.operands)
   if i.mnemonic=='mov' and op[0].type==3 and not op[0].mem.base and not op[0].mem.index and (op[0].mem.disp&0xffffffff)>=0xfe800000:
    assert op[0].size==4
    return f'pio_action(s,0x{i.address:x}u,{helpers["addr"](i,op[0])},{helpers["get"](i,op[1])});'
  return action
 preamble='''
void pio_action(WorldState& s,std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
    auto* owner=dynamic_cast<SourceSoundPioActionMemory*>(&s.memory);
    if(!owner)throw std::runtime_error("Native PIO action owner is required");
    std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));
    owner->write_pio_action(pc,address,bytes);
    if(s.observe)s.observe({pc,address,{},std::vector<std::byte>(bytes.begin(),bytes.end())});
}
'''
 generate(a,paired,code_override=code,extra_instruction_factory=factory,extra_preamble=preamble)
 text=a.output.read_text().replace('halo2/physics_live_support_tu15.hpp','halo2/source_sound_pio_storage_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_sound_pio_storage_tu15').replace('source_physics_live_support_tu15_owns_entry','source_sound_pio_storage_tu15_owns_entry');a.output.write_text(text)
 report=json.loads(a.manifest.read_text());report.update(pio_generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),generated_cpp_sha256=hashlib.sha256(a.output.read_bytes()).hexdigest(),action_preimage='unknown; no added hardware load',native_constructor_return_supplied=False);a.manifest.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(dict(entries=len(entries),sites=len(instructions))))
if __name__=='__main__':main()
