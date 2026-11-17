#!/usr/bin/env python3
"""Physical unchanged274570 snapshot/allocation/initialization/copy closure."""
import argparse,copy,gzip,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_area_perform_tu15 as ref
import oracle_native_campaign_ai_path_search_tu15 as physical
import oracle_native_campaign_ai_perception_group_pressure_focus_tu15 as pressure
from oracle_native_campaign_ai_perception_group_actor_tu15 import BASE,GROUPS,GROWS,HEADERS,BODY,CLUMP

SOURCE=BASE+0xe500
ENTRIES=(0x274570,0x2744e0,0x274200,0x274190,0x274020,0x2660f0,0x1e7810,0x160a60,0xc1e90,
         0x16e6a0,0x16e8a0,0x274800,0x2748e0,0x274970,0x1ea400,0x299aa0,0x1bda70,0x1c8e90,0x1c75b0,0x1c9750)

def make(image,seed,name,*,entry=0x274570,source=SOURCE,destination=SOURCE+0x200,
         priority=None,kind=2,body_kind=0,attachment=0,profile=physical.FP_PROFILES[0],alignment=0):
    row=copy.deepcopy(seed);banks={b['address']:bytearray.fromhex(b['before'])for b in row['banks']}
    banks[0x46a000]=bytearray(image.read(0x46a000,0x1000))
    def put(at,value,width=4):
        for first,data in banks.items():
            if first<=at and at+width<=first+len(data):data[at-first:at-first+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
        raise ValueError(hex(at))
    for offset in range(0xc4):put(source+offset,(offset*13+7)&255,1)
    put(source+2,kind,2);put(source+8,0xea000001);put(source+0xc,seed.get('request_priority',3)if priority is None else priority,2)
    put(HEADERS+20,BODY);put(BODY+0xaa,body_kind,1);put(BODY+0x134,attachment)
    put(BODY+0x13a,0x400,2);put(BODY+0x12e,2,2);put(BODY+0x404,0xe1000000);put(BODY+0x408,0xffffffff)
    gp=[0x11335577+k*0x10101 for k in range(8)];gp[0]=source;gp[2]=destination;gp[3]=CLUMP;gp[4]=ref.SP+alignment
    row.update(name=name,entry=entry,gp=gp,matrix=0x12345678,source=0x87654321,stack_arguments=[],
        banks=[dict(address=p,before=bytes(v).hex())for p,v in sorted(banks.items())],
        before_fp=list(profile),live_x87=[],xmm=[[0x6b000000+r*4+k for k in range(4)]for r in range(8)],
        stop_before_entries=[],max_steps=20000,strict_writes=False)
    return row

def inputs(image):
    seeds=[r for r in pressure.inputs(image)if r['entry']==0x274200];rows=[]
    ordinary=seeds[0]
    for p,profile in enumerate(physical.FP_PROFILES):
        for n,seed in enumerate(seeds):rows.append(make(image,seed,f'clone_pool{n}_fp{p}',profile=profile,alignment=4*(n%4)))
        for kind in(-1,0,1,2,8):
            for body,attachment in((0,0),(1,0),(5,0),(12,0),(12,1),(12,2)):
                rows.append(make(image,ordinary,f'clone_kind{kind}_body{body}_attachment{attachment}_fp{p}',kind=kind,body_kind=body,attachment=attachment,priority=3,profile=profile))
        # The destination may be the old source slot. Snapshotting must precede
        # the real allocation memset and initializer publications.
        rows.append(make(image,ordinary,f'clone_source_reused_slot_fp{p}',source=GROWS,priority=3,profile=profile))
        for offset in(-16,-4,-1,0,1,4,16,0x200):
            rows.append(make(image,ordinary,f'copy_alias{offset}_fp{p}',entry=0x2744e0,destination=SOURCE+offset,profile=profile))
    for n,row in enumerate(rows):row['id']=n
    return rows

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args()
    assert not a.output.exists();assert ref.sha(NEW_DEFAULT)==NEW_PIN
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);meta=ref.metadata(image,ENTRIES)
    owned=ref.prepare(image,meta,scratch);rows=physical.physical(owned,inputs(image),'whole');assert all(r['completed']for r in rows)
    reached=set().union(*(set(r['executed_sites'])for r in rows));code=_Code(image,'.text')
    coverage={hex(e):dict(reached=len(set(complete_body(code,e).code)&reached),total=len(complete_body(code,e).code),missing=[hex(pc)for pc in sorted(set(complete_body(code,e).code)-reached)])for e in ENTRIES}
    assert not coverage['0x274570']['missing'] and not coverage['0x2744e0']['missing'],coverage
    child_pcs=sorted(s['instruction']for s in meta['sites']if s['instruction']not in set(complete_body(code,0x274570).code)|set(complete_body(code,0x2744e0).code)and any(o['write']for o in s['operands']))
    count=len(rows);stores=sum(len(r['writes'])for r in rows)
    fixture=dict(fixture_kind='native-perception-group-clone-v1',image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,unattributed_child_store_pcs=child_pcs,bank_encoding='sparse-runs-v1',cases=ref.compact(rows))
    encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert encoded==a.verify.read_bytes(),'Independent original repeat differs'
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,cases=count,coverage=coverage,ordered_stores=stores,fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=ref.sha(a.output),original_sha256=NEW_PIN,owned_scratch=str(scratch),repeat_identical=bool(a.verify))
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
