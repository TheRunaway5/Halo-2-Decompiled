#!/usr/bin/env python3
"""Join actual native eligibility inputs to authenticated original numeric rules.

This records current MCC target generations and due/eligibility/blast order.
It does not certify original full gameplay, every authored definition, visuals
or sound output. Restore/replay observations are explicitly excluded.
"""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import struct


def sha(path):
    with Path(path).open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('reports','counts','manifest','pending-summary','output'):parser.add_argument('--'+name,type=Path,required=True)
    a=parser.parse_args();Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if a.output.exists():parser.error('Preserve previous evidence')
    manifest=json.loads(a.manifest.read_text());packed=a.counts.read_bytes();raw=gzip.decompress(packed)
    assert sha(a.counts)==manifest['count_vectors']['compressed_sha256']
    assert hashlib.sha256(raw).hexdigest()==manifest['count_vectors']['raw_sha256']
    assert raw[:8]==b'REG15V01' and len(raw)==12+65536*4
    pending=json.loads(a.pending_summary.read_text());assert pending['passed'] and pending['module_pending_passed']
    inputs={str(p.resolve()):sha(p) for p in (a.counts,a.manifest,a.pending_summary)}
    cases={};total=0;outcomes=[0,0,0]
    for name in ('power_module','warthog','warthog_gauss','ghost','banshee','scorpion','wraith','spectre'):
        path=a.reports/('destruction-'+name+'.json');report=json.loads(path.read_text());inputs[str(path.resolve())]=sha(path)
        assert report['runtime_cache_source']=='MCC' and not report['damage_or_destruction_results_supplied']
        if not report['passed']:
            cases[name]=dict(passed=False,error=report.get('error'),native_calls=0);continue
        assert report['eligibility_restore_comparison_steps_excluded_from_journal']
        rows=report['native_target_region_eligibility'];last_order=last_tick=0;counts=[0,0,0]
        for row in rows:
            assert row['target']==report['target'] and row['definition']==report['definition']
            assert row['full_generation_live'] and row['authored_fields_readback']
            assert row['order']>last_order and row['input_match_tick']>=last_tick
            last_order,last_tick=row['order'],row['input_match_tick']
            extent=row['descriptor_extent'];assert -32768<=extent<=32767
            original_count=struct.unpack_from('<i',raw,12+(extent&65535)*4)[0]
            assert row['region_count']==original_count
            assert row['active_base']==(row['body']+row['descriptor_offset'])&0xffffffff
            region=row['runtime_region'];requested=row['requested_state']
            assert -32768<=region<=32767 and -32768<=requested<=32767
            if region<0 or region>=original_count:
                assert row['current_state'] is None;allowed,reason=True,0
            else:
                current=row['current_state'];assert isinstance(current,int) and -128<=current<=127
                allowed=current<requested;reason=1 if allowed else 2
            assert row['allowed']==allowed and row['reason']==reason
            counts[reason]+=1;outcomes[reason]+=1;total+=1
        cases[name]=dict(passed=True,native_calls=len(rows),missing_region=counts[0],below_requested=counts[1],already_at_or_above=counts[2],
                         actual_target=report['target'],definition=report['definition'],journal=rows)
    module=json.loads((a.reports/'destruction-power_module.json').read_text())
    assert module['passed'] and module['native_due_eligibility_before_original_blast']
    due=max(row['order'] for row in module['native_target_pending_steps'] if row['status']==2)
    eligibility=module['native_pending_due_eligibility_order'];blast=module['native_pending_target_blast_order']
    assert due<eligibility<blast
    assert any(row['order']==eligibility and row['allowed'] for row in module['native_target_region_eligibility'])
    passed=all(row['passed'] for row in cases.values())
    result=dict(passed=passed,scope=__doc__,inputs=inputs,vehicle_cases=cases,native_eligibility_calls=total,
        outcome_counts=dict(missing_region=outcomes[0],below_requested=outcomes[1],already_at_or_above=outcomes[2]),
        module_order=dict(native_pending_due=due,native_eligibility=eligibility,original_target_blast=blast),
        module_due_eligibility_blast_order_passed=True,complete_world_comparisons=pending['complete_world_comparisons'],
        original_full_gameplay_or_audible_parity_verified=False,complete_37_definition_goal=False,
        original_xbe_sha256=manifest['original_xbe_sha256'],tool_sha256=sha(__file__),retained_large_artifacts=[])
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:result[k] for k in ('passed','native_eligibility_calls','outcome_counts','module_order','complete_world_comparisons')}))
    return 0 if passed else 1


if __name__=='__main__':raise SystemExit(main())
