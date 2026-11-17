#!/usr/bin/env python3
"""Private exact Arena idle-schedule candidate; issues no production policy."""
import argparse,hashlib,json,os,platform,shlex,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FILES=('src/physics/runtime/native_tu15_arena_schedule_policy.hpp','tests/native_tu15_arena_schedule_policy_tests.cpp','tools/validate_native_arena_schedule_policy_tu15.py','src/physics/native_tu15_arena_bootstrap.cpp')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def barrier(source):
    start=source.index('struct WorkerIdleBarrier {');end=source.index('\n};',start)+3
    return source[start:end]
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',action='append');parser.add_argument('--sanitizers',action='store_true');args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pins={p:sha(ROOT/p)for p in FILES};headers={p.resolve():sha(p)for folder in('include','src')for p in(ROOT/folder).rglob('*')if p.suffix in('.h','.hpp','.inc')}
    report=dict(passed=False,private_unbound=True,production_policy_issued=False,owned_scratch=str(scratch),source_before=pins,retained_large_artifacts=[])
    try:
        from native_arena_initialize_replacements import canonical_bootstrap
        pins['tools/native_arena_initialize_replacements.py']=sha(ROOT/'tools/native_arena_initialize_replacements.py')
        original=barrier(canonical_bootstrap((ROOT/FILES[-1]).read_text()));candidate=barrier((ROOT/FILES[0]).read_text())
        if original!=candidate:raise ValueError('Private barrier differs from the complete actual bootstrap body')
        results=[]
        for compiler in args.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d');command=[compiler,'-std=c++20','-O2','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/FILES[1]),'-pthread','-o',str(binary),'-MD','-MF',str(dep)]
            if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            done=subprocess.run(command,capture_output=True,text=True)
            if done.returncode:raise ValueError('Private schedule compile failed: '+done.stderr[-6000:])
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path.is_relative_to(ROOT)and path.is_file():pins[str(path.relative_to(ROOT))]=headers.get(path,pins.get(str(path.relative_to(ROOT)),sha(path)))
            done=subprocess.run([str(binary)],capture_output=True,text=True,timeout=15)
            if done.returncode:raise ValueError('Private schedule mismatch: '+(done.stdout+done.stderr)[-6000:])
            results.append(dict(compiler=compiler,command=command,result=json.loads(done.stdout)))
        report.update(passed=True,barrier_complete_body_sha256=hashlib.sha256(original.encode()).hexdigest(),compilers=results)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={p:sha(ROOT/p)for p in pins}
        if pins!=report['source_after']:report.update(passed=False,source_drift=True)
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
