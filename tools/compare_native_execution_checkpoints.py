#!/usr/bin/env python3
"""Compare completed frozen accounting cohorts without changing raw counters.

Run through tools/run_test.py. Retains bounded metadata only; equality here is
a regression comparison between native builds, not original-frame parity.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re

from native_execution_report import analyze

FIELDS=('entries','body_entries','literal_statements','returns','unwinds',
        'returns_without_body','unwinds_without_body','active')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(path):
    return json.loads(path.read_text())


def checked_cohort(path):
    report=load(path);root=path.parent
    assert report['passed'] and report['status']=='passed',path
    assert not Path(report['owned_scratch']).exists(),'Completed cohort still owns scratch'
    assert all(step['exit_code']==0 for step in report['steps'])
    assert sha(root/'source-hashes.json')==report['source_inventory_sha256']
    manifest=load(root/'source-hashes.json')
    coverage=load(root/'cmake/evidence/native-execution-coverage.json')
    aliases={(row['source'],row['canonical_sha256'],row['input_sha256']) for row in coverage['aliases']}
    total=Counter();runs={};unwinds={};unresolved={}
    for run in report['accounting_runs']:
        label=run['label'];assert label not in runs
        raw_path=root/Path(run['runtime_report']).name
        summary_path=root/Path(run['analysis']).name
        assert sha(raw_path)==run['runtime_report_sha256']
        assert sha(summary_path)==run['analysis_sha256']
        raw=load(raw_path);saved=load(summary_path);current=analyze(raw)
        assert all(saved[key]==value for key,value in current.items()),label
        assert saved['runtime_report_sha256']==sha(raw_path)
        assert raw['registered_records']==len(raw['rows'])
        for key in ('omitted_records','registration_failures','orphan_literal_events','active_scopes'):
            assert raw[key]==0,(label,key,raw[key])
        subtotal=Counter()
        for values in raw['rows']:
            row=dict(zip(raw['row_fields'],values,strict=True))
            unit=raw['units'][row['unit']];image=raw['image_names'][row['image']]
            assert tuple(unit[:3]) in aliases
            assert manifest[unit[0]]==unit[1]
            counts={key:row[key] for key in FIELDS}
            assert counts['entries']==counts['returns']+counts['unwinds']+counts['active']
            subtotal.update(counts)
            identity=(unit[0],unit[1],row['symbol'],image)
            reason=[]
            if row['dynamic_forwarders']:reason.append('dynamic_instruction_forwarder')
            if not row['literal_sites']:reason.append('no_literal_sites')
            if image=='unknown':reason.append('unknown_image_identity')
            def record(table):
                value=table.setdefault(identity,dict(source=unit[0],canonical_sha256=unit[1],
                    symbol=row['symbol'],image=image,input_sha256=[],runs={},reasons=reason))
                if unit[2] not in value['input_sha256']:value['input_sha256'].append(unit[2])
                old=value['runs'].setdefault(label,{key:0 for key in FIELDS})
                for key,value_count in counts.items():old[key]+=value_count
            if reason:record(unresolved)
            if counts['unwinds']:record(unwinds)
        assert all(subtotal[key]==saved['totals'][key] for key in FIELDS)
        total.update(subtotal)
        capture=run['capture'];assert capture['valid'] and capture['exit_code']==0
        command=list(capture['command'])
        runtime=str(Path(report['build'])/'halo2_native')
        assert command.count(runtime)==1
        command[command.index(runtime)]='@frozen-native-runtime@'
        for flag,replacement in (('--mp-input-script','@authenticated-input-script@'),
                                 ('--screenshot','@owned-frame-output@')):
            assert command.count(flag)==1
            index=command.index(flag)+1
            assert Path(command[index]).is_relative_to(Path(report['owned_scratch']))
            command[index]=replacement
        assert capture['cwd']==report['build']
        runs[label]=dict(frame_sha256=capture['sha256'],frame_bytes=capture['bytes'],
                        input_script_sha256=run['input_script_sha256'],
                        input_script_bytes=run['input_script_bytes'],normalized_command=command,
                        counters=dict(subtotal),raw_sha256=sha(raw_path),analysis_sha256=sha(summary_path))
    tests=(root/'cmake/tests.log').read_text()
    match=re.search(r'100% tests passed[^\n]*out of (\d+)',tests);assert match
    return dict(report_path=str(path),report_sha256=sha(path),component_tests=int(match[1]),
                source_inventory_sha256=report['source_inventory_sha256'],runtime_sha256=report['runtime_sha256'],
                runtime_bytes=report['runtime_bytes'],map_sha256=report['map_sha256'],
                totals=dict(total),runs=runs,unwind_rows=list(unwinds.values()),
                unresolved_rows=list(unresolved.values()),owned_scratch_removed=True,
                retained_large_artifacts=[])


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--before',type=Path,required=True)
    parser.add_argument('--after',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    before=checked_cohort(args.before);after=checked_cohort(args.after)
    assert before['map_sha256']==after['map_sha256']
    assert before['runs'].keys()==after['runs'].keys() and after['runs']
    frames=[]
    for label,current in after['runs'].items():
        previous=before['runs'][label]
        for key in ('normalized_command','input_script_sha256','input_script_bytes','frame_bytes'):
            assert previous[key]==current[key],(label,key)
        frames.append(dict(label=label,before_sha256=previous['frame_sha256'],
                           after_sha256=current['frame_sha256'],
                           identical=previous['frame_sha256']==current['frame_sha256']))
    result=dict(schema='native-execution-checkpoint-comparison-v1',before=before,after=after,
                delta_after_minus_before={key:after['totals'][key]-before['totals'][key] for key in FIELDS},
                frames=frames,all_frame_hashes_match=all(row['identical'] for row in frames),
                source_identity_and_raw_activation_totals_verified=True,
                original_frame_parity=False,performance_qualified=False,native_closure_proved=False,
                retained_large_artifacts=[])
    encoded=json.dumps(result,indent=2,sort_keys=True)+'\n'
    assert len(encoded.encode())<=1048576,'Comparison exceeds bounded report contract'
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(encoded)
    print(json.dumps(dict(frames=len(frames),all_frame_hashes_match=result['all_frame_hashes_match'],
                         after_totals=after['totals'],delta=result['delta_after_minus_before'],
                         unwind_owner_identities=len(after['unwind_rows']),
                         unresolved_owner_identities=len(after['unresolved_rows']))))
    return 0


if __name__=='__main__':
    raise SystemExit(main())
