#!/usr/bin/env python3
"""Run both unchanged public Arena bootstrap checks with actual owner sources.

This bounded fixture avoids the full systems archive's unrelated Rust EOS
worker link dependency. No source function or required child is replaced.
"""
import argparse,hashlib,json,os,platform,shlex,subprocess
from pathlib import Path
from validate_native_arena_initialize_owner_tu15 import SOURCES
ROOT=Path(__file__).resolve().parents[1]
EXTRA=('src/physics/native_tu15_arena_bootstrap.cpp','src/platform/native_tu15_heap_data.cpp',
       'src/platform/native_tu15_heap_bootstrap.cpp','src/platform/source_process_heap_runtime_generated.cpp',
       'src/platform/source_process_heap_platform.cpp','src/platform/source_process_shutdown.cpp',
       'src/platform/source_process_startup_records.cpp','src/physics/physics_platform_locks.cpp',
       'src/physics/physics_platform_locks_continuation.cpp','src/physics/physics_platform_call.cpp',
       'src/runtime/source_runtime_host_virtual_memory.cpp','src/runtime/source_runtime_image.cpp')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='g++');p.add_argument('--xbe',type=Path,required=True);p.add_argument('--sanitizers',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64'):return 77
    if not a.xbe.is_file():return 77
    from native_arena_initialize_replacements import installed_entries,replace_arena_owner,replace_arena_bootstrap
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);sources=('tests/native_tu15_arena_data_tests.cpp',*SOURCES[1:],*EXTRA)
    fixture=ROOT/'analysis/multiplayer/sandbox-source/physics-game-arena-tu15-20261012/original-v2/report.json'
    pins={n:sha(ROOT/n)for n in(*sources,'tools/validate_native_arena_initialize_bootstrap_tu15.py','tools/native_arena_initialize_replacements.py')};headers={x.resolve():sha(x)for tree in('include','src')for x in(ROOT/tree).rglob('*')if x.suffix in('.h','.hpp','.inc')}
    report=dict(passed=False,owned_scratch=str(scratch),source_before=pins,retained_large_artifacts=[],actual_public_driver=sources[0],scope='Both unchanged public CTest driver invocations with real compiled heap/Arena/page/worker/file owners; direct-source link fixture, not an acceptance of the unrelated full systems/Rust target.')
    try:
        if sha(a.xbe)!='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd':raise ValueError('Wrong original TU1.5 XBE')
        if installed_entries((ROOT/'src/physics/physics_game_arena_runtime_generated.cpp').read_text())!=['126640']:raise ValueError('Missing installed Arena entry')
        for name,installer in(('src/physics/physics_game_arena_native.cpp',replace_arena_owner),('src/physics/native_tu15_arena_bootstrap.cpp',replace_arena_bootstrap)):
            text=(ROOT/name).read_text()
            if installer(text)!=text:raise ValueError('Actual public producer/bootstrap default missing')
        flags=['-std=c++20','-O1','-g0','-fno-fast-math','-ffp-contract=off','-frounding-math','-ffunction-sections','-fdata-sections','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src')]
        if a.sanitizers:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        commands=[];objects=[]
        for index,name in enumerate(sources):
            target=scratch/(str(index)+'.o');dep=target.with_suffix('.d');command=[a.compiler,*flags,'-c',str(ROOT/name),'-o',str(target),'-MD','-MF',str(dep)];commands.append(command);done=subprocess.run(command,capture_output=True,text=True)
            if done.returncode:raise ValueError('Actual public bootstrap compile failed '+name+': '+done.stderr[-7000:])
            for word in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(word).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            objects.append(target)
        binary=scratch/'public-bootstrap';command=[a.compiler,*flags,*map(str,objects),'-Wl,--gc-sections','-pthread','-lcrypto','-o',str(binary)];commands.append(command);done=subprocess.run(command,capture_output=True,text=True)
        if done.returncode:raise ValueError('Actual public bootstrap link failed '+done.stderr[-10000:])
        rows=[]
        for name,args in(('native_tu15_arena_bootstrap',[]),('original_tu15_arena_bootstrap_data',['--reference-xbe',str(a.xbe),'--original-fixture',str(fixture)])):
            command=[str(binary),*args];done=subprocess.run(command,capture_output=True,text=True,timeout=180);rows.append(dict(name=name,command=command,exit_code=done.returncode,diagnostic=(done.stdout+done.stderr)[-5000:]))
            if done.returncode:raise ValueError('Actual public bootstrap regression failed '+name+': '+(done.stdout+done.stderr)[-7000:])
        report.update(passed=True,compiler=a.compiler,sanitizers=a.sanitizers,commands=commands,regressions=rows,original_xbe_sha256=sha(a.xbe),original_fixture_sha256=sha(fixture))
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={n:sha(ROOT/n)for n in pins}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
