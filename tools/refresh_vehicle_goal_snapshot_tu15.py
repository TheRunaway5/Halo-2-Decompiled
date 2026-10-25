#!/usr/bin/env python3
"""Replace an authenticated owned cohort with a current, coherent source freeze.

The build stays in the same run_test.py owner and CMake source path. Original
assets and installed runtimes are borrowed unchanged. No build runs here.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

from weapon_test_cache import sha

ROOT=Path(__file__).resolve().parents[1]


def save(path,value):path.write_text(json.dumps(value,indent=2)+'\n')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepared',type=Path,required=True)
    parser.add_argument('--cohort',type=Path,required=True)
    parser.add_argument('--refresh',type=Path,required=True)
    parser.add_argument('--mcc-root',type=Path,required=True)
    parser.add_argument('--inventory',type=Path,required=True)
    parser.add_argument('--cases',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source=owner/'source'
    prepared=json.loads(args.prepared.read_text());cohort=json.loads(args.cohort.read_text())
    refresh=json.loads(args.refresh.read_text())
    if (Path(prepared['runner_scratch'])!=owner or Path(cohort['owner'])!=owner or
            Path(refresh['owner'])!=owner or source.is_symlink()):
        raise ValueError('Only the same active private source owner may be refreshed')
    journal=ROOT/'analysis/vehicles-tu15-goal-20261023/session-v1/session.json'
    for command in json.loads(journal.read_text())['commands']:
        if 'finished' not in command and not any(
                '\n' not in value and Path(value).name==Path(__file__).name for value in command['argv']):
            raise ValueError('Another source/build command is still running')
    expected=dict(prepared['registered_snapshot_source_files_sha256'])
    for change in cohort['changes']:
        if expected[change['path']]!=change['before']:raise ValueError('Cohort preimage differs')
        expected[change['path']]=change['after']
    expected.update({'data/'+name:pin for name,pin in cohort['private_data_sha256'].items()})
    expected.update(refresh['refreshed_source_sha256'])
    def authenticate():
        drift=[name for name,pin in expected.items() if sha(source/name)!=pin]
        if drift:raise ValueError('Undeclared frozen drift: '+', '.join(drift[:10]))
    authenticate()
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    staging=owner/args.output.name
    staging.mkdir(exist_ok=False)
    original_report=args.output/'prepared-at-staging'
    command=[sys.executable,'-B',str(ROOT/'tools/validate_weapon_material_snapshot_tu15.py'),
        '--output',str(original_report),'--mcc-root',str(args.mcc_root.resolve(strict=True)),
        '--vehicle-weapon-fire','--vehicle-weapon-secondary-fire',
        '--owned-build-worker','--prepare-only','--release-only','--copy-drift-retries','5']
    result=subprocess.run(command,env=dict(os.environ,HALO2_TEST_SCRATCH=str(staging)))
    if result.returncode:return result.returncode
    fresh=json.loads((original_report/'report.json').read_text())
    pins=fresh['registered_snapshot_source_files_sha256'];candidate=staging/'source'
    if any(sha(candidate/name)!=pin for name,pin in pins.items()):
        raise ValueError('Fresh source changed before installation')
    maps=Path(fresh['runtime_cache_inputs']['maps_directory'])
    inventory=json.loads(args.inventory.read_text());cases=json.loads(args.cases.read_text())
    required={row['map'] for row in cases['cases']}
    originals={Path(row['mcc']).name:row['mcc_sha256'] for row in inventory['source_files'] if row.get('mcc')}
    map_inputs=[]
    for name in sorted(required):
        actual=sha(maps/name)
        if actual!=originals.get(name):raise ValueError('Current map differs from the variant inventory: '+name)
        map_inputs.append(dict(path=str(maps/name),sha256=actual,bytes=(maps/name).stat().st_size))
    fresh['runtime_cache_inputs']['all_variant_map_inputs']=map_inputs
    authenticate()
    changes=[]
    for name,pin in pins.items():
        old=source/name;new=candidate/name
        before=sha(old) if old.is_file() else None
        if before==pin:
            stamp=old.stat();os.utime(new,ns=(stamp.st_atime_ns,stamp.st_mtime_ns))
        else:
            os.utime(new,None);changes.append(dict(path=name,before=before,after=pin))
    retired=owner/(args.output.name+'-retired-source')
    if retired.exists():raise ValueError('A previous source replacement has not been retired')
    source.rename(retired)
    try:candidate.rename(source)
    except BaseException:
        retired.rename(source);raise
    if any(sha(source/name)!=pin for name,pin in pins.items()):
        raise ValueError('Installed source does not match the fresh snapshot')
    relocated=dict(fresh,runner_scratch=str(owner),private_source=str(source),
        preparation_relocated_from=str(candidate),original_preparation_report=str(original_report/'report.json'),
        original_preparation_report_sha256=sha(original_report/'report.json'))
    save(args.output/'prepared.json',relocated)
    save(args.output/'refresh.json',dict(status='prepared',passed=False,validation_not_run=True,
        owner=str(owner),source=str(source),current_snapshot=str(args.output/'prepared.json'),
        previous_prepared_sha256=sha(args.prepared),previous_cohort_sha256=sha(args.cohort),
        previous_refresh_sha256=sha(args.refresh),original_preparation_command=command,
        variant_inventory_sha256=sha(args.inventory),variant_cases_sha256=sha(args.cases),
        all_variant_map_inputs=map_inputs,
        changed_files=changes,previous_private_source_removed=False,
        native_execution='Compiled C++; original CPU execution is an offline oracle only',
        full_vehicle_parity=False,tool_sha256=sha(Path(__file__))))
    # Both trees are private copies owned by the enclosing runner. rmtree
    # unlinks borrowed asset/reference aliases without traversing their targets.
    shutil.rmtree(retired);staging.rmdir()
    receipt=json.loads((args.output/'refresh.json').read_text())
    receipt['previous_private_source_removed']=not retired.exists()
    save(args.output/'refresh.json',receipt)
    print(json.dumps(dict(status='prepared',changed_files=len(changes),
        prepared=str(args.output/'prepared.json'))),flush=True)
    return 0


if __name__=='__main__':raise SystemExit(main())
