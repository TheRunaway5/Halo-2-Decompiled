#!/usr/bin/env python3
"""Prove provider exceptions cannot impersonate an object-lookup handoff."""
import argparse,hashlib,json,os,platform,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true');p.add_argument('--semantic-mutants',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    files=['src/gameplay/objects/native_tu15_object_body_source_frame.hpp','src/physics/runtime/physics_step_internal.hpp','src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp','tests/native_tu15_object_body_foreign_continuation_tests.cpp','tools/validate_native_object_body_foreign_continuation_tu15.py']
    pins={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in files};report=dict(passed=False,owned_scratch=str(scratch),source_before=pins,retained_large_artifacts=[])
    try:
        results=[]
        for compiler in a.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name;command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_object_body_foreign_continuation_tests.cpp'),'-o',str(binary)]
            if a.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            done=subprocess.run(command,capture_output=True,text=True)
            if done.returncode:raise ValueError('Foreign continuation compile failed: '+done.stderr[-7000:])
            done=subprocess.run([str(binary)],capture_output=True,text=True)
            if done.returncode:raise ValueError('Foreign continuation mismatch: '+(done.stdout+done.stderr)[-7000:])
            defects=[]
            if a.semantic_mutants:
                relative='src/gameplay/objects/native_tu15_object_body_source_frame.hpp';source=(ROOT/relative).read_text();old='if(resume.owner!=&token)throw;'
                if source.count(old)!=1:raise ValueError('Foreign handoff defect seam drift')
                overlay=scratch/(Path(compiler).name+'-foreign');target=overlay/relative;target.parent.mkdir(parents=True);target.write_text(source.replace(old,'',1));changed=list(command);changed.insert(1,'-I'+str(overlay/'src'));changed[changed.index('-o')+1]=str(overlay/'bad')
                built=subprocess.run(changed,capture_output=True,text=True)
                if built.returncode:raise ValueError('Foreign handoff defect compile failed: '+built.stderr[-6000:])
                bad=subprocess.run([str(overlay/'bad')],capture_output=True,text=True);diagnostic=(bad.stdout+bad.stderr)[-7000:]
                if not bad.returncode or not any(text in diagnostic for text in('Object lookup foreign exception differs','Foreign fixture missed exact completed two-PUSH boundary')):raise ValueError('Foreign continuation defect survived/unrecognized: '+diagnostic)
                defects.append(dict(name='swallow-foreign-provider-continuation',rejected=True,diagnostic=diagnostic))
            results.append(dict(compiler=compiler,command=command,result=json.loads(done.stdout),compiled_defects=defects))
        report.update(passed=True,compilers=results)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in files}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())
