#!/usr/bin/env python3
"""Survey declared campaign firing inputs against immutable native Cache libraries.

Use tools/run_test.py. Compiler output and the probe executable stay in that
owner; the borrowed source/build is read-only. No world execution is performed.
"""
import argparse
import json
import os
from pathlib import Path
import shlex
import subprocess

from refresh_destruction_current_cohort import sha

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cohort',required=True,type=Path)
    parser.add_argument('--native-admission',required=True,type=Path)
    parser.add_argument('--map',required=True,type=Path)
    parser.add_argument('--bsp',required=True,type=int)
    parser.add_argument('--definition',required=True)
    parser.add_argument('--output',required=True,type=Path)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    cohort = json.loads(args.cohort.read_text());admission = json.loads(args.native_admission.read_text())
    frozen = Path(cohort['scratch']).resolve(strict=True)
    if frozen==owner or admission['scratch']!=str(frozen):
        raise ValueError('Input audit requires its own owner and the declared borrowed native cohort')
    worker = json.loads((args.native_admission.parent/'validation/report.json').read_text())
    if not any(step['name']=='build' and step.get('exit_code')==0 for step in worker['steps']):
        raise ValueError('Borrow only libraries from the completed native build phase')
    if sha(frozen/'source/src/assets/mcc_cache.cpp')!=admission['patches']['src/assets/mcc_cache.cpp']['after']:
        raise ValueError('Borrowed native Cache source changed after admission')
    args.output = args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    link = shlex.split((frozen/'build/CMakeFiles/mcc_destruction_model_cache_tests.dir/link.txt').read_text())
    libraries = []
    for item in link[link.index('-o')+2:]:
        if item.startswith('-'):raise ValueError('Unexpected native Cache library flags')
        libraries.append(Path(item) if Path(item).is_absolute() else frozen/'build'/item)
    hashes = {str(path):sha(path) for path in libraries}
    source = ROOT/'tools/audit_destruction_firing_lane_inputs.cpp'
    collision = frozen/'source/src/physics/collision/collision.cpp'
    headers = {str(path):sha(path) for path in (frozen/'source/include').rglob('*.hpp')}
    report = dict(passed=False,scope='Read-only static BSP firing-lane input survey; no native gameplay acceptance',
                  full_goal_complete=False,borrowed_cohort=str(args.cohort.resolve()),
                  source_sha256=sha(source),collision_source_sha256=sha(collision),
                  borrowed_header_hashes=headers,borrowed_library_hashes=hashes,
                  map_sha256=sha(args.map),bsp=args.bsp,definition=args.definition,
                  retained_large_artifacts=[])
    compile_command = [link[0],'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                       '-I'+str(frozen/'source/include'),str(source),str(collision),'-o',str(owner/'survey')]
    compile_command += [str(path) for path in libraries]
    report['compile_command'] = compile_command
    result = subprocess.run(compile_command,cwd=owner,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    (args.output/'compile.log').write_bytes(result.stdout[-2*1024*1024:])
    report['compile_exit_code'] = result.returncode
    code = result.returncode
    if code==0:
        command = [str(owner/'survey'),str(args.map.resolve()),str(args.map.resolve().parent),
                   str(args.bsp),args.definition,str(args.output/'survey.json')]
        report['command'] = command
        result = subprocess.run(command,cwd=owner,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        (args.output/'execution.log').write_bytes(result.stdout[-2*1024*1024:]);code=result.returncode
        report['execution_exit_code'] = code
    report['borrowed_inputs_unchanged'] = all(sha(path)==digest for path,digest in dict(hashes,**headers).items())
    report['passed'] = code==0 and report['borrowed_inputs_unchanged']
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],report=str(args.output/'report.json'))))
    return 0 if report['passed'] else 1


if __name__=='__main__':
    raise SystemExit(main())
