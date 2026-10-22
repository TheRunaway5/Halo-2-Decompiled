#!/usr/bin/env python3
"""Authenticate immutable operands of the existing2CC8F0 callback dependency.

The real MCC callback reaches its original2CCA66 FLD m64. Audit the complete
380-instruction body and export its five immutable operand ranges,24 bytes.
The existing native algorithm is unchanged; no orientation or result is seeded.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86 import X86_OP_MEM
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from export_final_startup_storage_data_tu15 import regions


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--report',type=Path,required=True);ap.add_argument('--verify',action='store_true');args=ap.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();assert args.output.resolve().is_relative_to(scratch)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');body=_trace(code,0x2cc8f0,{0x2cc8f0})
    path=Path(__file__).resolve().parents[1]/'src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp'
    source=path.read_text();cpp=source.split('static void run_2cc8f0(WorldState &s) {',1)[1].split('\n}',1)[0]
    sites={int(p,16) for p in re.findall(r'services\.instruction\(0x([0-9a-f]+)u?\)',cpp)}
    assert len(body.code)==380 and sites==set(body.code)
    points=set();reads=[]
    for pc in sorted(body.code):
        ins=code.at(pc)
        for op in ins.operands:
            if op.type!=X86_OP_MEM or op.mem.base or op.mem.index:continue
            sec=next((s for s in image.sections if s.va<=op.mem.disp<op.mem.disp+op.size<=s.va+s.vsize),None)
            if sec and sec.name=='.rdata':
                assert op.access&CS_AC_READ and not op.access&CS_AC_WRITE
                points.update(range(op.mem.disp,op.mem.disp+op.size))
                reads.append(dict(pc=pc,address=op.mem.disp,bytes=op.size,instruction_hex=bytes(ins.bytes).hex()))
    operands=[[p,image.read(p,n).hex()] for p,n in regions(points)]
    assert len(operands)==5 and len(points)==24
    fixture=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,entry=0x2cc8f0,instructions=380,
        immutable_operands=operands,reads=reads,instruction_patches=[],supplied_dependency_returns=[])
    raw=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==raw,'Preserve original numeric operand reference'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(raw)
    lines=['// Original immutable operands of existing2CC8F0; no orientation/state/result values.']
    for address,bits in operands:
        data=bytes.fromhex(bits);lines.append(f'literal(memory,0x{address:x}u,std::array<std::uint8_t,{len(data)}>{{'+
            ','.join(f'0x{v:02x}' for v in data)+'});')
    args.output.write_text('\n'.join(lines)+'\n')
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,immutable_bytes=24,ranges=5,reads=len(reads),
        fixture_bytes=len(raw),fixture_sha256=hashlib.sha256(raw).hexdigest(),data_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),existing_owner_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
        existing_owner_changed=False,full_numeric_algorithm_qualified=False,runtime_reference_required=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
