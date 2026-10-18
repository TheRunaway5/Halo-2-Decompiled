import sys,json,hashlib,argparse
from pathlib import Path
sys.dont_write_bytecode=True
root=Path(__file__).resolve().parents[1];sys.path.insert(0,str(root/'tools'))
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate
parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,default=root/'src/source_unit_view_reset_tu15_generated.cpp');parser.add_argument('--manifest',type=Path,default=root/'tools/source_unit_view_reset_tu15_generated.json');args=parser.parse_args()
import tempfile
workspace=tempfile.TemporaryDirectory(prefix='halo2-unit-view-reset-');out=Path(workspace.name)
code=_Code(Xbe(NEW_DEFAULT),'.text');entries={0x158690,0xbcdf0,0x246cf0};functions={};pending=list(entries)
while pending:
 entry=pending.pop();fn=_trace(code,entry,entries);functions[entry]=fn
 for p in fn.code:
  i=code.at(p)
  if i.mnemonic=='call'and i.operands and i.operands[0].type==2:
   target=i.operands[0].imm
   if 0x246000<=target<0x248000 and target not in entries:entries.add(target);pending.append(target)
 assert len(entries)<40,'Camera closure exceeded its declared scope'
# Retrace with every recovered callee boundary declared.
functions={p:_trace(code,p,entries)for p in sorted(entries)}
body_sites={f'{p:08x}':sorted(fn.code)for p,fn in functions.items()};sites=set().union(*(set(fn.code)for fn in functions.values()))
paired={'image':'English TU1.5','original_sha256':NEW_PIN,'entries':{f'{p:08x}':p for p in sorted(entries)},'instruction_map':{f'{p:08x}':p for p in sorted(sites)},'address_map':{f'{p:08x}':p for p in sorted(entries)},'body_sites':body_sites,'routines':[{'entry':p,'tables':[],'byte_tables':[]}for p in sorted(entries)]}
correspondence=out/'tu15-bodies.json';correspondence.write_text(json.dumps(paired,indent=2)+'\n');additions=out/'entries.json';additions.write_text(json.dumps({'image_sha256':NEW_PIN,'entries':sorted(entries)},indent=2)+'\n')
a=argparse.Namespace(output=args.output,manifest=args.manifest,correspondence=correspondence,additional_entries=additions)
generate(a,paired)
s=a.output.read_text().replace('#include "physics_world_runtime_internal.hpp"','#include "physics_world_runtime_internal.hpp"\n#include "halo2/source_physics_entry_observation.hpp"').replace('tools/generate_physics_live_support_tu15.py','tools/generate_source_unit_view_reset_tu15.py').replace('source_run_physics_live_support_tu15','source_run_unit_view_reset_tu15').replace('source_physics_live_support_tu15_owns_entry','source_unit_view_reset_tu15_owns_entry').replace('#include "halo2/physics_live_support_tu15.hpp"','#include "halo2/source_unit_view_reset_tu15.hpp"')
needle='static void run_158690(WorldState &s) {';assert s.count(needle)==1;s=s.replace(needle,needle+'\nSourcePhysicsEntryObservation diagnostic(s.memory,0x158690u,[&]{return s.save();});')
a.output.write_text(s)
report=json.loads(a.manifest.read_text());report['generated_cpp_sha256']=hashlib.sha256(s.encode()).hexdigest();report['original_sha256']=NEW_PIN;report['later_original_sha256']=NEW_PIN;report['boundary']='Actual English TU1.5 reachable scalar/SSE/x87 bodies; no production opcode decoder. External owners preserve the full source ABI.';a.manifest.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({'complete_entries':len(entries),'instructions':len(sites),'entries':[hex(p)for p in sorted(entries)],'external_owners':report['external_owners']}))
