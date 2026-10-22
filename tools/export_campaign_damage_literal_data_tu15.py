#!/usr/bin/env python3
"""Export fixed readonly operands of the existing original damage-delivery owner.

Use its authenticated selected-image observation sites, then inspect unchanged
TU1.5 instructions. Export only exact fixed reads in .rdata. Indexed extents,
writable fields, source bindings and live results remain required owners.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86 import X86_OP_MEM
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from export_final_startup_storage_data_tu15 import regions


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True);ap.add_argument('--fixture',type=Path,required=True)
    ap.add_argument('--report',type=Path,required=True);ap.add_argument('--verify',action='store_true');args=ap.parse_args()
    assert args.output.resolve().is_relative_to(Path(os.environ['HALO2_TEST_SCRATCH']).resolve())
    root=Path(__file__).resolve().parents[1];source=root/'src/gameplay/units/source_damage_delivery_runtime_generated.cpp';raw_source=source.read_bytes()
    pcs={int(v,16)for v in re.findall(r'damage_observation_pc\(0x[0-9a-f]+u,0x([0-9a-f]+)u,s\)',raw_source.decode())}
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'.text')
    reads=[];indexed=[];points=set()
    for pc in sorted(pcs):
        ins=code.at(pc)
        for op in ins.operands:
            if op.type!=X86_OP_MEM or not op.access&CS_AC_READ or op.mem.segment:continue
            section=next((s for s in image.sections if s.va<=op.mem.disp<op.mem.disp+op.size<=s.va+s.vsize),None)
            if not section or section.name!='.rdata':continue
            assert not op.access&CS_AC_WRITE
            row=dict(pc=pc,instruction_hex=bytes(ins.bytes).hex(),address=op.mem.disp,bytes=op.size)
            if op.mem.base or op.mem.index:indexed.append(row);continue
            points.update(range(op.mem.disp,op.mem.disp+op.size));reads.append(row)
    assert set(range(0x46ab2c,0x46ab30))<=points
    operands=[[at,image.read(at,size).hex()]for at,size in regions(points)]
    lines=['// Exact original damage-delivery fixed readonly reads; no indexed gaps or live state.']
    for at,bits in operands:
        raw=bytes.fromhex(bits);lines.append(f'literal(memory,0x{at:x}u,std::array<std::uint8_t,{len(raw)}>{{'+','.join(f'0x{b:02x}'for b in raw)+'});')
    args.output.write_text('\n'.join(lines)+'\n')
    fixture=dict(original_xbe_sha256=NEW_PIN,source_owner_sha256=hashlib.sha256(raw_source).hexdigest(),instruction_patches=[],
        supplied_dependency_returns=[],immutable_operands=operands,original_fixed_reads=reads,indexed_operands_unqualified=indexed)
    raw=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==raw,'Preserve original damage operand reference'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(raw)
    assert source.read_bytes()==raw_source,'Damage owner changed during discovery'
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,source_owner_sha256=fixture['source_owner_sha256'],observed_source_sites=len(pcs),
        fixed_readonly_operands=len(reads),readonly_bytes=len(points),readonly_regions=len(operands),indexed_operands_unqualified=indexed,
        data_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),fixture_sha256=hashlib.sha256(raw).hexdigest(),fixture_bytes=len(raw),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),full_damage_delivery_qualified=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
