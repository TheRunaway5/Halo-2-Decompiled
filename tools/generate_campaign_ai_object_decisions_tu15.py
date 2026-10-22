#!/usr/bin/env python3
"""Lower complete original AI object-decision admission and dependency bodies.

The real MCC frame requires26EFA0 at26E60E. Its missing26EEF0 query and265620
state publisher stay complete, including every unobserved branch. The original
tracking interface also requires2753C0. Reuse existing Frame/HS owners in the
same bank, never supplying their results. Runtime operands are authenticated
immutable source bytes; no tag, actor state or completed decision is exported.
The later reached1FB3C0 actor-state body reuses whole1FB2A0/200A40 in Frame.
The Marine tick45 call274A40 requires the whole274E30 object/property owner
lookup, preserving its direct/related full-handle guards and pool traversal.
The next274A4E call requires274450/274490 creation. Reuse whole273B70,
274200 and274020 for classification, real allocation and initialization.
"""
import argparse
import json
import os
from pathlib import Path
import re
from types import SimpleNamespace
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86 import X86_OP_MEM
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha
from export_final_startup_storage_data_tu15 import regions

ROOT=Path(__file__).resolve().parents[1]
ENTRIES=(0x26efa0,0x26eef0,0x265620,0x2753c0,0x1fb3c0,0x274e30,0x274450,0x274490)
FRAME=(0x1e7810,0x1ecd00,0x213d30,0x25980,0x1fb2a0,0x200a40,0x273b70,0x274200,0x274020)
MODULE='source_campaign_ai_object_decisions_tu15_generated.cpp'


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True);ap.add_argument('--directory',type=Path,required=True)
    ap.add_argument('--data-output',type=Path,required=True);args=ap.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    assert args.output.resolve().is_relative_to(scratch) and args.data_output.resolve().is_relative_to(scratch)
    for file in (ROOT/'src').rglob('*_generated.cpp'):
        if file.name==MODULE:continue
        owned={int(p,16) for p in re.findall(r'^static void run_([0-9a-f]+)\(WorldState &s\) \{',file.read_text(),re.M)}
        assert not owned&set(ENTRIES),f'Original decision owner already exists: {file}'
    frame=(ROOT/'src/campaign/ai/source_ai_frame_tu15_generated.cpp').read_text()
    for entry in FRAME:
        assert f'static void run_{entry:x}(WorldState &s) {{' in frame
        assert f'case 0x{entry:x}u:run_{entry:x}(s);break;' in frame
    assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    assert image.read(0x26e60e,5).hex()=='e88d090000'
    assert image.read(0x1fbed6,5).hex()=='e8e5f4ffff'
    assert code.at(0x274a40).mnemonic=='call' and code.at(0x274a40).operands[0].imm==0x274e30
    assert code.at(0x274a4e).mnemonic=='call' and code.at(0x274a4e).operands[0].imm==0x274450
    assert code.at(0x274484).mnemonic=='call' and code.at(0x274484).operands[0].imm==0x274490
    # The original four-row interface names the separate missing handler.
    assert image.read(0x47d6ec,4).hex()=='c0532700'
    sites,addresses,routines,points,reads={},{},[],set(),[]
    for entry in ENTRIES:
        body=_trace(code,entry,set(ENTRIES));assert body.code
        sites[f'{entry:08x}']=sorted(body.code)
        assert not body.tables
        routines.append(dict(entry=entry,tables=[],byte_tables=[]))
        for pc in sorted(body.code):
            ins=code.at(pc);assert image.read(pc,ins.size)==bytes(ins.bytes)
            addresses[pc]=pc;addresses[pc+ins.size]=pc+ins.size
            for op in ins.operands:
                if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
                elif op.type==X86_OP_MEM:
                    addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
                    if not op.mem.base and not op.mem.index:
                        section=next((s for s in image.sections if s.va<=op.mem.disp<op.mem.disp+op.size<=s.va+s.vsize),None)
                        if section is not None and section.name=='.rdata':
                            assert op.access&CS_AC_READ and not op.access&CS_AC_WRITE
                            points.update(range(op.mem.disp,op.mem.disp+op.size))
                            reads.append(dict(pc=pc,address=op.mem.disp,bytes=op.size,instruction_hex=bytes(ins.bytes).hex()))
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{p:08x}':p for p in ENTRIES},
        body_sites=sites,instruction_map={f'{p:08x}':p for pcs in sites.values() for p in pcs},
        address_map={f'{p:08x}':p for p in addresses},routines=routines)
    args.directory.mkdir(parents=True,exist_ok=True)
    correspondence=args.directory/'bodies.json';correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    manifest=args.directory/'generation.json'
    generate(SimpleNamespace(output=args.output,manifest=manifest,correspondence=correspondence,
        additional_entries=Path(__file__)),paired,runtime_header='halo2/source_campaign_ai_object_decisions_tu15.hpp')
    source=args.output.read_text().replace('tools/generate_physics_live_support_tu15.py',
        'tools/generate_campaign_ai_object_decisions_tu15.py').replace('source_run_physics_live_support_tu15',
        'source_run_campaign_ai_object_decisions_body_tu15').replace('source_physics_live_support_tu15_owns_entry',
        'source_campaign_ai_object_decisions_tu15_owns_entry')
    for header in ('physics_live_support_tu15','source_projectile_effect_observer','source_object_attachment_tu15'):
        source=source.replace(f'#include "halo2/{header}.hpp"\n','')
    source=source.replace('#include "physics/runtime/physics_world_runtime_internal.hpp"',
        '#include "physics/runtime/physics_world_runtime_internal.hpp"\n#include "halo2/source_ai_frame_tu15.hpp"')
    source+='''
namespace halo2 {
void source_run_campaign_ai_object_decisions_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices &services,const SourcePhysicsContactRecordObserver &observe) {
    auto nested=services;
    nested.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &stores) {
        const auto control=state.floating_point.value_or(fp);
        if('''+' || '.join(f'call.target==0x{p:x}u' for p in FRAME)+''') {
            source_run_ai_frame_tu15(bank,call.target,state,control,nested,stores);return;
        }
        if(services.required_owner) {services.required_owner(bank,call,state,stores);return;}
        throw std::runtime_error("Required original campaign AI object-decision dependency unavailable");
    };
    source_run_campaign_ai_object_decisions_body_tu15(memory,entry,abi,fp,nested,observe);
}
}
'''
    args.output.write_text(source)
    ranges=regions(points);lines=['// Original immutable operands of whole AI object-decision bodies; no live state.']
    for address,count in ranges:
        raw=image.read(address,count)
        lines.append(f'literal(memory,0x{address:x}u,std::array<std::uint8_t,{count}>{{'+','.join(f'0x{v:02x}' for v in raw)+'});')
    args.data_output.write_text('\n'.join(lines)+'\n')
    report=json.loads(manifest.read_text());report.update(scope=__doc__,original_xbe_sha256=NEW_PIN,
        generated_cpp_sha256=sha(args.output),generator_sha256=sha(Path(__file__)),
        original_instructions=sum(map(len,sites.values())),entry_instructions={k:len(v) for k,v in sites.items()},
        immutable_operand_reads=reads,immutable_ranges=ranges,immutable_bytes=sum(n for _,n in ranges),
        data_sha256=sha(args.data_output),reused_frame_entries=FRAME,
        reused_frame_sha256=sha(ROOT/'src/campaign/ai/source_ai_frame_tu15_generated.cpp'),source_behavior_results_supplied=False)
    manifest.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(original_instructions=report['original_instructions'],immutable_bytes=report['immutable_bytes'])))


if __name__=='__main__':main()
