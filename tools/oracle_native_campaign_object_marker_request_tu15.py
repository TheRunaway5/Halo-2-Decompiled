#!/usr/bin/env python3
"""Physical wholeBAD60 through real marker lookup/geometry/inheritance."""
import argparse,copy,gzip,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import oracle_native_campaign_ai_perception_group_link_tu15 as group
import oracle_native_campaign_object_markers_tu15 as markers
import validate_native_geometry_tu15 as geometry
BASE=markers.BASE
OBJECTS,HEADERS,BODY,PARENT,OBJECT_TAG,MODEL=BASE+0x9000,BASE+0x9100,BASE+0xa000,BASE+0xb000,BASE+0xc000,BASE+0xd000
sha=common.sha
def inputs(image):
    rows=[];seeds=markers.inputs(image)
    def make(seed,name=None,direct=False,missing_model=False,marker=0x4000095,capacity=3,mirrored=False,inherited=False,owner_none=False,destination=markers.DEST):
        row=copy.deepcopy(seed);banks={b['address']:bytearray.fromhex(b['before'])for b in row['banks']};banks[0x4ec000]=bytearray([0x63]*4096)
        def put(p,v,w=4):
            for start,data in banks.items():
                if start<=p and p+w<=start+len(data):data[p-start:p-start+w]=(v&((1<<(w*8))-1)).to_bytes(w,'little');return
            raise ValueError(hex(p))
        for p,v in[(0x4ecbd0,OBJECTS),(OBJECTS+0x44,HEADERS),(HEADERS+12+8,BODY),(HEADERS+24+8,PARENT),(markers.TAGS+8,OBJECT_TAG),(markers.TAGS+2*16+8,MODEL),(markers.TAGS+3*16+8,OBJECT_TAG+0x100),(MODEL+4,0xe4000001)]:put(p,v)
        put(OBJECT_TAG+0x38,0xffffffff if missing_model else 0xe4000002);put(OBJECT_TAG+0x100+0x38,0xffffffff if missing_model else 0xe4000002);put(OBJECT_TAG+2,0x10 if inherited else 0,1)
        for i,record in enumerate((BODY,PARENT)):
            put(record,0xe4000000 if i==0 else 0xe4000003);put(record+4,0x400 if mirrored else 0);put(record+0x14,0xea000002 if inherited and not i else 0xffffffff);put(record+0xaa,0,1);put(record+0x114,13,2);put(record+0x116,markers.NODES-record,2);put(record+0x11a,markers.VARIANTS-record,2)
        if owner_none:put(BODY+4,1);put(BODY+0xaa,2,1);put(BODY+0x12c,1,1);put(BODY+0x154,0xffffffff)
        n=len(rows);row.update(id=n,name=(name or row['name'])+'_'+str(n),entry=0xbad60,object=0xea000001,direct=direct,marker=marker,capacity=capacity,destination=destination,arguments=[0xea000001,marker,capacity&0xffffffff],banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())]);row['gp'][1]=(row['gp'][1]&0xffffff00)|int(direct);row['gp'][3]=destination;rows.append(row)
    for seed in seeds:
        if seed['entry']!=0x1e1200 or seed['name'].startswith(('fault_seed','marker_alias','signed_node','no_group')):continue
        # Preserve real controls/numeric/variant input data while selecting
        # the actual object caller's node map and signed capacity interface.
        make(seed,capacity=seed['capacity'],mirrored=seed['mirrored'])
    seed=seeds[0]
    for direct in(False,True):
        for missing in(False,True):
            for marker in(0,0xffffffff,0x4000095,0x5000001):
                for mirrored in(False,True):make(seed,'request_gate',direct=direct,missing_model=missing,marker=marker,mirrored=mirrored)
    for capacity in(-32768,-1,0,1,2,32767):make(seed,'signed_capacity',capacity=capacity)
    make(seed,'inherited_owner',inherited=True);make(seed,'missing_owner',owner_none=True)
    for destination in(markers.NODES+4,BODY+0x110,markers.MARKERS):make(seed,'fallback_alias',missing_model=True,mirrored=True,destination=destination)
    make(seed,'fault_seed_named',capacity=1,mirrored=True)
    make(seed,'fault_seed_fallback',missing_model=True,mirrored=True)
    make(seed,'fault_seed_inherited',inherited=True,capacity=1)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();assert a.verify or not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);common.ENTRIES=(0xbad60,0xbacd0,0xfc510,0x1e11b0,0x1e1200,0x144910,0x145560);meta=common.metadata(image)
    for site in meta['sites']:
        if site['bytes'].startswith('0f17'):assert len(site['operands'])==1;site['operands'][0].update(read=False,write=True)
        if site['bytes'][:4]=='f3a5':site['repeat_count_register']=1
    owned=markers.prepare(image,meta,Path(os.environ['HALO2_TEST_SCRATCH']));rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in(r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for n in range(len(events)):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));root=[s['instruction']for s in meta['sites']if 0xbad60<=s['instruction']<=0xbaf05];missing=[hex(s)for s in root if s not in visited];assert not missing,missing
    fixture=group.intern_banks(dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows));encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert a.output.read_bytes()==encoded,'BAD60 original repeat differs'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,owners=meta['owners'],cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),root_sites=len(root),closure_sites=len(visited),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),original_instructions=sum(r['original_instructions']for r in rows),fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=sha(a.output),owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[],source_hashes={name:sha(ROOT/name)for name in[str(Path(__file__).relative_to(ROOT)),'tools/observe_native_campaign_object_markers_tu15.cpp','tools/oracle_native_campaign_object_markers_tu15.py']});a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k not in('owners','source_hashes')}))
if __name__=='__main__':main()
