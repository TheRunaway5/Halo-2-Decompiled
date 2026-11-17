#!/usr/bin/env python3
"""Unchanged physical200310 actor and278FA0 scenario jump-parameter selection."""
import argparse,gzip,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_area_perform_tu15 as ref
import oracle_native_campaign_ai_path_search_tu15 as physical

ENTRIES=(0x200310,0x278fa0,0x1ecd00)
BASE=0x1050000;POOL=BASE;ACTORS=BASE+0x100;OBJECT_POOL=BASE+0x2000;OBJECT_ROWS=BASE+0x2100
BODY=BASE+0x2200;TAGS=BASE+0x2800;CHARACTER=BASE+0x2900;PARENT=BASE+0x2a00
PROPERTY=BASE+0x2b00;DEFINITION=BASE+0x2c00;SCENARIO=BASE+0x3000;VALUES=BASE+0x3200;OUT=BASE+0x3300

def make(image,name,*,entry=0x200310,count=1,depth=0,character=True,body_flag=0,
         scenario=True,scenario_count=1,kind=0,output='out',index=0,salt=0xe362,
         default=0x3f800000,profile=physical.FP_PROFILES[0]):
    raw=bytearray(0x3400)
    def put(at,value,size=4):raw[at-BASE:at-BASE+size]=(value&((1<<(size*8))-1)).to_bytes(size,'little')
    put(POOL+0x44,ACTORS);put(OBJECT_POOL+0x44,OBJECT_ROWS)
    for i in range(3):
        put(ACTORS+i*0x888+0x18,0xd3450000|i);put(ACTORS+i*0x888+0x54,0xa7650000 if character else 0xffffffff)
        put(OBJECT_ROWS+i*12+8,BODY)
    put(BODY,0xc5670002);put(BODY+0xaa,body_flag,1)
    put(TAGS+8,CHARACTER);put(TAGS+16+8,PARENT);put(TAGS+32+8,DEFINITION)
    put(CHARACTER+8,0xa7650001 if depth else 0xffffffff);put(PARENT+8,0xffffffff)
    put(CHARACTER+0x5c,0 if depth else 1);put(CHARACTER+0x60,PROPERTY)
    put(PARENT+0x5c,1 if depth==1 else 0);put(PARENT+0x60,PROPERTY)
    put(PROPERTY+0x1a,count,2);put(DEFINITION+0x1f8,0x41234567)
    put(SCENARIO+0xc8,scenario_count);put(SCENARIO+0xcc,VALUES)
    for k in range(6):put(VALUES+0x80+k*4,0x3e800001+k*0x123456)
    put(OUT,0xabcdef77)
    gp=[0x11331100+k*0x10101 for k in range(8)]
    gp[0]=(salt<<16)|index;gp[1]=0x9abc0000|(kind&0xffff);gp[4]=ref.SP
    gp[7]=0 if output=='none' else PROPERTY+0x1a if output=='count_alias' else OUT
    banks={BASE:raw,0x5020c8:POOL.to_bytes(4,'little'),0x4ecbd0:OBJECT_POOL.to_bytes(4,'little'),
        0x4f0414:TAGS.to_bytes(4,'little'),0x4ecc1c:(SCENARIO if scenario else 0).to_bytes(4,'little'),
        0x449fd4:default.to_bytes(4,'little')}
    return dict(name=name,entry=entry,gp=gp,matrix=0x12345678,source=0xfedcba98,stack_arguments=[],
        banks=[dict(address=p,before=bytes(v).hex())for p,v in sorted(banks.items())],
        before_fp=list(profile),live_x87=[],xmm=[[0x6b000000+r*4+k for k in range(4)]for r in range(8)],
        max_steps=500,strict_writes=False)

def inputs(image):
    rows=[]
    for profile in physical.FP_PROFILES:
        for count in(-32768,-1,0,1,5,6,7,32767):
            for output in('none','out','count_alias'):
                for depth in(0,1):rows.append(make(image,f'actor_count{count}_{output}_depth{depth}_fp{profile}',count=count,output=output,depth=depth,profile=profile))
        for character,depth in((False,0),(True,2)):
            for body_flag in(0,1,0xff):
                for default in(0,0x80000000,0x7f800001,0x7fc12345):rows.append(make(image,f'fallback_character{character}_depth{depth}_flag{body_flag}_default{default:x}_fp{profile}',character=character,depth=depth,body_flag=body_flag,default=default,profile=profile))
        for scenario,scenario_count in((False,1),(True,-1),(True,0),(True,1)):
            for kind in(-32768,-1,0,1,5,6,7,32767):rows.append(make(image,f'scenario{scenario}_count{scenario_count}_kind{kind}_fp{profile}',entry=0x278fa0,scenario=scenario,scenario_count=scenario_count,kind=kind,profile=profile))
            rows.append(make(image,f'actor_scenario{scenario}_count{scenario_count}_fp{profile}',scenario=scenario,scenario_count=scenario_count,profile=profile))
        for salt in(0,0xffff):
            for index in(0,1,2):rows.append(make(image,f'actor_salt{salt:x}_index{index}_fp{profile}',salt=salt,index=index,profile=profile))
    for n,row in enumerate(rows):row['id']=n
    return rows

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args()
    assert not a.output.exists();assert ref.sha(NEW_DEFAULT)==NEW_PIN
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);meta=ref.metadata(image,ENTRIES)
    owned=ref.prepare(image,meta,scratch);rows=physical.physical(owned,inputs(image),'whole');assert all(r['completed']for r in rows)
    reached=set().union(*(set(r['executed_sites'])for r in rows));code=_Code(image,'.text')
    coverage={hex(e):dict(reached=len(set(complete_body(code,e).code)&reached),total=len(complete_body(code,e).code),missing=[hex(pc)for pc in sorted(set(complete_body(code,e).code)-reached)])for e in ENTRIES}
    assert all(not x['missing']for x in coverage.values()),coverage
    fixture=dict(fixture_kind='native-movement-link-parameters-v1',image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,bank_encoding='sparse-runs-v1',cases=ref.compact(rows))
    encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert encoded==a.verify.read_bytes(),'Independent original repeat differs'
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,cases=len(rows),coverage=coverage,ordered_stores=sum(len(r['writes'])for r in rows),fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=ref.sha(a.output),original_sha256=NEW_PIN,owned_scratch=str(scratch),retained_large_artifacts=[],repeat_identical=bool(a.verify))
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
