#!/usr/bin/env python3
"""Physical whole26FA40 campaign scoring; genuine160A60 call remains a boundary."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import oracle_native_campaign_ai_perception_policy_tu15 as policy
import oracle_native_campaign_ai_perception_group_link_tu15 as transport
import validate_native_geometry_tu15 as geometry
BASE=policy.BASE
ACTOR=policy.character.ACTORS
GROUPS,GROWS,AI,GAME,CLOCK=BASE+0xb0c0,BASE+0xf400,BASE+0xf800,BASE+0xf900,BASE+0xf940
ENTRIES=(0x26fa40,0x2673a0,0x2676c0,0x1ea2e0,0x1ed240,0x1ed4f0,0x27c710,0x27c6c0,0x27c7e0,0xc1e90,0x1c1d70,0x1ea1d0,0x1ed530)
sha=common.sha
def prepare(image,meta,scratch):
    owned=transport.prepare(image,meta,scratch)
    result=subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_perception_score_tu15.cpp'),'-o',str(owned/'observer')],capture_output=True,text=True);assert not result.returncode,result.stderr
    return owned
def inputs(image):
    rows=[];seeds=policy.inputs();seed=next(s for s in seeds if s['entry']==0x1ed240 and s['name'].startswith('inherited_policy_1ed100_on_True_0'))
    def make(source=seed,name='score',query=True,vehicle=False,distance=0x40000000,state=2,stale=0,
             team=2,mode=1,watched=0xffffffff,friend=True,required=False,changes=(),fp=None,live=0):
        n=len(rows);banks={b['address']:bytearray.fromhex(b['before'])for b in source['banks']};banks[0x46a000]=bytearray(image.read(0x46a000,4096));banks[0x47d000]=bytearray(image.read(0x47d000,4096))
        def put(p,v,w=4):
            for first,data in banks.items():
                if first<=p and p+w<=first+len(data):data[p-first:p-first+w]=(v&((1<<(8*w))-1)).to_bytes(w,'little');return
            banks[p]=bytearray((v&((1<<(8*w))-1)).to_bytes(w,'little'))
        actor=source['actor'];record=ACTOR+(actor&0xffff)*0x888
        for p,v in [(0x50eef4,GROUPS),(GROUPS+0x44,GROWS),(policy.PROWS+8,0xe8000000),(policy.PROWS+0x14,0xe6000000 if query else 0xffffffff),(policy.PROWS+0x20,0xe5000002),(policy.PROWS+0x28,distance),(0x51df54,CLOCK),(CLOCK+4,0x3d088889),(0x5020c4,AI),(0x4f3218,GAME),(GAME+8,mode),(record+0x26c,watched),(record+0x338,0xffffffff),(policy.QROWS+0xc0,0xffffffff),(policy.QROWS+0x40,0xffffffff),(GROWS+0x94,0xffffffff)]:put(p,v)
        put(record+7,0,1);put(record+0x269,0,1);put(record+0x265,0,1);put(record+0x24,team,2);put(record+0x86,0,2)
        put(GROWS+0x26,0,1);put(GROWS+0x23,1,1);put(policy.PROWS+0x18,1,2);put(policy.PROWS+0x24,state,2)
        # Type1 uses actual query +4 or group +58 position storage.
        position=policy.QROWS+4 if query else GROWS+0x58
        for offset in (0x5e,0x63,0x67,0x69):put(position+offset,0,1)
        put(position+0x5e,stale,1);put(position+0x5c,0,2);put(position+0x3c,0xffffffff)
        for offset in (0x2a,0x39,0x88):put(policy.QROWS+0x70+offset,0,1)
        put(policy.QROWS+0x76,4,2);put(policy.QROWS+0xd0,0x3f000000)
        for i in range(8):put(AI+0xc4+i*4,0xffffffff if friend else 0)
        # Keep source actor/vehicle relations; change only its real tag policy.
        put(policy.character.TAGS+3*0x400+0x12f,0 if vehicle else 1,1)
        for offset,value in [(4,0x41200000),(0x18,0x41200000),(0x28,0x41a00000),(0x30,0x41f00000),(0xc,0x40800000)]:
            for i in range(4):put(BASE+0xe800+i*0x300+0xcc+offset,value)
        for i in range(4):put(policy.character.MOVEMENT+i*0x100+4,0x41200000)
        for i in range(3):
            put(record+0x238+i*4,0x3f800000);put(position+4+i*4,(0x40400000,0x40800000,0x40a00000)[i]);put(policy.QROWS+0x9c+i*4,(0x40000000,0x40400000,0x40800000)[i])
            put(policy.BODIES[4]+0x150+i*4,(0x3f800000,0x40000000,0x40400000)[i])
        put(policy.BODIES[3]+0x10,0xe5000004);put(policy.BODIES[4]+0xaa,0,1);put(policy.BODIES[4]+0x13c,0xe9000000)
        for p,v,w in changes:put(p,v,w)
        gp=source['gp'].copy();gp[4]=geometry.SP+n%4*4
        rows.append(dict(id=n,name=f'{name}_{n}',entry=0x26fa40,gp=gp,matrix=actor,source=0xe7000000,actor=actor,perception=0xe7000000,child_output=0,required_owner_boundary=required,banks=[dict(address=p,before=data.hex())for p,data in sorted(banks.items())],before_fp=fp or[0x27f,0,0x1f80],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3).hex()for k in range(live)]))
    for s in seeds:
        if s['entry']==0x1ed240 and s['name'].startswith('inherited_'):
            raw=bytes.fromhex(next(b['before']for b in s['banks']if b['address']==BASE));at=policy.character.TAGS+2*0x400-BASE
            override=struct.unpack_from('<b',raw,at+0x2e)[0];variant=override-1 if override else struct.unpack_from('<h',raw,at+0x24)[0]
            # The standalone pointer query also tests negative unbacked block
            # identities. A full scoring call reads block+4, so declare only
            # variants whose actual backing is present in this input bank.
            if variant>=-256:make(source=s,name='real_inherited_policy',vehicle=False)
        if s['entry']==0x1ed4f0 and s['name'].startswith('real_vehicle_admission'):make(source=s,name='real_vehicle_policy',vehicle=True)
    for changes in [[(GROWS+0x26,1,1)],[(GROWS+0x23,0,1)],[(policy.PROWS+0x24,0,2)],[(policy.PROWS+0x24,0xffff,2)],[(policy.BODIES[2]+0xaa,15,1)],[(policy.PROWS+0x18,0,2)],[(policy.PROWS+0x18,6,2)],[(ACTOR+7,1,1)],[(policy.QROWS+0xc0,0xe5000002,4)],[(ACTOR+0x269,1,1)],[(ACTOR+0x269,1,1),(ACTOR+0x26c,0xe5000002,4),(policy.QROWS+0x40,0xe5000002,4)]]:make(changes=changes,name='early_and_rank_policy')
    for query in (False,True):
        for state in (1,2,3,4,32767):
            for extension_mode in (0,1,4):
                make(query=query,state=state,changes=[(policy.QROWS+0x76,extension_mode,2)],name='state_rank')
    for flags in [(0x2a,1,1),(0x88,1,1),(0x39,0xff,1),(0x39,2,1)]:
        for visible in (0,1):make(state=3,changes=[(policy.QROWS+0x70+flags[0],flags[1],flags[2]),(policy.QROWS+0x76,0,2),(policy.QROWS+4+0x63,visible,1)],name='tracking_state_flags')
    for vehicle in (False,True):
        for distance in (0,0x3e800000,0x3f800000,0x40000000,0x40c00000,0x41c80000,0x7fc12345,0x7f812345):
            for remembered in (0,1):make(vehicle=vehicle,distance=distance,state=3,changes=[(policy.QROWS+0xf8,remembered,1)],name='range_distance')
    for stale in (0,1):
        for ticks in (-32768,0,1,30,32767):make(stale=stale,changes=[(policy.QROWS+4+0x5c,ticks,2)],name='stale_age')
    for active in (0,1):
        for recognized in (0,1):make(changes=[(ACTOR+0x338,0xe7000000,4),(ACTOR+0x86,5,2),(ACTOR+0x354,active,1),(policy.QROWS+4+0x67,recognized,1)],name='attention_recognition')
    for cw in (0x7f,0x27f,0x37f):
        for rounding in range(4):
            for value in (0x3eaaaaab,0xbf800000,0x7fc12345,0x7f812345,0x7f800000,1):
                make(fp=[cw|(rounding<<10),((rounding+1)<<11)|1,0x1f80|(rounding<<13)|(0x8040 if value&1 else 0)],live=(0,1,6)[rounding%3],changes=[(policy.QROWS+0xd0,value,4)],name='confidence_fp')
    for distance in (0xbf800000,0xff800000,0x7fc12345,0x7f812345,0x80000000):make(distance=distance,name='proximity_fp')
    for query in (False,True):
        for mode,team,friend in [(1,0,True),(1,2,True),(1,2,False),(1,0xffff,True),(1,0xfffe,True),(1,16,True),(0,1,True),(0,2,True),(3,1,True)]:
            make(query=query,mode=mode,team=team,friend=friend,watched=0xe5000003,name='player_alignment')
    for velocity in (0xbf800000,0x7fc12345,0x7f812345,0x80000001):make(watched=0xe5000003,changes=[(policy.BODIES[4]+0x158,velocity,4)],name='alignment_numeric')
    make(changes=[(policy.QROWS+0x40,0xe5000002,4)],name='range_retained_target')
    make(changes=[(policy.QROWS+4+0x69,1,1)],name='range_target_kind')
    for override in (2,3):make(vehicle=True,changes=[(policy.character.TAGS+2*0x400+0x31,override,1)],name='vehicle_authored_range_variant')
    make(vehicle=True,distance=0x40a00000,changes=[(BASE+0xe800+i*0x300+0xcc+offset,value,4)for i in range(4)for offset,value in[(0x18,0x3f800000),(0xc,0x41200000)]],name='vehicle_near_range_rank')
    make(changes=[(policy.QUERYPOOL+0x44,GROWS,4)],name='readonly_query_group_alias')
    make(query=False,changes=[(GROUPS+0x44,ACTOR,4),(ACTOR+0x23,1,1),(ACTOR+0x26,0,1)],name='readonly_group_actor_alias')
    for confidence_nan,factor_nan in [(0x7fc12345,0x7ff8000056789000),(0x7f812345,0x7ff0000056789000)]:
        make(watched=0xe5000003,fp=[0x27f,0,0x1f80],changes=[(policy.QROWS+0xd0,confidence_nan,4),(0x46a6d0,factor_nan,8),(policy.BODIES[4]+0x158,0x7fc98765,4),(ACTOR+0x338,0xe7000000,4),(ACTOR+0x86,5,2),(0x46a16c,0x7fc54321,4),(0x46a11c,0x7fc45678,4)],name='paired_nan_operand_order')
    make(mode=2,watched=0xe5000003,required=True,name='required_multiplayer_owner')
    for kwargs in [dict(name='fault_seed_plain'),dict(name='fault_seed_vehicle',vehicle=True),dict(name='fault_seed_alignment',watched=0xe5000003),dict(name='fault_seed_group_position',watched=0xe5000003,query=False)]:make(**kwargs)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();assert a.verify or not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;meta=common.metadata(image)
    # C1E90 TEST has explicit READ metadata in this Capstone version.
    owned=prepare(image,meta,Path(os.environ['HALO2_TEST_SCRATCH']));rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in(r for r in rows if r['name'].startswith('fault_seed')):
        for n in range(len(seed['reads'])):
            row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_before_read_'+str(n),fault=dict(phase='before_read',ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));fixture=transport.intern_banks(dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows));encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert a.output.read_bytes()==encoded,'Perception score independent physical repeat differs'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,owners=meta['owners'],cases=len(rows),complete_calls=sum(r['completed']for r in rows),required_owner_boundaries=sum(r['boundary']is not None and r['boundary']['phase']=='required_owner'for r in rows),fault_prefixes=len(prefix),original_sites_executed=len(visited),uncovered_sites=[hex(s['instruction'])for s in meta['sites']if s['instruction']not in visited],fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=sha(a.output),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_ai_perception_score_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_ai_perception_score_tu15.cpp')});a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
