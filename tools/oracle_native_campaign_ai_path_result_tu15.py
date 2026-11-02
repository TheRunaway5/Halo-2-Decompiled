#!/usr/bin/env python3
"""Unchanged physical27A460 path cost, clearance and retained route direction.

The reference follows real retained hash/parent links and all real geometry
dependencies. Inputs contain no private intermediate or supplied path outcome.
"""
import argparse
import copy
import gzip
import json
import os
from pathlib import Path
import struct
import subprocess

from oracle_smg_firing_1_1 import ROOT, Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
import oracle_native_campaign_ai_area_tu15 as areas
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry

ENTRIES=(0x27a460,0x27c410,0x27c450,0x218c40,0x218b20,0x218780,
         0x145070,0x219030,0x218eb0,0x218880,0x145140,0x30bd0)
BASE,GOAL,COST,CLEARANCE,DIRECTION,WORKSPACE,FRAMES,POOL,OBJECTS,BODY=(areas.BASE+n for n in
    (0,0x100,0x300,0x304,0x310,0x4000,0x19000,0x1a000,0x1b000,0x1c000))


def bits(value):
    return struct.unpack('<I',struct.pack('<f',value))[0]


def inputs(image):
    rows=[]
    fallback=int.from_bytes(image.read(0x474d04,4),'little')
    def make(name,found=True,chain=3,key=0x890001ff,enabled=True,
             clearance=CLEARANCE,direction=DIRECTION,cost=COST,
             steps=(.4,.5,.6),radius=.75,node_cost=2.,frame=0xffff,goal_frame=0xffff,
             positions=None,goal=(5.,2.,0.),query=(-1.,1.,0.),changes=(),timeline=()):
        n=len(rows);raw=bytearray(0x24000)
        def put(address,value,width=4):
            assert BASE<=address and address+width<=BASE+len(raw),(hex(address),width)
            raw[address-BASE:address-BASE+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little')
        raw[WORKSPACE+0x120b6-BASE:WORKSPACE+0x140b6-BASE]=b'\xff'*0x2000
        def point(address,xyz,coordinate_frame):
            for axis,value in enumerate(xyz):put(address+axis*4,bits(value))
            put(address+0xc,coordinate_frame,2)
        point(GOAL,goal,goal_frame);point(WORKSPACE+0x14,(0.,0.,0.),frame)
        put(WORKSPACE+0x28,enabled,1)
        for axis,value in enumerate(query):put(WORKSPACE+0x2c+axis*4,bits(value))
        for node in range(chain):
            record=WORKSPACE+0xb0+node*0x44
            put(record,0x9191,2);put(record+2,node-1 if node else 0xffff,2)
            put(record+8,key if node==chain-1 else key^0x10000000)
            xyz=positions[node%len(positions)] if positions else (float(node+1),0.,0.)
            point(record+0x18,xyz,frame)
            put(record+0x28,bits(steps[node%len(steps)]))
            put(record+0x2c,bits(radius));put(record+0x30,bits(node_cost))
        if found:
            assert chain>0
            slot=(key&0x1ff)<<3
            put(WORKSPACE+0x120b6+slot*2,chain-1,2)
        put(FRAMES,0x198b0000);put(FRAMES+6,0,2);put(FRAMES+0xa,2,1);put(FRAMES+0xb,1,1)
        put(POOL+0x44,OBJECTS);put(OBJECTS+8,BODY)
        put(BODY+0x114,52,2);put(BODY+0x116,0x200,2)
        for offset,value in ((0,1.),(4,1.),(0x14,1.),(0x24,1.),(0x28,1.)):
            put(BODY+0x200+offset,bits(value))
        for address,value,width in changes:put(address,value,width)
        gp=[0x5a010000+n*31+k*7 for k in range(8)]
        gp[0]=GOAL;gp[1]=WORKSPACE;gp[2]=key;gp[4]=geometry.SP+n%4*4
        banks=[dict(address=BASE,before=raw.hex())]
        for address,value in ((0x505e78,FRAMES),(0x4ecbd0,POOL)):
            banks.append(dict(address=address,before=struct.pack('<I',value).hex()))
        for address,size in ((0x44acb0,36),(0x46a0d0,4),(0x46a0e8,4),(0x46a0ec,4),
                             (0x456910,4),(0x45695c,4),(0x474d04,4),(fallback,12)):
            banks.append(dict(address=address,before=image.read(address,size).hex()))
        depth=(0,1,3)[n%3]
        rows.append(dict(id=n,name=name,entry=0x27a460,gp=gp,arguments=[cost,clearance,direction],
            workspace=WORKSPACE,goal=GOAL,key=key,cost=cost,clearance=clearance,direction=direction,
            banks=sorted(banks,key=lambda b:b['address']),input_timeline=list(timeline),
            before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),
                       0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|(n*7%64)],
            live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex() for k in range(depth)]))
    for found in (False,True):
        for enabled in (False,True):
            for clearance in (0,CLEARANCE):
                for direction in (0,DIRECTION):
                    make(f'admission_{found}_{enabled}_{clearance:x}_{direction:x}',found=found,
                         enabled=enabled,clearance=clearance,direction=direction)
    for chain in (1,2,3,5):
        for steps in ((0.,),(.9,),(.4,.5,.6),(-1.,),(float('nan'),)):
            make('chain_'+str(chain)+'_'+str(steps),chain=chain,steps=steps)
    for radius in (0.,-0.,.1,1.,10.,-1.,float('inf'),float('nan')):
        make('radius_'+str(radius),radius=radius)
    for cost in (0.,-1.,float('inf'),float('nan')):
        make('node_cost_'+str(cost),node_cost=cost)
    for first,second in ((0xffff,0xffff),(0,0xffff),(0xffff,0),(0,0)):
        make('frames_'+str(first)+'_'+str(second),frame=first,goal_frame=second)
    for point in (((0.,0.,0.),),((5.,2.,0.),),((1.e-6,0.,0.),)):
        make('geometry_'+str(point),positions=point)
    for direction in (COST,WORKSPACE+0x14,WORKSPACE+0xb0+0x18):
        make('direction_alias_'+hex(direction),direction=direction)
    for cost in (WORKSPACE+0x14,WORKSPACE+0xb0+2*0x44+0x18):
        make('cost_alias_'+hex(cost),cost=cost)
    for value in (1,0x80000001,0x7f800000,0x7fc12345,0x7f812345):
        make('goal_exception_'+hex(value),changes=((GOAL,value,4),))
    def publication(address,occurrence,target,value):
        return dict(on_read=dict(address=address,occurrence=occurrence),writes=[dict(address=target,bytes=struct.pack('<I',value).hex())])
    cap=WORKSPACE+0xb0+2*0x44+0x2c
    make('fresh_cap',timeline=(publication(cap,1,cap,bits(8.)),))
    for n,options in enumerate((dict(),dict(enabled=False),dict(found=False),dict(frame=0),dict(direction=COST))):
        make('fault_seed_'+str(n),**options)
    return rows


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--verify',type=Path)
    parser.add_argument('--complete-only',action='store_true')
    args=parser.parse_args();assert not args.output.exists();assert common.sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;metadata=common.metadata(image);code=_Code(image,'.text')
    metadata['private_stack']=dict(begin=geometry.STACK,bytes=65536)
    for site in metadata['sites']:
        if code.at(site['instruction']).mnemonic in ('cmp','test'):
            for operand in site['operands']:operand.update(read=True,write=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=areas.prepare(image,metadata,scratch)
    result=subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_path_result_tu15.cpp'),'-o',str(owned/'observer')],capture_output=True,text=True)
    assert result.returncode==0,result.stderr
    complete=geometry.physical(owned,inputs(image),'complete');prefix=[]
    if not args.complete_only:
        for seed in (row for row in complete if row['name'].startswith('fault_seed')):
            for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
                for ordinal in range(len(events)):
                    row=copy.deepcopy(seed);row.update(id=len(complete)+len(prefix),name=seed['name']+'_'+phase+'_'+str(ordinal),fault=dict(phase=phase,ordinal=ordinal));prefix.append(row)
    cases=complete+geometry.physical(owned,prefix,'prefix') if prefix else complete
    faults=[]
    if not args.complete_only:
        for key,register,argument in (('goal',0,None),('workspace',1,None),
                                     ('cost',None,0),('clearance',None,1),('direction',None,2)):
            row=copy.deepcopy(complete[15])
            row.update(id=len(cases)+len(faults),name='actual_missing_'+key,expected_fault=True)
            row[key]=0xdead0000
            if register is not None:row['gp'][register]=row[key]
            else:row['arguments'][argument]=row[key]
            faults.append(row)
        cases+=geometry.physical(owned,faults,'actual-faults')
    reached=set().union(*(set(r['executed_sites'])for r in complete))
    roots={s['instruction']for s in metadata['sites']if 0x27a460<=s['instruction']<0x27a640}
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=metadata['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=cases)
    data=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if args.verify:assert data==args.verify.read_bytes(),'Original path-result repeat differs'
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(data)
    assert roots-reached=={0x27a5c7,0x27a5c9},sorted(roots-reached)
    report=dict(passed=True,original_sha256=NEW_PIN,cases=len(cases),complete_calls=len(complete),fault_prefixes=len(prefix),actual_faults=len(faults),root_sites=len(roots),reached_root_sites=len(roots&reached),unreached_root_sites=[hex(p)for p in sorted(roots-reached)],reached_closure_sites=len(reached),fixture=str(args.output),fixture_bytes=len(data),fixture_sha256=common.sha(args.output),independent_repeat_identical=True if args.verify else None,strict_retained_operands=True,native_implementation_qualified=False,actual_world_route_qualified=False,source_sha256={str(p.relative_to(ROOT)):common.sha(p)for p in (Path(__file__),ROOT/'tools/observe_native_campaign_ai_path_result_tu15.cpp')},owned_scratch=str(scratch),retained_large_artifacts=[],scope_limits=['Ordinary whole-entry data/output records disjoint from the callee private frame','Masked arithmetic with at most three older x87 values; no stack-fault or unmasked trap owner','Two root sites remain unexecuted; their control-flow invariant inference is not native implementation proof'],unexecuted_inference='Within the admitted ordinary frame profile, saved found AX is notFFFF. Every parent-loop backedge requires the next CX notFFFF, so the last visited DX is notFFFF at27A57E. Every scan-loop entry likewise has CX notFFFF, since its backedge27A5BB tests that value and its initial CX is that last DX. The two predecessors to27A5C7 therefore appear false in this profile. Private-callee-frame output aliases remain outside the profile.')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k]for k in ('cases','complete_calls','root_sites','reached_root_sites','unreached_root_sites')}))


if __name__=='__main__':
    main()
