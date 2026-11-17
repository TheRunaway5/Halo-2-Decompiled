#!/usr/bin/env python3
"""Unchanged physical1CACE0 through native goal/query/search or named probes."""
import argparse,copy,gzip,json,os,re
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_area_perform_tu15 as ref
import oracle_native_campaign_ai_publication_tu15 as publication
import oracle_native_campaign_ai_path_search_tu15 as physical
ENTRIES=tuple(dict.fromkeys((0x1cace0,0x1ca900,0x1caa20,0x2673a0,0x30bd0)+ref.ENTRIES+publication.ENTRIES))
STOP=tuple(dict.fromkeys((0x1ca560,)+tuple(e for e in ref.STOP_ENTRIES if e!=0x2013b0)+tuple(e for e in publication.STOP_ENTRIES if e!=0x1d2cf0)))
STATE=publication.BASE+0x5500

def inputs(image,include_legacy_goals=False):
    rows=[]
    def make(name,*,guards=(0,0,1),done=0,flags=0,used=(0,0),profile=physical.FP_PROFILES[0],position=(1.,1.,0.),frame=0xffff,state=STATE):
        s=publication.Scene();s.actor(publication.ACTOR,position=position,raw={7:(1,1),0x605:(guards[0],1),0x264:(guards[1],1),0x40:(guards[2],1),0x28c:0,0x288:frame,0x58:0xffffffff})
        s.put(state+0xc,flags);s.put(state+0x10,done,1)
        row=publication.make(image,rows,name,0x1cace0,s,(publication.ACTOR,state),fp=profile)
        # Releasing the first lease retires the pool; the second acquisition
        # initializes descriptors from51DF44. Retain that actual backing too.
        row['banks'].append(dict(address=0x1150000,before=bytes(0x452e8).hex()))
        row['banks'].sort(key=lambda b:b['address'])
        for bank in row['banks']:
            if bank['address']==0x474d04:
                data=bytearray.fromhex(bank['before']);data[4:8]=(0x44ac28).to_bytes(4,'little');bank['before']=data.hex()
            if bank['address']==0x50208c:
                data=bytearray.fromhex(bank['before']);data[0]=used[0];data[12]=used[1];bank['before']=data.hex()
        row['stop_before_entries']=list(STOP);row['strict_store_pcs']=False
        return row
    for p,fp in enumerate(physical.FP_PROFILES):
        for guard in ((1,0,1),(0x80,1,1),(0,1,1),(0,0xff,0),(0,0,0)):
            for flags in (0,5,0xffffffff):make(f'guard_{guard}_flags{flags:x}_fp{p}',guards=guard,flags=flags,profile=fp)
        for done in(1,0xff):
            for used in((0,0),(1,0),(0,1),(1,1)):make(f'goal_excluded_done{done}_used{used}_fp{p}',done=done,used=used,profile=fp)
        for flags in(0,1,4,5,0xffffffff):
            for position in((1.,1.,0.),(3.,2.,0.),(100.,100.,0.)):
                make(f'path_flags{flags:x}_position{position}_fp{p}',flags=flags,position=position,profile=fp)
        for flags in(0,1,5):
            make(f'actual_actor_embedded_state_flags{flags}_fp{p}',flags=flags,state=publication.Scene.row(publication.ACTOR)+0xd0,profile=fp)
    # Existing independently-authored goal selection scenes enter the new
    # parent with the same actor/character/order/firing-position inputs.
    # Older goal-only scenes stop before later child ownership. They are
    # available for diagnosis, not part of this parent qualification cohort.
    seeds=[s for s in ref.case_rows(image)if s['name'].startswith(('no_goal_select','compose_firing','compose_dynamic','compose_squad'))]if include_legacy_goals else []
    for n,seed in enumerate(seeds):
        row=copy.deepcopy(seed);banks={b['address']:bytearray.fromhex(b['before'])for b in row['banks']if not ref.STACK<=b['address']<ref.STACK+ref.STACK_SIZE}
        def put(at,value,width=4):
            for start,data in banks.items():
                if start<=at and at+width<=start+len(data):data[at-start:at-start+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little');return
            raise ValueError(hex(at))
        actor=ref.ACTOR;state=row['source'];put(actor+0x605,0,1);put(actor+0x264,0,1);put(actor+0x40,1,1);put(state+0x10,1,1)
        # The older pre-query scenes did not own a command datum. The new
        # real navigation filter must receive an actual NONE association.
        put(actor+0x858,0xffffffff)
        row.update(name='goal_parent_'+row['name'],entry=0x1cace0,live_x87=[],before_fp=list(physical.FP_PROFILES[n%6]),stop_before_entries=list(STOP),
            banks=[dict(address=a,before=bytes(b).hex())for a,b in sorted(banks.items())],strict_writes=False)
        rows.append(row)
    for n,row in enumerate(rows):row['id']=n
    return rows

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);p.add_argument('--select',help='Diagnostic case-name regular expression');p.add_argument('--include-legacy-goals',action='store_true',help='Diagnostic only: older goal scenes with incomplete downstream ownership');a=p.parse_args()
    assert not a.output.exists();assert ref.sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);meta=ref.metadata(image,ENTRIES)
    owned=ref.prepare(image,meta,scratch)
    declared=inputs(image,a.include_legacy_goals)
    if a.select:declared=[r for r in declared if re.search(a.select,r['name'])]
    try:rows=physical.physical(owned,declared,'whole')
    except Exception as error:
        a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(dict(passed=False,owned_scratch=str(scratch),error=str(error)),indent=2)+'\n');raise
    assert all(r['completed']or r.get('stopped_at')in STOP for r in rows)
    code=_Code(image,'.text');sites=set(complete_body(code,0x1cace0).code);reached=set().union(*(set(r['executed_sites'])for r in rows))
    counts={hex(e):sum(r.get('stopped_at')==e for r in rows)for e in STOP if any(r.get('stopped_at')==e for r in rows)}
    child_pcs=sorted(s['instruction']for s in meta['sites']if s['instruction']not in sites and any(o['write']for o in s['operands']))
    report=dict(passed=True,selection=a.select,cases=len(rows),whole_returns=sum(r.get('stopped_at')==ref.STOP for r in rows),named_refusals=counts,coverage=dict(total=len(sites),reached=len(sites&reached),missing=[hex(p)for p in sorted(sites-reached)]),ordered_stores=sum(len(r['writes'])for r in rows),owned_scratch=str(scratch),original_sha256=NEW_PIN)
    fixture=dict(fixture_kind='native-behavior129-perform-v1',image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,unattributed_child_store_pcs=child_pcs,bank_encoding='sparse-runs-v1',cases=ref.compact(rows))
    data=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert data==a.verify.read_bytes(),'Independent physical repeat differs'
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(data);report.update(fixture=str(a.output),fixture_bytes=len(data),fixture_sha256=ref.sha(a.output),repeat_identical=bool(a.verify))
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
