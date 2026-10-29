#!/usr/bin/env python3
"""Physical inherited marker-owner and current-motion reference getters."""
import argparse,copy,gzip,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import oracle_native_campaign_ai_perception_group_link_tu15 as group
import oracle_native_campaign_object_markers_tu15 as markers
import validate_native_geometry_tu15 as geometry
BASE=0x1000000
POOL,COMPONENTS,TAGS,DEFS,HEADERS=BASE+0x100,BASE+0x200,BASE+0x600,BASE+0x700,BASE+0x1000
RECORDS,COMP_ROWS,BODY_ROWS,BODY,MOTION,ZERO,OUT=BASE+0x3000,BASE+0x8000,BASE+0xa000,BASE+0xb000,BASE+0xc000,BASE+0xe000,BASE+0xf000
sha=common.sha
def inputs(image):
    rows=[]
    def make(entry=0xbc200,name='motion',parents=0,component=True,index=0,extent=1,zero_motion=False,body_disabled=False,linear=True,angular=True,object=0xea000001,inherit=False,kind=2,flags=1,pickup=1,destination=OUT,changes=()):
        n=len(rows);banks={BASE:bytearray((k*19+n*29+5)&255 for k in range(0x10000)),0x4ec000:bytearray([0x53]*4096),0x4f0000:bytearray([0x59]*4096),0x52b000:bytearray([0x61]*4096),0x474000:bytearray(image.read(0x474000,4096))}
        def put(p,v,w=4):
            for start,data in banks.items():
                if start<=p and p+w<=start+len(data):data[p-start:p-start+w]=(v&((1<<(w*8))-1)).to_bytes(w,'little');return
            raise ValueError(hex(p))
        for p,v in[(0x4ecbd0,POOL),(POOL+0x44,HEADERS),(0x4f0414,TAGS),(0x52be5c,COMPONENTS),(COMPONENTS+0x44,COMP_ROWS),(0x474d04,ZERO),(COMP_ROWS+0x70,BODY_ROWS),(COMP_ROWS+0x74,extent),(BODY_ROWS+0x40,BODY),(BODY+0x3c,MOTION)]:put(p,v)
        put(COMP_ROWS+0x18,index,1);put(BODY+0x40,int(body_disabled),1)
        for i in range(3):
            record=RECORDS+i*0x1000;put(HEADERS+(i+1)*12+8,record);put(record,0xe4000000+i);put(record+0x14,0xea000002+i if i<parents else 0xffffffff);put(record+0xb4,0xe2000000 if component else 0xffffffff);put(record+0xc1,int(zero_motion),1);put(record+4,flags,1);put(record+0xaa,kind,1);put(record+0x12c,pickup,1);put(record+0x154,0xea00a0a0+i)
            put(TAGS+i*16+8,DEFS+i*0x100);put(DEFS+i*0x100+2,0x10 if inherit else 0,1)
            for j in range(6):put(record+0x88+j*4,0x3f000000+i*0x10000+j*0x1000)
        for j in range(3):put(ZERO+j*4,0x3e800000+j*0x10000);put(MOTION+0x40+j*4,0x3f800000+j*0x10000);put(MOTION+0x50+j*4,0xbf800000+j*0x10000)
        if object==0xffffffff:
            put(POOL+0x44,BASE-0xffff*12);put(BASE+8,RECORDS)
        for p,v,w in changes:put(p,v,w)
        gp=[0x5a100000+n*13+k*7 for k in range(8)];gp[4]=geometry.SP+n%4*4;args=[]
        if entry==0xbc200:gp[0]=object;gp[1]=destination if linear else 0;args=[destination+0x10 if angular else 0]
        elif entry in(0x1d8c80,0x1d8d80):gp[0]=index&0xffffffff;gp[1]=destination;gp[2]=COMP_ROWS
        elif entry==0xbacd0:args=[object]
        else:gp[1]=object
        rows.append(dict(id=n,name=name+'_'+str(n),entry=entry,gp=gp,arguments=args,object=object,component=COMP_ROWS,index=index&0xffffffff,destination=destination,linear=destination if linear else 0,angular=destination+0x10 if angular else 0,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|(n*3%64),0x1f80|((n%4)<<13)|(n*5%64)],live_x87=[]))
    for parents in(0,1,2):
        for component in(False,True):
            for linear in(False,True):
                for angular in(False,True):
                    for zero in(False,True):
                        for disabled in(False,True):make(parents=parents,component=component,linear=linear,angular=angular,zero_motion=zero,body_disabled=disabled)
    for index,extent in[(0,0),(0,-1),(1,1),(0x80,1000),(0xff,1000),(0x7f,0x7f)]:make(name='signed_motion_admission',index=index,extent=extent)
    make(name='none_object_wrap',object=0xffffffff,component=False)
    for entry in(0x1d8c80,0x1d8d80):
        for disabled in(False,True):
            for destination in(OUT,MOTION+0x44,ZERO+4,BODY_ROWS+0x40):make(entry,'component_alias',body_disabled=disabled,destination=destination)
    for destination in(RECORDS+0x8c,MOTION+0x44,ZERO+4):make(name='motion_alias',destination=destination)
    for kind in range(256):make(0xbacd0,'marker_kind',kind=kind)
    for pickup in range(8):make(0xfc510,'pickup_flags',pickup=pickup);make(0xbacd0,'pickup_owner',pickup=pickup)
    for parents in(0,1,2):
        for inherit in(False,True):
            for flags in(0,1,0xff):make(0xbacd0,'inheritance',parents=parents,inherit=inherit,flags=flags)
    make(0xbacd0,'fault_seed_owner',parents=2,inherit=True)
    make(name='fault_seed_motion')
    make(name='fault_seed_fallback',component=False)
    make(0x1d8c80,'fault_seed_component')
    make(0x1d8d80,'fault_seed_zero',body_disabled=True)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();assert a.verify or not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);common.ENTRIES=(0xbacd0,0xfc510,0xbc200,0x1d8c80,0x1d8d80);meta=common.metadata(image);owned=markers.prepare(image,meta,Path(os.environ['HALO2_TEST_SCRATCH']));rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in(r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for n in range(len(events)):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));missing=[hex(s['instruction'])for s in meta['sites']if s['instruction']not in visited];assert not missing,missing
    fixture=group.intern_banks(dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows));encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert a.output.read_bytes()==encoded,'Object motion original repeat differs'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,owners=meta['owners'],cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),all_sites=len(visited),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),original_instructions=sum(r['original_instructions']for r in rows),fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=sha(a.output),owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[],source_hashes={name:sha(ROOT/name)for name in[str(Path(__file__).relative_to(ROOT)),'tools/observe_native_campaign_object_markers_tu15.cpp','tools/oracle_native_campaign_object_markers_tu15.py']});a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k not in('owners','source_hashes')}))
if __name__=='__main__':main()
