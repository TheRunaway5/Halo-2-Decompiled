#!/usr/bin/env python3
"""Complete missing bodies across the original campaign query selector group.

Retain every reachable instruction, including the shared2868A0 code reached by
286460. Indirect source callbacks remain required; no callback result, source
context or successful birth is supplied. Reuse the existing writable .data
sign-mask owner; do not change its access or export another copy.
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
ENTRIES={0x284300,0x285cf0,0x2861a0,0x286460,0x2868a0,0x290b50,0x2912a0,0x291740,0x291e30}
ENTRIES|={0x2843f0,0x2849f0,0x284ee0,0x285630,0x289240,0x289920,0x289d60,0x28a3f0,
    0x28cc70,0x28d310,0x28d6e0,0x28dd20}
SELECTORS={0x284090,0x2840d0,0x284140,0x2841b0,0x284220,0x284290,0x284300,0x284330,0x284360,0x284390,0x2843c0}
MODULE='source_campaign_birth_query_variants_tu15_generated.cpp'


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--directory',type=Path,required=True);args=ap.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    assert args.output.resolve().is_relative_to(scratch)
    assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    existing={};selector_owners={}
    for path in (ROOT/'src').rglob('*_generated.cpp'):
        if path.name==MODULE:continue
        owners={int(p,16) for p in re.findall(r'^static void run_([0-9a-f]+)\(WorldState &s\) \{',path.read_text(),re.M)}
        assert not owners&ENTRIES,f'Reuse existing original owner: {path}'
        for entry in owners:existing.setdefault(entry,[]).append(path.name)
        for entry in SELECTORS&owners:
            source=path.read_text().split(f'static void run_{entry:x}(WorldState &s) {{',1)[1].split('\n}',1)[0]
            assert {int(p,16) for p in re.findall(r'services\.instruction\(0x([0-9a-f]+)u?\)',source)}==set(_trace(code,entry,SELECTORS).code)
            selector_owners.setdefault(f'{entry:08x}',[]).append(dict(source=path.name,sha256=sha(path)))
    targets=set().union(*(set(_trace(code,p,SELECTORS).tail_jumps) for p in SELECTORS))
    assert ENTRIES=={0x284300}|(targets-set(existing))
    assert image.read(0x284137,5).hex()=='e9c4010000'
    assert image.read(0x2843a7,5).hex()=='e9f4ce0000'
    sites,addresses,routines,points,reads,indirect={},{},[],set(),[],[]
    for entry in sorted(ENTRIES):
        body=_trace(code,entry,ENTRIES);assert not body.tables
        assert not body.calls and set(body.tail_jumps)<=ENTRIES
        sites[f'{entry:08x}']=sorted(body.code);routines.append(dict(entry=entry,tables=[],byte_tables=[]))
        for pc in sorted(body.code):
            ins=code.at(pc);assert image.read(pc,ins.size)==bytes(ins.bytes)
            addresses[pc]=pc;addresses[pc+ins.size]=pc+ins.size
            if ins.mnemonic=='call':
                assert ins.operands[0].type==X86_OP_MEM
                indirect.append(dict(entry=entry,pc=pc,operands=ins.op_str,bytes=bytes(ins.bytes).hex()))
            for op in ins.operands:
                if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
                elif op.type==X86_OP_MEM:
                    addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
                    if not op.mem.base and not op.mem.index:
                        section=next((s for s in image.sections if s.va<=op.mem.disp<op.mem.disp+op.size<=s.va+s.vsize),None)
                        if section and section.name=='.rdata':
                            assert op.access&CS_AC_READ and not op.access&CS_AC_WRITE
                            points.update(range(op.mem.disp,op.mem.disp+op.size))
                            reads.append(dict(pc=pc,address=op.mem.disp,bytes=op.size))
    expected=dict(zip(
        ('00284300','00285cf0','002861a0','00286460','002868a0','00290b50','002912a0','00291740','00291e30'),
        (12,309,214,477,198,417,295,390,271)))
    expected.update(dict(zip(('002843f0','002849f0','00284ee0','00285630','00289240','00289920',
        '00289d60','0028a3f0','0028cc70','0028d310','0028d6e0','0028dd20'),
        (241,339,302,275,396,284,374,257,386,260,358,244))))
    assert {k:len(v) for k,v in sites.items()}==expected
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{p:08x}':p for p in ENTRIES},
        body_sites=sites,instruction_map={f'{p:08x}':p for pcs in sites.values() for p in pcs},
        address_map={f'{p:08x}':p for p in addresses},routines=routines)
    args.directory.mkdir(parents=True,exist_ok=True);correspondence=args.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n');manifest=args.directory/'generation.json'
    def original_breakpoint(ins):
        if ins.mnemonic!='int3':return None
        assert bytes(ins.bytes)==b'\xcc'
        return f'throw std::runtime_error("Original breakpoint at0x{ins.address:x}");'
    generate(SimpleNamespace(output=args.output,manifest=manifest,correspondence=correspondence,
        additional_entries=Path(__file__)),paired,runtime_header='halo2/source_campaign_birth_query_variants_tu15.hpp',
        extra_instruction=original_breakpoint)
    source=args.output.read_text().replace('tools/generate_physics_live_support_tu15.py',
        'tools/generate_campaign_birth_query_variants_tu15.py').replace('source_run_physics_live_support_tu15',
        'source_run_campaign_birth_query_variants_tu15').replace('source_physics_live_support_tu15_owns_entry',
        'source_campaign_birth_query_variants_tu15_owns_entry')
    for header in ('physics_live_support_tu15','source_projectile_effect_observer','source_object_attachment_tu15'):
        source=source.replace(f'#include "halo2/{header}.hpp"\n','')
    args.output.write_text(source)
    assert points==set(range(0x46a0d0,0x46a0d4)) and image.read(0x46a0d0,4).hex()=='0000803f'
    mask=image.read(0x48c7e0,16);assert mask.hex()=='00000080000000000000000000000000'
    owner=ROOT/'src/rendering/renderer/native_tu15_renderer_startup_data.cpp'
    assert ('m.mount(0x48c7e0u,encode(std::array<std::uint32_t,4>{0x80000000u,0u,0u,0u}),\n'
        '        SourceRuntimeMemory::Access::ReadWrite,') in owner.read_text()
    report=json.loads(manifest.read_text());report.update(scope=__doc__,original_xbe_sha256=NEW_PIN,
        generated_cpp_sha256=sha(args.output),generator_sha256=sha(Path(__file__)),
        original_instructions=sum(map(len,sites.values())),unique_original_sites=len(set().union(*(set(v) for v in sites.values()))),
        entry_instructions={k:len(v) for k,v in sites.items()},
        reused_immutable_operands=[[0x46a0d0,'0000803f']],new_immutable_bytes=0,
        reused_mutable_data=dict(address=0x48c7e0,bytes=mask.hex(),owner=str(owner),owner_sha256=sha(owner)),
        immutable_reads=reads,required_indirect_callbacks=indirect,callback_results_supplied=False,live_context_supplied=False)
    report.update(selector_interfaces=sorted(SELECTORS),selector_targets=sorted(targets),reused_selector_owners=selector_owners,
        all_static_selector_targets_have_bodies=targets<=ENTRIES|set(existing))
    manifest.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(functions=len(ENTRIES),instructions=report['original_instructions'],unique_sites=report['unique_original_sites'],new_immutable_bytes=0)))


if __name__=='__main__':main()
