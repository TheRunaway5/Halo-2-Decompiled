#!/usr/bin/env python3
"""Physical unchanged1A5B60 basename lookup and32C0A0 last-character search."""
import argparse,copy,gzip,json,os,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import oracle_native_campaign_level_transition_tu15 as level
import oracle_native_campaign_ai_perception_group_link_tu15 as transport
import validate_native_geometry_tu15 as geometry
BASE,SCENARIO,GLOBALS,HEADER,ROWS=level.BASE,level.SCENARIO,level.GLOBALS,level.HEADER,level.ROWS
INPUT=BASE+0xe000;ENTRIES=(0x1a5b60,0x32c0a0);sha=common.sha
def prepare(image,meta,scratch):
    owned=transport.prepare(image,meta,scratch)
    result=subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_scenario_lookup_tu15.cpp'),'-o',str(owned/'observer')],capture_output=True,text=True);assert not result.returncode,result.stderr
    return owned
def inputs(actual):
    rows=[]
    def make(entry=0x1a5b60,name='scenario',records=actual,text=b'01a_tutorial',character=0x5c,scenario=SCENARIO,globals=GLOBALS,outer=1,header=HEADER,count=None):
        n=len(rows);banks={BASE:bytearray(0x10000)}
        def put(p,v):
            for first,data in banks.items():
                if first<=p and p+4<=first+len(data):data[p-first:p-first+4]=(v&0xffffffff).to_bytes(4,'little');return
            banks[p]=bytearray((v&0xffffffff).to_bytes(4,'little'))
        put(0x4ecc20,scenario);put(0x4ecc1c,globals);put(GLOBALS+0x170,outer);put(GLOBALS+0x174,header);put(HEADER,len(records)if count is None else count);put(HEADER+4,ROWS)
        for i,r in enumerate(records):
            put(ROWS+i*0x108,r['campaign']);put(ROWS+i*0x108+4,r['map']);value=r['scenario_path'].encode()+b'\0';assert len(value)<=256;banks[BASE][ROWS-BASE+i*0x108+8:ROWS-BASE+i*0x108+8+len(value)]=value
        raw=text+b'\0';banks[BASE][INPUT-BASE:INPUT-BASE+len(raw)]=raw
        gp=[0x5a000000+n*37+i*11 for i in range(8)];gp[4]=geometry.SP+n%4*4
        rows.append(dict(id=n,name=f'{name}_{n}',entry=entry,gp=gp,matrix=INPUT,source=character,input=INPUT,input_hex=text.hex(),character=character,catalog=records,catalog_profile=bool(entry==0x1a5b60 and scenario and globals and outer and header and(count is None or count==len(records))),rows_address=ROWS,child_output=0,private_output=False,banks=[dict(address=p,before=data.hex())for p,data in sorted(banks.items())],before_fp=[0x27f|(n%4<<10),(n%8<<11)|0x41,0x1f80|(n%4<<13)|0x8040],live_x87=[]))
    for r in actual:
        path=r['scenario_path'];leaf=path.rsplit('\\',1)[-1]
        for value in (path,leaf,'different\\directory\\'+leaf,path.upper(),path.replace('\\','/')):make(text=value.encode(),name='actual_mcc_case_sensitive_basename')
    for value in ('','\\','folder\\','not_a_map','01A_tutorial','01a_tutorial.map','01a_tutorial\0ignored','prefix/01a_tutorial'):
        make(text=value.encode(),name='exact_separator_terminator')
    duplicate=[dict(campaign=2,map=900,scenario_path='other\\01a_tutorial'),dict(campaign=1,map=101,scenario_path='canonical\\01a_tutorial')]
    for records in (duplicate,list(reversed(duplicate)),list(reversed(actual))):make(records=records,name='first_basename_regardless_campaign')
    for kwargs in [dict(scenario=0),dict(globals=0),dict(outer=0),dict(outer=0xffffffff),dict(header=0),dict(count=0),dict(count=-1),dict(count=-2147483648),dict(records=[])]:make(name='original_header_gates',**kwargs)
    for character in range(256):make(entry=0x32c0a0,text=bytes(range(1,256)),character=character,name='all_byte_characters')
    for text in (b'',b'a',b'\\',b'abc\\def\\',b'abc\\d',b'aa\0hidden\\'):
        for character in (0,ord('a'),0x5c,0xff):make(entry=0x32c0a0,text=text,character=character,name='last_occurrence_terminator')
    make(text=actual[0]['scenario_path'].encode(),name='fault_seed_lookup')
    make(entry=0x32c0a0,text=b'abc\\d',character=0x5c,name='fault_seed_last_character')
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--mainmenu',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();assert a.verify or not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    actual,owner=level.catalogue(a.mainmenu);image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;meta=common.metadata(image);owned=prepare(image,meta,Path(os.environ['HALO2_TEST_SCRATCH']));rows=geometry.physical(owned,inputs(actual),'complete');prefix=[]
    for seed in(r for r in rows if r['name'].startswith('fault_seed')):
        for n in range(len(seed['reads'])):
            row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_before_read_'+str(n),fault=dict(phase='before_read',ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));fixture=transport.intern_banks(dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,catalog_owner=owner,cases=rows));encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert a.output.read_bytes()==encoded,'Scenario lookup independent physical repeat differs'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,owners=meta['owners'],cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),catalog_owner=owner,original_sites_executed=len(visited),uncovered_sites=[hex(s['instruction'])for s in meta['sites']if s['instruction']not in visited],fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=sha(a.output),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_scenario_lookup_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_scenario_lookup_tu15.cpp')});a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='catalog_owner'}))
if __name__=='__main__':main()
