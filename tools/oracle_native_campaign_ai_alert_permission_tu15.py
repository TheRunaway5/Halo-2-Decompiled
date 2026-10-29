#!/usr/bin/env python3
"""Physical unchanged265620/25980 alert permission and retained RNG reference."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import oracle_native_campaign_ai_perception_policy_tu15 as policy
import oracle_native_campaign_ai_character_tu15 as character
import oracle_native_campaign_ai_perception_group_link_tu15 as transport
import validate_native_geometry_tu15 as geometry
BASE=policy.BASE;RECORD=character.ACTORS;NAV=BASE+0xf000;SEED=BASE+0xf600
ENTRIES=(0x265620,0x25980,0x1ecd00)
sha=common.sha
def prepare(image,meta,scratch):
    owned=transport.prepare(image,meta,scratch)
    result=subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_alert_permission_tu15.cpp'),'-o',str(owned/'observer')],capture_output=True,text=True);assert not result.returncode,result.stderr
    return owned
def inputs(image):
    rows=[];seed=next(s for s in policy.inputs()if s['name'].startswith('fault_seed'))
    def make(name='permission',selected=0xe7000000,active=0,reason=0,kind=2,unit=0xe5000000,random=0x12345678,owner=SEED,threshold=0x3f000000,chain=(0,-1,1),character_datum=0xe24e0000,changes=(),fp=(0x27f,0,0x1f80),live=0,missing=False):
        n=len(rows);banks={b['address']:bytearray.fromhex(b['before'])for b in seed['banks']}
        banks[0x46a000]=bytearray(image.read(0x46a000,4096))
        def put(p,v,w=4):
            for first,data in banks.items():
                if first<=p and p+w<=first+len(data):data[p-first:p-first+w]=(v&((1<<(8*w))-1)).to_bytes(w,'little');return
            banks[p]=bytearray((v&((1<<(8*w))-1)).to_bytes(w,'little'))
        put(RECORD+0x368,selected);put(RECORD+0x35e,active,1);put(RECORD+0x35a,reason,2);put(RECORD+0x358,kind,2);put(RECORD+0x18,unit);put(RECORD+0x35c,0x79,1);put(RECORD+0x54,character_datum)
        for i,c in enumerate(chain):
            tag=character.TAGS+i*0x400
            put(tag+0x5c,c);put(tag+0x60,NAV+i*0x40);put(tag+8,0xe24e0000+i+1 if i+1<len(chain)else 0xffffffff)
            put(NAV+i*0x40+0xc,threshold)
        put(0x4f3cd8,owner);put(owner,random)
        for p,v,w in changes:put(p,v,w)
        gp=seed['gp'].copy();gp[0]=seed['actor'];gp[1]=seed['perception'];gp[4]=geometry.SP+n%4*4
        def get(p,w=4):
            for first,data in banks.items():
                if first<=p and p+w<=first+len(data):return int.from_bytes(data[p-first:p-first+w],'little')
            raise ValueError(hex(p))
        required=get(RECORD+0x368)==seed['perception'] and get(RECORD+0x35e,1)!=0 and get(RECORD+0x18)!=0xffffffff
        rows.append(dict(id=n,name=f'{name}_{n}',entry=0x265620,gp=gp,matrix=0,source=0,actor=seed['actor'],perception=seed['perception'],child_output=0,private_output=False,required_owner_boundary=required,expected_unmapped_read=missing,banks=[dict(address=p,before=data.hex())for p,data in sorted(banks.items())],before_fp=list(fp),live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3).hex()for k in range(live)]))
    for selected in (0xffffffff,0xe7000000,0x12340000):
        for active in (0,1,255):
            for unit in (0xffffffff,0xe5000000):make(selected=selected,active=active,unit=unit,name='selection_speech_gates')
    for reason in (0,1,0x8000,0xffff):
        for kind in (0,1,2,3,4,0xffff):make(reason=reason,kind=kind,name='actual_permission_kind')
    for chain in ((1,),(0,1),(-1,0,1),(0,0,0)):
        make(chain=chain,missing=chain==(0,0,0),name='inherited_probability')
    make(character_datum=0xffffffff,missing=True,name='actual_null_probability_fault')
    for random in (0,1,0xffffffff,0x80000000,0x12345678,0xcafebabe):
        for threshold in (0,0x80000000,0x3f000000,0x3f800000,1,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
            for precision in (0x7f,0x27f,0x37f):
                for rounding in range(4):make(random=random,threshold=threshold,fp=(precision|(rounding<<10),((rounding+1)<<11)|0x41,0x1f80|(rounding<<13)|0x8040),live=(0,1,6)[rounding%3],name='extended_probability_controls')
    for alias in (NAV+0x80+0xc,RECORD+0x35c,RECORD+0x368,policy.PROWS+0x20,character.POOL+0x44):make(owner=alias,random=0x78 if alias==RECORD+0x35c else 0xe7000000 if alias==RECORD+0x368 else RECORD if alias==character.POOL+0x44 else 0x12345678,name='captured_rng_alias')
    # Real retained pointer owner may point back to itself; the captured owner
    # is used for the mutation even when its publication changes the global.
    make(owner=0x4f3cd8,random=0x4f3cd8,name='rng_global_self_alias')
    for kwargs in [dict(name='fault_seed_probability'),dict(name='fault_seed_speech',active=1),dict(name='fault_seed_kind',kind=1),dict(name='fault_seed_rng_alias',owner=NAV+0x80+0xc)]:make(**kwargs)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();assert a.verify or not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;meta=common.metadata(image);owned=prepare(image,meta,Path(os.environ['HALO2_TEST_SCRATCH']));rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in(r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for n in range(len(events)):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n),required_owner_boundary=False);prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));fixture=transport.intern_banks(dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows));encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert a.output.read_bytes()==encoded,'Alert permission independent physical repeat differs'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,owners=meta['owners'],cases=len(rows),complete_calls=sum(r['completed']for r in rows),required_owner_boundaries=sum(r['boundary']is not None and r['boundary']['phase']=='required_owner'for r in rows),actual_mapping_faults=sum(r['boundary']is not None and r['boundary']['phase']=='missing_backing'for r in rows),fault_prefixes=len(prefix),original_sites_executed=len(visited),uncovered_sites=[hex(s['instruction'])for s in meta['sites']if s['instruction']not in visited],fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=sha(a.output),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_ai_alert_permission_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_ai_alert_permission_tu15.cpp')});a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
