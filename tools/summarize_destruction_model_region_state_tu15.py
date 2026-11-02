#!/usr/bin/env python3
"""Summarize actual target-native model calls and their live region bytes.

Source-frame identity and authored eligibility are checked, not synthesized.
Original-code component proof remains separate from current MCC gameplay and
from audible/visual/full-definition acceptance. Restores are excluded.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path


def sha(path):
    with Path(path).open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def signed(value):return value if value<0x80000000 else value-0x100000000


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('reports','preflight','eligibility-summary','output'):parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args();Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.output.exists()
    preflight=json.loads(args.preflight.read_text());assert preflight['passed'] and not preflight['drift']
    eligibility=json.loads(args.eligibility_summary.read_text());assert eligibility['passed']
    inputs={str(path.resolve()):sha(path)for path in (args.preflight,args.eligibility_summary)}
    cases={};total=changed=matched=fx_changes=0
    for name in ('power_module','warthog','ghost','warthog_gauss','banshee','scorpion','wraith','spectre'):
        path=args.reports/('destruction-'+name+'.json');data=json.loads(path.read_text());inputs[str(path.resolve())]=sha(path)
        assert data['runtime_cache_source']=='MCC' and not data['damage_or_destruction_results_supplied']
        assert data['passed'] and data['model_region_restore_comparison_steps_excluded_from_journal']
        rows=data['native_target_model_region_calls'];assert rows
        matches=[];effects=[];previous_tick=0;selected=0;mutations=0
        for index,row in enumerate(rows):
            assert row['target']==data['target'] and row['definition']==data['definition']
            assert row['full_generation_live'] and row['authored_fields_readback'] and row['returned']
            assert row['input_match_tick']>=previous_tick;previous_tick=row['input_match_tick']
            count=row['region_count'];assert 0<count<=16 and row['states_base']==row['active_base']+count*2
            assert len(row['arguments'])==10 and row['arguments'][0]==row['target'] and row['arguments'][1]==row['model']
            assert row['arguments'][8]==row['active_base'] and row['arguments'][9]==row['states_base']
            assert row['returned_al'] in (0,1) and row['entry_x87_depth']==row['return_x87_depth']
            for field,size in (('active_before',count),('active_after',count),('states_before',count*8),('states_after',count*8)):
                assert len(row[field])==size and all(isinstance(x,int)and 0<=x<256 for x in row[field])
            region,requested=signed(row['arguments'][3]),signed(row['arguments'][4])
            for response in data['native_target_region_eligibility']:
                if response['allowed'] and response['runtime_region']==region and response['requested_state']==requested and response['input_match_tick']==row['input_match_tick']:
                    matches.append(dict(call=index,eligibility_order=response['order'],region=region,requested_state=requested));break
            selected+=row['returned_al'];mutations+=row['active_before']!=row['active_after']or row['states_before']!=row['states_after']
            for region_index in range(count):
                start=region_index*8+4
                before=int.from_bytes(bytes(row['states_before'][start:start+4]),'little')
                after=int.from_bytes(bytes(row['states_after'][start:start+4]),'little')
                if before!=after:effects.append(dict(call=index,region=region_index,before=before,after=after))
        # Match a real native model call to allowed, authored damage-response
        # fields in the same live input step. This is correlation evidence;
        # it does not invent a strict nested-call order or sound outcome.
        assert matches,'Target-native model journal has no matching authored damage response'
        cases[name]=dict(passed=True,target=data['target'],definition=data['definition'],native_calls=len(rows),
            selected_returns=selected,region_byte_changes=mutations,authored_response_matches=matches,
            effect_handle_changes=effects,journal=rows)
        total+=len(rows);changed+=mutations;matched+=len(matches);fx_changes+=len(effects)
    result=dict(passed=True,scope=__doc__,inputs=inputs,cases=cases,native_model_calls=total,
        calls_with_region_byte_changes=changed,authored_response_matches=matched,effect_handle_changes=fx_changes,
        complete_world_comparisons=eligibility['complete_world_comparisons'],
        exact_current_world_from_og_xbox_verified=False,all_proper_sounds_verified=False,
        complete_37_definition_goal=False,selected_launcher_runtime_promoted=False,
        retained_large_artifacts=[],tool_sha256=sha(__file__))
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({key:result[key]for key in ('passed','native_model_calls','calls_with_region_byte_changes','authored_response_matches','effect_handle_changes','complete_world_comparisons')}))


if __name__=='__main__':main()
