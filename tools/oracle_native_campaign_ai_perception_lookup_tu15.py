#!/usr/bin/env python3
"""Offline physical whole267520 with actual native-owner reference closure."""
import argparse,copy,gzip,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
import oracle_native_campaign_ai_perception_group_link_tu15 as group
from oracle_native_campaign_ai_perception_group_link_tu15 import BASE,AROWS,PROWS,GROWS,BODY,CLOCK
sha=common.sha
ENTRIES=(0x267520,0x1eb730,0x2749f0,0x274450,0x274e30,0x273b70,0x274490,0x274200,0x274190,0x274020,0x2660f0,0x274670,0x2745e0,0x274750,0xbcdf0,0xbbe00,0x1e7810,0x160a60,0xc1e90,0x29a990,0xc0ca0,0x16e6a0,0x16e8a0,0x274800,0x2748e0,0x274970,0x1ea400,0x299aa0,0x1bda70,0x1c8e90,0x1c75b0,0x1c9750)
def inputs(image):
    rows=[];seeds=group.inputs(image)
    def make(seed,name=None,create=1,object=0xea000001,self_object=False,active=1,missing_clump=False,chain=0,object_match=-1,owner_match=-1,owner_none=False):
        row=copy.deepcopy(seed);banks={b['address']:bytearray.fromhex(b['before'])for b in row['banks']}
        def put(p,v,w=4):
            for start,data in banks.items():
                if start<=p and p+w<=start+len(data):data[p-start:p-start+w]=(v&((1<<(w*8))-1)).to_bytes(w,'little');return
            raise ValueError(hex(p))
        put(BODY+0x12c,0xe1000000 if self_object else 0xe1000003)
        # Preserve the typed attachment path's real owner as well.
        put(BODY+0x404,0xe1000000 if self_object else 0xe1000003)
        put(GROWS+0x1c,0xe1000003)
        # Existing properties in a real group carry that group's datum. The
        # older2749F0 fixture did not consume this field after finding one.
        for i in range(16):put(PROWS+i*0x3c+8,0xe3000000)
        put(AROWS+9,active,1);put(AROWS+0x58,0xe2000000 if chain else 0xffffffff)
        if missing_clump:put(AROWS+0x7c,0xffffffff)
        put(CLOCK+2,(0,1,30,60,0x7fff,0x8000,0xffff)[len(rows)%7],2)
        if chain:
            for i in range(chain):
                p=PROWS+i*0x3c;g=GROWS+i*0xc4
                put(p+8,0xe3000000+i);put(p+0x20,object if i==object_match else 0xea00000a+i)
                put(p+0x2c,0xe2000000+i+1 if i+1<chain else 0xffffffff)
                put(g+0x22,1 if i==owner_match else 0,1)
                put(g+0x1c,0xffffffff if owner_none else 0xe1000003)
        n=len(rows);row.update(id=n,name=(name or row['name'])+'_'+str(n),entry=0x267520,object=object,actor=0xe1000000,create=create,matrix=0xe1000000,source=create,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())]);row['gp'][0]=object;rows.append(row)
    for seed in seeds:
        if seed['name'].startswith('fault_seed'):continue
        make(seed)
    seed=seeds[0]
    for create in (0,1,0x100,0x80,0xffffffff):
        for active in (0,1,0xff):make(seed,'creation_gate',create=create,active=active)
    for create in (0,1):
        make(seed,'object_none',object=0xffffffff,create=create)
        make(seed,'self_object',self_object=True,create=create)
        make(seed,'no_clump',missing_clump=True,create=create)
    for chain in (1,2,3):
        make(seed,'no_match_chain',chain=chain,create=0)
        for match in range(chain):
            make(seed,'first_object_match',chain=chain,object_match=match,create=0)
            make(seed,'attached_owner_match',chain=chain,owner_match=match,create=0)
            make(seed,'attached_owner_none',chain=chain,owner_match=match,owner_none=True,create=0)
        make(seed,'object_before_owner',chain=chain,object_match=0,owner_match=chain-1,create=1)
    make(seed,'fault_seed_create')
    make(seed,'fault_seed_lookup',chain=3,owner_match=2,create=0)
    make(seed,'fault_seed_no_match',chain=2,create=0)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args()
    assert a.verify or not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT)
    common.ENTRIES=ENTRIES;meta=common.metadata(image)
    for site in meta['sites']:
        if site['bytes'][:4]in('f3aa','f3ab'):site['repeat_count_register']=1
        if site['instruction']in(0x16e6c5,0x2742b1):
            assert site['bytes']in('852c8b','85048e');site['operands'][0]['read']=True
        if site['instruction']==0x2747eb:assert site['bytes']=='d95b28';site['operands'][0].update(read=False,write=True)
    owned=group.prepare(image,meta,Path(os.environ['HALO2_TEST_SCRATCH']));rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in (r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for n in range(len(events)):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix')
    visited=set().union(*(set(r['executed_sites'])for r in rows));root=[s['instruction']for s in meta['sites']if 0x267520<=s['instruction']<=0x267672];uncovered=[hex(p)for p in root if p not in visited]
    assert not uncovered,uncovered
    fixture=group.intern_banks(dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows));encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert a.output.read_bytes()==encoded,'Independent267520 physical repeat differs'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,owners=meta['owners'],cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),root_sites=len(root),root_uncovered=uncovered,closure_sites=len(visited),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),original_instructions=sum(r['original_instructions']for r in rows),fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=sha(a.output),owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[],private_numeric_fault_transport_closed=False,source_hashes={name:sha(ROOT/name)for name in[str(Path(__file__).relative_to(ROOT)),'tools/observe_native_campaign_ai_perception_group_link_tu15.cpp','tools/oracle_native_campaign_ai_perception_group_link_tu15.py','tools/oracle_native_campaign_ai_perception_group_actor_tu15.py']})
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k not in('owners','source_hashes')}))
if __name__=='__main__':main()
