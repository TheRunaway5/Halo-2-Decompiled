#!/usr/bin/env python3
"""Unchanged physical-i386 object-navigation and biped cached-point paths.

Parent chains, original type bytes, optional outputs and real caller frames
are inputs. Refresh and lookup children execute unchanged original code;
F35D0/276E10 remain named boundaries, never supplied outcomes. Refresh
compositions compare retained DATA and FP with owned semantic collision DATA.
Executable observers and raw memory banks live in HALO2_TEST_SCRATCH only.
"""
import argparse
import copy
import gzip
import json
import os
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_area_perform_tu15 as reference
import oracle_native_campaign_ai_goal_evaluation_tu15 as goals
import oracle_native_campaign_ai_biped_refresh_tu15 as refresh
import oracle_native_campaign_ai_navigation_parents_tu15 as lookup_parents

OWNED = (0x275ba0, 0x275cb0, 0xe2300, 0xe04b0, 0xe22c0, 0x2186f0)
ENTRIES = tuple(dict.fromkeys(OWNED + (0xbcdf0, 0xbbe00) + refresh.ENTRIES + lookup_parents.ENTRIES))
STOPS = (0xf35d0, 0x276e10)
BASE = reference.areas.BASE
POOL, ROWS, BODIES, SCENARIO, OUTPUT = (BASE+n for n in (0x100,0x200,0x1000,0x12000,0x13000))
NONE = 0xffffffff
FP = ((0x27f,0,0x1f80),(0x37f,0x20,0x1fa0),(0x67f,0x3800,0x9fc0))

def inputs(image):
    result=[]
    eligible=[dict(name='other',kind=2),dict(name='signed_type',kind=255),
        dict(name='unit_state0',kind=0,state=0,flags=255),
        dict(name='unit_state2',kind=0,state=2,flags=255),
        *[dict(name=f'unit_flag{flag}',kind=0,state=1,flags=flag) for flag in (0,1,2,128,255)],
        dict(name='device_state0',kind=12,state=0,flags=255),
        dict(name='device_state2',kind=12,state=2,flags=255),
        *[dict(name=f'device_flag{flag}',kind=12,state=1,flags=flag) for flag in (0,1,127,128,255)]]
    point=[dict(name='unit_boundary',kind=0),dict(name='vehicle_boundary',kind=1),
        dict(name='negative_count',kind=2,count=-1),dict(name='empty',kind=2,count=0),
        dict(name='empty_signed_type',kind=255,count=0),
        dict(name='null_scenario_block',kind=2,count=1,block=0),
        dict(name='projection_boundary',kind=2,count=1,block=0xa5150001)]
    for entry,variants in ((0x275ba0,eligible),(0x275cb0,point)):
        for chain in (0,1,7,31):
            for variant in variants:
                for fp in FP:
                    raw=bytearray(0x14000)
                    def put(address,value,width=4):
                        raw[address-BASE:address-BASE+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')
                    put(POOL+0x44,ROWS)
                    for index in range(chain+1):
                        body=BODIES+index*0x500
                        put(ROWS+index*12+8,body)
                        put(body+0x14,(0x81370000+index+1) if index<chain else NONE)
                        put(body+0xaa,variant['kind'] if index==chain else 1,1)
                        put(body+0x3dc,variant.get('state',0),1);put(body+0x17c,variant.get('state',0),1)
                        put(body+0x348,variant.get('flags',0),1);put(body+0x12c,variant.get('flags',0),1)
                        for offset,value in ((0x30,0x7fc12345),(0x34,0x80000000),(0x38,1)):
                            put(body+offset,value)
                    put(SCENARIO+0xc4,variant.get('count',0));put(SCENARIO+0xc8,variant.get('block',0))
                    raw[OUTPUT-BASE:OUTPUT-BASE+0x20]=bytes(range(0xa0,0xc0))
                    n=len(result);gp=[0xa6120000+n*31+k*7 for k in range(8)]
                    gp[4]=reference.SP;gp[0 if entry==0x275ba0 else 1]=0xa1370000
                    stack=bytearray(struct.pack('<I',0xdef01234+n)*0x60)
                    # Both absent and present optional outputs; point pointer is
                    # preserved into the real callee ABI, including null.
                    index_out=OUTPUT if (n//3)%2 else 0
                    point_out=OUTPUT+4 if (n//6)%2 else 0
                    struct.pack_into('<III',stack,0x160,reference.STOP,index_out,point_out)
                    banks={BASE:bytes(raw),0x4ecbd0:struct.pack('<I',POOL),
                        0x4ecc18:struct.pack('<I',SCENARIO),0x474d10:struct.pack('<I',0xaabbccdd),
                        reference.SP-0x160:bytes(stack)}
                    result.append(dict(id=n,name=f'{entry:x}_{variant["name"]}_parents{chain}_{fp[0]:x}',
                        entry=entry,gp=gp,matrix=index_out,source=point_out,
                        banks=[dict(address=a,before=b.hex()) for a,b in sorted(banks.items())],
                        before_fp=list(fp),live_x87=[],xmm=[[0x6b100000+n*37+r*4+k for k in range(4)] for r in range(8)],
                        stop_before_entries=list(STOPS),max_steps=2000,strict_writes=True,
                        xmm_exact=True,query_scratch_exact=True,object_navigation_exact=True))
    # The sentinel input still dereferences the sentinel slot in the original.
    # Supply that slot explicitly; do not invent a null-handle early return.
    for row in [r for r in result if 'parents0_' in r['name'] and r['before_fp']==list(FP[0])]:
        row=json.loads(json.dumps(row));row['id']=len(result);row['name']+='_sentinel_slot'
        row['gp'][0 if row['entry']==0x275ba0 else 1]=NONE
        row['banks'].append(dict(address=ROWS+0xffff*12+8,before=struct.pack('<I',BODIES).hex()))
        row['banks'].sort(key=lambda b:b['address']);result.append(row)
    # Direct E2300 entries independently declare cached navigation state,
    # invalid/live moving objects, update ticks and aliased optional outputs.
    # Children are stopped before execution when their native owners are absent.
    variants=[dict(name='cached'),dict(name='dead',state=2),dict(name='dead_attached',state=2,attached=True),
        dict(name='none_no_update',sector=NONE,tick=8,last=8),
        dict(name='none_older_tick',sector=NONE,tick=7,last=8),
        dict(name='refresh',sector=NONE,tick=9,last=8),
        dict(name='refresh_invalid_object',sector=NONE,tick=9,last=8,object=0x81370002),
        dict(name='refresh_valid_object',sector=NONE,tick=9,last=8,object=0x81370000),
        dict(name='dynamic',bsp=NONE,dynamic=1,tick=9,last=8),
        dict(name='static',bsp=NONE,dynamic=0),
        dict(name='sector_lookup',index=NONE),
        dict(name='invalid_object',object=0x81370002),
        dict(name='dead_salt',object=0x81370000,salt=0),
        dict(name='wrong_salt',object=0x81370000,salt=0x8136),
        dict(name='live_object',object=0x81370000),
        dict(name='aliased_index',alias='index'),dict(name='aliased_point',alias='point')]
    for variant in variants:
        for fp in FP:
            row=json.loads(json.dumps(result[0]));row['entry']=0xe2300;row['name']='e2300_'+variant['name']+'_'+hex(fp[0])
            raw=bytearray(0x14000)
            def put(address,value,width=4):
                raw[address-BASE:address-BASE+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')
            put(POOL+0x44,ROWS);put(POOL+0x38,1);put(POOL+0x24,12)
            put(ROWS,variant.get('salt',0x8137),2);put(ROWS+3,0,1);put(ROWS+8,BODIES)
            put(BODIES+0x14,NONE);put(BODIES+0x3dc,variant.get('state',0),1)
            if variant.get('attached'):
                parent=BODIES+0x500;matrix=parent+0x100
                put(BODIES+0x14,0x81370001);put(BODIES+0x18,0,1);put(ROWS+20,parent)
                put(parent+0x116,0x100,2)
                for offset,value in ((0,0x3fc00000),(4,0x3f800000),(0x14,0x3f800000),(0x24,0x3f800000),
                    (0x28,0x40800000),(0x2c,0x40a00000),(0x30,0x40c00000)):
                    put(matrix+offset,value)
            for offset,value in ((0x350,variant.get('last',8)),(0x354,variant.get('sector',3)),
                (0x358,variant.get('index',7)),(0x35c,variant.get('bsp',4)),
                (0x360,variant.get('object',NONE)),(0x364,0x12340002),
                (0x64,0x3f800000),(0x68,0x40000000),(0x6c,0x40400000),
                (0x368,0x80000000),(0x36c,0x7fc12345),(0x370,1)):
                put(BODIES+offset,value)
            put(SCENARIO+8,variant.get('tick',8));put(SCENARIO+0x2c,SCENARIO+0x100)
            put(SCENARIO+0x100+3*8+4,variant.get('dynamic',0),1)
            stack=bytearray(struct.pack('<I',0xdef0a125)*0x60)
            args=[OUTPUT,OUTPUT+4,OUTPUT+8,OUTPUT+20,OUTPUT+24]
            if variant.get('alias')=='index':args[0]=BODIES+0x358
            if variant.get('alias')=='point':args[2]=BODIES+0x354
            struct.pack_into('<IIIIII',stack,0x160,reference.STOP,*args)
            banks={BASE:bytes(raw),0x4ecbd0:struct.pack('<I',POOL),
                0x4ecc10:struct.pack('<I',SCENARIO),0x51df54:struct.pack('<I',SCENARIO),
                reference.SP-0x160:bytes(stack)}
            if variant.get('attached'):banks[0x46a0d0]=struct.pack('<I',0x3f800000)
            row.update(id=len(result),gp=[0x81370000,0x99887766,0x88776655,0x12345678,reference.SP,0xabcdaaaa,0xabcdbbbb,0xabcdcccc],
                banks=[dict(address=a,before=b.hex()) for a,b in sorted(banks.items())],
                before_fp=list(fp),matrix=args[0],source=args[1],max_steps=4000)
            if variant.get('attached'):row['xmm_exact']=False
            result.append(row)
            if variant['name'] in ('cached','dead','none_no_update','live_object','refresh'):
                composed=json.loads(json.dumps(row));composed.update(entry=0x275cb0,id=len(result),name='275cb0_'+row['name'])
                composed['gp'][0]=0x99112233;composed['gp'][1]=0x81370000
                composed['matrix']=OUTPUT;composed['source']=OUTPUT+8
                for bank in composed['banks']:
                    if bank['address']==BASE:
                        data=bytearray.fromhex(bank['before']);data[BODIES-BASE+0xaa]=0;bank['before']=data.hex()
                    if bank['address']==reference.SP-0x160:
                        data=bytearray.fromhex(bank['before']);struct.pack_into('<III',data,0x160,reference.STOP,OUTPUT,OUTPUT+8);bank['before']=data.hex()
                result.append(composed)
            if variant['name'] in ('cached','dead','none_no_update','static','dynamic'):
                for entry in (0xe04b0,0xe22c0):
                    leaf=json.loads(json.dumps(row));leaf['entry']=entry;leaf['id']=len(result);leaf['name']=hex(entry)+'_'+row['name'];result.append(leaf)
    for row in result:
        if row['entry']==0xe2300:
            row['implicit_stack_addresses']=[reference.SP-n for n in range(0x10,0x40,4)]
        elif row['entry']==0x275cb0:
            row['implicit_stack_addresses']=[reference.SP-0x98-n for n in range(0x10,0x40,4)]
    # Former refresh/lookup boundary rows did not declare collision or
    # navigation DATA. Replace those placeholders with complete original
    # compositions; every unrelated numeric-frame row stays exact.
    result=[row for row in result if not (
        row['entry'] in (0xe2300,0x275cb0) and (
            'e2300_refresh' in row['name'] or row['name'].startswith('e2300_dynamic_') or
            row['name'].startswith('e2300_sector_lookup_') or row['name'].startswith('275cb0_e2300_live_object_')))]
    for row in lookup_parents.inputs(image):
        # This independent cohort qualifies whole numeric caller frames;
        # operand faults belong to the standalone lookup-owner fixture.
        row['navigation_composition_whole_only']=True;result.append(row)
    for row in refresh.cases(image):
        if row['entry'] not in (0xe2300,0x275cb0):continue
        row.update(retained_refresh_data=True,xmm_exact=False,query_scratch_exact=False,
            object_navigation_exact=False,xmm=[[0]*4 for _ in range(8)],stop_before_entries=list(STOPS))
        result.append(row)
    for n,row in enumerate(result):row['id']=n
    return result

def prefixes(rows,bodies):
    exact=[r for r in rows if not r.get('retained_refresh_data') and not r.get('navigation_composition_whole_only')]
    result=goals.select_prefixes(exact,bodies);seen=set()
    for row in rows:
        if not row.get('retained_refresh_data'):continue
        for ordinal,event in enumerate(row['writes']):
            pc=event['instruction']
            if pc not in bodies or pc in seen:continue
            seen.add(pc);r=copy.deepcopy(row);r.update(name=row['name']+'_after_'+hex(pc),fault=dict(phase='after_write',ordinal=ordinal))
            for b in r['banks']:b.pop('after',None)
            result.append(r)
    return result

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True);parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--verify',type=Path)
    args=parser.parse_args();assert reference.sha(NEW_DEFAULT)==NEW_PIN
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT)
    meta=reference.metadata(image,ENTRIES);owned=reference.prepare(image,meta,scratch)
    rows=reference.physical(owned,inputs(image),'complete');assert all(row['completed'] for row in rows)
    code=_Code(image,'.text');bodies={entry:set(complete_body(code,entry).code) for entry in OWNED}
    checkpoints=prefixes(rows,set().union(*bodies.values()))
    for n,row in enumerate(checkpoints):row['id']=len(rows)+n
    rows+=reference.physical(owned,checkpoints,'prefix')
    reached=set().union(*(set(row['executed_sites']) for row in rows))
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],
        instruction_patches=[],supplied_original_outcomes=False,stop_entries=list(STOPS),
        bank_encoding='sparse-runs-v1',cases=reference.compact(rows))
    data=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if args.verify:assert data==args.verify.read_bytes(),'Independent object-navigation reference differs'
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(data)
    report=dict(passed=True,cases=len(rows),whole=len(rows)-len(checkpoints),prefixes=len(checkpoints),
        coverage={hex(e):dict(reached=len(bodies[e]&reached),total=len(bodies[e])) for e in OWNED},
        original_instructions=sum(r['original_instructions'] for r in rows),fixture_sha256=reference.sha(args.output),
        fixture_bytes=len(data),independent_repeat_identical=True if args.verify else None,
        scratch=str(scratch),retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))

if __name__=='__main__':main()
