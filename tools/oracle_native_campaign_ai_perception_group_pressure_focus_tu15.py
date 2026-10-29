#!/usr/bin/env python3
"""Focused physical274200/274190/274490 from genuine perception pool states."""
import argparse,copy,gzip,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
import oracle_native_campaign_ai_perception_group_link_tu15 as link
from oracle_native_campaign_ai_perception_group_actor_tu15 import BASE,GROUPS,GROWS,HEADERS,BODY,CLUMP
sha=common.sha
def inputs(image):
    source=link.inputs(image);rows=[]
    ordinary=source[0];pressures=[r for r in source if r['name'].startswith('real_pressure_retirement')]
    def make(seed,entry,name,priority=3,minimum=1,request=None,changes=()):
        row=copy.deepcopy(seed);banks={b['address']:bytearray.fromhex(b['before'])for b in row['banks']}
        def put(p,v,w=4):
            for first,data in banks.items():
                if first<=p and p+w<=first+len(data):data[p-first:p-first+w]=(v&((1<<(8*w))-1)).to_bytes(w,'little');return
            raise ValueError((hex(p),w))
        if request is not None:
            #274490 consumes only kind/object/priority. Leave the unused+C
            # word alone; it can alias the actual pool bitmap pointer.
            put(request,2,2);put(request+4,0xea000001);put(request+8,priority,2)
            put(HEADERS+8,BODY)
        for p,v,w in changes:put(p,v,w)
        n=len(rows);row.update(id=n,name=name+'_'+str(n),entry=entry,clump_record=CLUMP,clump_datum=0xe4000001,request_priority=priority,minimum_priority=minimum,request=request or 0)
        if entry==0x274200:row['matrix']=CLUMP;row['source']=priority&0xffff
        elif entry==0x274190:row['gp'][0]=row['clump_datum'];row['matrix']=minimum&0xffff;row['source']=0
        else:row['gp'][3]=request;row['matrix']=CLUMP;row['source']=0
        row['before_fp']=[0x27f|((n%4)<<10),((n%8)<<11)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]]
        row['banks']=[dict(address=p,before=data.hex())for p,data in sorted(banks.items())];rows.append(row)
    for seed in [ordinary,*pressures]:
        for priority in (-32768,-1,0,1,2,3,4,32767):make(seed,0x274200,'allocate_actual_pool_state',priority=priority)
    for seed in pressures:
        for minimum in (-32768,-1,0,1,2,32767):make(seed,0x274190,'prune_actual_group_state',minimum=minimum)
        for priority in (0,3):make(seed,0x274490,'construct_private_shape',priority=priority,request=BASE+0xf700)
    for request in (BASE+0xf700,GROUPS+0x3c,GROWS+0x10,CLUMP+0x14):make(ordinary,0x274490,'retained_request_alias',request=request)
    other=CLUMP+0x50
    for old_priority in (-2,-1,0,3,4):
        make(pressures[0],0x274200,'threshold_progression',changes=[(other+0x24,old_priority,2),(GROWS+0xc,old_priority,2)])
    # Both score families, ordered clamps, wrapped source time and no-positive
    # candidate refusal. Original real operations choose any actual eviction.
    for previous in (0,999,1000,1001,2000):
        for seconds in (0,0x3d088889,0x3f800000,0xbf800000,1,0x7fc12345):
            make(pressures[0],0x274200,'age_scoring',changes=[(other+0x1c,0,1),(other+0x20,previous,4),(link.CLOCK+4,seconds,4)])
    for rank in (-1,0,1,2,3,4,32767):make(pressures[0],0x274200,'active_rank_scoring',changes=[(other+0x26,rank,2)])
    make(pressures[0],0x274200,'no_groups_to_reclaim',changes=[(other+0x14,0xffffffff,4)])
    make(pressures[0],0x274200,'disabled_pressure_scan',changes=[(link.STATE+1,0,1)])
    make(pressures[0],0x274200,'empty_clump_iterator',changes=[(link.CLUMPS+0x38,0,4)])
    make(pressures[0],0x274200,'bitmap_hole_before_candidate',changes=[(link.CLUMP_BITS,2,4)])
    # Equal candidate rank/priority: strict greater retains the first one.
    make(pressures[0],0x274200,'strict_candidate_tie',changes=[(link.CLUMPS+0x38,3,4),(link.CLUMP_BITS,7,4),(CLUMP+0xa0+0x24,0,2),(CLUMP+0xa0+0x26,0,2),(CLUMP+0xa0+0x1c,1,1),(CLUMP+0xa0+0x14,0xffffffff,4)])
    make(pressures[0],0x274190,'skip_head_then_retire',minimum=1,changes=[(GROWS,0x8000,2),(GROWS+0xc,2,2),(GROWS+0x14,0xe3000001,4),(GROWS+0xc4,0x8001,2),(GROWS+0xc4+0xc,0,2),(GROWS+0xc4+0x14,0xffffffff,4),(GROWS+0xc4+0x18,0xffffffff,4),(GROUPS+0x38,2,4),(GROUPS+0x3c,2,4),(GROUPS+0x34,2,4),(link.GROUP_BITS,3,4)])
    make(pressures[-1],0x274200,'fault_seed_allocate')
    make(pressures[-1],0x274190,'fault_seed_prune')
    make(ordinary,0x274490,'fault_seed_request_alias',request=GROUPS+0x3c)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();assert a.verify or not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT)
    common.ENTRIES=(0x274200,0x274190,0x274490,0x274020,0x2660f0,0x1e7810,0x160a60,0xc1e90,0x16e6a0,0x16e8a0,0x274800,0x2748e0,0x274970,0x1ea400,0x299aa0,0x1bda70,0x1c8e90,0x1c75b0,0x1c9750);meta=common.metadata(image)
    for site in meta['sites']:
        if site['bytes'][:4] in ('f3aa','f3ab'):site['repeat_count_register']=1
        if site['instruction']==0x16e6c5:assert site['bytes']=='852c8b';site['operands'][0]['read']=True
        if site['instruction']==0x2742b1:assert site['bytes']=='85048e';site['operands'][0]['read']=True
    owned=link.prepare(image,meta,Path(os.environ['HALO2_TEST_SCRATCH']));rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in (r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for n in range(len(events)):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited];fixture=link.intern_banks(dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows));encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert a.output.read_bytes()==encoded,'Focused pressure physical repeat differs'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    own_coverage={hex(entry):dict(total=meta['owners'][hex(entry)]['sites'],executed=sum(entry<=pc<end for pc in visited))for entry,end in [(0x274200,0x274441),(0x274190,0x2741f0),(0x274490,0x2744de)]}
    report=dict(passed=True,owners=meta['owners'],focused_coverage=own_coverage,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),original_sites_executed=len(visited),uncovered_sites=[hex(p)for p in uncovered],fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=sha(a.output),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/oracle_native_campaign_ai_perception_group_link_tu15.py':sha(ROOT/'tools/oracle_native_campaign_ai_perception_group_link_tu15.py'),'tools/observe_native_campaign_ai_perception_group_link_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_ai_perception_group_link_tu15.cpp')});a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
