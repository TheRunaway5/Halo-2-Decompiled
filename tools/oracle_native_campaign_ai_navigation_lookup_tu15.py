#!/usr/bin/env python3
"""Physical unchanged-i386 202660/2185E0 navigation lookup closure.

The reference runs unmodified source code and all eight actual callees. No
original result is supplied. Compiler products and raw banks require the
standard HALO2_TEST_SCRATCH owner; the sparse fixture is a reference input.
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
import oracle_native_campaign_ai_frame_geometry_tu15 as frame_geometry

ENTRIES=(0x202660,0x2185e0,0x29ff40,0x2028a0,0x202850,0x14ce30,0x123210,0x1231d0,0x1f7e00,0x218960,0x145200,0x218fe0)
BASE=reference.areas.BASE
POOL,ROWS,BODY,SCENARIO,NAV,MODELS,INSTANCES,SECTIONS,FRAMES,BSP,SURFACES,PLANES,ROOTS,NODES,POINT,TAGS,TAG,MODEL,COLLISION=(BASE+n for n in (0x100,0x200,0x400,0x1000,0x2000,0x2100,0x2200,0x2300,0x2500,0x3000,0x3100,0x3200,0x3300,0x3400,0x3500,0x4000,0x4200,0x4400,0x4600))
NONE=0xffffffff
FP=((0x27f,0,0x1f80),(0x37f,0x20,0x1fa0),(0x67f,0x3800,0x9fc0),(0xf7f,0x4000,0x7f80))

def inputs(image):
    result=[]
    variants=[]
    for entry in (0x202660,0x2185e0):
        for count in (0,-1):variants.append(dict(entry=entry,name=f'empty{count}',count=count))
        variants.append(dict(entry=entry,name='null_navigation',block=0))
    for kind in (6,7,0,255):
        variants.append(dict(entry=0x2185e0,name=f'frame_kind{kind}',kind=kind))
    for name,changes in (
        ('negative_model',dict(model_index=-1)),('high_model',dict(model_index=2)),
        ('negative_model_count',dict(model_count=-1)),('no_frame_flag',dict(flags=0)),
        ('other_flags',dict(flags=254)),('all_flags',dict(flags=255)),
        ('sentinel_encoded',dict(encoded=NONE)),('missing_model',dict(model=NONE)),
        ('missing_collision',dict(collision=NONE)),('frame_bound',dict(encoded=3)),
        ('frame_negative_count',dict(frame_count=-1)),('low_byte',dict(encoded=0xff00aa02)),
        ('frame_ffff',dict(frame_value=65535)),('frame_signed',dict(frame_value=32768)),
        ('signed_region',dict(region=-120)),('signed_nodes',dict(nodes=-64))):
        variants.append(dict(entry=0x2185e0,name=name,**changes))
    for route in ('instance','object'):
        for name,changes in (
            ('base',{}),('relative_last',dict(sector=5)),('relative_after',dict(sector=6)),
            ('relative_negative',dict(sector=-2)),('base_none',dict(base=NONE)),
            ('base_negative',dict(base=0xfffffffe)),('sum_negative',dict(base=0x7fffffff,end=0x80000000,sector=1)),
            ('sector_bound',dict(sector_count=12)),('negative_sector_count',dict(sector_count=-1)),
            ('model_negative',dict(model_index=-1)),('model_bound',dict(model_index=2)),
            ('negative_models',dict(model_count=-1))):
            variants.append(dict(entry=0x202660,name=route+'_'+name,route=route,**changes))
    for name,changes in (
        ('negative_instance',dict(instance=-2)),('instance_bound',dict(instance=2)),
        ('negative_instances',dict(instance_count=-1))):
        variants.append(dict(entry=0x202660,name=name,route='instance',**changes))
    for name,changes in (
        ('kind7',dict(kind=7)),('other_kind',dict(kind=1)),('empty_sections',dict(section_count=0)),
        ('negative_sections',dict(section_count=-1)),('second_section',dict(encoded=0xaabbccdd)),
        ('third_section',dict(encoded=0x12345678)),('missing_section',dict(encoded=0x87654321))):
        variants.append(dict(entry=0x202660,name='object_'+name,route='object',**changes))
    for name,changes in (
        ('null_bsp',dict(bsp=0)),('none_surface',dict(sector=NONE)),('negative_surface',dict(sector=-2)),
        ('surface_bound',dict(sector=3)),('negative_surfaces',dict(surface_count=-1)),
        ('none_root',dict(root=NONE)),('leaf_root',dict(root=9)),('invalid_node',dict(root=0x80000002)),
        ('negative_nodes',dict(node_count=-1)),('double_tree',dict(double_tree=True)),
        ('positive_leaf',dict(point=(8.,6.,4.))),('negative_leaf',dict(point=(-8.,-6.,-4.))),
        ('epsilon_equal',dict(point=(0.,0.,0.),distance=0.)),
        ('epsilon_above',dict(point=(0.,0.,1e-5),distance=0.)),
        ('nan_point',dict(point_bits=(0x7fc12345,0,0))),
        ('snan_point',dict(point_bits=(0x7f812345,0,0))),
        ('denormal_point',dict(point_bits=(1,0x80000001,1)))):
        variants.append(dict(entry=0x202660,name='static_'+name,route='static',**changes))
    normals=((3.,2.,1.,4.),(1.,3.,2.,4.),(1.,2.,3.,4.),(1.,1.,1.,0.),
             (0.,0.,0.,0.),(-3.,2.,1.,4.),(1.,-3.,2.,4.),(1.,2.,-3.,4.))
    for axis,normal in enumerate(normals):
        for flipped in (False,True):
            variants.append(dict(entry=0x202660,name=f'static_normal{axis}_flip{flipped}',route='static',normal=normal,flipped=flipped))
    for name,normal in (('qnan',(0x7fc12345,0x3f800000,0)),('snan',(0x7f812345,0x40000000,0)),
                        ('denormal',(1,0x80000002,3)),('infinite',(0x7f800000,0xff800000,0))):
        for flipped in (False,True):variants.append(dict(entry=0x202660,name=f'static_{name}_flip{flipped}',route='static',normal_bits=normal+(0,),flipped=flipped))
    for variant in variants:
        for fp in FP:
            raw=bytearray(0x5000)
            def put(address,value,width=4):raw[address-BASE:address-BASE+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')
            def floats(address,values):
                for n,value in enumerate(values):put(address+n*4,reference.bits(value))
            put(POOL+0x44,ROWS);put(ROWS+8,BODY);put(BODY,0xa1110001);put(BODY+0xaa,variant.get('kind',6),1)
            put(BODY+0x130,variant.get('model_index',0),2);put(BODY+0x1d4,variant.get('model_index',0),2)
            put(BODY+0x116,variant.get('nodes',0x180),2);put(BODY+0x11a,variant.get('region',0x190),2)
            put(SCENARIO+0xc4,variant.get('count',1));put(SCENARIO+0xc8,variant.get('block',NAV))
            put(NAV,variant.get('sector_count',50));put(NAV+0x30,variant.get('model_count',2));put(NAV+0x34,MODELS)
            put(NAV+0x40,variant.get('instance_count',2));put(NAV+0x44,INSTANCES);put(INSTANCES,variant.get('model_index',0),2)
            put(MODELS,variant.get('flags',1),1);put(MODELS+4,variant.get('base',10));put(MODELS+8,variant.get('end',15))
            put(MODELS+12,variant.get('section_count',3));put(MODELS+16,SECTIONS)
            for n,key in enumerate((0x11000002,0xaabbccdd,0x12345678)):
                put(SECTIONS+n*16,key);put(SECTIONS+n*16+4,variant.get('base',10));put(SECTIONS+n*16+8,variant.get('end',15))
            put(MODELS+20,variant.get('frame_count',3));put(MODELS+24,FRAMES)
            for n in range(3):put(FRAMES+n*4,variant.get('frame_value',100+n),2)
            put(TAGS+1*16+8,TAG);put(TAG+0x38,variant.get('model',0xb2220002));put(TAGS+2*16+8,MODEL)
            put(MODEL+12,variant.get('collision',0xc3330003));put(TAGS+3*16+8,COLLISION)
            put(BSP+0x2c,SURFACES);put(BSP+12,PLANES)
            put(NAV+0x10,variant.get('surface_count',3));put(NAV+0x14,ROOTS)
            put(NAV+0x18,variant.get('node_count',2));put(NAV+0x1c,NODES)
            for n in range(3):put(SURFACES+n*8,0x8000 if variant.get('flipped') else 0,2);put(ROOTS+n*4,variant.get('root',0x80000000))
            for n,value in enumerate(variant.get('normal_bits',tuple(reference.bits(x) for x in variant.get('normal',(1.,2.,3.,4.))))):put(PLANES+n*4,value)
            for n,value in enumerate(variant.get('point_bits',tuple(reference.bits(x) for x in variant.get('point',(2.,3.,4.))))):put(POINT+n*4,value)
            floats(NODES,(1.,0.5,variant.get('distance',0.25)));put(NODES+12,0x80000001 if variant.get('double_tree') else 7);put(NODES+16,8)
            floats(NODES+20,(-1.,0.25,-0.125));put(NODES+32,12);put(NODES+36,13)
            entry=variant['entry'];route=variant.get('route','object');sector=variant.get('sector',2)
            encoded=variant.get('encoded',0x11000002)
            gp=[0xa8123456,0x99887766,0x88776655,0x12345678,reference.SP,0xabcdaaaa,0xabcdbbbb,0xabcdcccc]
            if entry==0x2185e0:gp[0]=encoded;gp[1]=0x81370000
            else:gp[0]=variant.get('instance',0) if route=='instance' else NONE;gp[1]=0x81370000 if route=='object' else NONE
            stack=bytearray(struct.pack('<I',0xdef0a125)*0x48);struct.pack_into('<IIII',stack,0x100,reference.STOP,sector&NONE,encoded,POINT)
            banks={BASE:bytes(raw),0x4ecbd0:struct.pack('<I',POOL),0x4ecc18:struct.pack('<I',SCENARIO),
                0x4ecc10:struct.pack('<I',variant.get('bsp',BSP)),0x4f0414:struct.pack('<I',TAGS),reference.SP-0x100:bytes(stack),
                0x44ac8c:image.read(0x44ac8c,36),0x46a0e8:image.read(0x46a0e8,16)}
            n=len(result)
            implicit=[reference.SP-k for k in ((0x1c,0x20,0x24,0x28,0x2c,0x30,0x34,0x38) if entry==0x202660 else (0x18,0x1c,0x20,0x24,0x28,0x2c,0x30,0x34,0x38))]
            result.append(dict(id=n,name=f'{entry:x}_{variant["name"]}_{fp[0]:x}',entry=entry,gp=gp,matrix=sector&NONE,source=encoded,
                banks=[dict(address=a,before=b.hex()) for a,b in sorted(banks.items())],before_fp=list(fp),live_x87=[],
                xmm=[[0x6b100000+n*37+r*4+k for k in range(4)] for r in range(8)],stop_before_entries=[],max_steps=10000,
                strict_writes=True,implicit_stack_addresses=implicit,xmm_exact=True,query_scratch_exact=True))
    # Reuse independently declared frame-geometry inputs, now retaining the
    # numeric caller stack and all GP/XMM results instead of value-only output.
    for seed in frame_geometry.rows(image):
        if seed['entry']!=0x218960:continue
        row=copy.deepcopy(seed);n=len(result);row['gp'][4]=reference.SP
        stack=bytearray(struct.pack('<I',0xdef0cafe)*0x48)
        struct.pack_into('<III',stack,0x100,reference.STOP,row['matrix'],row['source'])
        row['banks'].append(dict(address=reference.SP-0x100,before=stack.hex()))
        row['banks'].sort(key=lambda b:b['address'])
        row.update(id=n,xmm=[[0x6b100000+n*37+r*4+k for k in range(4)] for r in range(8)],
            stop_before_entries=[],max_steps=10000,strict_writes=True,
            implicit_stack_addresses=[reference.SP-k for k in (0x10,0x14,0x18,0x1c,0x20,0x24)],
            xmm_exact=True,query_scratch_exact=True)
        result.append(row)
    return result

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',required=True,type=Path);parser.add_argument('--report',required=True,type=Path)
    parser.add_argument('--verify',type=Path);parser.add_argument('--select')
    args=parser.parse_args();assert not args.output.exists();assert reference.sha(NEW_DEFAULT)==NEW_PIN
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);meta=reference.metadata(image,ENTRIES)
    owned=reference.prepare(image,meta,scratch);selected=inputs(image)
    if args.select:
        import re
        selected=[r for r in selected if re.search(args.select,r['name'])]
        for n,r in enumerate(selected):r['id']=n
    rows=reference.physical(owned,selected,'complete')
    assert all(r['completed'] for r in rows)
    code=_Code(image,'.text');bodies={e:set(complete_body(code,e).code) for e in ENTRIES}
    prefixes=goals.select_prefixes(rows,set().union(*bodies.values()))
    for n,row in enumerate(prefixes):row['id']=len(rows)+n
    rows+=reference.physical(owned,prefixes,'prefix')
    reached=set().union(*(set(row['executed_sites']) for row in rows))
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],
        supplied_original_outcomes=False,stop_entries=[],bank_encoding='sparse-runs-v1',cases=reference.compact(rows))
    data=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if args.verify:assert data==args.verify.read_bytes(),'Independent navigation lookup reference differs'
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(data)
    report=dict(passed=True,cases=len(rows),whole=len(rows)-len(prefixes),prefixes=len(prefixes),
        coverage={hex(e):dict(reached=len(bodies[e]&reached),total=len(bodies[e]),unreached=[hex(s) for s in sorted(bodies[e]-reached)]) for e in ENTRIES},
        original_instructions=sum(r['original_instructions'] for r in rows),fixture_sha256=reference.sha(args.output),fixture_bytes=len(data),
        independent_repeat_identical=True if args.verify else None,scratch=str(scratch),retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
