#!/usr/bin/env python3
"""Whole original274670 ABI calls over the qualified actor/property input bank."""
import argparse,copy,gzip,json,os
from pathlib import Path
import oracle_native_campaign_ai_perception_group_actor_tu15 as source
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def inputs(image):
    rows=source.inputs(image)
    # The existing direct274750 numeric/control scenes also contain a genuine
    # one-actor clump and allocatable property pool. Enter their whole274670
    # parent with those declared inputs; no reference result is reused.
    for n,row in enumerate(rows):
        row['entry']=0x274670;row['gp'][0]=row['clump'];row['id']=n
        row['name']='whole_target_link_'+row['name']
    return rows

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True)
    p.add_argument('--verify',action='store_true');a=p.parse_args()
    assert a.verify or not a.output.exists();assert source.sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);source.common.ENTRIES=(0x274670,0x274750,0x2745e0,0x16e6a0,0xbbe00)
    meta=source.common.metadata(image)
    for site in meta['sites']:
        if site['bytes'].startswith('f3'):site['repeat_count_register']=1
        if site['instruction']==0x16e6c5:
            assert site['bytes']=='852c8b';site['operands'][0]['read']=True
        if site['instruction']==0x2747eb:
            assert site['bytes']=='d95b28';site['operands'][0].update(read=False,write=True)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=source.prepare(image,meta,scratch)
    rows=source.geometry.physical(owned,inputs(image),'whole')
    seed=next(row for row in rows if 'fault_seed_group' in row['name']);prefix=[]
    for phase,key in [('before_read','reads'),('before_write','writes'),('after_write','writes')]:
        for ordinal in sorted(set((0,len(seed[key])//2,len(seed[key])-1))):
            row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=f'prefix_{phase}_{ordinal}',fault=dict(phase=phase,ordinal=ordinal));prefix.append(row)
    rows+=source.geometry.physical(owned,prefix,'prefix')
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',fixture_kind='target_link',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows)
    encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert a.output.read_bytes()==encoded,'Independent original274670 repeat differs'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    sites=set().union(*(set(row['executed_sites'])for row in rows));own=[s['instruction']for s in meta['sites']if 0x274670<=s['instruction']<0x274742]
    report=dict(passed=True,cases=len(rows),complete_calls=sum(row['completed']for row in rows),prefix_calls=len(prefix),ordered_reads=sum(len(row['reads'])for row in rows),ordered_stores=sum(len(row['writes'])for row in rows),own_sites=len(own),own_sites_reached=sum(s in sites for s in own),unreached_own_sites=[hex(s)for s in own if s not in sites],fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=source.sha(a.output),owned_scratch=str(scratch),required_reference=True,instruction_patches=[],supplied_outcomes=False,source_hashes={str(Path(__file__).relative_to(ROOT)):source.sha(__file__)},retained_large_artifacts=[])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
