#!/usr/bin/env python3
"""Lower complete original TU1.5 cloth construction, geometry and retirement."""
import argparse,json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body,generate,sha

ENTRIES={1151072: (36, {1153728, 1501344, 1500832}, set()), 1153728: (88, {1153152, 1154672}, set()), 1154672: (196, {1155424, 1157904}, set()), 1157904: (75, {1155424}, set()), 1155424: (145, {765280, 770560}, set()), 1153152: (159, {765280, 3329036, 3322054}, set())}
ENTRIES[0x1190d0]=(4,{0x16e8a0},set())

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,default=Path('src/physics/source_cloth_tu15_generated.cpp'))
    p.add_argument('--directory',type=Path,required=True)
    a=p.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    sites,instructions,addresses,routines={},{},{},[]
    for entry,(count,calls,indirect) in ENTRIES.items():
        body=complete_body(code,entry)
        assert len(body.code)==count and not body.tables
        actual_calls={code.at(pc).operands[0].imm for pc in body.code if code.at(pc).mnemonic=='call' and code.at(pc).operands[0].type==2}
        actual_indirect={pc for pc in body.code if code.at(pc).mnemonic=='call' and code.at(pc).operands[0].type!=2}
        assert actual_calls==calls and actual_indirect==indirect
        sites[f'{entry:08x}']=sorted(body.code);routines.append(dict(entry=entry,tables=[],byte_tables=[]))
        for pc in sorted(body.code):
            i=code.at(pc);assert image.read(pc,i.size)==bytes(i.bytes)
            instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
            for op in i.operands:
                if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
                elif op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,
        entries={f'{q:08x}':q for q in ENTRIES},body_sites=sites,instruction_map=instructions,
        address_map={f'{q:08x}':v for q,v in addresses.items()},routines=routines)
    a.directory.mkdir(parents=True,exist_ok=False);correspondence=a.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    manifest=a.directory/'generation.json'
    generate(SimpleNamespace(output=a.output,manifest=manifest,correspondence=correspondence,
        additional_entries=Path(__file__)),paired,code_override=code)
    s=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_cloth_tu15.py')
    s=s.replace('halo2/physics_live_support_tu15.hpp','halo2/source_cloth_tu15.hpp')
    s=s.replace('source_run_physics_live_support_tu15','source_run_cloth_tu15')
    s=s.replace('source_physics_live_support_tu15_owns_entry','source_cloth_tu15_owns_entry')
    s=s.replace('#include "halo2/source_cloth_tu15.hpp"',
        '#include "halo2/source_cloth_tu15.hpp"\n#include "halo2/source_physics_entry_observation.hpp"')
    marker='FpScope scope(fp);LiveState s{m,observe,services,image};s.load(abi);'
    assert s.count(marker)==1
    s=s.replace(marker,marker+'\n    SourcePhysicsEntryObservation phase(m,entry,[&]{return s.save();});')
    a.output.write_text(s);report=json.loads(manifest.read_text());report.update(generated_cpp_sha256=sha(a.output),generator_sha256=sha(Path(__file__)))
    manifest.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
