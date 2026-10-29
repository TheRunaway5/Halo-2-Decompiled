#!/usr/bin/env python3
"""Physical perception snapshot, attachment selection and actor-point owners."""
import argparse,copy,gzip,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import oracle_native_campaign_ai_perception_group_link_tu15 as group
import oracle_native_campaign_object_markers_tu15 as markers
import oracle_native_campaign_object_marker_request_tu15 as request
import validate_native_geometry_tu15 as geometry
BASE=markers.BASE
CLUSTERS,CROWS,ACTORS,AROWS,QUERY=BASE+0x9200,BASE+0x9300,BASE+0x9400,BASE+0x9500,BASE+0xf100
sha=common.sha
ENTRIES=(0x1ebcb0,0x26ddd0,0x11b4f0,0x299770,0x26ded0,0xbbe00,0xbcdf0,0xbc200,0x1d8c80,0x1d8d80,0xbad60,0xbacd0,0xfc510,0x1e11b0,0x1e1200,0x144910,0x145560)
def inputs(image):
    rows=[];seeds=request.inputs(image);bits=lambda v:struct.unpack('<I',struct.pack('<f',v))[0]
    def make(seed,entry=0x1ebcb0,name=None,kind=0,actor_kind=1,actor_cluster=True,chain=1,closest=0,missing_marker=False,true_fault=False,destination=markers.DEST,extension_offset=0x400,changes=()):
        row=copy.deepcopy(seed);banks={b['address']:bytearray.fromhex(b['before'])for b in row['banks']};banks[0x456000]=bytearray(image.read(0x456000,4096));banks[0x510000]=bytearray([0x67]*4096);banks[0x502000]=bytearray([0x71]*4096)
        def put(p,v,w=4):
            for start,data in banks.items():
                if start<=p and p+w<=start+len(data):data[p-start:p-start+w]=(v&((1<<(w*8))-1)).to_bytes(w,'little');return
            raise ValueError(hex(p))
        for p,v in[(0x510fa8,CLUSTERS),(CLUSTERS+0x44,CROWS),(CROWS+0x18,0xea000001 if chain else 0xffffffff),(0x5020c8,ACTORS),(ACTORS+0x44,AROWS),(AROWS+0x1c,0xe2000000 if actor_cluster else 0xffffffff)]:put(p,v)
        put(AROWS+7,actor_kind,1)
        for i,record in enumerate((request.BODY,request.PARENT)):
            put(record+0xaa,kind,1);put(record+0xb4,0xffffffff);put(record+0x134,1 if true_fault and i==0 else 0);put(record+0x13a,extension_offset,2);put(record+extension_offset+12,0xea000002 if i==0 and chain>1 else 0xffffffff)
            for lane in range(3):
                put(record+0x30+lane*4,bits(10.+i+lane));put(record+0x64+lane*4,bits(1.+i+lane));put(record+0x70+lane*4,bits(0.25+i+lane));put(record+0x88+lane*4,bits(0.5+i+lane));put(record+extension_offset+0x14+lane*4,bits((1.+i if lane==0 else 0.)if i!=closest else 0.))
            put(record+0x28,0x12345678+i);put(record+0x2c,0xabcdef00+i);put(record+0x18,0,1)
        for lane in range(3):put(QUERY+lane*4,0)
        for word in range(14):put(AROWS+0x22c+word*4,0x31400000+word*0x100)
        if missing_marker:put(request.OBJECT_TAG+0x38,0xffffffff);put(request.OBJECT_TAG+0x100+0x38,0xffffffff)
        for p,v,w in changes:put(p,v,w)
        n=len(rows);row.update(id=n,name=(name or row['name'])+'_'+str(n),entry=entry,object=0xea000001,actor=0xe1000000,cluster=0xe2000000,query=QUERY,destination=destination,arguments=[destination],banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())]);row['gp'][7]=0xea000001
        if entry==0x26ddd0:row['gp'][7]=destination;row['arguments']=[0xea000001]
        elif entry==0x11b4f0:row['gp'][0]=0xea000001;row['gp'][6]=destination;row['arguments']=[]
        elif entry==0x299770:row['gp'][0]=0xe2000000;row['gp'][1]=QUERY
        elif entry==0x26ded0:row['gp'][0]=0xe1000000;row['gp'][1]=destination;row['arguments']=[QUERY]
        if true_fault:row['alignment_fault']=True;row['actual_missing_read']=0x18
        rows.append(row)
    for seed in seeds:
        if seed['name'].startswith(('fault_seed','fallback_alias')):continue
        make(seed,kind=0);make(seed,entry=0x26ddd0,kind=0)
    seed=seeds[0]
    for kind in range(256):make(seed,name='object_kinds',kind=kind);make(seed,entry=0x26ddd0,name='state_kinds',kind=kind)
    for missing in(False,True):make(seed,entry=0x11b4f0,name='attachment_center',kind=12,missing_marker=missing)
    for chain in(0,1,2):
        for closest in(0,1):
            make(seed,entry=0x299770,name='closest_attachment',chain=chain,closest=closest,kind=2)
            make(seed,entry=0x26ded0,name='actor_attachment',chain=chain,closest=closest,kind=2)
    for actor_kind in(0,1,0xff):
        for actor_cluster in(False,True):make(seed,entry=0x26ded0,name='actor_gate',actor_kind=actor_kind,actor_cluster=actor_cluster,kind=2)
    for value in(0x80000000,1,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
        make(seed,entry=0x299770,name='query_numeric',chain=2,kind=2,changes=[(QUERY,value,4)])
        make(seed,entry=0x299770,name='candidate_numeric',chain=2,kind=2,changes=[(request.BODY+0x400+0x14,value,4)])
    for entry in(0x1ebcb0,0x26ddd0):
        for destination in(request.BODY+0x64,request.BODY+0x110):make(seed,entry=entry,name='snapshot_alias',kind=2,destination=destination)
    make(seed,entry=0x26ded0,name='forward_actor_copy_alias',actor_kind=0,destination=AROWS+0x230,kind=2)
    make(seed,entry=0x299770,name='signed_extension',kind=2,chain=2,closest=1,extension_offset=-0x600)
    make(seed,entry=0x299770,name='first_tie',kind=2,chain=2,closest=0,changes=[(request.PARENT+0x400+0x14,0,4)])
    make(seed,entry=0x299770,name='query_output_alias',kind=2,chain=2,destination=QUERY)
    make(seed,entry=0x299770,name='real_null_extension_fault',true_fault=True)
    make(seed,name='fault_seed_object')
    make(seed,entry=0x26ddd0,name='fault_seed_state',kind=12,missing_marker=True)
    make(seed,entry=0x299770,name='fault_seed_cluster',kind=2,chain=2,closest=1)
    make(seed,entry=0x26ded0,name='fault_seed_actor_copy',actor_kind=0,kind=2)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();assert a.verify or not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;meta=common.metadata(image)
    for site in meta['sites']:
        if site['bytes'].startswith('0f17'):site['operands'][0].update(read=False,write=True)
        if site['bytes'][:4]=='f3a5':site['repeat_count_register']=1
    owned=markers.prepare(image,meta,Path(os.environ['HALO2_TEST_SCRATCH']));rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in(r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for n in range(len(events)):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));roots=[s['instruction']for s in meta['sites']if any(lo<=s['instruction']<=hi for lo,hi in[(0x1ebcb0,0x1ebd9e),(0x26ddd0,0x26dec5),(0x26ded0,0x26df1b),(0x299770,0x29985a),(0x11b4f0,0x11b557)])];missing=[hex(s)for s in roots if s not in visited];assert not missing,missing
    fixture=group.intern_banks(dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows));encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert a.output.read_bytes()==encoded,'Perception-point original repeat differs'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,owners=meta['owners'],cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),root_sites=len(roots),closure_sites=len(visited),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),original_instructions=sum(r['original_instructions']for r in rows),fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=sha(a.output),owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[],source_hashes={name:sha(ROOT/name)for name in[str(Path(__file__).relative_to(ROOT)),'tools/observe_native_campaign_object_markers_tu15.cpp','tools/oracle_native_campaign_object_marker_request_tu15.py','tools/oracle_native_campaign_object_markers_tu15.py']});a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k not in('owners','source_hashes')}))
if __name__=='__main__':main()
