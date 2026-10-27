#!/usr/bin/env python3
"""Locate authenticated Xbox TU1.5 loading owners from full retail bodies.

Relocated instruction signatures locate candidates; every original normalized
instruction/internal branch and direct-call pair is reported separately.
This is not a renderer execution or a claim that external GPU services match.
Run through tools/run_test.py; retain only the small source report.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import re

from capstone.x86 import X86_OP_IMM, X86_OP_MEM
from oracle_tu15_team_balance import Xbe, NEW_DEFAULT, NEW_PIN, ROOT
from oracle_smg_firing_1_1 import _Code, _trace
from research_mp_sandbox_update_functions import full_tokens

ENTRIES=(0x288e0,0x28950,0x28980,0x289e0,0x29080,0x29370,0x296c0,
    0x298f0,0x29b00,0x2a2f0,0x2a8d0,0x2bcd0,0x2c5224,0x2c52e7,
    0x2c52af,0x2233a0,0x163890,0x2353a5,0x12b6f0,0x2ca00,
    0x18f100,0x18eaa0,0x18e8b0,0x18e810,0x6ec80,0x13edb0,0x13ec70,0x223320)
RETAIL_PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    retail=ROOT/'assets/disc/default.xbe'
    assert hashlib.sha256(retail.read_bytes()).hexdigest()==RETAIL_PIN
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
    old,new=Xbe(retail),Xbe(NEW_DEFAULT);oc,nc=_Code(old,'.text'),_Code(new,'.text')
    oldstarts={int(r['address'],16)for r in csv.DictReader((ROOT/'analysis/retail/functions.tsv').open(),delimiter='\t')}
    newstarts={int(r['address'],16)for r in csv.DictReader((ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(),delimiter='\t')}
    section=new.section('.text');data=new.section_bytes(section);rows=[]
    for address in ENTRIES:
        original_trace=_trace(oc,address,oldstarts|set(ENTRIES))
        original=[oc.at(p)for p in sorted(original_trace.code)]
        candidates=[]
        for length in (16,12,8):
            if len(original)<length:continue
            pattern=[]
            for instruction in original[:length]:
                wild=set()
                if instruction.imm_size and (instruction.mnemonic=='call' or instruction.mnemonic.startswith('j') or
                    any(o.type==X86_OP_IMM and 0x10000<=o.imm<0x810000 for o in instruction.operands)):
                    wild.update(range(instruction.imm_offset,instruction.imm_offset+instruction.imm_size))
                if instruction.disp_size and any(o.type==X86_OP_MEM and not o.mem.base and
                    0x10000<=o.mem.disp<0x810000 for o in instruction.operands):
                    wild.update(range(instruction.disp_offset,instruction.disp_offset+instruction.disp_size))
                pattern.extend(b'.'if i in wild else re.escape(bytes([v]))for i,v in enumerate(instruction.bytes))
            candidates=[section.va+m.start()for m in re.finditer(b''.join(pattern),data,re.DOTALL)]
            if len(candidates)==1:break
        if len(candidates)>1:
            complete=[]
            for candidate in candidates:
                try:
                    trace=_trace(nc,candidate,newstarts|{candidate})
                    body=[nc.at(p)for p in sorted(trace.code)]
                    if full_tokens(original,old)==full_tokens(body,new):complete.append(candidate)
                except (ValueError,AssertionError):pass
            if len(complete)==1:candidates=complete
        row=dict(retail=f'{address:08x}',candidates=[f'{p:08x}'for p in candidates],prefix_instructions=length)
        if len(candidates)==1:
            target=candidates[0];trace=_trace(nc,target,newstarts|{target});updated=[nc.at(p)for p in sorted(trace.code)]
            equal=full_tokens(original,old)==full_tokens(updated,new)
            row.update(tu15=f'{target:08x}',retail_instructions=len(original),tu15_instructions=len(updated),
                normalized_instructions_and_internal_branches_equal=equal)
            if equal:
                row['direct_calls']=[dict(retail=f'{a.operands[0].imm:08x}',tu15=f'{b.operands[0].imm:08x}')
                    for a,b in zip(original,updated)if a.mnemonic=='call'and a.operands[0].type==X86_OP_IMM and b.operands[0].type==X86_OP_IMM]
        rows.append(row)
    report=dict(scope=__doc__,retail_sha256=RETAIL_PIN,tu15_sha256=NEW_PIN,rows=rows,
        matched=sum(r.get('normalized_instructions_and_internal_branches_equal',False)for r in rows),
        entries=len(rows),native_runtime_changed=False,source_assets_changed=False,scratch=str(scratch),scratch_retained=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(matched=report['matched'],entries=len(rows),report=str(args.report))))


if __name__=='__main__':main()
