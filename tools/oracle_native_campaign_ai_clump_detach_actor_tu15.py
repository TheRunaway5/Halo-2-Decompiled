#!/usr/bin/env python3
"""Unchanged physical2731E0 and274970 intrusive actor/perception detach."""
import argparse,gzip,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_area_perform_tu15 as ref
import oracle_native_campaign_ai_path_search_tu15 as physical

ENTRIES=(0x2731e0,0x274970)
BASE=0x1050000;CLUMPS=BASE+0x100;ACTOR_POOL=BASE+0x200
ACTORS=BASE+0x1000;PROPERTIES=BASE+0x4000;GROUPS=BASE+0x5000
PROPERTY_POOL=BASE+0x300;GROUP_POOL=BASE+0x400;NONE=0xffffffff

def make(name,count,match,properties,previous,following,profile,clump_index=0,salt_mismatch=False,initial_count=None):
    raw=bytearray(0x6000)
    def put(at,value,size=4):raw[at-BASE:at-BASE+size]=(value&((1<<(size*8))-1)).to_bytes(size,'little')
    put(BASE+0x44,CLUMPS);put(ACTOR_POOL+0x44,ACTORS)
    put(PROPERTY_POOL+0x44,PROPERTIES);put(GROUP_POOL+0x44,GROUPS)
    clump=0xec630000|clump_index;record=CLUMPS+clump_index*0x50
    actor_ids=[0xe3630000+i for i in range(count)]
    put(record+0x18,actor_ids[0] if actor_ids else NONE)
    put(record+0x10,count if initial_count is None else initial_count,2)
    for i,datum in enumerate(actor_ids):
        actor=ACTORS+i*0x888;put(actor+0x80,actor_ids[i+1] if i+1<count else NONE)
        put(actor+0x7c,clump);put(actor+0x58,NONE)
    wanted=(0xe3640000 if salt_mismatch else 0xe3630000)|match
    if match<count:
        put(ACTORS+match*0x888+0x58,0xa5670001 if properties else NONE)
    for i in range(properties):
        group=0xb6780000|i;datum=0xa5670000|(3*i+1)
        before=0xa5670000|(3*i);after=0xa5670000|(3*i+2)
        row=PROPERTIES+(3*i+1)*0x3c
        put(row+0x2c,(0xa5670000|(3*(i+1)+1)) if i+1<properties else NONE)
        put(row+8,group);put(row+0x38,before if previous else NONE);put(row+0x34,after if following else NONE)
        put(PROPERTIES+3*i*0x3c+0x34,datum)
        put(PROPERTIES+(3*i+2)*0x3c+0x38,datum)
        put(GROUPS+i*0xc4+0x18,before if previous else datum)
    gp=[0x11331100+i*0x10101 for i in range(8)];gp[0]=clump;gp[2]=wanted;gp[4]=ref.SP
    banks={BASE:raw,0x50eef8:BASE.to_bytes(4,'little'),0x5020c8:ACTOR_POOL.to_bytes(4,'little'),
        0x50eef0:PROPERTY_POOL.to_bytes(4,'little'),0x50eef4:GROUP_POOL.to_bytes(4,'little')}
    return dict(name=name,entry=0x2731e0,gp=gp,matrix=0x12345678,source=0xfedcba98,stack_arguments=[],
        banks=[dict(address=p,before=bytes(v).hex())for p,v in sorted(banks.items())],before_fp=list(profile),
        live_x87=[],xmm=[[0x6b000000+r*4+k for k in range(4)]for r in range(8)],max_steps=1000,strict_writes=False)

def inputs():
    rows=[]
    for profile in physical.FP_PROFILES:
        for count in range(4):
            for match in range(count+1):
                for properties in (0,1,3):
                    for previous,following in ((False,False),(True,False),(False,True),(True,True)):
                        rows.append(make(f'count{count}_match{match}_props{properties}_prev{previous}_next{following}_fp{profile}',
                            count,match,properties,previous,following,profile))
        for initial_count in (0,0x8000,0xffff):
            rows.append(make(f'word_wrap{initial_count}_fp{profile}',3,1,3,True,True,profile,1,initial_count=initial_count))
        for match in range(3):
            rows.append(make(f'full_salt_identity{match}_fp{profile}',3,match,1,False,False,profile,salt_mismatch=True))
    for n,row in enumerate(rows):row['id']=n
    return rows

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args()
    assert not a.output.exists();assert ref.sha(NEW_DEFAULT)==NEW_PIN
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);meta=ref.metadata(image,ENTRIES)
    owned=ref.prepare(image,meta,scratch);rows=physical.physical(owned,inputs(),'whole')
    assert all(r['completed']for r in rows)
    reached=set().union(*(set(r['executed_sites'])for r in rows));code=_Code(image,'.text')
    coverage={hex(e):dict(reached=len(set(complete_body(code,e).code)&reached),total=len(complete_body(code,e).code),
        missing=[hex(pc)for pc in sorted(set(complete_body(code,e).code)-reached)])for e in ENTRIES}
    assert all(not r['missing']for r in coverage.values()),coverage
    fixture=dict(fixture_kind='native-clump-detach-actor-v1',image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',
        owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,bank_encoding='sparse-runs-v1',cases=ref.compact(rows))
    encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert encoded==a.verify.read_bytes(),'Independent original repeat differs'
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,cases=len(rows),coverage=coverage,ordered_stores=sum(len(r['writes'])for r in rows),fixture=str(a.output),
        fixture_bytes=len(encoded),fixture_sha256=ref.sha(a.output),original_sha256=NEW_PIN,owned_scratch=str(scratch),
        retained_large_artifacts=[],repeat_identical=bool(a.verify))
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
