#!/usr/bin/env python3
"""Physical unchanged next-map/path lookup against actual MCC campaign data."""
import argparse,copy,gzip,json,os,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from mcc_content import MccCache
import oracle_native_campaign_screen_lifecycle_tu15 as common
import oracle_native_campaign_ai_perception_group_link_tu15 as transport
import validate_native_geometry_tu15 as geometry
BASE=0x1000000;SCENARIO=BASE+0x1000;GLOBALS=BASE+0x2000;HEADER=BASE+0x4000;ROWS=BASE+0x5000
ENTRIES=(0x1a5c80,0x1a5ab0);sha=common.sha
def prepare(image,meta,scratch):
    owned=transport.prepare(image,meta,scratch)
    result=subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_level_transition_tu15.cpp'),'-o',str(owned/'observer')],capture_output=True,text=True);assert not result.returncode,result.stderr
    return owned
def catalogue(path):
    cache=MccCache(path);globals=[t for t in cache.tags.values()if t['group']=='matg'];assert len(globals)==1
    headers=cache.block(globals[0]['offset']+0x170,8);assert headers
    records=cache.block(headers[0],0x108);rows=[]
    for p in records:
        rows.append(dict(campaign=int.from_bytes(cache.data[p:p+4],'little',signed=True),map=int.from_bytes(cache.data[p+4:p+8],'little',signed=True),scenario_path=cache.string(p+8,256)))
    return rows,dict(path=str(path),sha256=sha(path),bytes=path.stat().st_size,matg=globals[0]['name'],records=len(rows),rows=rows)
def inputs(actual):
    rows=[]
    def make(entry=0x1a5c80,name='lookup',records=actual,campaign=1,current=105,scenario=SCENARIO,globals=GLOBALS,outer=1,header=HEADER,count=None,rows_pointer=ROWS):
        n=len(rows);banks={BASE:bytearray(0x10000)}
        def put(p,v):
            for first,data in banks.items():
                if first<=p and p+4<=first+len(data):data[p-first:p-first+4]=(v&0xffffffff).to_bytes(4,'little');return
            banks[p]=bytearray((v&0xffffffff).to_bytes(4,'little'))
        put(0x4ecc20,scenario);put(0x4ecc1c,globals);put(GLOBALS+0x170,outer);put(GLOBALS+0x174,header);put(HEADER,len(records)if count is None else count);put(HEADER+4,rows_pointer)
        for i,r in enumerate(records):
            put(ROWS+i*0x108,r['campaign']);put(ROWS+i*0x108+4,r['map']);value=r['scenario_path'].encode()+b'\0';assert len(value)<=256;banks[BASE][ROWS-BASE+i*0x108+8:ROWS-BASE+i*0x108+8+len(value)]=value
        gp=[0x5a000000+n*37+i*11 for i in range(8)];gp[4]=geometry.SP+n%4*4
        rows.append(dict(id=n,name=f'{name}_{n}',entry=entry,gp=gp,matrix=campaign&0xffffffff,source=current&0xffffffff,campaign=campaign,current=current,catalog=records,catalog_profile=bool(scenario and globals and outer and header and rows_pointer==ROWS and(count is None or count==len(records))),rows_address=ROWS,child_output=0,private_output=False,banks=[dict(address=p,before=data.hex())for p,data in sorted(banks.items())],before_fp=[0x27f|(n%4<<10),(n%8<<11)|0x41,0x1f80|(n%4<<13)|0x8040],live_x87=[]))
    for entry in ENTRIES:
        for campaign in (-1,0,1,2):
            for current in (-2147483648,-1,0,105,110,115,805,2147483647):make(entry=entry,campaign=campaign,current=current,name='actual_mcc_campaign_paths')
        for r in actual:make(entry=entry,campaign=r['campaign'],current=r['map'],name='every_actual_mcc_map_key')
        for kwargs in [dict(scenario=0),dict(globals=0),dict(outer=0),dict(outer=0xffffffff),dict(header=0),dict(count=0),dict(count=-1),dict(count=-2147483648),dict(records=[])]:make(entry=entry,name='original_header_gates',**kwargs)
        for records in [list(reversed(actual)),actual[3:]+actual[:3],[dict(campaign=1,map=k,scenario_path='actual-test-path-'+str(i))for i,k in enumerate((130,110,120,110,-1,-2147483648,2147483647))],[dict(campaign=c,map=k,scenario_path='mixed-test-path-'+str(i))for i,(c,k)in enumerate(((2,110),(1,120),(1,110),(2,105),(1,110)))]]:
            for current in (-2147483648,-2,-1,0,105,110,120,2147483647):make(entry=entry,records=records,current=current,name='signed_minimum_first_path')
        make(entry=entry,name='fault_seed_'+hex(entry))
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--mainmenu',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();assert a.verify or not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    actual,owner=catalogue(a.mainmenu);image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;meta=common.metadata(image);owned=prepare(image,meta,Path(os.environ['HALO2_TEST_SCRATCH']));rows=geometry.physical(owned,inputs(actual),'complete');prefix=[]
    for seed in(r for r in rows if r['name'].startswith('fault_seed')):
        for n in range(len(seed['reads'])):
            row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_before_read_'+str(n),fault=dict(phase='before_read',ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));fixture=transport.intern_banks(dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,catalog_owner=owner,cases=rows));encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert a.output.read_bytes()==encoded,'Level transition independent physical repeat differs'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,owners=meta['owners'],cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),catalog_owner=owner,original_sites_executed=len(visited),uncovered_sites=[hex(s['instruction'])for s in meta['sites']if s['instruction']not in visited],fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=sha(a.output),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_level_transition_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_level_transition_tu15.cpp')});a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='catalog_owner'}));print(json.dumps(owner['rows']))
if __name__=='__main__':main()
