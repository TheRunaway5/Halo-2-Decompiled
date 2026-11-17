#!/usr/bin/env python3
"""Unchanged physical FF1A0 calls with actual200420 optional scalar outputs."""
import argparse,copy,gzip,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_jump_arc_tu15 as arc
import oracle_native_campaign_ai_area_perform_tu15 as ref
import oracle_native_campaign_ai_path_search_tu15 as physical
ENTRIES=(0xff1a0,0xff580,0x30bd0)
OUTPUT=arc.BASE+0x1800
def inputs(image):
    rows=[]
    def append(seed,mask=7,alias=None):
        row=copy.deepcopy(seed);n=len(rows);row.update(id=n,name=seed['name']+f'_outputs{mask}_alias{alias}',live_x87=[],
            xmm=[[0x6b000000+n*37+r*4+k for k in range(4)]for r in range(8)],strict_writes=False,max_steps=2000)
        row['gp'][4]=ref.SP;row['gp'][6]=OUTPUT
        outputs=[OUTPUT+0x10,OUTPUT+0x14,OUTPUT+0x18]
        if alias is not None:outputs=[OUTPUT+alias]*3
        args=[row['speed_bits'],0x3f800000,outputs[0] if mask&1 else 0,0,0,1,0,0,0,outputs[1] if mask&2 else 0,outputs[2] if mask&4 else 0]
        row.update(matrix=args[0],source=args[1],stack_arguments=args[2:]);row['banks'].append(dict(address=OUTPUT,before=(bytes([0xa5])*0x40).hex()))
        row['banks'].sort(key=lambda b:b['address']);rows.append(row)
    seeds=[r for r in arc.inputs(image)if r['entry']==0xff1a0 and not r.get('actual_read_fault')and not r['name'].startswith('fault_seed')]
    for row in seeds:append(row)
    for row in [r for r in seeds if r['name'].startswith('controls_')][:12]:
        for mask in range(7):append(row,mask)
        for alias in (0,4,0x10):append(row,alias=alias)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True)
    p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert ref.sha(NEW_DEFAULT)==NEW_PIN
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);meta=ref.metadata(image,ENTRIES);owned=ref.prepare(image,meta,scratch)
    rows=physical.physical(owned,inputs(image),'whole');assert all(r['completed']for r in rows)
    reached=set().union(*(set(r['executed_sites'])for r in rows));code=_Code(image,'.text')
    coverage={hex(e):dict(reached=len(set(complete_body(code,e).code)&reached),total=len(complete_body(code,e).code),missing=[hex(pc)for pc in sorted(set(complete_body(code,e).code)-reached)])for e in ENTRIES}
    ref.compact(rows)
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',fixture_kind='movement_jump',instruction_patches=[],supplied_original_outcomes=False,cases=rows)
    encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert encoded==a.verify.read_bytes(),'Independent original repeat differs'
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,cases=len(rows),stores=sum(len(r['writes'])for r in rows),coverage=coverage,fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=ref.sha(a.output),original_sha256=NEW_PIN,owned_scratch=str(scratch),repeat_identical=bool(a.verify),retained_large_artifacts=[])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
