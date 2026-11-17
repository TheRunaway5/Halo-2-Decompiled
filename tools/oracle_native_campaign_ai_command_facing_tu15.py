#!/usr/bin/env python3
"""Physical unchanged TU1.5 point/object-facing leaves, including NONE context."""
import argparse,gzip,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_area_perform_tu15 as ref
import oracle_native_campaign_ai_path_search_tu15 as physical

ENTRIES=(0x280c90,0x280d70)
POOL=0x1050000;ROWS=POOL+0x100;NONE=0xffffffff

def inputs():
    rows=[]
    for entry in ENTRIES:
        for profile in physical.FP_PROFILES:
            for enabled in (0,1,0x80,0xff):
                for index in (None,0,1,2):
                    for target in ((0xdeadbeef,) if index is None else (0,1,0xffff,0x10002)):
                        raw=bytearray(0x400)
                        raw[0x44:0x48]=ROWS.to_bytes(4,'little')
                        for n in range(3):
                            begin=0x100+n*0xd4
                            raw[begin:begin+0xd4]=bytes((k*13+n*17+0x37)&0xff for k in range(0xd4))
                        datum=NONE if index is None else 0xa7230000|index
                        banks={0x50eee8:datum.to_bytes(4,'little')}
                        # NONE must not inspect the command pool or any row.
                        if index is not None:banks.update({0x50eee0:POOL.to_bytes(4,'little'),POOL:raw})
                        gp=[0x11331100+n*0x10101 for n in range(8)]
                        gp[2]=0xabcdef00|enabled;gp[4]=ref.SP;gp[7]=target
                        rows.append(dict(id=len(rows),name=f'{entry:x}_enable{enabled:x}_index{index}_target{target:x}_fp{profile}',
                            entry=entry,gp=gp,matrix=0x12345678,source=0xfedcba98,stack_arguments=[],
                            banks=[dict(address=p,before=bytes(v).hex()) for p,v in sorted(banks.items())],
                            before_fp=list(profile),live_x87=[],xmm=[[0x6b000000+r*4+k for k in range(4)] for r in range(8)],
                            max_steps=100,strict_writes=False))
    return rows

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',required=True,type=Path);parser.add_argument('--report',required=True,type=Path)
    parser.add_argument('--verify',type=Path);args=parser.parse_args()
    assert not args.output.exists();assert ref.sha(NEW_DEFAULT)==NEW_PIN
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT)
    meta=ref.metadata(image,ENTRIES);owned=ref.prepare(image,meta,scratch)
    rows=physical.physical(owned,inputs(),'whole');assert all(r['completed'] for r in rows)
    reached=set().union(*(set(r['executed_sites']) for r in rows));code=_Code(image,'.text')
    coverage={hex(e):dict(reached=len(set(complete_body(code,e).code)&reached),total=len(complete_body(code,e).code)) for e in ENTRIES}
    assert all(r['reached']==r['total'] for r in coverage.values())
    fixture=dict(fixture_kind='native-hs-command-builtins-v1',image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',
        owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,bank_encoding='sparse-runs-v1',cases=ref.compact(rows))
    encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if args.verify:assert encoded==args.verify.read_bytes(),'Independent original repeat differs'
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(encoded)
    report=dict(passed=True,cases=len(rows),coverage=coverage,stores=sum(len(r['writes']) for r in rows),
        none_cases=sum('_indexNone_' in r['name'] for r in rows),fixture=str(args.output),fixture_bytes=len(encoded),fixture_sha256=ref.sha(args.output),
        original_sha256=NEW_PIN,owned_scratch=str(scratch),retained_large_artifacts=[],repeat_identical=bool(args.verify))
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))

if __name__=='__main__':main()
