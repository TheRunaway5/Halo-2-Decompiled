#!/usr/bin/env python3
"""Real hardware FP ownership across retained DATA observers, including pending #MF."""
import argparse,hashlib,json,os,platform,subprocess,sys
from pathlib import Path
from validate_native_raw_x87_scope_tu15 import corpus
ROOT=Path(__file__).resolve().parents[1]
OLD_HEADER_PIN='c8858215b8e136a3f7a38ddb43b200a198bcb0a4c94a25065e822b7815ab9f22'
INPUTS=('include/halo2/native_tu15_retained_store_observer.hpp','cmake/NativeTu15RetainedObserverFp.cmake','include/halo2/native_tu15_retained_store_observation.hpp','src/physics/runtime/native_tu15_raw_x87_scope.hpp','src/physics/native_tu15_joint_constraint_numeric_internal.hpp','tests/native_tu15_retained_observer_fp_tests.cpp','tools/validate_native_retained_observer_fp_tu15.py','tools/validate_native_raw_x87_scope_tu15.py')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--report',type=Path,required=True);p.add_argument('--expect-pending-failure',action='store_true');p.add_argument('--negative-control',action='store_true');p.add_argument('--regression',action='store_true');args=p.parse_args()
 if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
 if sys.platform!='linux' or platform.machine().lower()not in('x86_64','amd64'):print('SKIP observer FP proof needs x86-64');return 77
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pins={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in INPUTS}
 rows=[row for row in corpus()if row['op']==0];fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')))
 binary=scratch/'observer-fp';cmd=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_retained_observer_fp_tests.cpp'),'-o',str(binary)]
 if args.sanitizers:cmd+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
 compiled=subprocess.run(cmd,capture_output=True,text=True)
 if compiled.returncode:raise ValueError('Observer FP compile failed: '+compiled.stderr[-5000:])
 result=subprocess.run([str(binary),str(fixture)]+(['--integration']if args.regression else []),capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
 if args.expect_pending_failure:passed=result.returncode in(-8,136)
 else:passed=result.returncode==0
 report=dict(passed=passed,pre_fix_expected_pending_sigfpe=args.expect_pending_failure,compiler=args.compiler,sanitizers=args.sanitizers,source_sha256=pins,exit=result.returncode,stdout=result.stdout[-1000:],stderr=result.stderr[-2500:],owned_scratch=str(scratch),retained_large_artifacts=[])
 if result.returncode==0:report['native']=json.loads(result.stdout)
 if passed and args.negative_control:
  original=(ROOT/INPUTS[0]).read_text();marker='            // Discard callback pending exceptions before the waiting FRSTOR.\n            asm volatile("fninit;frstor %0"'
  if original.count(marker)!=1:raise ValueError('Pending-FP fix marker changed')
  old=original.replace(marker,'            asm volatile("frstor %0"')
  if hashlib.sha256(old.encode()).hexdigest()!=OLD_HEADER_PIN:raise ValueError('Historical unprotected FP owner body drift')
  override=scratch/'old/halo2/native_tu15_retained_store_observer.hpp';override.parent.mkdir(parents=True);override.write_text(old)
  negative=scratch/'old-observer-fp';command=[v for v in cmd if not v.startswith('-fsanitize=')];command.insert(1,'-I'+str(override.parent.parent));command[command.index('-o')+1]=str(negative)
  old_compile=subprocess.run(command,capture_output=True,text=True)
  if old_compile.returncode:raise ValueError('Historical negative control did not compile: '+old_compile.stderr[-3000:])
  bad=subprocess.run([str(negative),str(fixture)],capture_output=True,text=True)
  report['negative_control']=dict(original_header_sha256=OLD_HEADER_PIN,exit=bad.returncode,actual_pending_sigfpe=bad.returncode in(-8,136))
  if not report['negative_control']['actual_pending_sigfpe']:raise ValueError('Historical pending-FP bug no longer detected')
 if pins!={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in INPUTS}:raise ValueError('Observer FP source drift')
 args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','exit','pre_fix_expected_pending_sigfpe')}));return 0 if passed else 1
if __name__=='__main__':raise SystemExit(main())
