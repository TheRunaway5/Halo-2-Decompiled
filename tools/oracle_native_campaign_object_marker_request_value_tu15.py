#!/usr/bin/env python3
"""Independent original BAD60 capacity-one outputs in its real caller's private layout.

Only declared before-input banks are derived from the canonical retained input.
No canonical outcomes are read as reference values; unchanged original BAD60
executes again with its output at the actual 1EBCB0 SP-relative location.
"""
import argparse,copy,gzip,json,os,sys
from pathlib import Path
sys.dont_write_bytecode=True
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_screen_lifecycle_tu15 as common
import oracle_native_campaign_ai_perception_group_link_tu15 as group
import oracle_native_campaign_object_markers_tu15 as markers
import validate_native_geometry_tu15 as geometry
SOURCE=ROOT/'tests/fixtures/native_campaign_object_marker_request_tu15.json.gz'
SOURCE_PIN='de2087cd8dc03f744108e65391d944be03fe96c90105834ab942667c736e87ad'
sha=common.sha

def inputs():
    assert sha(SOURCE)==SOURCE_PIN,'Canonical BAD60 input changed'
    canonical=json.loads(gzip.decompress(SOURCE.read_bytes()));rows=[];excluded=[]
    for source in canonical['cases']:
        if not source['completed']:continue
        if source['destination']!=markers.DEST:
            excluded.append(dict(source_id=source['id'],name=source['name'],reason='retained output aliases actual engine inputs; not a private-value caller'))
            continue
        row={name:copy.deepcopy(source[name])for name in ('name','entry','gp','arguments','object','direct','marker','before_fp','live_x87')}
        row['banks']=[dict(address=b['address'],before=canonical['bank_data'][b['before_ref']])for b in source['banks']]
        initial=None
        for bank in row['banks']:
            if bank['address']<=markers.DEST and markers.DEST+0x70<=bank['address']+len(bank['before'])//2:
                start=(markers.DEST-bank['address'])*2;initial=bank['before'][start:start+0xe0];break
        assert initial is not None
        destination=row['gp'][4]+0x1c
        row.update(id=len(rows),name='private_capacity1_'+source['name'],source_id=source['id'],capacity=1,destination=destination,private_output=dict(address=destination,bytes=0x70))
        row['gp'][3]=destination;row['arguments'][2]=1
        row['banks'].append(dict(address=destination,before=initial));rows.append(row)
    assert len(rows)==113 and len(excluded)==3
    return rows,excluded

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args()
    assert a.verify or not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);common.ENTRIES=(0xbad60,0xbacd0,0xfc510,0x1e11b0,0x1e1200,0x144910,0x145560);meta=common.metadata(image)
    for site in meta['sites']:
        if site['bytes'].startswith('0f17'):assert len(site['operands'])==1;site['operands'][0].update(read=False,write=True)
        if site['bytes'][:4]=='f3a5':site['repeat_count_register']=1
    code=_Code(image,'.text');caller=[]
    for pc in sorted(complete_body(code,0x1ebcb0).code):
        if pc<=0x1ebd16:
            ins=code.at(pc);caller.append(f'{pc:08X} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}')
    assert '001EBD0D 8d5c2418 lea ebx, [esp + 0x18]'in caller
    owned=markers.prepare(image,meta,Path(os.environ['HALO2_TEST_SCRATCH']));declared,excluded=inputs();rows=geometry.physical(owned,declared,'private-complete')
    engine_reads=private_reads=stores=0
    for row in rows:
        assert row['completed'];first=row['destination'];last=first+0x70
        for event in row['reads']:
            at=event['address'];n=event['bytes'];private=first<=at and at+n<=last
            assert private or at+n<=first or at>=last,'A data read crosses the actual private output'
            private_reads+=private;engine_reads+=not private
        for event in row['writes']:
            assert first<=event['address'] and event['address']+len(event['written'])//2<=last,'Original private-profile call mutates retained engine memory'
            stores+=1
        for bank in row['banks']:
            if bank['address']!=first:assert bank['before']==bank['after'],'Private output input owner aliases engine state'
    fixture=group.intern_banks(dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,private_value_profile='actual1EBCB0-BAD60-entrySP+1C-capacity1',canonical_input=dict(path=str(SOURCE.relative_to(ROOT)),sha256=SOURCE_PIN),caller_prefix_listing=caller,whole_parent_executed=False,cases=rows))
    encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    assert len(encoded)<=1024*1024
    if a.verify:assert a.output.read_bytes()==encoded,'Original private marker value repeat differs'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,cases=len(rows),completed_original_calls=len(rows),actual_one_record_capacity=True,meaningful_fields_per_case=28,engine_reads=engine_reads,excluded_private_reads=private_reads,private_stores=stores,original_instructions=sum(r['original_instructions']for r in rows),closure_sites=len(set().union(*(set(r['executed_sites'])for r in rows))),canonical_input_sha256=SOURCE_PIN,excluded_alias_inputs=excluded,actual_private_layout='BAD60 entrySP+1C from original1EBCB0 1EBD0D',whole_parent_executed=False,fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=sha(a.output),owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[],required_reference_inputs=[str(a.output)],source_hashes={name:sha(ROOT/name)for name in (str(Path(__file__).relative_to(ROOT)),'tools/observe_native_campaign_object_markers_tu15.cpp','tools/oracle_native_campaign_object_markers_tu15.py','tools/oracle_native_campaign_object_marker_request_tu15.py')})
    assert sha(SOURCE)==SOURCE_PIN;a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k not in ('source_hashes','excluded_alias_inputs')}))
if __name__=='__main__':main()
