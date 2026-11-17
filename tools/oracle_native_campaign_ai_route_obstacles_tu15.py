#!/usr/bin/env python3
"""Whole unchanged-i386 2CABE0 obstacle collection over real retained banks.

All spatial, perception, object, marker and numeric callees execute original
code. The contract retains DATA and caller-saved ownership, excluding private
stack history and unconsumed volatile registers. No callee outcomes supplied.
"""
import argparse,copy,gzip,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_area_perform_tu15 as reference
import oracle_native_campaign_ai_path_search_tu15 as search
import oracle_native_campaign_ai_route_obstacle_geometry_tu15 as geometry
import oracle_native_campaign_object_marker_request_tu15 as requests
import oracle_native_campaign_object_markers_tu15 as markers

ENTRIES=tuple(dict.fromkeys((0x2cabe0,0x32a660,0x11e600,0xbd080,0xba970,0xbaa40,0x14d0e0,0x14cdb0,
    0x14cea0,0x243450,0x123210,0xbbd10,0x267520,0x1eb730,0x1e7810,0xbbe00,0x30bd0,0x24570,
    0xc1e90,0x160a60,0xbc190,0xbac00,0x144bf0,0x145070)+geometry.ENTRIES))
BASE=markers.BASE;NONE=0xffffffff;BODY=requests.BODY;OBJECT=0xea000001;ACTOR=0xea000000
COLLISION,SPHERES,SHAPE,STRUCTURE,CLUSTERS,REF_TABLE,REF_POOL,REF_ROWS,UP,START,DIRECTION,VELOCITY,POLICY,PRIMARY,SECONDARY,ACTOR_POOL,ACTORS,UNIT,PROP_POOL,GROUP_POOL,PROP_ROWS,GROUP_ROWS,GAME,AI,PHYSICS,PHYSICS_ROWS=(BASE+n for n in (0x10000,0x10100,0x10200,0x11000,0x11200,0x12000,0x12100,0x12200,0x13000,0x13010,0x13020,0x13030,0x13100,0x14000,0x15000,0x16000,0x16100,0x18000,0x19000,0x19100,0x19200,0x19400,0x19600,0x19700,0x19800,0x19900))

def inputs(image):
    rows=[];base=requests.inputs(image)[0]
    variants=[('base',{})]
    for kind in(0,1,7,11):variants.append((f'kind{kind}',dict(kind=kind)))
    for name,changes in(
        ('empty',dict(empty=True)),('multiple',dict(multiple=True)),('no_cluster',dict(cluster=65535)),('no_location',dict(no_location=True)),
        ('ignore_first',dict(ignore=OBJECT)),('ignore_second',dict(ignore_second=OBJECT)),
        ('deleted',dict(flags=1)),('hidden',dict(flags=0x800000)),('opaque',dict(flags=0x400000)),
        ('opaque_rejected',dict(flags=0x400000,sphere_flags=1)),('definition_skip',dict(tag_flags=8)),
        ('missing_model',dict(model=NONE)),('missing_collision',dict(collision=NONE)),('no_spheres',dict(spheres=0)),
        ('negative_spheres',dict(spheres=-1)),('range_miss',dict(position=(100.,0.,0.))),
        ('sphere_miss',dict(sphere_position=(100.,0.,0.))),('sphere_above',dict(sphere_position=(0.,0.,8.),direction=(0.,0.,-1.))),
        ('weighted_range_miss',dict(position=(0.,0.,0.),sphere_position=(3.6,0.,1.3))),
        ('sphere_below',dict(sphere_position=(0.,0.,-8.),direction=(0.,0.,1.))),
        ('node_sphere',dict(node=0)),('parent_matrix',dict(parent=True)),
        ('device_closed',dict(kind=7,device_flags=1)),('device_open',dict(kind=7,device_flags=3,device_position=1.)),
        ('device_transition',dict(kind=7,device_flags=3,device_position=.5,device_state=1)),
        ('device_state_clear',dict(kind=7,device_flags=3,device_position=.5)),('device_sphere_excluded',dict(kind=7,sphere_flags=0)),
        ('attached_sphere',dict(sphere_flags=3)),('crate_static',dict(kind=11,physics_class=1)),('crate_dynamic',dict(kind=11,physics_class=3)),
        ('unit_dead',dict(kind=0,unit_flags=4)),('unit_moving',dict(kind=0,position=(1.,0.,0.),velocity=(3.,0.,0.))),
        ('output_full',dict(count=64)),('output_last',dict(count=63)),('output_negative',dict(count=-1)),
        ('shape_not_usable',dict(shape_flags=2,shape_class=3,object_scale=0.)),
        ('extra_flag',dict(shape_flags=2,shape_class=0,object_scale=1.,mask=64)),
        ('policy_ignored',dict(shape_class=3,policy_object=OBJECT)),
        ('step',dict(actor=True,mask=1,shape_class=2,step=2)),
        ('jump',dict(actor=True,mask=4,shape_class=2,jump=2)),
        ('jump_slow',dict(actor=True,mask=4,shape_class=2,jump=2,speed=(0.,0.,0.))),
        ('climb',dict(actor=True,mask=2,shape_class=3,climb=(2,4))),
        ('climb_wrong_class',dict(actor=True,mask=2,shape_class=1,climb=(2,4))),
        ('jump_no_secondary',dict(actor=True,mask=4,shape_class=2,jump=2,secondary=0)),
        ('actor_unattached',dict(actor=True)),('actor_attached',dict(actor=True,attached=True,sphere_flags=3)),
        ('actor_excluded_unit',dict(actor=True,own_state=1,own_target=OBJECT)),
        ('unit_perception_none',dict(actor=True,kind=0)),('unit_perception_zero',dict(actor=True,kind=0,property_level=0)),
        ('unit_perception_live',dict(actor=True,kind=0,property_level=1)),
        ('unit_actor_allied',dict(actor=True,kind=0,other_actor=True)),('unit_actor_hostile',dict(actor=True,kind=0,other_actor=True,hostile=True)),
        ('unit_actor_padding',dict(actor=True,kind=0,other_actor=True,own_flag=1)),
        ('unit_actor_attached_padding',dict(actor=True,attached=True,kind=0,other_actor=True,hostile=True)),
        ('unit_facing_away',dict(actor=True,kind=0,other_actor=True,other_active=True,position=(2.,0.,0.),aim=(1.,0.,0.))),
        ('unit_facing_toward',dict(actor=True,kind=0,other_actor=True,other_active=True,position=(2.,0.,0.),aim=(-1.,0.,0.))),
        ('unit_player',dict(actor=True,kind=0,player=True)),('unit_player_hostile',dict(actor=True,attached=True,kind=0,player=True,hostile=True)),
        ('unit_player_padding',dict(actor=True,kind=0,player=True,own_flag=1)),
        ('unit_player_team_none',dict(actor=True,kind=0,player=True,own_team=65535)),
        ('unit_player_mode0',dict(actor=True,kind=0,player=True,mode=0)),
        ('unit_player_no_engine',dict(actor=True,kind=0,player=True,mode=2)),
        ('vehicle_allied',dict(actor=True,kind=1)),('vehicle_hostile',dict(actor=True,kind=1,hostile=True)),
        ('vehicle_same_team',dict(actor=True,kind=1,team=2)),
        ('mounted_actor',dict(actor=True,attached=True,kind=1,team=2,rider=True,position=(2.,0.,0.),aim=(1.,0.,0.),other_active=True)),
        ('mounted_velocity',dict(actor=True,attached=True,kind=1,team=2,rider=True,position=(2.,0.,0.),velocity=(1.,0.,0.))),
        ('mounted_stationary',dict(actor=True,attached=True,kind=1,team=2,rider=True,position=(2.,0.,0.))),
        ('mounted_other_team',dict(actor=True,attached=True,kind=1,team=3,hostile=True,rider=True,position=(2.,0.,0.))),
    ):variants.append((name,changes))
    for mask in(8,16,32,56):
        for marker_flags in(0,0x1fe3):variants.append((f'markers_{mask}_{marker_flags}',dict(mask=mask,marker_flags=marker_flags)))
        for position in((-1.,0.,0.),(0.,0.,0.),(1.,0.,0.),(10.,0.,0.)):
            for direction in((0.,1.,0.),(1.,0.,0.)):
                variants.append((f'markers_valid_{mask}_{position}_{direction}',dict(mask=mask,marker_flags=0x1fe3,valid_marker=position,direction=direction)))
    for name,v in variants:
      for profile in search.FP_PROFILES:
        row={};banks={b['address']:bytearray.fromhex(b['before'])for b in base['banks']}
        # Preserve authored marker inputs, replacing unrelated seed padding
        # with zero. This keeps the required reference fixture compact.
        seed_data=banks[BASE];clean=bytearray(len(seed_data))
        for start,size in((0x100,0x300),(0x2000,0x100),(0x2fc0,0x200),(0x3f80,0x1a0),(0x9000,0x200)):
            clean[start:start+size]=seed_data[start:start+size]
        banks[BASE]=clean
        banks[0x4ec000]=bytearray(4096);banks[0x4f0000]=bytearray(4096)
        banks[BASE+0x10000]=bytearray(0xb000)
        # Only the imported rdata page is a numeric input. All mutable globals
        # and dynamic owner rows below are coherent authored inputs.
        for at in(0x4eab00,0x4f3c00,0x502000,0x50ee00,0x52be00,0x474c00):banks[at]=bytearray(0x100)
        def put(at,value,width=4):
            for start,data in banks.items():
                if start<=at and at+width<=start+len(data):data[at-start:at-start+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
            banks[at]=bytearray((value&((1<<(width*8))-1)).to_bytes(width,'little'))
        def floats(at,values):
            for n,value in enumerate(values):put(at+n*4,search.f(value))
        def clear(at,size):
            for start,data in banks.items():
                if start<=at and at+size<=start+len(data):data[at-start:at-start+size]=bytes(size);return
            raise ValueError(hex(at))
        clear(BODY,0x500);clear(requests.PARENT,0x500);clear(requests.OBJECT_TAG,0x200);clear(requests.MODEL,0x100)
        put(0x4f0414,markers.TAGS);put(0x4ecbd0,requests.OBJECTS);put(requests.OBJECTS+0x44,requests.HEADERS)
        put(requests.HEADERS+8,UNIT);put(requests.HEADERS+20,BODY);put(requests.HEADERS+32,requests.PARENT)
        put(BODY,0xe4000000);put(BODY+4,v.get('flags',0));put(BODY+0x14,0xea000002 if v.get('parent')else NONE)
        put(BODY+0xaa,v.get('kind',7),1);put(BODY+0xb4,0xea000000 if 'physics_class'in v else NONE)
        put(BODY+0x12c,0xea000001 if v.get('other_actor')else NONE);put(BODY+0x13c,0xea000000 if v.get('player')else NONE)
        put(BODY+0x248,0xea000002 if v.get('rider')else NONE);put(BODY+0x138,v.get('team',3),2)
        put(BODY+0x28,0);put(BODY+0x2c,v.get('cluster',0));put(BODY+0xec,search.f(v.get('object_scale',1.)))
        floats(BODY+0x30,v.get('position',(1.,0.,0.)));put(BODY+0x3c,search.f(10.))
        floats(BODY+0x64,v.get('position',(1.,0.,0.)));floats(BODY+0x70,(1.,0.,0.));floats(BODY+0x7c,(0.,0.,1.))
        floats(BODY+0x88,v.get('velocity',(0.,0.,0.)));put(BODY+0x10a,v.get('unit_flags',0),1)
        put(BODY+0x114,13*52,2);put(BODY+0x116,markers.NODES-BODY,2);put(BODY+0x11a,markers.VARIANTS-BODY,2)
        put(BODY+0x140,search.f(v.get('device_position',0.)));put(BODY+0x1cc,v.get('device_state',0),1)
        put(requests.PARENT+0x14,NONE);put(requests.PARENT+0x116,markers.NODES-requests.PARENT,2);put(requests.PARENT+0x12c,0xea000001)
        put(requests.PARENT+0xaa,0,1);put(requests.PARENT+0x4,0)
        put(requests.OBJECT_TAG+2,v.get('tag_flags',0),1);put(requests.OBJECT_TAG+0x38,v.get('model',0xe4000002))
        put(requests.OBJECT_TAG+0x11e,v.get('device_flags',0),2)
        put(requests.OBJECT_TAG+0x5c,1 if ('shape_class'in v or 'shape_flags'in v)else 0);put(requests.OBJECT_TAG+0x60,SHAPE)
        put(SHAPE,v.get('shape_flags',0),1);put(SHAPE+12,v.get('shape_class',0),2)
        put(requests.MODEL+4,0xe4000001);put(requests.MODEL+12,v.get('collision',0xe4000004));put(markers.TAGS+4*16+8,COLLISION)
        put(COLLISION+0x24,v.get('spheres',1));put(COLLISION+0x28,SPHERES);put(SPHERES,v.get('node',65535),2)
        put(SPHERES+2,v.get('sphere_flags',5),1);floats(SPHERES+4,v.get('sphere_position',(0.,0.,0.)));put(SPHERES+16,search.f(.375))
        put(0x4ecc18,STRUCTURE);put(STRUCTURE+0xa0,CLUSTERS);put(CLUSTERS+0x8c,0)
        put(0x4eabb0,REF_TABLE);put(0x4eabb4,REF_POOL);put(REF_POOL+0x44,REF_ROWS);put(REF_POOL+0x24,0x20)
        put(REF_TABLE,NONE if v.get('empty')else 0xea000000);put(REF_ROWS+4,OBJECT);put(REF_ROWS+8,NONE)
        put(REF_ROWS+12,v.get('kind',7),1);floats(REF_ROWS+16,(0.,0.,0.));put(REF_ROWS+28,search.f(100.))
        put(0x4eabcc,5);put(0x4eabd4,0);put(0x4f3ce4,8);put(0x4f3ce8,0);put(0x474d10,UP);floats(UP,(0.,0.,1.));put(0x474c24,65535,2)
        floats(START,(0.,0.,0.));floats(DIRECTION,v.get('direction',(1.,0.,0.)));floats(VELOCITY,v.get('speed',(4.,0.,0.)))
        put(POLICY+4,v.get('marker_flags',0),2);put(POLICY+14,v.get('step',0),2);put(POLICY+16,v.get('jump',0),2)
        for i,value in enumerate(v.get('climb',(0,0))):put(POLICY+18+i*2,value,2)
        put(POLICY+24,v.get('policy_object',NONE));put(PRIMARY+2,v.get('count',0),2);put(SECONDARY+2,0,2)
        put(0x5020c8,ACTOR_POOL);put(ACTOR_POOL+0x44,ACTORS);put(ACTORS+0x18,0xea000000);put(ACTORS+0x24,v.get('own_team',2),2)
        put(ACTORS+0x26c,OBJECT if v.get('attached')else NONE);put(ACTORS+0x58,0xea000000 if 'property_level'in v else NONE)
        put(ACTORS+0x223,v.get('own_flag',0),1);put(UNIT+0xaa,0,1);put(UNIT+0x3dc,v.get('own_state',0),1);put(UNIT+0x448,v.get('own_target',NONE))
        put(ACTORS+0x888+0x24,3,2);put(ACTORS+0x888+0x5d0,int(v.get('other_active',False)),1);floats(ACTORS+0x888+0x5ec,v.get('aim',(1.,0.,0.)))
        put(0x50eef0,PROP_POOL);put(PROP_POOL+0x44,PROP_ROWS);put(0x50eef4,GROUP_POOL);put(GROUP_POOL+0x44,GROUP_ROWS)
        put(PROP_ROWS+8,0xea000000);put(PROP_ROWS+0x20,OBJECT);put(PROP_ROWS+0x24,v.get('property_level',1),2);put(PROP_ROWS+0x2c,NONE)
        put(0x4f3218,GAME);put(GAME+8,v.get('mode',1));put(0x5020c4,AI)
        put(0x4f63b8,BASE+0x19a00);put(BASE+0x19a00+0xc14,0);put(0x56dfa0,0)
        if v.get('hostile'):put(AI+0xc4+4,0xffffffff)
        put(0x52be5c,PHYSICS);put(PHYSICS+0x44,PHYSICS_ROWS);put(PHYSICS_ROWS+0x1c,v.get('physics_class',3),1)
        names=(0x0f000526,0x11000527,0x0f00052a,0x1100052b,0x1000052c,0x1100052f,0x10000530)
        put(markers.TAG+0x58,len(names))
        for n,key in enumerate(sorted(names)):put(markers.GROUPS+n*12,key);put(markers.GROUPS+n*12+4,2);put(markers.GROUPS+n*12+8,markers.MARKERS)
        if 'valid_marker'in v:
            for k in range(2):
                at=markers.NODES+k*52
                # Forward points up; left points along the query direction;
                # the third axis and authored position define the plane gate.
                floats(at,(1.,0.,0.,1.,0.,1.,0.,-1.,0.,0.,*v['valid_marker']))
                marker=markers.MARKERS+k*0x24;put(marker,255,1);put(marker+2,k,1)
                floats(marker+4,(0.,0.,0.));floats(marker+0x10,(0.,0.,0.,1.));put(marker+0x20,search.f(4.))
        if v.get('multiple'):
            for start,data in banks.items():
                if start<=BODY and BODY+0x500<=start+len(data):body_copy=bytes(data[BODY-start:BODY-start+0x500]);break
            banks[BASE+0x10000][0xa000:0xa500]=body_copy
            put(requests.HEADERS+3*12+8,BASE+0x1a000);put(REF_ROWS+8,0xea000001)
            put(REF_ROWS+32+4,0xea000003);put(REF_ROWS+32+8,NONE);put(REF_ROWS+32+12,7,1)
            floats(REF_ROWS+32+16,(0.,0.,0.));put(REF_ROWS+32+28,search.f(100.))
        args=[ACTOR if v.get('actor')else NONE,v.get('mask',0),POLICY,PRIMARY,v.get('secondary',SECONDARY),START,search.f(4.),DIRECTION,NONE if v.get('no_location')else v.get('ignore',0xea000002),v.get('ignore_second',NONE)]
        # The default ignored parent supplies a coherent location but is not a candidate.
        put(requests.PARENT+0x28,0);put(requests.PARENT+0x2c,v.get('cluster',0))
        n=len(rows);gp=[0x5a100000+n*23+k*3 for k in range(8)];gp[1]=VELOCITY if 'speed'in v else 0;gp[4]=reference.SP
        row.update(id=n,name=name+'_'+str(n),entry=0x2cabe0,gp=gp,matrix=args[0],source=args[1],stack_arguments=args[2:],
            before_fp=list(profile),live_x87=[],xmm=[[0x6b100000+n*37+r*4+k for k in range(4)]for r in range(8)],
            stop_before_entries=[],max_steps=100000,strict_writes=False,banks=[dict(address=a,before=bytes(d).hex())for a,d in sorted(banks.items())])
        rows.append(row)
    return rows

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');p.add_argument('--replace',action='store_true');a=p.parse_args()
    assert a.verify or a.replace or not a.output.exists();assert reference.sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);meta=reference.metadata(image,ENTRIES)
    for site in meta['sites']:
        if site['bytes'].startswith('0f17'):site['operands'][0].update(read=False,write=True)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=reference.prepare(image,meta,scratch);rows=reference.physical(owned,inputs(image),'whole')
    for row in rows:assert row['completed']and row['stopped_at']==reference.STOP,row['name']
    reached=set().union(*(set(r['executed_sites'])for r in rows));body=set(complete_body(_Code(image,'.text'),0x2cabe0).code)
    instructions=sum(r['original_instructions']for r in rows);read_count=sum(len(r['reads'])for r in rows)
    for row in rows:
        # These traces are outside the declared retained-DATA contract. Keep
        # their coverage/count receipt, not redundant unused per-case payloads.
        for key in('reads','executed_sites','boundary_xmm','boundary_eflags','calls'):row.pop(key,None)
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,
        stop_entries=[],bank_encoding='sparse-runs-v1',contract='retained-DATA-and-callee-saved-GP',cases=reference.compact(rows))
    data=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert data==a.output.read_bytes(),'Independent physical original changed'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(data)
    report=dict(passed=True,cases=len(rows),original_instructions=instructions,original_reads=read_count,stores=sum(len(r['writes'])for r in rows),root_sites=len(body&reached),root_total=len(body),missing_sites=[hex(pc)for pc in sorted(body-reached)],
        fixture=str(a.output),fixture_bytes=len(data),fixture_sha256=reference.sha(a.output),original_sha256=NEW_PIN,source_sha256=reference.sha(__file__),owned_scratch=str(scratch),retained_large_artifacts=[])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
