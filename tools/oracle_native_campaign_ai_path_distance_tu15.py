#!/usr/bin/env python3
"""Physical unchanged whole200C00 with real point-frame dependencies.

Only retained actor/path/frame data precedes entry. No dependency return,
path decision, private coordinate, instruction or resumed fault is supplied.
"""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_ai_area_tu15 as areas
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry

ENTRIES=(0x200c00,0x218780,0x145070,0x219030)
BASE,ACTOR_POOL,ACTORS,FRAMES,POOL,OBJECTS,BODY=(areas.BASE+n for n in (0,0x100,0x10000,0x30000,0x31000,0x32000,0x34000))


def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]


def inputs(image):
    rows=[]
    def make(name,start=0,end=3,active=1,state=1,frame=0xffff,actor=0xe3610000,
             position=(0.,0.,0.),points=((3.,4.,0.),(3.,4.,12.),(0.,0.,0.)),
             changes=(),timeline=(),precision=None,rounding=None,denorm=None):
        n=len(rows);raw=bytearray(0x38000);record=ACTORS+(actor&0xffff)*0x888
        def put(address,value,width=4):
            assert BASE<=address and address+width<=BASE+len(raw),(name,hex(address),width)
            raw[address-BASE:address-BASE+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little')
        put(ACTOR_POOL+0x44,ACTORS)
        for index in range(start,max(end,start)+4):
            point=record+0x548+index*0x1c
            for axis,value in enumerate(points[(index-start)%len(points)]):put(point+axis*4,bits(value))
            put(point+0xc,frame,2)
        put(record+0x50c,active,1);put(record+0x504,state,2)
        for axis,value in enumerate(position):put(record+0x238+axis*4,bits(value))
        put(record+0x53a,start,1);put(record+0x539,end,1)
        put(POOL+0x44,OBJECTS);put(OBJECTS+8,BODY)
        for index in (-2,0,1,2):
            row=FRAMES+index*16;put(row,0xffffffff if index==1 else 0x198b0000)
            put(row+6,-1 if index==2 else 0,2);put(row+0xa,1,1);put(row+0xb,1,1)
        put(BODY+0x114,52,2);put(BODY+0x116,0x200,2)
        for offset,value in ((0,1.25),(4,1.),(0x14,1.),(0x24,1.),(0x28,.125),(0x2c,-.25),(0x30,.5)):
            put(BODY+0x200+offset,bits(value))
        global_changes=[]
        for address,value,width in changes:
            if BASE<=address<BASE+len(raw):put(address,value,width)
            else:global_changes.append((address,value,width))
        gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[0]=actor;gp[4]=geometry.SP+n%4*4
        p=(0,2,3)[n%3]if precision is None else precision;r=n%4 if rounding is None else rounding
        d=(0,0x40,0x8000,0x8040)[n%4]if denorm is None else denorm
        ranges=((ACTOR_POOL,0x50),(ACTORS-0x1000,0x6000),(FRAMES-0x20,0x60),
                (POOL,0x50),(OBJECTS,0x10),(BODY,0x240))
        banks=[dict(address=address,before=raw[address-BASE:address-BASE+size].hex())for address,size in ranges]
        for address,value in ((0x5020c8,ACTOR_POOL),(0x505e78,FRAMES),(0x4ecbd0,POOL)):
            banks.append(dict(address=address,before=struct.pack('<I',value).hex()))
        for address,width in ((0x44acb0,36),(0x46a0d0,4)):
            banks.append(dict(address=address,before=image.read(address,width).hex()))
        for address,value,width in global_changes:
            bank=next(b for b in banks if b['address']==address);assert len(bank['before'])==width*2
            bank['before']=(value&((1<<(width*8))-1)).to_bytes(width,'little').hex()
        depth=(0,1,3)[n%3]
        rows.append(dict(id=n,name=name,entry=0x200c00,gp=gp,arguments=[],actor=actor,
            banks=sorted(banks,key=lambda b:b['address']),input_timeline=list(timeline),
            before_fp=[0x7f|(p<<8)|(r<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|(r<<13)|d|(n*7%64)],
            live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range(depth)]))
        return rows[-1]
    for active in (0,1,2,255):
        for state in (0,1,2,0xffff):make(f'gate_{active}_{state}',active=active,state=state)
    for start,end in ((0,0),(0,1),(0,3),(1,3),(3,1),(2,2),(-128,-127),(-3,-1),(127,-128),(0,127)):
        make(f'indices_{start}_{end}',start=start,end=end)
    for frame in (0xffff,0,1,2,0xfffe):
        for salt in (0,0xe361,0xffff):make(f'frame_actor_{frame}_{salt}',frame=frame,actor=(salt<<16)|1)
    for precision in (0,2,3):
        for rounding in range(4):
            for denorm in (0,0x40,0x8000,0x8040):
                make(f'fp_{precision}_{rounding}_{denorm}',frame=0,precision=precision,rounding=rounding,denorm=denorm,
                    position=(.125,-.25,.5),points=((.75,1.125,-.375),(-.5,.25,-.125),(0.,0.,0.)))
    for value in (0.,-0.,1.e-20,1.e-15,1.e15,1.e30,float('inf'),-float('inf'),float('nan')):
        make('magnitude_'+str(value),position=(value,-value,value),points=((-value,value,-value),(value,value,value)))
    for axis in range(3):
        for value in (0x80000000,1,0x00800000,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
            make(f'exception_{axis}_{value:x}',changes=((ACTORS+0x548+axis*4,value,4),))
        for first,second in ((0x7fc12345,0xffc54321),(0x7f812345,0xffc54321),(0x7fc12345,0xff854321),(0x7f812345,0xff854321)):
            make(f'paired_nan_{axis}_{first:x}_{second:x}',changes=((ACTORS+0x238+axis*4,first,4),(ACTORS+0x548+axis*4,second,4)))
    def publication(address,occurrence,target,value,width=4):
        return dict(on_read=dict(address=address,occurrence=occurrence),writes=[dict(address=target,bytes=(value&((1<<(8*width))-1)).to_bytes(width,'little').hex())])
    make('fresh_end_shrink',timeline=(publication(ACTORS+0x539,1,ACTORS+0x539,1,1),))
    make('fresh_end_grow',end=1,timeline=(publication(ACTORS+0x548+0xc,1,ACTORS+0x539,3,1),))
    make('previous_point_captured',timeline=(publication(ACTORS+0x53a,1,ACTORS+0x238,bits(70.)),))
    make('failed_frame_second_copy',frame=1,timeline=(publication(ACTORS+0x548,1,ACTORS+0x548,bits(8.)),))
    for frame in (0xffff,0,1,2):make('fault_seed_'+str(frame),frame=frame,end=2)
    row=make('unmapped_actor_pool',changes=((0x5020c8,0xdead0000,4),));row['expected_fault']=True
    row=make('unmapped_actor_rows',changes=((ACTOR_POOL+0x44,0xdead0000,4),));row['expected_fault']=True
    row=make('unmapped_frames',frame=0,changes=((0x505e78,0xdead0000,4),));row['expected_fault']=True
    row=make('unmapped_objects',frame=0,changes=((0x4ecbd0,0xdead0000,4),));row['expected_fault']=True
    row=make('unmapped_body',frame=0,changes=((OBJECTS+8,0xdead0000,4),));row['expected_fault']=True
    return rows


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True);parser.add_argument('--verify',type=Path)
    parser.add_argument('--complete-only',action='store_true');args=parser.parse_args()
    assert not args.output.exists();assert common.sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;metadata=common.metadata(image);code=_Code(image,'.text')
    metadata['private_stack']=dict(begin=geometry.STACK,bytes=65536)
    for site in metadata['sites']:
        if code.at(site['instruction']).mnemonic in ('cmp','test'):
            for operand in site['operands']:operand.update(read=True,write=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=areas.prepare(image,metadata,scratch)
    result=subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_path_distance_tu15.cpp'),'-o',str(owned/'observer')],capture_output=True,text=True)
    assert not result.returncode,result.stderr
    complete=geometry.physical(owned,inputs(image),'complete');prefix=[]
    if not args.complete_only:
        for seed in(r for r in complete if r['name'].startswith('fault_seed')):
            for ordinal in range(len(seed['reads'])):
                row=copy.deepcopy(seed);row.update(id=len(complete)+len(prefix),name=seed['name']+'_read_'+str(ordinal),fault=dict(phase='before_read',ordinal=ordinal));prefix.append(row)
    cases=complete+geometry.physical(owned,prefix,'prefix')if prefix else complete
    assert not any(r['writes']for r in cases),'Path distance published retained storage'
    reached=set().union(*(set(r['executed_sites'])for r in complete));roots={s['instruction']for s in metadata['sites']if 0x200c00<=s['instruction']<=0x200d2a}
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=metadata['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=cases)
    data=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if args.verify:assert data==args.verify.read_bytes(),'Original path-distance repeat differs'
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(data)
    report=dict(passed=True,original_sha256=NEW_PIN,cases=len(cases),complete_calls=sum(r['completed']for r in complete),
        true_memory_faults=sum(not r['completed']for r in complete),fault_prefixes=len(prefix),
        root_sites=len(roots),reached_root_sites=len(roots&reached),unreached_root_sites=[hex(p)for p in sorted(roots-reached)],
        reached_closure_sites=len(reached),strict_retained_operands=True,fixture=str(args.output),fixture_bytes=len(data),fixture_sha256=common.sha(args.output),
        independent_repeat_identical=True if args.verify else None,native_implementation_qualified=False,actual_world_route_qualified=False,
        source_sha256={str(p.relative_to(ROOT)):common.sha(p)for p in (Path(__file__),ROOT/'tools/observe_native_campaign_ai_path_distance_tu15.cpp')},
        owned_scratch=str(scratch),retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k]for k in ('cases','complete_calls','root_sites','reached_root_sites','unreached_root_sites')}))


if __name__=='__main__':main()
