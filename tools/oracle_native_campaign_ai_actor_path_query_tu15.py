#!/usr/bin/env python3
"""Physical unchanged-i386 2013B0 actor path query and 275E90 point cache.

Actual actor/object/character banks and caller stack are inputs. Uncached
biped point/refresh children run through; vehicle/projection and BBF20
remain named stops, never supplied outcomes. Every
generated executable, raw bank and intermediate stays in HALO2_TEST_SCRATCH.
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
import oracle_native_campaign_ai_object_navigation_tu15 as navigation
import oracle_native_campaign_ai_biped_refresh_tu15 as refresh

OWNED = (0x2013b0, 0x275e90)
ENTRIES = OWNED + (0x1ecd00, 0x1ed700, 0x1d2cf0) + navigation.ENTRIES
STOPS = navigation.STOPS + (0xbbf20,)
BASE = reference.areas.BASE
POOL, ACTORS, TAGS, CHILD, PARENT, NAV, RECORDS, OBJECTS, ROWS, BODY, QUERY = (
    BASE + n for n in (0x100, 0x1000, 0x4000, 0x4200, 0x4400, 0x4600, 0x5000,
                     0x6000, 0x6100, 0x6200, 0x7000))
NONE = 0xffffffff
SCENARIO = BASE+0x6800
ACTOR, OBJECT, DEFINITION = 0xe3610001, 0x82340000, 0x12340004
FP = ((0x27f, 0, 0x1f80), (0x37f, 0x20, 0x1fa0), (0x67f, 0x3800, 0x9fc0))


def inputs(image):
    cases = []
    variants = [dict(name='cached'), dict(name='flying', flying=1),
        dict(name='short_action', action=-32768), dict(name='short_behavior', behavior=2),
        dict(name='boundary_action', action=3, behavior=3),
        dict(name='missing_property', attached=1, property=False),
        dict(name='positive_property', attached=1, radius=0x40200000),
        dict(name='zero_property', attached=1, radius=0),
        dict(name='negative_property', attached=1, radius=0xbf800000),
        dict(name='qnan_property', attached=1, radius=0x7fc12345),
        dict(name='snan_property', attached=1, radius=0x7f812345),
        dict(name='denormal_property', attached=1, radius=1),
        dict(name='inherited_property', attached=1, inherited=True, radius=0x40800000),
        dict(name='uncached_attached', cached=0, attached=1),
        dict(name='uncached_unit', cached=0),
        dict(name='uncached_unit_denied',cached=0,object_type=0,unit_state=1,unit_flags=1),
        dict(name='uncached_unit_boundary',cached=0,object_type=0),
        dict(name='uncached_vehicle_boundary',cached=0,object_type=1),
        dict(name='uncached_projection_boundary',cached=0,navigation=1),
        dict(name='uncached_no_unit', cached=0, unit=NONE)]
    for entry in OWNED:
        selected = variants if entry == 0x2013b0 else [
            dict(name='cached'), dict(name='uncached_no_unit', cached=0, unit=NONE),
            dict(name='uncached_attached', cached=0, attached=1),
            dict(name='uncached_unit', cached=0),
            dict(name='uncached_unit_denied',cached=0,object_type=0,unit_state=1,unit_flags=1),
            dict(name='uncached_unit_boundary',cached=0,object_type=0),
            dict(name='uncached_vehicle_boundary',cached=0,object_type=1),
            dict(name='uncached_projection_boundary',cached=0,navigation=1),
            dict(name='aim_unit', cached=0, flying=1),
            dict(name='aim_attached', cached=0, flying=1, attached=1),
            dict(name='aim_physics_refusal', cached=0, flying=1, physics=0x1234)]
        for variant in selected:
            for fp in FP:
                raw = bytearray(0x8000)
                def put(address, value, width=4):
                    raw[address-BASE:address-BASE+width] = (value & ((1 << (width*8))-1)).to_bytes(width,'little')
                row = ACTORS + (ACTOR & 0xffff) * 0x888
                put(POOL+0x44,ACTORS);put(TAGS+2*16+8,CHILD);put(TAGS+3*16+8,PARENT)
                put(CHILD+8,0xabcd0003);put(PARENT+8,NONE)
                put(CHILD+0x5c,1);put(CHILD+0x60,NAV);put(NAV+4,0x3f000000)
                inherited=variant.get('inherited',False)
                put(CHILD+0xe4,0 if inherited else int(variant.get('property',True)))
                put(CHILD+0xe8,RECORDS);put(PARENT+0xe4,int(inherited));put(PARENT+0xe8,RECORDS)
                put(RECORDS+4,DEFINITION);put(RECORDS+0x14,variant.get('radius',0x3f800000))
                put(row+0x54,0x43210002);put(row+0x18,variant.get('unit',OBJECT))
                put(row+0x86,variant.get('action',3),2);put(row+0x328,variant.get('behavior',3),2)
                put(row+0x266,variant.get('attached',0),1)
                put(row+0x26c,OBJECT if variant.get('attached',0) else NONE)
                put(row+0x229,variant.get('flying',0),1);put(row+0x278,variant.get('cached',1),1)
                put(row+0x3e4,0xa7,1)
                for offset,value in ((0x238,0x3f800000),(0x23c,0x40000000),(0x240,0x40400000),
                                     (0x27c,0x40800000),(0x280,0x40a00000),(0x284,0x40c00000),
                                     (0x288,0x76540002),(0x28c,0x12340003)):
                    put(row+offset,value)
                put(OBJECTS+0x44,ROWS);put(ROWS+8,BODY);put(BODY,DEFINITION)
                put(BODY+0xaa,variant.get('object_type',2),1);put(BODY+0x14,NONE)
                put(BODY+0xb4,variant.get('physics',NONE))
                put(BODY+0x3dc,variant.get('unit_state',0),1);put(BODY+0x348,variant.get('unit_flags',0),1)
                put(SCENARIO+0xc4,variant.get('navigation',0));put(SCENARIO+0xc8,0x12345678)
                for offset,value in ((0x30,0x41100000),(0x34,0x41200000),(0x38,0x41300000)):
                    put(BODY+offset,value)
                raw[QUERY-BASE:QUERY-BASE+0x50] = bytes(range(0x50))
                n=len(cases);gp=[0x5a100000+n*31+k*7 for k in range(8)]
                gp[3],gp[4]=QUERY,reference.SP
                if entry==0x275e90:gp[2]=ACTOR
                stack=bytearray(struct.pack('<I',0xbeef1234+n)*0x60)
                struct.pack_into('<III',stack,0x160,reference.STOP,ACTOR,0xabcddcba)
                banks={BASE:bytes(raw),0x5020c8:struct.pack('<I',POOL),
                       0x4f0414:struct.pack('<I',TAGS),0x4ecbd0:struct.pack('<I',OBJECTS),
                       0x4ecc18:struct.pack('<I',SCENARIO),0x474d10:struct.pack('<I',0xaabbccdd),
                       reference.SP-0x160:bytes(stack)}
                for address in (0x46a0e8,0x46a0d0,0x46a100):banks[address]=image.read(address,4)
                cases.append(dict(id=n,name=f'{entry:x}_{variant["name"]}_{fp[0]:x}_{fp[2]:x}',entry=entry,
                    gp=gp,matrix=ACTOR,source=0xabcddcba,banks=[dict(address=a,before=b.hex()) for a,b in sorted(banks.items())],
                    before_fp=list(fp),live_x87=[],xmm=[[0x6b000000+n*37+r*4+k for k in range(4)] for r in range(8)],
                    stop_before_entries=list(STOPS),max_steps=2000,strict_writes=True,xmm_exact=entry==0x2013b0,
                    query_scratch_exact=entry==0x2013b0 or 'aim_' not in variant['name']))
                if not variant.get('cached',1) and not variant.get('flying',0) and variant.get('unit',OBJECT)!=NONE:
                    position_sp=reference.SP-(0x20 if entry==0x2013b0 else 0)
                    child_sp=position_sp-(0x14 if variant.get('attached',0) else 0x18)
                    cases[-1]['implicit_stack_addresses']=[child_sp-n for n in range(0x78,0x9c,4)]
                    if variant.get('object_type',2)==1:cases[-1]['implicit_stack_addresses'].append(child_sp-0x9c)
                    if variant.get('object_type',2)==0:
                        cases[-1]['implicit_stack_addresses'] += [child_sp-0x98-n for n in range(0x10,0x40,4)]
    # Complete actor -> point cache -> biped refresh compositions. Character
    # DATA moves beyond the collision banks; its old private CPU frame is not
    # an input to this retained-DATA contract.
    seeds=refresh.cases(image)
    for entry in OWNED:
        actor_seed=next(r for r in cases if r['entry']==entry and '_uncached_unit_boundary_27f_' in r['name'])
        for seed in seeds:
            if seed['entry']!=0xe20f0 or not any(seed['name'].startswith('e20f0_'+v+'_rc') for v in ('default','fall','contact')):continue
            row=copy.deepcopy(actor_seed);banks={b['address']:bytearray.fromhex(b['before']) for b in seed['banks']}
            actor_data=bytearray.fromhex(next(b['before'] for b in actor_seed['banks'] if b['address']==BASE));shift=0x40000
            for offset in range(0,len(actor_data)-3,4):
                value=struct.unpack_from('<I',actor_data,offset)[0]
                if BASE<=value<BASE+len(actor_data):struct.pack_into('<I',actor_data,offset,value+shift)
            banks[BASE+shift]=actor_data
            banks[0x5020c8]=bytearray(struct.pack('<I',POOL+shift));banks[0x4f0414]=bytearray(struct.pack('<I',TAGS+shift))
            actor=ACTORS+(ACTOR&65535)*0x888
            struct.pack_into('<I',actor_data,actor-BASE+0x18,0x81370000)
            data=banks[refresh.BASE]
            struct.pack_into('<I',data,refresh.BODY-refresh.BASE,DEFINITION)
            for offset in (0x354,0x358,0x35c,0x360,0x364):struct.pack_into('<I',data,refresh.BODY-refresh.BASE+offset,NONE)
            struct.pack_into('<I',data,refresh.BODY-refresh.BASE+0x350,0);struct.pack_into('<I',data,0x2208,1)
            row['gp'][3]=QUERY+shift
            row.update(id=len(cases),name=f'{entry:x}_composed_'+seed['name'],before_fp=seed['before_fp'],
                retained_refresh_data=True,strict_writes=False,xmm_exact=False,query_scratch_exact=False,stack_arguments=[],implicit_stack_addresses=[],max_steps=100000,
                banks=[dict(address=p,before=b.hex()) for p,b in sorted(banks.items())])
            cases.append(row)
    return cases


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True);parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--verify',type=Path)
    args=parser.parse_args();assert reference.sha(NEW_DEFAULT)==NEW_PIN
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT)
    meta=reference.metadata(image,ENTRIES);owned=reference.prepare(image,meta,scratch)
    rows=reference.physical(owned,inputs(image),'complete')
    assert all(row['completed'] for row in rows)
    code=_Code(image,'.text');bodies={entry:set(complete_body(code,entry).code) for entry in ENTRIES}
    prefixes=navigation.prefixes(rows,set().union(*(bodies[e] for e in OWNED)))
    for n,row in enumerate(prefixes):row['id']=len(rows)+n
    rows+=reference.physical(owned,prefixes,'prefix')
    reached=set().union(*(set(row['executed_sites']) for row in rows))
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],
        instruction_patches=[],supplied_original_outcomes=False,stop_entries=list(STOPS),
        bank_encoding='sparse-runs-v1',cases=reference.compact(rows))
    data=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if args.verify:assert data==args.verify.read_bytes(),'Independent actor path-query reference differs'
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(data)
    report=dict(passed=True,cases=len(rows),whole=len(rows)-len(prefixes),prefixes=len(prefixes),
        coverage={hex(e):dict(reached=len(bodies[e]&reached),total=len(bodies[e])) for e in ENTRIES},
        original_instructions=sum(r['original_instructions'] for r in rows),fixture_sha256=reference.sha(args.output),
        fixture_bytes=len(data),independent_repeat_identical=True if args.verify else None,
        scratch=str(scratch),retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))


if __name__=='__main__':main()
