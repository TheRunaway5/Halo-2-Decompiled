#!/usr/bin/env python3
"""Join actual MCC pending-word observations to unchanged TU1.5 numeric results.

This is the eight-case gameplay/continuation gate, not original full-gameplay,
framebuffer, speaker-output, all-vehicle or all-sound acceptance. Reports and
required numeric references remain small; no snapshots or banks are copied.
"""
import argparse,gzip,hashlib,json,os,struct
from pathlib import Path

VEHICLES=('warthog','warthog_gauss','ghost','banshee','scorpion','wraith','spectre')
PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--reports',type=Path,required=True)
    p.add_argument('--reference',type=Path,required=True);p.add_argument('--vectors',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if a.output.exists():p.error('Preserve prior qualification results')
    reference=json.loads(a.reference.read_text());assert reference['xbe_sha256']==PIN and reference['word_cases']==65536
    packed=a.vectors.read_bytes();raw=gzip.decompress(packed)
    assert sha(a.vectors)==reference['vector_sha256'] and hashlib.sha256(raw).hexdigest()==reference['vector_raw_sha256']
    assert raw[:8]==b'PND15V01' and len(raw)==12+65536*6
    cases={};inputs={str(a.reference.resolve()):sha(a.reference),str(a.vectors.resolve()):sha(a.vectors)}
    for name in ('power_module',*VEHICLES):
        path=a.reports/('destruction-'+name+'.json');row=json.loads(path.read_text());inputs[str(path.resolve())]=sha(path)
        assert row['runtime_cache_source']=='MCC' and not row['damage_or_destruction_results_supplied']
        if row['passed']:
            assert row['source_dead'] or row['target_retired']
            continuation=row['post_destruction_continuation']
            assert continuation['passed'] and continuation['complete_encoded_world_sha256_comparisons']==16
            cases[name]=dict(passed=True,target=row['target'],definition=row['definition'],shots=row['shots'],source_dead=row['source_dead'],
                target_retired=row['target_retired'],projectile_count=len(row['source_projectiles']),debris_count=len(row['source_debris']),
                complete_world_comparisons=16,source_report_sha256=sha(path))
        else:
            assert row.get('error'),'A failed case must preserve its diagnostic'
            cases[name]=dict(passed=False,target=row['target'],definition=row['definition'],error=row['error'],
                observed_death_stores=len(row.get('original_dead_flag_stores',[])),
                complete_world_comparisons=0,source_report_sha256=sha(path))
    module=json.loads((a.reports/'destruction-power_module.json').read_text());assert module['passed']
    rows=module['native_target_pending_steps']
    assert rows and module['native_target_pending_counts']['waiting']>0 and module['native_target_pending_counts']['due']>0
    assert module['far_control_unchanged'] and module['witness_pristine_at_target_blast'] and module['chain_witness_retired']
    assert module['pending_response_continuation']['passed'] and module['pending_response_continuation']['returned_to_initial_cut']
    assert module['pending_response_continuation']['complete_encoded_world_sha256_comparisons']==16
    assert module['pending_restore_comparison_steps_excluded_from_journal']
    prior_order=0;prior_tick=0;due=[]
    for row in rows:
        assert row['target']==module['target'] and row['definition']==module['definition'] and row['full_generation_live'] and row['post_store_readback']
        assert row['store_instruction'] in (0xd8423,0xd74d3) and row['order']>prior_order and row['input_match_tick']>=prior_tick
        before=row['before'];assert 0<=before<65536
        after,count,index,status=struct.unpack_from('<HHBB',raw,12+before*6)
        assert (row['after'],row['response'],row['status'])==(after,index,status)
        assert status in(1,2),'Only committed active words belong to this journal'
        if status==2:due.append(row['order'])
        prior_order=row['order'];prior_tick=row['input_match_tick']
    assert due and min(due)<module['native_pending_target_blast_order']
    for section in module['actual_pending_response_cut']['sections']:
        assert section['pending_word']>>3 and any(r['section']==section['section'] and section['pending_word'] in(r['before'],r['after']) for r in rows)
    blasts=dict(module['original_destruction_emitter_stores']);assert blasts[module['target']]==1
    suite_passed=all(row['passed']for row in cases.values())
    result=dict(passed=suite_passed,module_pending_passed=True,scope=__doc__,original_xbe_sha256=PIN,inputs=inputs,vehicle_cases=cases,
        actual_target_native_word_comparisons=len(rows),actual_target_due_transitions=len(due),native_steps=rows,
        actual_pending_cut=module['actual_pending_response_cut'],native_due_before_original_target_blast=True,
        original_target_blast_count=blasts[module['target']],module_chain_controls_passed=True,
        complete_world_comparisons=sum(row['complete_world_comparisons'] for row in cases.values())+16,
        original_CPU_full_destruction_or_audible_parity_verified=False,complete_37_definition_goal=False,
        tool_sha256=sha(Path(__file__)),retained_large_artifacts=[])
    text=json.dumps(result,indent=2)+'\n';assert len(text)<1024*1024
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(text)
    print(json.dumps(dict(passed=suite_passed,module_pending_passed=True,vehicle_cases=len(VEHICLES),
        passing_vehicles=sum(cases[name]['passed']for name in VEHICLES),actual_target_native_words=len(rows),
        due_transitions=len(due),complete_world_comparisons=result['complete_world_comparisons'])),flush=True)
    return 0 if suite_passed else 1


if __name__=='__main__':raise SystemExit(main())
