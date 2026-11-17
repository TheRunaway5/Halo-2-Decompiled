#!/usr/bin/env python3
"""Run the predicate matrix on one owned immutable source copy."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

from build_launcher_engine import freeze_source
from native_metadata import retain_bytes

ROOT=Path(__file__).resolve().parents[1]
SUBJECTS=(
    'include/halo2/native_tu15_contact_predicate_kernel.hpp',
    'src/physics/runtime/native_tu15_contact_predicate_frame.hpp',
    'src/physics/runtime/native_tu15_contact_predicate_source_frame.hpp',
    'src/physics/runtime/native_tu15_contact_predicate_source_owner.hpp',
    'tools/native_contact_predicate_replacements.py',
    'tools/validate_native_contact_predicate_source_tu15.py',
    'tools/validate_native_contact_predicate_tu15.py',
    'tools/validate_native_contact_predicate_value_tu15.py',
    'tests/native_tu15_contact_predicate_source_tests.cpp',
    'tests/native_tu15_contact_predicate_tests.cpp',
)


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--stage',action='append',choices=('source-current','source-fresh','physical','public-value'))
    parser.add_argument('--installed',action='store_true',help='Require already bound raw and prepared source routes')
    args=parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Use tools/run_test.py')
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    requested=set(args.stage or ('source-current','source-fresh','physical','public-value'))
    report=dict(passed=False,private_unbound=not args.installed,installed_routes=args.installed,
                owned_scratch=str(owner),requested_stages=sorted(requested),
                retained_large_artifacts=[],runs=[])
    def save():
        (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    try:
        source=owner/'source'
        test_pins={str(path.relative_to(ROOT)):sha(path) for path in (ROOT/'tests').iterdir() if path.is_file()}
        manifest=freeze_source(ROOT,source)
        for name,pin in test_pins.items():
            if sha(source/name)!=pin or sha(ROOT/name)!=pin:
                raise ValueError('Test source changed during freezing '+name)
        manifest={**manifest,**test_pins}
        for name in SUBJECTS:
            if manifest.get(name)!=sha(source/name):
                raise ValueError('Frozen predicate subject not covered by manifest '+name)
        payload=(json.dumps(manifest,sort_keys=True,indent=2)+'\n').encode()
        report.update(source_manifest=retain_bytes(payload,out/'source-hashes.json',out),
            subject_sha256={name:manifest[name] for name in SUBJECTS})
        save()
        print('SNAPSHOT_READY',report['source_manifest']['raw_sha256'],owner,flush=True)
        clean={name:value for name,value in os.environ.items() if not name.startswith('HALO2_REFERENCE_')}
        shared=['--sanitizers','--ordered-reference','--bound-candidate','--mutations','--structure']
        if args.installed:shared.append('--require-installed')
        steps=(
            ('source-current','validate_native_contact_predicate_source_tu15.py',shared),
            ('source-fresh','validate_native_contact_predicate_source_tu15.py',[*shared,'--fresh']),
            ('physical','validate_native_contact_predicate_tu15.py',['--sanitizers','--prefixes','--mutations']),
            ('public-value','validate_native_contact_predicate_value_tu15.py',['--sanitizers']),
        )
        children=[]
        for label,script,flags in steps:
            if label not in requested:continue
            receipt=out/label/'report.json'
            command=[sys.executable,'-B',str(source/'tools/run_test.py'),'--scratch-root',str(owner/'children'),
                     '--',sys.executable,'-B',str(source/'tools'/script),*flags,'--report',str(receipt)]
            log=out/(label+'.log')
            started=time.monotonic()
            print('STAGE_STARTED',label,flush=True)
            with log.open('w') as stream:
                with subprocess.Popen(command,cwd=source,env=clean,stdout=stream,stderr=subprocess.STDOUT) as child:
                    while True:
                        remaining=1200-(time.monotonic()-started)
                        if remaining<=0:
                            child.terminate()
                            try:child.wait(timeout=10)
                            except subprocess.TimeoutExpired:child.kill();child.wait()
                            raise subprocess.TimeoutExpired(command,1200)
                        try:
                            code=child.wait(timeout=min(30,remaining))
                            break
                        except subprocess.TimeoutExpired:
                            print('STAGE_RUNNING',label,'elapsed_seconds',round(time.monotonic()-started),
                                  'log_bytes',log.stat().st_size,flush=True)
            row=dict(name=label,command=command,exit_code=code,log=str(log),log_sha256=sha(log))
            report['runs'].append(row)
            if receipt.is_file():
                result=json.loads(receipt.read_text())
                child=Path(result['owned_scratch'])
                if not child.is_relative_to(owner/'children'):
                    raise ValueError('Unexpected predicate child owner')
                children.append(child)
                row.update(report=str(receipt),report_sha256=sha(receipt),passed=result['passed'],
                           child_owner=str(child),child_owner_removed=not child.exists())
            save()
            if code or not row.get('passed') or not row.get('child_owner_removed'):
                raise ValueError('Frozen private predicate matrix failed '+label)
            print('STAGE_PASSED',label,'elapsed_seconds',round(time.monotonic()-started),flush=True)
        if any(sha(source/name)!=pin for name,pin in report['subject_sha256'].items()):
            raise ValueError('Frozen predicate subjects changed')
        report.update(passed=True,removed_child_owners=[str(p) for p in children],
            scope=('Exact installed source routes' if args.installed else 'Selected private predicate stages')+
                  ' from one immutable owned source. No parent/runtime or other-image claim.')
    except BaseException as error:
        report['error']=repr(error)
        raise
    finally:
        save()
    return 0


if __name__=='__main__':
    raise SystemExit(main())
