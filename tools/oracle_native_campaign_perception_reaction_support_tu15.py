#!/usr/bin/env python3
"""Physical transition-plane, hierarchy and reaction-state prerequisites."""
import argparse,copy,gzip,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import oracle_native_campaign_ai_perception_group_link_tu15 as group
import oracle_native_campaign_object_markers_tu15 as markers
import oracle_native_campaign_object_motion_refs_tu15 as motion
import validate_native_geometry_tu15 as geometry
BASE=motion.BASE
BSP,CLUSTERS,PLANES,LOCATION,POINT,DEST,CLOCK=BASE+0x9000,BASE+0xa000,BASE+0xb000,BASE+0xc000,BASE+0xc100,BASE+0xc200,BASE+0xd000
sha=common.sha
ENTRIES=(0x11e850,0xbcf70,0x1d2cc0,0xbcfb0,0xf34f0,0x275c20,0x275ba0)
def inputs(image):
    rows=[];seed=motion.inputs(image)[0];bits=lambda v:struct.unpack('<I',struct.pack('<f',v))[0]
    def make(entry=0x11e850,name='transition',cluster=0,selector=0x80,sector=0x1234,parents=2,flags=3,kind=0,header_kind=1,definition_flags=0x80,state=0,previous=1000,current=1000,datum=0xffffffff,unit_flag=1,object=0xea000001,output=DEST,changes=()):
        row=copy.deepcopy(seed);banks={b['address']:bytearray.fromhex(b['before'])for b in row['banks']};banks[0x51d000]=bytearray([0x73]*4096)
        def put(p,v,w=4):
            for start,data in banks.items():
                if start<=p and p+w<=start+len(data):data[p-start:p-start+w]=(v&((1<<(w*8))-1)).to_bytes(w,'little');return
            raise ValueError(hex(p))
        for p,v in[(0x4ecc18,BSP),(BSP+0xa0,CLUSTERS),(BSP+0x68,PLANES),(0x51df54,CLOCK),(CLOCK+8,current)]:put(p,v)
        put(LOCATION+4,cluster,2)
        if cluster!=0xffff:
            signed=cluster-0x10000 if cluster&0x8000 else cluster;put(CLUSTERS+signed*0xb0+0x70,selector,1)
        plane=PLANES+(selector&0x7f)*24;put(plane+2,sector,2)
        for lane,v in enumerate((1.,2.,3.)):put(plane+4+lane*4,bits(v));put(POINT+lane*4,bits(v))
        put(plane+16,bits(20.))
        for i in range(3):
            record=motion.RECORDS+i*0x1000
            put(motion.HEADERS+(i+1)*12+3,header_kind,1);put(record+0x14,0xea000002+i if i<parents else 0xffffffff);put(record+4,0x4000000 if flags&(1<<i) else 0);put(record+0xaa,kind,1);put(record+0x17c,unit_flag,1);put(record+0x12c,state,1);put(record+0x348,state,1);put(record+0x3dc,unit_flag,1);put(record+0x354,datum);put(record+0x350,previous);put(motion.DEFS+i*0x100+0x1ec,definition_flags)
        if object==0xffffffff:
            put(motion.POOL+0x44,BASE-0xffff*12);put(BASE+8,motion.RECORDS);put(BASE+3,header_kind,1)
        for p,v,w in changes:put(p,v,w)
        n=len(rows);gp=[0x5a100000+n*11+k*7 for k in range(8)];gp[4]=geometry.SP+n%4*4;args=[]
        if entry==0x11e850:gp[1]=LOCATION;gp[7]=POINT;args=[output]
        elif entry==0x1d2cc0:gp[6]=object
        elif entry in(0xbcfb0,0x275c20):gp[1]=object
        else:gp[0]=object
        row.update(id=n,name=name+'_'+str(n),entry=entry,gp=gp,arguments=args,object=object,location=LOCATION,point=POINT,sector_destination=output,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[(n//4)%4]|(n*5%64)],live_x87=[])
        # A fully flagged ancestry ending in NONE reaches the original
        # wrapped header byte. Keep the genuine missing-read fault.
        ancestry=(1<<(parents+1))-1
        if entry==0x1d2cc0 and object!=0xffffffff and flags&ancestry==ancestry:
            row['alignment_fault']=True;row['actual_missing_read']=motion.HEADERS+0xffff*12+3
        rows.append(row)
    for cluster in(0,1,2,0xfffe,0xffff):
        for selector in(0,0x7f,0x80,0xfe,0xff):
            for sector in(0,0x8000,0xffff):make(cluster=cluster,selector=selector,sector=sector)
    for output in(0,DEST,POINT,PLANES+4,LOCATION+4):make(name='optional_alias',output=output)
    for lane in range(3):
        for value in(0,0x80000000,1,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
            make(name='plane_numeric',changes=[(PLANES+4+lane*4,value,4)])
            make(name='point_numeric',changes=[(POINT+lane*4,value,4)])
    for value in(0x7fc12345,0x7f812345):make(name='plane_pair_nan',changes=[(PLANES+4,value,4),(PLANES+8,value^0x80000000,4)])
    for entry in ENTRIES[1:]:
        for parents in(0,1,2):
            for flags in range(8):make(entry,name='root_chain',parents=parents,flags=flags)
        for header in(0,1,0xff):make(entry,name='none_wrapped',object=0xffffffff,header_kind=header)
    for kind in range(256):
        for entry in(0x275c20,0x275ba0):make(entry,name='root_kind',kind=kind,previous=999)
    for entry in(0x275c20,0x275ba0):
        for kind in(0,12):
            for state in(0,1,0x80,0xff):
                for flag in(0,1,2):make(entry,name='root_state',kind=kind,state=state,unit_flag=flag,previous=999,datum=0xffffffff)
        make(entry,name='current_tick');make(entry,name='live_datum',datum=0xe1000003)
    for flags in(0,1,0x7f,0x80,0x100,0xffffffff):make(0xf34f0,name='definition_flag',definition_flags=flags)
    make(name='fault_seed_plane');make(name='fault_seed_accepted',selector=0)
    for entry in ENTRIES[1:]:make(entry,name='fault_seed_'+hex(entry),kind=12)
    make(0x275c20,name='fault_seed_clock',kind=0)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();assert a.verify or not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;meta=common.metadata(image);owned=markers.prepare(image,meta,Path(os.environ['HALO2_TEST_SCRATCH']));rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in(r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for n in range(len(events)):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));missing=[hex(s['instruction'])for s in meta['sites']if s['instruction']not in visited];assert not missing,missing
    fixture=group.intern_banks(dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows));encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert a.output.read_bytes()==encoded,'Reaction support original repeat differs'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,owners=meta['owners'],cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),original_sites=len(visited),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),original_instructions=sum(r['original_instructions']for r in rows),fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=sha(a.output),owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[],source_hashes={name:sha(ROOT/name)for name in[str(Path(__file__).relative_to(ROOT)),'tools/observe_native_campaign_object_markers_tu15.cpp','tools/oracle_native_campaign_object_markers_tu15.py','tools/oracle_native_campaign_object_motion_refs_tu15.py']});a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k not in('owners','source_hashes')}))
if __name__=='__main__':main()
