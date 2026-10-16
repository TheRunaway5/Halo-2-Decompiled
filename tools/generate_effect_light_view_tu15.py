"""Isolated complete OG Xbox TU1.5 native light-view math/iteration owner."""
import argparse
from pathlib import Path
from types import SimpleNamespace
import json
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--directory',type=Path,required=True)
parser.add_argument('--output',type=Path,default=Path('src/source_effect_light_view_tu15_generated.cpp'))
args=parser.parse_args();BASE=args.directory;BASE.mkdir(parents=True,exist_ok=True)
ENTRIES=(0xc2a80,0xc3300,0xc35d0,0x16ea60)
assert sha(NEW_DEFAULT)==NEW_PIN
image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');sites={};instructions={};addresses={};routines=[]
for entry in ENTRIES:
 body=_trace(code,entry,set(ENTRIES));assert body.code and not body.tables
 sites[f'{entry:08x}']=sorted(body.code);routines.append(dict(entry=entry,tables=[],byte_tables=[]))
 for pc in sorted(body.code):
  ins=code.at(pc);assert image.read(pc,ins.size)==bytes(ins.bytes)
  instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+ins.size]=pc+ins.size
  for op in ins.operands:
   if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
   elif op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
calls={0xc35dc:0x16ea60,0xc362c:0xc3300,0xc334b:0xc2a80,0xc3375:0xc2a80,0xc339c:0xc2a80,0xc33c3:0xc2a80,0xc33f0:0xc2a80}
for pc,target in calls.items():
 ins=code.at(pc);assert ins.mnemonic=='call' and ins.operands[0].imm==target
paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{p:08x}':p for p in ENTRIES},body_sites=sites,instruction_map=instructions,address_map={f'{p:08x}':q for p,q in addresses.items()},routines=routines)
correspondence=BASE/'bodies.json';correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
output=args.output;manifest=BASE/'generation.json'
generate(SimpleNamespace(output=output,manifest=manifest,correspondence=correspondence,additional_entries=Path(__file__)),paired)
s=output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_effect_light_view_tu15.py').replace('halo2/physics_live_support_tu15.hpp','halo2/source_effect_light_view_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_effect_light_view_tu15').replace('source_physics_live_support_tu15_owns_entry','source_effect_light_view_tu15_owns_entry');output.write_text(s)
report=json.loads(manifest.read_text());report.update(generated_cpp_sha256=sha(output),module_generator_sha256=sha(Path(__file__)),exact_source_callers={f'{p:08x}':f'{q:08x}'for p,q in calls.items()},scope='Complete original light-view bodies. World routes onlyC2A80/C3300/C35D0; existing16EA60 owner unchanged. No main query/native cache join.')
manifest.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps({'entries':len(ENTRIES),'complete_sites':len(instructions),'generated_sha256':sha(output)}))
