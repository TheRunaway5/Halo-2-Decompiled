#!/usr/bin/env python3
"""Actual retained first-PUSH empty-to-installed observer counterexample."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
from validate_native_solver_hull_edge_frame_tu15 import ROOT,body
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);original=body();(scratch/'edge_observer_reference.inc').write_text(original.replace('run_313470(','owner(',1))
    names=['src/physics/runtime/native_tu15_solver_hull_edge_frame.hpp','src/physics/runtime/native_tu15_solver_hull_edge_source_owner.hpp','tests/native_tu15_solver_hull_edge_observer_entry_tests.cpp',str(Path(__file__).relative_to(ROOT))]
    pins={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in names};report=dict(passed=False,scratch_owner=str(scratch),source_sha256=pins,retained_large_artifacts=[])
    try:
        command=['c++','-std=c++20','-O2','-Wno-ignored-attributes','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_solver_hull_edge_observer_entry_tests.cpp'),'-o',str(scratch/'witness')]
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Witness compile failed: '+built.stderr[-10000:])
        env=os.environ.copy();env.pop('HALO2_REFERENCE_SOLVER_HULL_EDGE',None)
        run=subprocess.run([str(scratch/'witness')],capture_output=True,text=True,env=env);report.update(result=json.loads(run.stdout.strip().splitlines()[-1]),exit_code=run.returncode)
        report['passed']=run.returncode==0
    except Exception as error:report['error']=repr(error)
    report['sources_unchanged']=all(hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==value for name,value in pins.items())
    if not report['sources_unchanged']:report['passed']=False
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','result','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
