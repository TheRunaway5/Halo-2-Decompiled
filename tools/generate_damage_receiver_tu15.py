#!/usr/bin/env python3
"""Compile complete original receiver owners and bind structured native kernels."""
import argparse
import hashlib
import json
import os
import re
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha

ENTRIES=(0xe89b0,0x1e9290,0x105460)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',required=True,type=Path)
    parser.add_argument('--directory',required=True,type=Path)
    args=parser.parse_args()
    assert os.environ.get('HALO2_TEST_SCRATCH'),'Use tools/run_test.py'
    assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    assert code.at(0xd91af).operands[0].imm==0xe89b0
    assert code.at(0xe8a18).operands[0].imm==0x1e9290
    sites={};instructions={};addresses={};routines=[]
    for entry,count in zip(ENTRIES,(40,105,23)):
        body=_trace(code,entry,set(ENTRIES));assert len(body.code)==count and not body.tables
        sites[f'{entry:08x}']=sorted(body.code);routines.append(dict(entry=entry,tables=[],byte_tables=[]))
        for pc in sorted(body.code):
            i=code.at(pc);assert image.read(pc,i.size)==bytes(i.bytes)
            instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
            for op in i.operands:
                if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
                elif op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{q:08x}':q for q in ENTRIES},
        body_sites=sites,instruction_map=instructions,address_map={f'{q:08x}':v for q,v in addresses.items()},routines=routines)
    args.directory.mkdir(parents=True,exist_ok=False);correspondence=args.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    generate(SimpleNamespace(output=args.output,manifest=args.directory/'generation.json',correspondence=correspondence,
        additional_entries=Path(__file__)),paired)
    source=args.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_damage_receiver_tu15.py')
    source=source.replace('halo2/physics_live_support_tu15.hpp','halo2/source_damage_receiver_tu15.hpp')
    source=source.replace('source_run_physics_live_support_tu15','source_run_damage_receiver_tu15')
    source=source.replace('source_physics_live_support_tu15_owns_entry','source_damage_receiver_tu15_owns_entry')
    source=source.replace('#include "halo2/source_damage_receiver_tu15.hpp"',
        '#include "halo2/source_damage_receiver_tu15.hpp"\n#include "halo2/source_physics_entry_observation.hpp"')
    source=source.replace('LiveState s{m,observe,services,image};s.load(abi);',
        'LiveState s{m,observe,services,image};s.load(abi);\n    SourcePhysicsEntryObservation diagnostic(m,entry,[&]{return s.save();});')
    kernel=Path(__file__).resolve().parents[1]/'src/gameplay/objects/source_damage_record_native_tu15_internal.hpp'
    original=re.search(r'static void run_105460\(WorldState &s\) \{\n.*?\n\}',source,re.S)
    assert original and original.group().count('s.services.instruction(')==23
    kernel_sites={int(pc,16) for pc in re.findall(r'site\((0x[0-9a-f]+)u\)',kernel.read_text())}
    assert kernel_sites==set(sites['00105460'])
    original_body_sha256=hashlib.sha256(original.group().encode()).hexdigest()
    source=source[:original.start()]+'''static void run_105460(WorldState &s) {
    physics_world_runtime_detail::damage_record_native_tu15(s);
}'''+source[original.end():]
    source=source.replace('#include "halo2/source_damage_receiver_tu15.hpp"',
        '#include "halo2/source_damage_receiver_tu15.hpp"\n#include "gameplay/objects/source_damage_record_native_tu15_internal.hpp"')
    ending='} // namespace halo2'
    assert source.count(ending)==1
    source=source.replace(ending,'''SourceDamageRecordNativeActivity source_damage_record_native_activity() {
    using namespace physics_world_runtime_detail;
    return {damage_record_native_calls.load(std::memory_order_relaxed),
        damage_record_native_admitted.load(std::memory_order_relaxed),
        damage_record_native_rejected.load(std::memory_order_relaxed)};
}
'''+ending)
    args.output.write_text(source)
    report=json.loads((args.directory/'generation.json').read_text());report['generated_cpp_sha256']=sha(args.output)
    report['structured_native_record_owner']=dict(entry=0x105460,complete_original_instructions=23,
        original_generated_body_sha256=original_body_sha256,kernel_path=str(kernel),kernel_sha256=sha(kernel),
        activity_header_sha256=sha(Path(__file__).resolve().parents[1]/'include/halo2/source_damage_record_native_tu15.hpp'),
        semantic_or_original_cpu_qualification_claimed_by_generation=False)
    (args.directory/'generation.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(final_cpp_sha256=report['generated_cpp_sha256'],structured_native_entry='105460',
        kernel_sha256=report['structured_native_record_owner']['kernel_sha256'])),flush=True)


if __name__=='__main__':main()
