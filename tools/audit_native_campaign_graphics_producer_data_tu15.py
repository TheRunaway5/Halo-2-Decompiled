#!/usr/bin/env python3
"""Compare primary229A90 producer operands with a real cold-constructor probe."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
from capstone.x86 import X86_OP_MEM
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ownership',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir()
    image=Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    source=Path('src/rendering/world/source_texture_camera_producer_tu15_generated.cpp')
    local={int(h,16) for h in re.findall(r'static void run_([0-9a-f]+)\(WorldState &s\)',source.read_text())}
    code=_Code(image,'.text')
    todo=[0x229a90]
    seen=set()
    uses=[]
    calls=[]
    while todo:
        entry=todo.pop()
        if entry in seen:
            continue
        seen.add(entry)
        body=_trace(code,entry,local)
        for pc in sorted(body.code):
            instruction=code.at(pc)
            for operand in instruction.operands:
                if operand.type==X86_OP_MEM and operand.mem.disp>=0x400000:
                    uses.append(dict(owner=hex(entry),pc=hex(pc),address=hex(operand.mem.disp),
                        bytes=operand.size,access=operand.access,
                        indexed=bool(operand.mem.base or operand.mem.index),
                        operation=instruction.mnemonic,operands=instruction.op_str,
                        unadmitted_null_branch=(entry==0x2f7e0 and 0x2f8a5<=pc<0x2f91c)))
        for pc,target,kind in body.sites:
            calls.append(dict(owner=hex(entry),pc=hex(pc),target=hex(target),
                              kind=kind,local=target in local))
            if target in local:
                todo.append(target)
    # Actual nonnull branch skips the default-angle/x87 block. Its shared
    # near/far/publication continuation begins2F91C and remains included.
    null_select=code.at(0x2f7e7)
    continuation=code.at(0x2f8a3)
    assert null_select.mnemonic=='je' and null_select.operands[0].imm==0x2f8a5
    assert continuation.mnemonic=='jmp' and continuation.operands[0].imm==0x2f91c
    ownership=json.loads(args.ownership.read_text())
    mapped={(row['address'],row['bytes']):row for row in ownership['fields']}
    required={(row['address'],row['bytes']) for row in uses if not row['unadmitted_null_branch']}
    all_fields={(row['address'],row['bytes']) for row in uses}
    assert all_fields==set(mapped)
    missing=[mapped[key] for key in sorted(required) if mapped[key]['access']=='MISSING']
    report=dict(passed=not missing,original_sha256=NEW_PIN,
        entries=[hex(entry) for entry in sorted(seen)],absolute_data_operands=uses,
        external_owners=[row for row in calls if not row['local']],calls_and_tails=calls,
        complete_source_local_field_count=len(all_fields),
        prepared_nonnull_required_field_count=len(required),
        prepared_nonnull_missing=missing,
        unadmitted_default_missing=[row for row in ownership['missing']
                                   if (row['address'],row['bytes']) not in required],
        primary_gate=dict(null_branch=hex(null_select.operands[0].imm),
            nonnull_jump=hex(continuation.address),common_continuation=hex(continuation.operands[0].imm),
            select_bytes=null_select.bytes.hex(),jump_bytes=continuation.bytes.hex()),
        cold_ownership_report=str(args.ownership),
        cold_ownership_report_sha256=hashlib.sha256(args.ownership.read_bytes()).hexdigest(),
        source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
        scope='Static-data ownership for source-local229A90 plus admitted nonnull2F7E0. External owners, runtime pointer targets, null/default action, full renderer and campaign completion are not qualified here.',
        retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],fields=len(required),missing=missing,
                         unadmitted_default_missing=report['unadmitted_default_missing'])))
    raise SystemExit(bool(missing))


if __name__=='__main__':
    main()
