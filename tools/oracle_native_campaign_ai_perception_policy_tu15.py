#!/usr/bin/env python3
"""Physical original authored perception/vehicle policy dependency closure."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import oracle_native_campaign_ai_character_tu15 as character
import oracle_native_campaign_ai_perception_group_link_tu15 as transport
import validate_native_geometry_tu15 as geometry
BASE=character.BASE
OBJECTS,HEADERS,PROPERTYPOOL,QUERYPOOL,PROWS,QROWS=BASE+0xb000,BASE+0xb100,BASE+0xb040,BASE+0xb080,BASE+0xdd00,BASE+0xe000
BODIES=[BASE+x for x in (0xc000,0xc400,0xc800,0xcc00,0xd000,0xd400,0xd800)]
ENTRIES=(0x2676c0,0x1ed240,0x1ed450,0x1ea1d0,0x1ea2e0,0x1ed4f0,0x1ed530,0x27c7e0,0x1c1d70,0x27c710,0x27c6c0)
sha=common.sha
def prepare(image,meta,scratch):
    owned=transport.prepare(image,meta,scratch)
    result=subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_perception_policy_tu15.cpp'),'-o',str(owned/'observer')],capture_output=True,text=True);assert not result.returncode,result.stderr
    return owned
def inputs():
    rows=[];seeds=[s for s in character.inputs()if s['entry']==0x1ed100];seed=next(s for s in seeds if s['name']=='policy_1ed100_on_True_0')
    def make(entry,source=seed,name='policy',changes=(),primary_seat=0,secondary_seat=1,secondary=0,
             unit=0xe5000000,secondary_unit=0xe5000001,vehicle=0xe5000002,vehicle_flags=0,unit_flags=0,
             category=0,perception_count=None,tracking_base=QROWS,query=0xe6000000,target=0xe5000002,
             child_kinds=(),child_players=(),child_output=True):
        n=len(rows);banks={BASE:bytearray(0x10000)}
        for b in source['banks']:
            p=b['address'];raw=bytes.fromhex(b['before'])
            if BASE<=p and p+len(raw)<=BASE+0x10000:banks[BASE][p-BASE:p-BASE+len(raw)]=raw
            else:banks[p]=bytearray(raw)
        def put(p,v,w=4):
            for first,data in banks.items():
                if first<=p and p+w<=first+len(data):data[p-first:p-first+w]=(v&((1<<(8*w))-1)).to_bytes(w,'little');return
            banks[p]=bytearray((v&((1<<(8*w))-1)).to_bytes(w,'little'))
        def get(p,w=4):
            for first,data in banks.items():
                if first<=p and p+w<=first+len(data):return int.from_bytes(data[p-first:p-first+w],'little')
            raise ValueError(hex(p))
        actor=source['actor'];record=character.ACTORS+(actor&0xffff)*0x888
        for i in range(4):
            definition=character.TAGS+i*0x400
            put(definition+0x7c,get(definition+0xa4));put(definition+0x80,character.MOVEMENT+i*0x100)
            put(definition+0x4c,get(definition+0xa4)if perception_count is None else perception_count);put(definition+0x50,character.NAV+i*0x100)
            put(definition+0xcc,get(definition+0xa4));put(definition+0xd0,BASE+0xe800+i*0x300)
            for j in range(3):put(BASE+0xe800+i*0x300+j*0xcc+8,0xe24e0003 if j==1 else 0x11110000+j)
        policy=character.TAGS+2*0x400;put(policy+0x2e,get(policy+0x2c,1),1);put(policy+0x31,0,1)
        for p,v in [(0x4ecbd0,OBJECTS),(OBJECTS+0x44,HEADERS),(0x50eef0,PROPERTYPOOL),(PROPERTYPOOL+0x44,PROWS),(0x50eeec,QUERYPOOL),(QUERYPOOL+0x44,tracking_base),(PROWS+0x14,query),(QROWS+0xc0,target),(record+0x18,unit),(record+0x274,secondary_unit)]:put(p,v)
        put(record+0x268,secondary,1);put(record+0x26c,0xffffffff);put(record+0x86,category,2)
        for i,body in enumerate(BODIES):put(HEADERS+i*12+8,body);put(body+0xc,0xffffffff);put(body+0x10,0xffffffff);put(body+0xaa,1,1);put(body+0x13c,0xffffffff)
        put(BODIES[0]+0x212,primary_seat,1);put(BODIES[1]+0x212,secondary_seat,1)
        for index,seat in [(0,primary_seat),(1,secondary_seat)]:
            signed=seat if seat<128 else seat-256
            if signed!=-1:put(BODIES[index]+0x218+signed*4,vehicle)
        put(BODIES[2],0xe24e0003);put(character.TAGS+3*0x400+0x12f,vehicle_flags,1);put(BODIES[0]+0x19,unit_flags,1)
        put(BODIES[3]+0x10,0xe5000004 if child_kinds else 0xffffffff)
        for i,kind in enumerate(child_kinds):
            body=BODIES[4+i];put(body+0xaa,kind,1);put(body+0x13c,child_players[i]if i<len(child_players)else 0xffffffff);put(body+0xc,0xe5000005+i if i+1<len(child_kinds)else 0xffffffff)
        for p,v,w in changes:put(p,v,w)
        gp=source['gp'].copy();gp[4]=geometry.SP+n%4*4;matrix=0xe24e0003
        if entry==0x2676c0:gp[1]=0xe7000000
        elif entry==0x1ed240:gp[3]=actor
        elif entry in(0x1ed450,0x1ea1d0,0x1ea2e0):gp[0]=actor
        elif entry==0x1ed4f0:gp[6]=actor
        elif entry==0x1ed530:gp[1]=actor
        elif entry==0x27c7e0:gp[1]=policy
        elif entry==0x1c1d70:gp[1]=0xe5000003;gp[7]=geometry.SP+0x400 if child_output else 0
        else:raise ValueError(hex(entry))
        rows.append(dict(id=n,name=f'{name}_{n}',entry=entry,gp=gp,matrix=matrix,source=0,actor=actor,vehicle=0xe24e0003,policy=policy,parent=0xe5000003,perception=0xe7000000,child_output=gp[7]if entry==0x1c1d70 else 0,banks=[dict(address=p,before=data.hex())for p,data in sorted(banks.items())],before_fp=source['before_fp'],live_x87=source['live_x87']))
    for s in seeds:
        for entry in (0x1ed240,0x1ed450,0x1ed530):make(entry,source=s,name='inherited_'+s['name'])
    for count in (-1,0,1,2,3,4):
        for category in (-1,0,1,3,4,5):
            for target in (0xffffffff,0xe5000000):make(0x1ed450,perception_count=count,category=category,changes=[(character.ACTORS+0x26c,target,4)],name='perception_variant')
    for value in range(256):make(0x27c7e0,changes=[(character.TAGS+2*0x400+0x31,value,1),(character.TAGS+2*0x400+0x24,0x8001,2)],name='all_signed_override_bytes')
    for entry in (0x1ea1d0,0x1ea2e0,0x1ed4f0):
        for secondary in (0,1):
            for primary_seat,secondary_seat,vehicle in [(-1,-1,0xffffffff),(-1,0,0xe5000002),(0,-1,0xe5000002),(0,1,0xe5000002),(-2,-2,0xe5000002),(127,0,0xe5000002)]:
                for vehicle_flags,unit_flags in ((0,0),(1,0),(0,2)):
                    make(entry,secondary=secondary,primary_seat=primary_seat,secondary_seat=secondary_seat,vehicle=vehicle,vehicle_flags=vehicle_flags,unit_flags=unit_flags,name='real_vehicle_admission')
        make(entry,unit=0xffffffff,secondary=1,name='vehicle_without_primary_unit');make(entry,unit=0xffffffff,secondary_unit=0xffffffff,secondary=1,name='all_units_none')
    for query in (0xffffffff,0xe6000000,0x80000001):
        for query_rows in (QROWS,0,0xffffff90):
            for target in (0xffffffff,0xe5000002):
                # Wrapped-null profiles must avoid an actual high-address read.
                if query==0x80000001 and query_rows!=QROWS:continue
                make(0x2676c0,query=query,tracking_base=query_rows,target=target,name='tracking_target_predicate')
    for kinds,players in [((),()),((1,),(0xe8000000,)),((0,),(0xffffffff,)),((0,),(0xe8000000,)),((1,0,0),(0xffffffff,0xffffffff,0xe8000002)),((0,0),(0xe8000000,0xe8000001))]:
        for output in (False,True):make(0x1c1d70,child_kinds=kinds,child_players=players,child_output=output,name='child_player_scan')
    for entry in ENTRIES[:9]:make(entry,name='fault_seed_'+hex(entry),secondary=1,child_kinds=(1,0),child_players=(0xffffffff,0xe8000001))
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();assert a.verify or not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;meta=common.metadata(image);owned=prepare(image,meta,Path(os.environ['HALO2_TEST_SCRATCH']));rows=geometry.physical(owned,inputs(),'complete');prefix=[]
    for seed in(r for r in rows if r['name'].startswith('fault_seed')):
        for n in range(len(seed['reads'])):
            row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_before_read_'+str(n),fault=dict(phase='before_read',ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));fixture=transport.intern_banks(dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows));encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert a.output.read_bytes()==encoded,'Perception policy independent physical repeat differs'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,owners=meta['owners'],cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),original_sites_executed=len(visited),uncovered_sites=[hex(s['instruction'])for s in meta['sites']if s['instruction']not in visited],fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=sha(a.output),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_ai_perception_policy_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_ai_perception_policy_tu15.cpp')});a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
