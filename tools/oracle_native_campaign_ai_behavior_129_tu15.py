#!/usr/bin/env python3
"""Physical unchanged behavior129 leaves, including caller-owned state aliases."""
import argparse,gzip,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_area_perform_tu15 as ref
import oracle_native_campaign_ai_path_search_tu15 as physical
BASE=0x1050000
ENTRIES=(0x1cacb0,0x1cacd0,0x1cae70)
def inputs(image):
    rows=[]
    for p,profile in enumerate(physical.FP_PROFILES):
        for state in(BASE,BASE+1,BASE+3,BASE+0x78,BASE+0x123,ref.SP-8,ref.SP-4,ref.SP+4,ref.SP+0x48):
            for actor in(0,0xffffffff,0xe3620001):
                raw=bytearray((n*13+p*7)&255 for n in range(0x200));stack=bytearray([0x95]*0x80)
                for at,value in((ref.SP,ref.STOP),(ref.SP+4,actor),(ref.SP+8,state)):
                    stack[at-(ref.SP-16):at-(ref.SP-16)+4]=value.to_bytes(4,'little')
                gp=[0x11331100+k*0x10101 for k in range(8)];gp[0]=0x1cacd0;gp[4]=ref.SP
                rows.append(dict(id=len(rows),name=f'state{state:x}_actor{actor:x}_fp{p}',entry=0x1cacd0,gp=gp,matrix=actor,source=state,stack_arguments=[],
                    banks=[dict(address=BASE,before=raw.hex()),dict(address=ref.SP-16,before=stack.hex())],
                    before_fp=list(profile),live_x87=[],xmm=[[0x6b000000+r*4+k for k in range(4)]for r in range(8)],max_steps=20,strict_writes=False))
        for state in(BASE,BASE+1,BASE+0x123):
            for flag in(0,1,0x80,0xff):
                raw=bytearray((n*13+p*7)&255 for n in range(0x200));raw[state-BASE+0x10]=flag
                gp=[0x11331100+k*0x10101 for k in range(8)];gp[4]=ref.SP
                rows.append(dict(id=len(rows),name=f'query_state{state:x}_flag{flag}_fp{p}',entry=0x1cacb0,gp=gp,matrix=0xe3620001,source=state,stack_arguments=[0x87654321],
                    banks=[dict(address=BASE,before=raw.hex()),dict(address=0x47c1a4,before=image.read(0x47c1a4,8).hex())],
                    before_fp=list(profile),live_x87=[],xmm=[[0x6b000000+r*4+k for k in range(4)]for r in range(8)],max_steps=20,strict_writes=False))
        for index in(0,1,2):
            for kind in(-32768,-1,0,1,2,3,32767,None):
                raw=bytearray([0xa5]*0x5000)
                def put(at,value,width=4):raw[at-BASE:at-BASE+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')
                pool=BASE;actors=BASE+0x100;properties=BASE+0x3000;records=BASE+0x3100
                put(pool+0x44,actors);put(properties+0x44,records)
                put(actors+index*0x888+0x338,0xffffffff if kind is None else 0xe4000000+index)
                put(records+index*0x3c+0x24,kind or 0,2)
                gp=[0x11331100+k*0x10101 for k in range(8)];gp[4]=ref.SP
                rows.append(dict(id=len(rows),name=f'property_actor{index}_kind{kind}_fp{p}',entry=0x1cae70,gp=gp,matrix=0xe3620000+index,source=0x87654321,stack_arguments=[],
                    banks=[dict(address=BASE,before=raw.hex()),dict(address=0x5020c8,before=pool.to_bytes(4,'little').hex()),dict(address=0x50eef0,before=properties.to_bytes(4,'little').hex())],
                    before_fp=list(profile),live_x87=[],xmm=[[0x6b000000+r*4+k for k in range(4)]for r in range(8)],max_steps=40,strict_writes=False))
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args()
    assert not a.output.exists();assert ref.sha(NEW_DEFAULT)==NEW_PIN
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);meta=ref.metadata(image,ENTRIES)
    owned=ref.prepare(image,meta,scratch);rows=physical.physical(owned,inputs(image),'whole');assert all(r['completed']for r in rows)
    reached=set().union(*(set(r['executed_sites'])for r in rows));code=_Code(image,'.text')
    coverage={hex(e):dict(reached=len(set(complete_body(code,e).code)&reached),total=len(complete_body(code,e).code))for e in ENTRIES}
    assert all(r['reached']==r['total']for r in coverage.values()),coverage
    count=len(rows);stores=sum(len(r['writes'])for r in rows)
    fixture=dict(fixture_kind='native-behavior-129-v1',image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,bank_encoding='sparse-runs-v1',cases=ref.compact(rows))
    encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert encoded==a.verify.read_bytes(),'Independent original repeat differs'
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,cases=count,sites=len(reached),coverage=coverage,ordered_stores=stores,fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=ref.sha(a.output),original_sha256=NEW_PIN,owned_scratch=str(scratch),repeat_identical=bool(a.verify))
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
